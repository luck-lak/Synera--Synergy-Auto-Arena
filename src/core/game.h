#ifndef CORE_GAME_H
#define CORE_GAME_H

#include <QObject>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <unordered_map>
#include <vector>
#include <QVector>
#include "board.h"

#include"bench.h"
#include"entity/player.h"
#include"boardgeometry.h"
#include"traitmanager.h"
#include "equipment/equipment.h"

#include <QTimer>


class Unit;
class QGraphicsScene;
class GridItem;
class UnitItem;
class EquipmentItem;


class Game : public QObject
{
    Q_OBJECT

//time
private:
    QTimer* m_combatTimer;
    int m_combatFrame = 0;
private slots:
    void onCombatTick();

//phase
public:
    enum class Phase{
        Prep,//准备阶段：可以拖拽布阵
        Combat,// 战斗阶段：AI接管，不可拖拽
        Resolve//结算阶段：显示结果
    };
    Phase currentPhase() const{return m_phase;}
    void setPhase(Phase p){m_phase = p;}
    void startCombat();
    int round() const { return m_round; }
    void nextRound();

private:
    Phase m_phase = Phase::Prep;
//结算阶段
    int m_round = 1;
    bool m_lastCombatWon = false;
    void checkCombatEnd();
    void spawnEnemiesForRound(int round);


//寻路
QVector<QPoint> findpath(const QPoint& start, const QPoint &end,Unit* self);

//
signals:
    void playerInfoChanged();
    void phaseChanged();
    void combatEnded(bool playerWon, int value);  // value = damage taken or gold earned
    void roundChanged();
    void unitSelected(Unit* unit);  // 右键点击角色，弹出详情面板
    void gameOver();                 // 玩家血量归零，游戏结束

public:
    explicit Game(QObject* parent = nullptr);
    ~Game();

    void initialize();
    void reset();
    Player* player(){return &m_player;}

    QGraphicsScene* scene() const { return m_scene; }
    BoardGeometry* boardGeometry() { return &m_geometry; }
    const QList<Unit*>& allUnits() const { return m_units; }

    void handleDragStarted(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);
    void handleDragMoved(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);
    void handleDropCommand(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);

    void addPlayerUnit(Unit* unit);
    void sellUnit(int unitId);
    void tryMergeStarUp(Unit* justAdded);
    bool canPlaceUnit() const;

    // 装备
    void addEquipment(const Equipment& eq);
    void tryDropEquipment();
    void onEquipDragStarted(EquipmentItem* item);
    void onEquipDragDropped(EquipmentItem* item, const QPointF& scenePos);
    void onUnitEquipClicked(int unitId, const QPointF& scenePos);

    // 存档
    void saveToFile() const;
    bool loadFromFile();


private:
    void createStarterUnitsIfNeeded();//示例，后续加入商店就要去掉了
    Unit* findUnitById(int unitId) const;
    GridItem* findGridItem(const QPoint& gridPos) const;
    UnitItem* findUnitItem(int unitId) const;
    void clearGridHighlights();
    bool canApplyDrop(int unitId, const QPoint& source, const QPoint& target) const;
    void applyDrop(int unitId, const QPoint& target);
    void buildScene();
    void syncUnits();

    void executeMovement(Unit* unit);
    void executeAttack(Unit* unit);

    BoardGeometry m_geometry;
    Board m_board;
    Bench m_bench;
    Player m_player;
    QList<Unit*> m_units;

    QGraphicsScene* m_scene;
    std::vector<GridItem*> m_gridItems;
    std::vector<UnitItem*> m_unitItems;

    bool m_dragActive;
    int m_activeUnitId;
    QPoint m_sourceGrid;
    std::unordered_map<int, UnitItem*> m_unitItemById;
    QRectF m_sellZoneRect;
    QHash<int, QPoint> m_preCombatPositions;  // unitId → 开战前位置，reset 时复原
    TraitManager m_traitManager;

    // 装备
    QList<Equipment> m_equipPool;          // 未穿戴装备
    QList<EquipmentItem*> m_equipItems;
    int m_equipDropCount = 0;
    static constexpr int kMaxEquipDrops = 2;
    int m_equipDragFromUnit = -1;  // 从角色身上拖装备时的来源 unitId
    QPointF m_equipDragHome;       // 拖拽起点，失败时归位

    //
};

#endif // CORE_GAME_H
