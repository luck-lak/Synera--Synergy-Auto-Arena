#include "game.h"
#include "entity/cinderella.h"
#include "entity/unit.h"
#include "entity/projectile.h"
#include "gui/griditem.h"
#include "gui/unititem.h"
#include "gui/equipmentitem.h"
#include "core/herofactory.h"
#include <QGraphicsScene>
#include <QtMath>
#include <QGraphicsRectItem>
#include <QQueue>
#include <QSet>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include "audio/soundmanager.h"

//角色
// #include"entity/cinderella.h"
// #include"entity/silvermane.h"
// #include"entity/magicmirror.h"
// #include"entity/mermaid.h"
// #include"entity/snowwhite.h"
// #include"entity/littlematch.h"
// #include"entity/uninvited.h"
// #include"entity/sleepingbeauty.h"


namespace {
constexpr qreal kZGrid = 0.0;
constexpr qreal kZUnit = 1.0;
constexpr qreal kZDraggingUnit = 2.0;
}

// ======================== 敌方模板系统 ========================
struct EnemyTemplate {
    QString name;
    int hp, atk, range, moveSpeed, attackSpeed, manaRecovery;
    QList<SkillEffect> passives;  // 永久被动，source=nullptr
};

static Unit* createEnemyFromTemplate(const EnemyTemplate& tmpl, int round)
{
    Unit* u = new Unit(tmpl.name);
    u->setOwner(Unit::Owner::EnemyCtrl);

    float scale = 1.0f + (round - 1) * 0.10f;  /* 每轮 10% 递增，给玩家发育空间 */
    u->setMaxHp(static_cast<int>(tmpl.hp * scale));
    u->setHp(u->maxHp());
    u->setAtk(static_cast<int>(tmpl.atk * scale));
    u->setRange(tmpl.range);
    u->setMoveSpeed(tmpl.moveSpeed);
    u->setAttackSpeed(tmpl.attackSpeed);
    u->setmanaRecoveryAmount(tmpl.manaRecovery);

    for (const SkillEffect& e : tmpl.passives)
        u->addEffect(e);

    return u;
}

// 0:盾卫 1:刺客 2:弩手 3:术士 4:祭司
static const EnemyTemplate kEnemyTemplates[] = {
    {QStringLiteral("盾卫"), 600, 6, 1, 110, 18, 6,
     {{EffectType::DamageTakenUp, -1, -0.20f, nullptr}}},
    {QStringLiteral("刺客"), 180, 25, 1, 60, 10, 8,
     {{EffectType::AttackSpeedUp, -1, -0.30f, nullptr},
      {EffectType::MoveSpeedUp, -1, -0.30f, nullptr}}},
    {QStringLiteral("弩手"), 250, 12, 3, 90, 20, 10, {}},
    {QStringLiteral("术士"), 200, 18, 3, 90, 18, 15, {}},
    {QStringLiteral("祭司"), 300, 5, 2, 90, 22, 10,
     {{EffectType::HealOverTime, -1, 3.0f, nullptr}}},
};

Game::Game(QObject* parent)
    : QObject(parent)
    , m_scene(new QGraphicsScene(this))
    , m_dragActive(false)
    , m_activeUnitId(-1)
    , m_sourceGrid(-1, -1)
    , m_geometry(Board::ROWS, Board::COLS, 90)//cellsize->100
    , m_combatTimer(new QTimer(this))
{
    connect(m_combatTimer,&QTimer::timeout,this,&Game::onCombatTick);
}

Game::~Game()
{
    qDeleteAll(m_units);
    m_units.clear();
}

/////////////////////////
//初始化
void Game::initialize()
{
    createStarterUnitsIfNeeded();
    buildScene();
    reset();
}

void Game::createStarterUnitsIfNeeded()
{
    if (!m_units.isEmpty())
        return;
    // 第一波敌人
    spawnEnemiesForRound(1);
}

bool Game::canPlaceUnit() const
{
    for (int i = 0; i < Bench::Slots; i++) {
        if (m_bench.isSlotEmpty(i)) return true;
    }
    return m_player.canDeploy();
}

void Game::addPlayerUnit(Unit* unit)
{
    if (!unit) return;
    unit->setOwner(Unit::Owner::PlayerCtrl);
    m_units.append(unit);

    // 创建 UnitItem
    auto* unitItem = new UnitItem(unit);
    unitItem->setZValue(kZUnit);
    m_scene->addItem(unitItem);
    m_unitItems.push_back(unitItem);
    m_unitItemById[unit->id()] = unitItem;

    connect(unitItem, &UnitItem::dragStarted, this, &Game::handleDragStarted);
    connect(unitItem, &UnitItem::dragMoved, this, &Game::handleDragMoved);
    connect(unitItem, &UnitItem::dragDropped, this, &Game::handleDropCommand);
    connect(unitItem, &UnitItem::equipClicked, this, &Game::onUnitEquipClicked);

    // 放到 bench 上空位
    for (int i = 0; i < Bench::Slots; i++) {
        if (m_bench.isSlotEmpty(i)) {
            m_bench.addUnit(unit, i);
            syncUnits();
            tryMergeStarUp(unit);
            return;
        }
    }

    // bench 满了，放棋盘
    for (int row = Board::ROWS - 1; row >= Board::ROWS / 2; --row) {
        for (int col = 0; col < Board::COLS; ++col) {
            QPoint pos(col, row);
            if (!m_board.hasUnitAt(pos)) {
                m_board.addUnit(unit, pos);
                m_player.changePopulationUsed(1);
                syncUnits();
                tryMergeStarUp(unit);
                return;
            }
        }
    }
}

