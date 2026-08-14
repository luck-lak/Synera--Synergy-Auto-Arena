#include "playerinfopanel.h"
#include "entity/player.h"
#include "entity/unit.h"
#include "core/game.h"
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>

PlayerInfoPanel::PlayerInfoPanel(Player* player, Game* game, QWidget* parent)
    : QWidget(parent)
    , m_player(player)
    , m_game(game)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    // 标题
    m_titleLabel = new QLabel(QStringLiteral("— 玩家状态 —"), this);
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0e0;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_titleLabel);

    // HP
    m_hpLabel = new QLabel(this);
    layout->addWidget(m_hpLabel);

    m_hpBar = new QProgressBar(this);
    m_hpBar->setMaximum(m_player->Hp());
    m_hpBar->setValue(m_player->Hp());
    m_hpBar->setTextVisible(true);
    m_hpBar->setMaximumHeight(14);
    m_hpBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #555;
            border-radius: 3px;
            background: #1e1e1e;
            color: #e0e0e0;
            font-size: 10px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #c0392b, stop:0.5 #e74c3c, stop:1 #c0392b);
            border-radius: 2px;
        }
    )");
    layout->addWidget(m_hpBar);

    // Gold
    m_goldLabel = new QLabel(this);
    m_goldLabel->setStyleSheet("color: #f1c40f; font-size: 13px;");
    layout->addWidget(m_goldLabel);

    // Level
    m_levelLabel = new QLabel(this);
    layout->addWidget(m_levelLabel);

    // 轮次
    m_roundLabel = new QLabel(this);
    layout->addWidget(m_roundLabel);

    // 羁绊状态
    m_traitLabel1 = new QLabel(this);
    m_traitLabel2 = new QLabel(this);
    m_traitLabel3 = new QLabel(this);
    m_traitLabel4 = new QLabel(this);
    layout->addWidget(m_traitLabel1);
    layout->addWidget(m_traitLabel2);
    layout->addWidget(m_traitLabel3);
    layout->addWidget(m_traitLabel4);

    layout->addStretch();

    // 升级按钮
    m_levelUpButton = new QPushButton(this);
    m_levelUpButton->setCursor(Qt::PointingHandCursor);
    m_levelUpButton->setStyleSheet(R"(
        QPushButton {
            background-color: #2a3a5c;
            border: 1px solid #4a7ab5;
            border-radius: 4px;
            color: #8ab4f8;
            font-size: 12px;
            padding: 4px 8px;
        }
        QPushButton:hover {
            background-color: #3a4a6c;
            color: #bcd4ff;
        }
    )");
    connect(m_levelUpButton, &QPushButton::clicked, this, [this]() {
        if (m_player->tryLevelUp()) {
            emit m_game->playerInfoChanged();
        }
    });
    layout->addWidget(m_levelUpButton);

    // 商店按钮
    m_shopButton = new QPushButton(QStringLiteral("商店"), this);
    connect(m_shopButton, &QPushButton::clicked, this, &PlayerInfoPanel::shopRequested);
    layout->addWidget(m_shopButton);

    setFixedWidth(180);
    setStyleSheet(R"(
        PlayerInfoPanel {
            background-color: #252525;
            border-left: 2px solid #3a3a3a;
        }
    )");
}

void PlayerInfoPanel::refresh()
{
    m_hpLabel->setText(QString("HP: %1 / %2").arg(m_player->Hp()).arg(100));
    m_hpBar->setMaximum(100);
    m_hpBar->setValue(qBound(0, m_player->Hp(), 100));  /* clamp 防止负值闪烁 */

    m_goldLabel->setText(QString("Gold: %1").arg(m_player->Gold()));

    m_levelLabel->setText(QString("Lv.%1  POP:%2/%3")
        .arg(m_player->Level())
        .arg(m_player->PopulationUsed())
        .arg(m_player->PopulationCap()));

    // 升级按钮
    if (m_player->Level() >= Player::kMaxLevel) {
        m_levelUpButton->setText(QStringLiteral("满级"));
        m_levelUpButton->setEnabled(false);
    } else {
        int cost = m_player->levelUpCost();
        m_levelUpButton->setText(QString("升级 (%1 G)").arg(cost));
        m_levelUpButton->setEnabled(m_player->Gold() >= cost);
    }

    m_roundLabel->setText(QString("Round: %1").arg(m_game->round()));

    // 羁绊统计
    QHash<QString, int> traitCounts;
    for (Unit* u : m_game->allUnits()) {
        if (!u || u->isDead()) continue;
        if (u->position().y() < 0) continue;
        if (u->owner() != Unit::Owner::PlayerCtrl) continue;
        for (const QString& t : u->traits())
            traitCounts[t]++;
    }

    auto traitLabel = [&](const QString& display, const QString& traitName,
                           int min2, int min3,
                           const QString& eff2, const QString& eff3) -> QString {
        int c = traitCounts.value(traitName, 0);

        if (min3 && c >= min3)
            return QString("%1 %2: %3").arg(display).arg(c).arg(eff3);
        if (min2 && c >= min2)
            return QString("%1 %2: %3").arg(display).arg(c).arg(eff2);
        return QString("%1 %2: 未激活").arg(display).arg(c);
    };

    m_traitLabel1->setText(traitLabel(
        QStringLiteral("金冕"), QStringLiteral("金冕盟约"), 2, 3,
        QStringLiteral("+150HP"), QStringLiteral("+300HP")));
    m_traitLabel2->setText(traitLabel(
        QStringLiteral("野血"), QStringLiteral("野血盟约"), 2, 0,
        QStringLiteral("攻速+25%移速+15%"), QString()));
    m_traitLabel3->setText(traitLabel(
        QStringLiteral("烬墨"), QStringLiteral("烬墨之裔"), 2, 3,
        QStringLiteral("回蓝+3"), QStringLiteral("回蓝+6")));

    m_traitLabel4->setText(traitLabel(
        QStringLiteral("异种"), QStringLiteral("异种"), 2, 3,
        QStringLiteral("+10ATK"), QStringLiteral("+20ATK +15%移速")));
    m_traitLabel4->setStyleSheet("color: #27ae60; font-size: 12px;");

    m_traitLabel1->setStyleSheet("color: #d4a017; font-size: 12px;");
    m_traitLabel2->setStyleSheet("color: #c0392b; font-size: 12px;");
    m_traitLabel3->setStyleSheet("color: #7d3c98; font-size: 12px;");
}
