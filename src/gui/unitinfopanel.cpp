#include "unitinfopanel.h"
#include "entity/unit.h"
#include "core/game.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>

// 升星属性倍率：与 tryMergeStarUp 保持一致
static float starMult(int star)
{
    if (star == 2) return 1.8f;
    if (star == 3) return 1.8f * 1.8f;
    return 1.0f;
}

UnitInfoPanel::UnitInfoPanel(Game* game, QWidget* parent)
    : QWidget(parent)
    , m_game(game)
{
    setFixedWidth(190);
    hide();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // 标题：名字 + 星标
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #f1c40f;");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_nameLabel);

    auto* sep1 = new QLabel(this);
    sep1->setFixedHeight(1);
    sep1->setStyleSheet("background-color: #444;");
    layout->addWidget(sep1);

    // 属性区：HP / Mana / ATK 及分解
    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("font-size: 11px; color: #ddd;");
    layout->addWidget(m_statsLabel);

    auto* sep2 = new QLabel(this);
    sep2->setFixedHeight(1);
    sep2->setStyleSheet("background-color: #444;");
    layout->addWidget(sep2);

    // 派系
    m_traitLabel = new QLabel(this);
    m_traitLabel->setStyleSheet("font-size: 11px; color: #aaa;");
    m_traitLabel->setWordWrap(true);
    layout->addWidget(m_traitLabel);

    auto* sep3 = new QLabel(this);
    sep3->setFixedHeight(1);
    sep3->setStyleSheet("background-color: #444;");
    layout->addWidget(sep3);

    // 装备信息
    m_equipLabel1 = new QLabel(this);
    m_equipLabel1->setStyleSheet("font-size: 11px; color: #bbb;");
    m_equipLabel1->setWordWrap(true);
    layout->addWidget(m_equipLabel1);

    m_equipLabel2 = new QLabel(this);
    m_equipLabel2->setStyleSheet("font-size: 11px; color: #bbb;");
    m_equipLabel2->setWordWrap(true);
    layout->addWidget(m_equipLabel2);

    layout->addStretch();

    setStyleSheet(R"(
        UnitInfoPanel {
            background-color: #252525;
            border-right: 2px solid #3a3a3a;
        }
    )");
}

void UnitInfoPanel::showUnit(Unit* unit)
{
    m_unit = unit;
    if (!unit) { hide(); return; }
    refresh();
    show();
    raise();
}

void UnitInfoPanel::hidePanel()
{
    m_unit = nullptr;
    hide();
}

void UnitInfoPanel::refresh()
{
    if (!m_unit) return;

    /*
     * 属性来源分解：
     *   当前值 = 基础值（1星基准 × 升星倍率） + 羁绊加成 + 装备加成
     *
     *   baseAtk / baseHp：从 star1Base 按星数倍率反推 ← 这和升星时 tryMergeStarUp 的公式相同
     *   equipAtk / equipHp：从装备数据直接读
     *   traitAtk / traitHp：总当前值 - 基础 - 装备 = 羁绊部分（可能是金冕HP或异种ATK，也可能等于0）
     */
    float mult = starMult(m_unit->starLevel());
    int baseAtk = static_cast<int>(m_unit->star1BaseAtk() * mult);   // 没任何加成的"干净"攻击力
    int baseHp  = static_cast<int>(m_unit->star1BaseHp() * mult);
    int equipAtk = (m_unit->equipment(0) ? m_unit->equipment(0)->atkBonus : 0)
                 + (m_unit->equipment(1) ? m_unit->equipment(1)->atkBonus : 0);
    int equipHp  = (m_unit->equipment(0) ? m_unit->equipment(0)->hpBonus : 0)
                 + (m_unit->equipment(1) ? m_unit->equipment(1)->hpBonus : 0);
    int traitAtk = m_unit->atk() - baseAtk - equipAtk;    // 当前 - 基础 - 装备 = 羁绊
    int traitHp  = m_unit->maxHp() - baseHp - equipHp;

    QString stars = QStringLiteral("★").repeated(m_unit->starLevel());
    m_nameLabel->setText(QString("%1 %2").arg(m_unit->name()).arg(stars));

    QStringList statLines;
    statLines << QString("HP:  %1 / %2").arg(m_unit->hp()).arg(m_unit->maxHp());
    statLines << QString("(基础 %1  +羁绊 %2  +装备 %3)").arg(baseHp).arg(traitHp).arg(equipHp);
    statLines << "";
    statLines << QString("Mana: %1 / %2").arg(m_unit->mana()).arg(m_unit->maxMana());
    statLines << "";
    statLines << QString("ATK: %1").arg(m_unit->atk());
    statLines << QString("(基础 %1  +羁绊 %2  +装备 %3)").arg(baseAtk).arg(traitAtk).arg(equipAtk);
    statLines << "";
    statLines << QString("射程: %1   回蓝: %2").arg(m_unit->range()).arg(m_unit->manaRecoveryAmount());
    statLines << QString("攻速: %1帧   移速: %2帧").arg(m_unit->attackSpeed()).arg(m_unit->moveSpeed());
    m_statsLabel->setText(statLines.join("\n"));

    // 派系列表：可能有多个（如大灰狼=野血盟约+异种）
    QStringList traitStrs;
    for (const QString& t : m_unit->traits())
        traitStrs << t;
    m_traitLabel->setText(QString("派系: %1").arg(traitStrs.join(", ")));

    // 装备槽：每个独立显示，点击卸下
    auto setEquipLabel = [](QLabel* lb, const Equipment* e) {
        if (e) {
            lb->setText(QString("— 装备 —\n%1\nATK+%2  HP+%3\n点击卸下")
                .arg(e->name).arg(e->atkBonus).arg(e->hpBonus));
            lb->setStyleSheet(R"(
                font-size: 11px; color: #f1c40f;
                background-color: #3a3a3a;
                border: 1px solid #666;
                border-radius: 4px;
                padding: 6px;
            )");
            lb->setCursor(Qt::PointingHandCursor);
        } else {
            lb->setText(QStringLiteral("装备槽: 空"));
            lb->setStyleSheet("font-size: 11px; color: #555;");
            lb->setCursor(Qt::ArrowCursor);
        }
    };
    setEquipLabel(m_equipLabel1, m_unit->equipment(0));
    setEquipLabel(m_equipLabel2, m_unit->equipment(1));
}

void UnitInfoPanel::mousePressEvent(QMouseEvent* event)
{
    // 点击装备槽 → 卸下该槽位装备并放回装备栏
    auto clicked = [&](int slot, QLabel* lb) -> bool {
        if (!m_unit || !m_unit->equipment(slot)) return false;
        if (!lb->geometry().contains(event->pos())) return false;

        Equipment eq = *m_unit->equipment(slot);
        m_unit->unequip(slot);
        m_game->addEquipment(eq);
        refresh();
        return true;
    };

    if (clicked(0, m_equipLabel1)) return;
    if (clicked(1, m_equipLabel2)) return;
    QWidget::mousePressEvent(event);
}