void Game::sellUnit(int unitId)
{
    Unit* unit = findUnitById(unitId);
    if (!unit || unit->cost() <= 0) return;  /* 非商店购买的不让卖 */

    int refund = unit->cost() * 7 / 10;      /* 70% 回收价，整数向下 */

    // 从 bench 或棋盘移除
    QPoint pos = unit->position();
    if (pos.y() == -1)
        m_bench.removeUnit(unit);
    else {
        m_board.removeUnit(unit);
        m_player.changePopulationUsed(-1);
    }

    // 删 UnitItem
    auto it = m_unitItemById.find(unitId);
    if (it != m_unitItemById.end()) {
        m_scene->removeItem(it->second);
        m_unitItems.erase(std::remove(m_unitItems.begin(), m_unitItems.end(), it->second), m_unitItems.end());
        delete it->second;
        m_unitItemById.erase(it);
    }

    // 删 Unit
    m_units.removeOne(unit);
    delete unit;

    m_player.changeGold(refund);
    syncUnits();
    emit playerInfoChanged();
}

void Game::tryMergeStarUp(Unit* justAdded)
{
    if (!justAdded) return;
    if (m_phase == Phase::Combat) return;  /* 战斗中不合成 */

    QString name = justAdded->name();
    int star = justAdded->starLevel();
    if (star >= 3) return;  // 已经满星

    // 收集同英雄同星级的玩家单位
    QList<Unit*> same;
    for (Unit* u : m_units) {
        if (u && u->name() == name && u->starLevel() == star
            && u->owner() == Unit::Owner::PlayerCtrl)
            same.append(u);
    }
    if (same.size() < 3) return;

    // 选 keeper：棋盘上优先，其次 bench
    Unit* keeper = nullptr;
    for (Unit* u : same) {
        if (u->position().y() >= 0) { keeper = u; break; }
    }
    if (!keeper) keeper = same[0];

    // 收集所有单位的装备（拷贝），升星后重新分配
    QList<Equipment> allGear;
    for (Unit* u : same) {
        for (int s = 0; s < 2; s++) {
            if (u->equipment(s))
                allGear.append(*u->equipment(s));
        }
    }

    // 卸掉牺牲单位的装备，防止析构时 delete 导致丢失
    for (Unit* u : same) {
        if (u == keeper) continue;
        u->unequip(-1);
    }

    // 卸掉 keeper 的装备（已拷贝到 allGear），升级后再穿
    keeper->unequip(-1);

    // 删除两个牺牲单位
    for (Unit* u : same) {
        if (u == keeper) continue;

        if (u->position().y() == -1)
            m_bench.removeUnit(u);
        else {
            m_board.removeUnit(u);
            m_player.changePopulationUsed(-1);
        }

        auto it = m_unitItemById.find(u->id());
        if (it != m_unitItemById.end()) {
            m_scene->removeItem(it->second);
            m_unitItems.erase(std::remove(m_unitItems.begin(), m_unitItems.end(), it->second), m_unitItems.end());
            delete it->second;
            m_unitItemById.erase(it);
        }

        m_units.removeOne(u);
        delete u;
    }

    // 升级 keeper
    int newStar = star + 1;
    keeper->setStarLevel(newStar);
    SoundManager::instance()->play("starup");
    float mult = (newStar == 2) ? 1.8f : 1.8f * 1.8f;
    keeper->setMaxHp(static_cast<int>(keeper->star1BaseHp() * mult));
    keeper->setHp(keeper->maxHp());
    keeper->setAtk(static_cast<int>(keeper->star1BaseAtk() * mult));
    keeper->setCost(keeper->cost() * 3); // 三合一，总价翻三倍

    // 重新分配装备：keeper 槽位优先，多余的放回装备栏
    for (const Equipment& eq : allGear) {
        if (keeper->canEquip())
            keeper->equip(&eq);
        else
            addEquipment(eq);
    }

    keeper->setBaseMaxHp(0);             // trait 重录
    keeper->setBaseAtk(0);
    keeper->setBaseManaRecovery(0);

    syncUnits();
    emit playerInfoChanged();

    // 升星后继续检查，连锁合并（如 3 个 2 星 → 3 星）
    tryMergeStarUp(keeper);
}

// ======================== 装备系统 ========================
void Game::addEquipment(const Equipment& eq)
{
    m_equipPool.append(eq);

    auto* item = new EquipmentItem(eq);

    connect(item, &EquipmentItem::dragStarted, this, &Game::onEquipDragStarted);
    connect(item, &EquipmentItem::dragDropped, this, [this](EquipmentItem* it, const QPointF& pos) {
        onEquipDragDropped(it, pos);
    });

    m_scene->addItem(item);
    m_equipItems.append(item);

    // 全部重排位置，避免删除造成的空位导致重叠
    qreal totalW = Bench::Slots * 70.0;
    qreal startX = (Board::COLS * m_geometry.cellSize() / 2.0) - (totalW / 2.0);
    qreal benchBottom = Board::ROWS * m_geometry.cellSize() + 50 + 80 + 10;
    qreal slotW = totalW / 8.0;
    for (int i = 0; i < m_equipItems.size(); i++) {
        int col = i % 8;
        int row = i / 8;
        m_equipItems[i]->setPos(startX + slotW * col + slotW / 2.0,
                                benchBottom + 24 + row * 50);
    }
}

void Game::tryDropEquipment()
{
    if (m_equipDropCount >= kMaxEquipDrops) return;

    float chance = 0.12f + m_round * 0.02f;
    if (QRandomGenerator::global()->generateDouble() > chance) return;

    QList<Equipment> pool = createEquipmentPool();
    if (pool.isEmpty()) return;
    int idx = QRandomGenerator::global()->bounded(pool.size());
    addEquipment(pool[idx]);
    m_equipDropCount++;
}

void Game::onEquipDragStarted(EquipmentItem* item)
{
    m_equipDragHome = item->pos();
}

void Game::onEquipDragDropped(EquipmentItem* item, const QPointF& scenePos)
{
    UnitItem* target = nullptr;
    for (UnitItem* ui : m_unitItems) {
        if (!ui || !ui->isVisible()) continue;
        if (ui->sceneBoundingRect().contains(scenePos)) {
            target = ui;
            break;
        }
    }

    // 从角色身上卸下来的装备
    if (m_equipDragFromUnit >= 0) {
        Unit* src = findUnitById(m_equipDragFromUnit);
        // 装备栏 y 基线
        qreal barY = Board::ROWS * m_geometry.cellSize() + 50 + 80 + 10;
        if (target && target->unit() && target->unit()->canEquip()) {
            // 转移给另一个角色
            target->unit()->equip(const_cast<Equipment*>(&item->equipment()));
            SoundManager::instance()->play("equip");
            emit unitSelected(target->unit());
        } else if (scenePos.y() > barY) {
            // 拖回装备栏：返还
            addEquipment(item->equipment());
        } else {
            // 放回原位
            if (src) src->equip(const_cast<Equipment*>(&item->equipment()));
            if (src) emit unitSelected(src);
        }
        m_scene->removeItem(item);
        delete item;
        m_equipDragFromUnit = -1;
        syncUnits();
        emit playerInfoChanged();
        return;
    }

    // 从装备栏拖出来的
    if (target && target->unit() && target->unit()->canEquip()) {
        target->unit()->equip(const_cast<Equipment*>(&item->equipment()));
        SoundManager::instance()->play("equip");
        m_equipPool.removeOne(item->equipment());
        m_scene->removeItem(item);
        m_equipItems.removeOne(item);
        delete item;
        emit unitSelected(target->unit());
        emit playerInfoChanged();
    } else {
        item->setPos(m_equipDragHome);
    }
}

void Game::onUnitEquipClicked(int unitId, const QPointF& /*scenePos*/)
{
    Unit* unit = findUnitById(unitId);
    if (unit)
        emit unitSelected(unit);
}

void Game::buildScene()
{
    m_scene->clear();
    m_gridItems.clear();
    m_unitItems.clear();
    m_unitItemById.clear();

    QRectF totalBounds;
    bool first = true;
    for (int row = 0; row < Board::ROWS; ++row) {
        for (int col = 0; col < Board::COLS; ++col) {
            GridItem* gridItem = new GridItem(row, col, m_geometry.cellSize());
            gridItem->setZValue(kZGrid);
            gridItem->setBaseColor(row < Board::ROWS / 2 ? QColor(80, 60, 60) : QColor(60, 60, 80));
            gridItem->setPos(m_geometry.gridToWorld(row, col).x()- m_geometry.cellSize()/2.0,
                             m_geometry.gridToWorld(row, col).y() - m_geometry.cellSize() / 2.0);

            m_scene->addItem(gridItem);
            m_gridItems.push_back(gridItem);

            const QRectF bounds = gridItem->sceneBoundingRect();
            totalBounds = first ? bounds : totalBounds.united(bounds);
            first = false;

        }
    }

    for (Unit* unit : m_units) {
        UnitItem* unitItem = new UnitItem(unit);
        unitItem->setZValue(kZUnit);
        m_scene->addItem(unitItem);
        m_unitItems.push_back(unitItem);
        m_unitItemById[unit->id()] = unitItem;

        connect(unitItem, &UnitItem::dragStarted,
                this, &Game::handleDragStarted);
        connect(unitItem, &UnitItem::dragMoved,
                this, &Game::handleDragMoved);
        connect(unitItem, &UnitItem::dragDropped,
                this, &Game::handleDropCommand);
        connect(unitItem, &UnitItem::equipClicked, this, &Game::onUnitEquipClicked);
    }

    for (int i=0;i<Bench::Slots;i++){
        QPointF center = m_geometry.benchSlotToWorld(i, Bench::Slots);
        QGraphicsRectItem* slot = new QGraphicsRectItem{
            center.x() - 35,// 左上角x（70/2=35）
            center.y() - 40,// 左上角y（80/2=40）
            70,// 宽
            80// 高
        };
        slot->setPen(QPen(QColor(80, 80, 80), 2));
        // 边框：深灰 2px
        slot->setBrush(QColor(50, 50, 50));
        // 填充：深灰
        slot->setZValue(kZGrid);
        // z=0，和棋盘格同层
        m_scene->addItem(slot);
    }

    // 回收区（bench 右侧）
    qreal boardBottom = Board::ROWS * m_geometry.cellSize();                         /* 720 */
    qreal benchTop = boardBottom + 50;                                                /* 770 */
    qreal totalBenchW = Bench::Slots * 70.0;                                          /* 560 */
    qreal benchStartX = (Board::COLS * m_geometry.cellSize() / 2.0) - (totalBenchW / 2.0); /* 80 */
    qreal benchEndX = benchStartX + totalBenchW;                                      /* 640 */

    m_sellZoneRect = QRectF(benchEndX + 10, benchTop, 90, 80);

    auto* sellZone = new QGraphicsRectItem(m_sellZoneRect);
    sellZone->setBrush(QColor(80, 30, 30, 180));   /* 暗红半透明底 */
    sellZone->setPen(QPen(QColor(180, 80, 80), 2)); /* 红边框 */
    sellZone->setZValue(kZGrid);
    m_scene->addItem(sellZone);

    auto* sellText = new QGraphicsSimpleTextItem(QStringLiteral("回收"));
    sellText->setPos(benchEndX + 22, benchTop + 28);
    sellText->setBrush(QColor(220, 180, 180));       /* 浅红字 */
    QFont sf = sellText->font();
    sf.setPixelSize(16);
    sf.setBold(true);
    sellText->setFont(sf);
    sellText->setZValue(kZGrid + 0.1);
    m_scene->addItem(sellText);

    QRectF benchBounds(
        m_geometry.benchSlotToWorld(0, Bench::Slots).x() - 35,
        m_geometry.benchSlotToWorld(0, Bench::Slots).y() - 40,
        Bench::Slots * 70.0,
        80
        );
    totalBounds = totalBounds.united(benchBounds);

    // 装备栏区域（bench 下方，最多 3 行）
    qreal equipY = benchTop + 80 + 10;
    QRectF equipBounds(benchStartX, equipY, totalBenchW, 150);
    totalBounds = totalBounds.united(equipBounds);

    m_scene->setSceneRect(totalBounds.adjusted(0, 0, 0, 10));
}

void Game::reset()
{
    m_board.clear();
    m_bench.clear();

    m_combatTimer->stop();
    m_phase =Phase::Prep;
    m_player.setPopulationUsed(0);
    m_traitManager.clearAll(m_units);   /* 重置前清理羁绊加成 */

    for(Unit* unit : m_units){
        if(!unit)continue;
        unit->resetToDefault();//回复到初始状态
        unit->clearProjectiles();
        if(unit->owner() == Unit::Owner::PlayerCtrl){
            bool placed = false;

            // 优先恢复开战前棋盘位置
            auto it = m_preCombatPositions.find(unit->id());
            if (it != m_preCombatPositions.end() && it->y() >= 0
                && !m_board.hasUnitAt(it.value())) {
                m_board.addUnit(unit, it.value());
                m_player.changePopulationUsed(1);
                placed = true;
            }

            if (!placed)
            for(int i=0; i<Bench::Slots;i++){
                if(m_bench.isSlotEmpty(i)){
                    m_bench.addUnit(unit,i);
                    placed = true;
                    break;
                }
            }
            if(placed)continue;
            for(int row=Board::ROWS - 1;row>=Board::ROWS/2;--row){
                for(int col =0 ; col <Board::COLS;++col){
                    if(!m_board.hasUnitAt(QPoint(col,row))){
                        m_board.addUnit(unit,QPoint(col,row));
                        placed = true;
                        m_player.changePopulationUsed(1);
                        break;
                    }
                    if(placed)break;
                }
                if(placed)break;
            }
        }else{
            // 敌方按 spawnEnemiesForRound 里预设的阵型位置放置
            QPoint pos = unit->position();
            if (m_board.isValidPosition(pos) && !m_board.hasUnitAt(pos))
                m_board.addUnit(unit, pos);
            else {
                // 兜底：预设位置被占或非法，走列优先
                bool placed = false;
                for (int col = 0; col < Board::COLS && !placed; ++col) {
                    for (int row = 0; row < Board::ROWS / 2; ++row) {
                        if (!m_board.hasUnitAt(QPoint(col, row))) {
                            m_board.addUnit(unit, QPoint(col, row));
                            placed = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    syncUnits();
    emit phaseChanged();
    emit playerInfoChanged();
}

/////////////////////////
//处理拖动操作
void Game::handleDragStarted(int unitId, const QPoint& sourceGrid, const QPointF&)
{
    Unit* unit = findUnitById(unitId);
    if(!unit || unit->owner() !=Unit::Owner::PlayerCtrl){
        return;
    }

    m_dragActive = true;
    m_activeUnitId = unitId;
    m_sourceGrid = sourceGrid;

    UnitItem* item = findUnitItem(unitId);
    if (item) {
        item->setZValue(kZDraggingUnit);
    }
}

void Game::handleDragMoved(int unitId, const QPoint&, const QPointF& scenePos)
{
    if (!m_dragActive) {
        return;
    }

    clearGridHighlights();

    // 拖到回收区上 → 不高亮任何格子
    if (m_sellZoneRect.contains(scenePos)) return;

    int benchSlot = m_geometry.benchSlotAt(scenePos, Bench::Slots);
    QPoint target;
    if (benchSlot >= 0) {
        target = QPoint(benchSlot, -1);   // Bench 坐标
    } else {
        target = m_geometry.worldToGrid(scenePos);   // 棋盘坐标
    }

    GridItem* targetItem = findGridItem(target);
    if (!targetItem) {
        return;
    }

    targetItem->setHoverActive(true);

    if (canApplyDrop(unitId, m_sourceGrid, target)) {
        targetItem->setDropActive(true);
    }
}

void Game::handleDropCommand(int unitId, const QPoint& sourceGrid, const QPointF& scenePos)
{
    if (!m_dragActive) {
        return;
    }

    // 拖到回收区 → 卖掉单位
    if (m_sellZoneRect.contains(scenePos) && m_phase == Phase::Prep) {
        clearGridHighlights();
        UnitItem* item = findUnitItem(m_activeUnitId);
        if (item) item->setZValue(kZUnit);
        m_dragActive = false;
        m_activeUnitId = -1;
        m_sourceGrid = QPoint(-1, -1);
        sellUnit(unitId);
        return;
    }

    int benchSlot = m_geometry.benchSlotAt(scenePos, Bench::Slots);
    QPoint target;
    if (benchSlot >= 0)
        target = QPoint(benchSlot, -1);
    else
        target = m_geometry.worldToGrid(scenePos);

    clearGridHighlights();
    if (canApplyDrop(unitId, sourceGrid, target)) {
        applyDrop(unitId, target);
    }

    UnitItem* item = findUnitItem(m_activeUnitId);
    if (item) {
        item->setZValue(kZUnit);
    }

    m_dragActive = false;
    m_activeUnitId = -1;
    m_sourceGrid = QPoint(-1, -1);

    syncUnits();
}

///////////////////////
//查找操作
Unit* Game::findUnitById(int unitId) const
{
    for (Unit* unit : m_units) {
        if (unit && unit->id() == unitId) {
            return unit;
        }
    }
    return nullptr;
}

GridItem* Game::findGridItem(const QPoint& gridPos) const
{
    for (GridItem* item : m_gridItems) {
        if (item && item->gridPos() == gridPos) {
            return item;
        }
    }
    return nullptr;
}

UnitItem* Game::findUnitItem(int unitId) const
{
    auto it = m_unitItemById.find(unitId);
    if (it == m_unitItemById.end()) {
        return nullptr;
    }
    return it->second;
}

///////////////////////
//
void Game::clearGridHighlights()
{
    for (GridItem* item : m_gridItems) {
        if (!item) {
            continue;
        }
        item->setHoverActive(false);
        item->setDropActive(false);
    }
}

bool Game::canApplyDrop(int unitId, const QPoint& source, const QPoint& target) const
{
    //当前阶段
    if(m_phase != Phase::Prep)return false;

    Unit* unit = findUnitById(unitId);
    if (!unit) {
        return false;
    }

    bool sourceOnBench = (source.y() == -1);
    bool targetOnBench = (target.y() == -1);

    //交换位置逻辑
    if(target == source)return false;
    Unit* target_unit;
    if(!targetOnBench)target_unit = m_board.getUnitAt(target);
    if(targetOnBench)target_unit = m_bench.getUnitAt(target.x());
    if(target_unit){
        if(target_unit->owner() == Unit::Owner::PlayerCtrl){
            return true;
        }
    }

    //上面和下面有重复，但无害
    //board ->board
    if (!sourceOnBench && !targetOnBench) {
        if (!m_board.isValidPosition(source) || !m_board.isValidPosition(target)) {
            return false;
        }
        if (!m_board.isPlayerHalf(source) || !m_board.isPlayerHalf(target)) {
            return false;
        }
        if (source == target || m_board.hasUnitAt(target)) {
            return false;
        }
        return m_board.getUnitAt(source) == unit;
    }
    //board -> bench
    if (!sourceOnBench && targetOnBench) {
        int slot = target.x();
        return slot >= 0 && slot < Bench::Slots && m_bench.isSlotEmpty(slot);
    }
    //bench -> board
    if (sourceOnBench && !targetOnBench) {
        if (!m_player.canDeploy()) return false;
        if (!m_board.isValidPosition(target)) return false;
        if (!m_board.isPlayerHalf(target)) return false;
        if (m_board.hasUnitAt(target)) return false;
        return true;
    }
    //bench -> bench
    if (sourceOnBench && targetOnBench) {
        int slot = target.x();
        return slot >= 0 && slot < Bench::Slots && m_bench.isSlotEmpty(slot);
    }
    //我还想设计一个交换逻辑

    return false;
}

void Game::applyDrop(int unitId, const QPoint& target)
{
    Unit* unit = findUnitById(unitId);
    if (!unit) {
        return;
    }

    bool sourceOnBench = (m_sourceGrid.y() == -1);
    bool targetOnBench = (target.y() == -1);

    //交换逻辑
    Unit* target_unit;
    if(!targetOnBench)target_unit = m_board.getUnitAt(target);
    if(targetOnBench)target_unit = m_bench.getUnitAt(target.x());
    if(target_unit){
        QPoint unit_point = unit->position();
        if (!sourceOnBench && !targetOnBench) {
            // 棋盘 → 棋盘
            m_board.removeUnit(unit);
            m_board.removeUnit(target_unit);
            m_board.addUnit(unit, target);
            m_board.addUnit(target_unit,unit_point);
        } else if (!sourceOnBench && targetOnBench) {
            // 棋盘 → Bench
            m_board.removeUnit(unit);
            m_bench.removeUnit(target_unit);
            m_bench.addUnit(unit, target.x());
            m_board.addUnit(target_unit,unit_point);
        }else if (sourceOnBench && !targetOnBench) {
            // Bench → 棋盘
            m_bench.removeUnit(unit);
            m_board.removeUnit(target_unit);
            m_bench.addUnit(target_unit,unit_point.x());
            m_board.addUnit(unit, target);
        }else if(sourceOnBench &&targetOnBench){
            // Bench → Bench（换槽）
            m_bench.removeUnit(unit);
            m_bench.removeUnit(target_unit);
            m_bench.addUnit(unit, target.x());
            m_bench.addUnit(target_unit,unit_point.x());
        }
        emit playerInfoChanged();
        return;
    }

    if (!sourceOnBench && !targetOnBench) {
        // 棋盘 → 棋盘
        m_board.removeUnit(unit);
        m_board.addUnit(unit, target);
    } else if (!sourceOnBench && targetOnBench) {
        // 棋盘 → Bench
        m_board.removeUnit(unit);
        m_bench.addUnit(unit, target.x());
        m_player.changePopulationUsed(-1);
    }else if (sourceOnBench && !targetOnBench) {
        // Bench → 棋盘
        m_bench.removeUnit(unit);
        m_board.addUnit(unit, target);
        m_player.changePopulationUsed(1);
    }else if(sourceOnBench &&targetOnBench){
        // Bench → Bench（换槽）
        m_bench.removeUnit(unit);
        m_bench.addUnit(unit, target.x());
    }
     emit playerInfoChanged();
}

void Game::syncUnits()
{
    clearGridHighlights();

    for (UnitItem* item : m_unitItems) {
        if (!item || !item->unit()) {
            continue;
        }

        const QPoint pos = item->unit()->position();
        // 情况1：在 Bench 上
        if (pos.y() == -1) {
            int slot = pos.x();
            if (slot >= 0 && slot < Bench::Slots && m_bench.getUnitAt(slot) ==item->unit()) {
                item->setVisible(true);
                item->setGridPos(pos);
                item->setPos(m_geometry.benchSlotToWorld(slot, Bench::Slots));
                item->setZValue(kZUnit);
                item->update();
            } else {
                item->setVisible(false);
            }
            continue;
        }

        // 情况2：在棋盘上
        if (!m_board.isValidPosition(pos) || m_board.getUnitAt(pos) != item->unit()) {
            item->setVisible(false);
            continue;
        }

        item->setVisible(true);
        item->setGridPos(pos);
        item->setPos(m_geometry.gridToWorld(pos.y(), pos.x()));
        item->setZValue(kZUnit);
        item->update();
    }
}

////////////////////////
//Timer，进入战斗模式启用
void Game::startCombat(){
    m_phase = Phase::Combat;
    m_combatFrame = 0;

    // 记录开战前玩家单位位置，reset 时还原
    m_preCombatPositions.clear();
    for (Unit* unit : m_units) {
        if (unit && unit->owner() == Unit::Owner::PlayerCtrl)
            m_preCombatPositions[unit->id()] = unit->position();
    }

    m_combatTimer->start(16);
    m_traitManager.recalculate(m_units);  // 开战即算羁绊
    SoundManager::instance()->play("round_start");
    emit phaseChanged();
}

void Game::onCombatTick(){
    if(m_phase != Phase::Combat)return;
    m_combatFrame++;

    for(Unit* unit :m_units){
        if(!unit || unit->isDead()) continue;
        if(unit->position().y() == -1)continue;

        unit->updateTarget(m_units);
        unit->decideState();
        unit->tickCooldowns(this);

        // 沉睡或封印 → 跳过行动，但冷却/效果保持倒计时
        if (unit->hasEffect(EffectType::Sleeping) || unit->hasEffect(EffectType::Sealed))
            continue;

        if (unit->state() == Unit::State::Moving && unit->moveCooldown() <= 0) {
            executeMovement(unit);
        }
        if (unit->state() == Unit::State::Attacking &&unit->attackCooldown() <= 0) {
            executeAttack(unit);
        }
    }

    // 更新所有投射物（不管射手死活，子弹已经飞出去了）
    for (Unit* unit : m_units) {
        if (!unit) continue;//原来这里角色死了就continue会导致其发射出去的子弹不再移动，一直显示的Bug
        unit->updateProjectiles();
    }

    // 移除死亡单位（飞弹到达致死的）
    for (Unit* unit : m_units) {
        if (!unit) continue;
        if (unit->state() == Unit::State::Dead && unit->position().y() != -1) {
            if (unit->owner() == Unit::Owner::EnemyCtrl)
                tryDropEquipment();
            m_board.removeUnit(unit);
        }
    }

    syncUnits();
    checkCombatEnd();
}

void Game::executeMovement(Unit* unit)
{
    Unit* target = unit->target();
    if (!target) return;

    QVector<QPoint>& path = unit->path();

    path = findpath(unit->position(), target->position(), unit);
    if (path.isEmpty()) {
        unit->setState(Unit::State::Idle);
        return;
    }

    QPoint nextPos = path.takeFirst();

    m_board.removeUnit(unit);
    m_board.addUnit(unit, nextPos);

    unit->setMoveCooldown(unit->moveSpeed());

    if (unit->isInRangeOf(target)) {
        unit->setState(Unit::State::Attacking);
        unit->setAttackCooldown(unit->attackSpeed());
    }
}

void Game::executeAttack(Unit* unit) {
    Unit* target = unit->target();
    if (!target) return;

    // 创建攻击特效 → 返回 nullptr 表示近战（直接扣血），否则飞弹
    Projectile* proj = unit->createProjectile(target, &m_geometry);

    if (proj) {
        // 远程：飞弹到达后才扣血
        unit->addProjectile(proj);
        m_scene->addItem(proj->item());
        QPoint attackerPos = unit->position();
        proj->setStartPos(m_geometry.gridToWorld(attackerPos.y(), attackerPos.x()));
    } else {
        // 近战：直接扣血
        if (target->isVulnerable()) {
            int dmg = static_cast<int>(unit->atk() * target->damageTakenMultiplier());
            target->setHp(target->hp() - dmg);
            target->triggerHitFlash();
        }
    }

    // 回蓝
    unit->setMana(unit->mana() + unit->manaRecoveryAmount());

    // 冷却
    unit->setAttackCooldown(unit->attackSpeed());

    // 目标死亡清理
    if (target->isDead()) {
        if (target->owner() == Unit::Owner::EnemyCtrl)
            tryDropEquipment();
        m_board.removeUnit(target);
        target->setState(Unit::State::Dead);
        unit->settarget(nullptr);
        unit->setState(Unit::State::Idle);
    }
}
////////////////////////
//寻路
QVector<QPoint> Game::findpath(const QPoint& start, const QPoint& end,Unit* self)
{
    //这里的end会传目标的位置，要注意不能两个占在同一个格子上了
    if(start == end){return {};}
    const QPoint dirs[] = {
        QPoint(0,-1),//上
        QPoint(0, 1),//下
        QPoint(-1,0),//左
        QPoint(1, 0) //右
    };
    QQueue<QPoint> queue;
    QHash<QPoint, QPoint> cameFrom;
    QSet<QPoint> visited;

    queue.enqueue(start);
    visited.insert(start);

    while(!queue.isEmpty()){
        QPoint cur = queue.dequeue();
        for (const QPoint& dir : dirs){
            QPoint next(cur.x() + dir.x() , cur.y() + dir.y());
            if(!m_board.isValidPosition(next))continue;
            if(visited.contains(next))continue;
            if (next == end) {
                cameFrom[next] = cur;
                QVector<QPoint> path;
                QPoint step = cur;//不能两个角色占在同一个格子
                while (step != start) {
                    path.prepend(step);
                    step = cameFrom[step];
                }
                return path;
            }
            Unit* occupant = m_board.getUnitAt(next);
            if(occupant && occupant!=self)continue;//排除格子上有人或者回到起点
            cameFrom[next] = cur;
            visited.insert(next);
            queue.enqueue(next);
        }
    }

    return {};
}

/////////////////////////////////////////////////////////////////////////
//结算阶段
void Game::checkCombatEnd()
{
    if (m_phase != Phase::Combat) return;

    bool hasPlayer = false;
    bool hasEnemy = false;

    for (Unit* unit : m_units) {
        if (!unit || unit->isDead()) continue;
        if (unit->position().y() == -1) continue; // bench 单位不参战

        if (unit->owner() == Unit::Owner::PlayerCtrl)
            hasPlayer = true;
        else
            hasEnemy = true;

        if (hasPlayer && hasEnemy) return; // 双方都还有人
    }

    m_combatTimer->stop();
    m_phase = Phase::Resolve;
    m_traitManager.clearAll(m_units);    /* 战斗结束清除羁绊 */

    // 清除所有飞行中的子弹
    for (Unit* unit : m_units) {
        if (unit) {
            unit->clearProjectiles();
            if (unit->state() == Unit::State::Casting) {
                unit->cancelCasting();
                unit->setState(Unit::State::Idle);
            }
        }
    }

    if (!hasPlayer) {
        //现在是默认平局也扣血
        int damage = 2 + m_round;
        m_player.setHp(m_player.Hp() - damage);
        m_lastCombatWon = false;
        SoundManager::instance()->play("defeat");
        emit combatEnded(false, damage);
        if (m_player.isDead())
            emit gameOver();
    } else {
        m_lastCombatWon = true;
        SoundManager::instance()->play("victory");
        int gold = 5 + qMin(m_player.Gold() / 10, 3) + 3;  /* 基础+利息+胜奖 */
        emit combatEnded(true, gold);
    }

    emit phaseChanged();
    emit playerInfoChanged();
}

void Game::nextRound()
{
    // 清除所有敌方单位及其 UnitItem
    for (int i = m_units.size() - 1; i >= 0; --i) {
        //倒着删时，被删元素后面的索引变化不影响还没遍历到的前面的元素。

        if (m_units[i] && m_units[i]->owner() == Unit::Owner::EnemyCtrl) {
            int uid = m_units[i]->id();
            // 清理对应的 UnitItem

            // 1) 查这个敌方有没有对应的UnitItem（场景中的图形项）
            auto it = m_unitItemById.find(uid);
            if (it != m_unitItemById.end()) {

                // 2) 从 QGraphicsScene中移除（否则绘制时引用野指针崩溃）
                m_scene->removeItem(it->second);

                // 3) 从 m_unitItems 向量中删掉
                //    std::remove 把匹配元素移到末尾 →erase 真正删除
                m_unitItems.erase(std::remove(m_unitItems.begin(), m_unitItems.end(), it->second), m_unitItems.end());

                // 4) 释放 UnitItem 内存
                delete it->second;

                // 5) 从 id→UnitItem 映射表删掉
                m_unitItemById.erase(it);
            }

            // 6) 释放 Unit 对象（逻辑数据）
            delete m_units[i];

            // 7) 从 m_units 列表删掉
            m_units.removeAt(i);
        }
    }

    // 清除场上残留子弹
    for (Unit* unit : m_units) {
        if (unit) unit->clearProjectiles();
    }

    m_round++;
    m_equipDropCount = 0;  /* 每轮重置掉落上限 */
    m_player.roundIncome(m_lastCombatWon);
    spawnEnemiesForRound(m_round);

    // 为新生成的敌方创建 UnitItem
    for (Unit* unit : m_units) {
        if (m_unitItemById.find(unit->id()) == m_unitItemById.end()) {
            // find() == end() 表示这个 Unit 在映射表里不存在
            // 也就是说它是刚刚 spawnEnemiesForRound() 新建的

            // 1) 创建 UnitItem，传入 Unit* 建立关联
            UnitItem* unitItem = new UnitItem(unit);
            unitItem->setZValue(kZUnit);
            // 2) 注册到 scene（不调用 addItem 就不会显示）
            m_scene->addItem(unitItem);
            // 3) 注册到 m_unitItems（syncUnits需要遍历它来更新位置/可见性）
            m_unitItems.push_back(unitItem);
            // 4) 建立 id → UnitItem 的快速查找
            m_unitItemById[unit->id()] = unitItem;
            // 5) 连接拖拽信号（敌方不需要拖拽，但 connect无害，实测拖不动，应该是有逻辑检查）
            connect(unitItem, &UnitItem::dragStarted, this, &Game::handleDragStarted);
            connect(unitItem, &UnitItem::dragMoved, this, &Game::handleDragMoved);
            connect(unitItem, &UnitItem::dragDropped, this, &Game::handleDropCommand);
            connect(unitItem, &UnitItem::equipClicked, this, &Game::onUnitEquipClicked);
        }
    }

    reset();
    saveToFile();
    emit roundChanged();
    emit phaseChanged();
}

void Game::spawnEnemiesForRound(int round)
{
    struct Slot { int tmpl; int count; };
    QVector<Slot> wave;
    QVector<QPoint> positions;  // 每轮每个敌人的站位

    switch (round) {
    case 1:  wave = {{2,1}, {0,2}}; break;                        // 1弩手 2盾卫
    case 2:  wave = {{2,2}, {0,2}}; break;                        // 2弩手 2盾卫
    case 3:  wave = {{1,2}, {0,2}}; break;                        // 2刺客 2盾卫
    case 4:  wave = {{2,2}, {1,1}, {0,2}}; break;                 // 2弩手 1刺客 2盾卫
    case 5:  wave = {{3,1}, {4,1}, {1,2}, {0,1}}; break;          // 1术士 1祭司 2刺客 1盾卫
    case 6:  wave = {{3,1}, {2,2}, {1,2}, {0,2}}; break;          // 1术士 2弩手 2刺客 2盾卫
    case 7:  wave = {{3,2}, {2,2}, {1,2}, {0,2}}; break;          // 2术士 2弩手 2刺客 2盾卫
    default: wave = {{3,2}, {2,2}, {4,1}, {1,2}, {0,1}}; break;   // R8+: 满编8人
    }

    /* 每轮设不同阵型，让远程在后排、近战在前排，不再挤成一团 */
    switch (round) {
    case 1:  positions = {{3,1}, {1,3}, {6,3}}; break;
    case 2:  positions = {{1,0}, {6,0}, {2,2}, {5,2}}; break;
    case 3:  positions = {{1,1}, {6,1}, {3,3}, {4,3}}; break;
    case 4:  positions = {{2,0}, {5,0}, {3,1}, {3,3}, {4,3}}; break;
    case 5:  positions = {{3,0}, {4,0}, {1,2}, {6,2}, {3,3}}; break;
    case 6:  positions = {{3,0}, {1,0}, {6,0}, {1,2}, {6,2}, {3,3}, {4,3}}; break;
    case 7:  positions = {{2,0}, {5,0}, {1,1}, {6,1}, {1,2}, {6,2}, {3,3}, {4,3}}; break;
    default: positions = {{2,0}, {5,0}, {1,1}, {6,1}, {3,1}, {1,2}, {6,2}, {3,3}}; break;
    }

    for (const Slot& s : wave)
        for (int j = 0; j < s.count; j++)
            m_units.append(createEnemyFromTemplate(kEnemyTemplates[s.tmpl], round));

    /* 给刚创建的单位写入站位，避免 reset() 时全部挤到左边 */
    int pi = 0;
    for (Unit* u : m_units) {
        if (!u || u->owner() != Unit::Owner::EnemyCtrl) continue;
        if (u->position().x() != 0 || u->position().y() != 0) continue;  /* 已经设过 */
        if (pi < positions.size())
            u->setPosition(positions[pi++]);
    }
}

// ======================== 存档系统 ========================

static QString saveFilePath()
{
    // 用户文档目录 + Synera 子目录，任何电脑都能读写，也容易找到
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Synera";
    QDir().mkpath(dir);
    return dir + "/save.json";
}

void Game::saveToFile() const
{
    QJsonObject root;
    root["round"] = m_round;
    root["lastCombatWon"] = m_lastCombatWon;
    root["equipDropCount"] = m_equipDropCount;

    /* 玩家状态 */
    QJsonObject p;
    p["hp"] = m_player.Hp();
    p["gold"] = m_player.Gold();
    p["level"] = m_player.Level();
    p["popCap"] = m_player.PopulationCap();
    root["player"] = p;

    /* 单位：只保存玩家单位 */
    QJsonArray unitsArr;
    for (Unit* u : m_units) {
        if (!u || u->owner() != Unit::Owner::PlayerCtrl) continue;

        QJsonObject uo;
        uo["type"] = u->name();                  /* "灰姑娘" → HeroFactory 可重建 */
        uo["pos"] = QJsonObject{{"x", u->position().x()}, {"y", u->position().y()}};
        uo["starLevel"] = u->starLevel();
        uo["cost"] = u->cost();
        uo["hp"] = u->hp();
        uo["mana"] = u->mana();

        /* 装备槽 */
        QJsonArray eqArr;
        for (int s = 0; s < 2; s++)
            eqArr.append(u->equipment(s) ? u->equipment(s)->id : -1);
        uo["equipment"] = eqArr;

        unitsArr.append(uo);
    }
    root["units"] = unitsArr;

    /* 装备栏 */
    QJsonArray poolArr;
    for (const Equipment& eq : m_equipPool)
        poolArr.append(eq.id);
    root["equipPool"] = poolArr;

    QFile f(saveFilePath());
    f.open(QIODevice::WriteOnly);
    f.write(QJsonDocument(root).toJson());
}

bool Game::loadFromFile()
{
    static const QList<Equipment> kPool = createEquipmentPool();  /* 按 id 查装备数据 */

    QFile f(saveFilePath());
    if (!f.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;
    QJsonObject root = doc.object();

    /* 清空现有状态 */
    qDeleteAll(m_units);
    m_units.clear();
    for (UnitItem* ui : m_unitItems) { m_scene->removeItem(ui); delete ui; }
    m_unitItems.clear();
    m_unitItemById.clear();
    for (EquipmentItem* ei : m_equipItems) { m_scene->removeItem(ei); delete ei; }
    m_equipItems.clear();
    m_equipPool.clear();

    /* 恢复轮次和玩家 */
    m_round = root["round"].toInt(1);
    m_lastCombatWon = root["lastCombatWon"].toBool(false);
    m_equipDropCount = root["equipDropCount"].toInt(0);

    QJsonObject p = root["player"].toObject();
    m_player.setHp(p["hp"].toInt(100));
    m_player.setGold(p["gold"].toInt(100));
    m_player.setLevel(p["level"].toInt(1));
    m_player.setPopulationCap(p["popCap"].toInt(3));

    /* 重建单位 */
    for (const QJsonValue& v : root["units"].toArray()) {
        QJsonObject uo = v.toObject();
        QString type = uo["type"].toString();
        Unit* u = HeroFactory::createHero(type);
        if (!u) continue;                        /* 不认识的名字，跳过 */

        u->setOwner(Unit::Owner::PlayerCtrl);

        QJsonObject posObj = uo["pos"].toObject();
        u->setPosition(QPoint(posObj["x"].toInt(), posObj["y"].toInt()));

        int star = uo["starLevel"].toInt(1);
        u->setStarLevel(star);
        u->setStar1BaseHp(u->maxHp());           /* Hero 构造时写好了 1星基准 */
        u->setStar1BaseAtk(u->atk());
        if (star >= 2) {
            float mult = (star == 2) ? 1.8f : 1.8f * 1.8f;
            u->setMaxHp(static_cast<int>(u->star1BaseHp() * mult));
            u->setAtk(static_cast<int>(u->star1BaseAtk() * mult));
        }
        u->setHp(uo["hp"].toInt());
        u->setMana(uo["mana"].toInt());
        u->setCost(uo["cost"].toInt(0));

        /* 恢复装备 */
        QJsonArray eqArr = uo["equipment"].toArray();
        for (int s = 0; s < 2 && s < eqArr.size(); s++) {
            int eqId = eqArr[s].toInt(-1);
            for (const Equipment& eq : kPool) {
                if (eq.id == eqId) { u->equip(&eq); break; }
            }
        }

        m_units.append(u);
    }

    /* 生成敌人 + 重置布阵 */
    spawnEnemiesForRound(m_round);
    buildScene();
    // Board/Bench 此时为空，reset() 会重新摆放
    reset();

    /* 恢复装备栏（必须在 buildScene 之后，否则会被 scene->clear() 清掉） */
    QJsonArray poolArr = root["equipPool"].toArray();
    for (const QJsonValue& v : poolArr) {
        int eqId = v.toInt(-1);
        for (const Equipment& eq : kPool)
            if (eq.id == eqId) { addEquipment(eq); break; }
    }

    return true;
}
























