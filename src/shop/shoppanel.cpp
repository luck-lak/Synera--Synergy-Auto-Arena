#include "shoppanel.h"
#include "entity/player.h"
#include "core/game.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QShowEvent>
#include <QResizeEvent>
#include <QRandomGenerator>
#include "herocard.h"
#include "herodetailpanel.h"
#include "core/herofactory.h"
#include "entity/unit.h"
#include "audio/soundmanager.h"

ShopPanel::ShopPanel(Player* player, Game* game, QWidget* parent)
    : QWidget(parent)
    , m_player(player)
    , m_game(game)
{
    // 覆盖层：与父窗口等大，半透明暗色背景
    setStyleSheet(R"(
        ShopPanel {
            background-color: rgba(0, 0, 0, 160);
        }
    )");
    hide();

    // 商店卡片容器
    m_card = new QWidget(this);
    m_card->setFixedSize(600, 420);
    m_card->setObjectName("shopCard");
    m_card->setStyleSheet(R"(
        #shopCard {
            background-color: #2a2a2a;
            border: 1px solid #555;
            border-radius: 8px;
        }
    )");

    auto* cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(16, 12, 16, 16);
    cardLayout->setSpacing(10);

    // 标题栏
    auto* titleBar = new QWidget(m_card);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel(QStringLiteral("— 商店 —"), titleBar);
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f1c40f;");

    m_refreshButton = new QPushButton(QStringLiteral("刷新"), titleBar);
    m_refreshButton->setFixedHeight(28);
    m_refreshButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;  /* 深灰底 */
            border: 1px solid #5a5a5a;  /* 灰边框 */
            border-radius: 3px;
            color: #ccc;                /* 浅灰字 */
            font-size: 12px;
            padding: 2px 10px;
        }
        QPushButton:hover {
            background-color: #555;
            color: #fff;
        }
    )");
    connect(m_refreshButton, &QPushButton::clicked, this, &ShopPanel::onRefreshClicked);

    m_closeButton = new QPushButton(QStringLiteral("X"), titleBar);
    m_closeButton->setFixedSize(36, 36);
    m_closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3f3f3f;   /* 深灰底 */
            border: 1px solid #666;       /* 灰色边框 */
            border-radius: 4px;
            color: #f0f0f0;               /* 白色文字 */
            font-size: 24px;
            font-weight: bold;
            padding: 0px;
        }
        QPushButton:hover {
            background-color: #555;
            color: #fff;
        }
    )");
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::hide);

    titleLayout->addStretch();
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_refreshButton);
    titleLayout->addWidget(m_closeButton);

    cardLayout->addWidget(titleBar);

    // 卡片区域（水平排列）
    m_cardsContainer = new QWidget(m_card);
    auto* cardsLayout = new QHBoxLayout(m_cardsContainer);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(12);
    cardsLayout->addStretch();
    cardLayout->addWidget(m_cardsContainer);

    cardLayout->addStretch();
}

void ShopPanel::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (parentWidget())
        setGeometry(parentWidget()->rect());
}

void ShopPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    int x = (width() - m_card->width()) / 2;
    int y = (height() - m_card->height()) / 2;
    m_card->move(x, y);
}

void ShopPanel::refreshGoods()
{
    // 清旧卡片
    for (HeroCard* card : m_heroCards) {
        m_cardsContainer->layout()->removeWidget(card);
        delete card;
    }
    m_heroCards.clear();
    m_goods.clear();

    // 英雄池
    const QList<ShopGoods> pool = {
        {QStringLiteral("灰姑娘"), 4, QStringLiteral("金冕盟约"), false,
         250, 8, 3,
         QStringLiteral("水晶鞋"),
         QStringLiteral("对目标造成 150 伤害"), 100,
         QStringLiteral("\"她比任何人都更需要相信那段故事是有意义的。\"")},
        {QStringLiteral("白雪"), 4, QStringLiteral("金冕盟约"), false,
         600, 8, 3,
         QStringLiteral("玻璃棺封印"),
         QStringLiteral("封印目标 2 秒（无法行动/无法被攻击）"), 80,
         QStringLiteral("\"她死过一次又醒来，那段沉睡让她比任何人都更懂得边界地带的样子。\"")},
        {QStringLiteral("睡美人"), 4, QStringLiteral("金冕盟约"), false,
         400, 10, 3,
         QStringLiteral("百年沉睡"),
         QStringLiteral("沉睡目标 3 秒（无法行动，可被攻击）"), 90,
         QStringLiteral("\"她的一生都是被诅咒塑造的，但她走完了。诅咒是可以等待的，荆棘是可以穿越的。\"")},
        {QStringLiteral("大灰狼"), 3, QStringLiteral("野血盟约/异种"), false,
         300, 15, 1,
         QStringLiteral("撕碎剧本"),
         QStringLiteral("自身攻速+50%、移速+50%，持续 5 秒"), 100,
         QStringLiteral("\"他不是因为没有选择而死，他是因为被写成反派而死。\n这个区别，对他就是一切。\"")},
        {QStringLiteral("人鱼"), 4, QStringLiteral("野血盟约/异种"), false,
         280, 10, 3,
         QStringLiteral("泡沫化身"),
         QStringLiteral("自身无敌 2 秒 + 周围 2 格敌人扣 100 血"), 120,
         QStringLiteral("\"她放弃了声音，忍受了每一步的疼痛；最后得到的是消失。\n现在她不能说话，但沉默本身就是一种语言。\"")},
        {QStringLiteral("魔镜"), 3, QStringLiteral("烬墨之裔/异种"), false,
         350, 6, 3,
         QStringLiteral("照见本质"),
         QStringLiteral("目标受到伤害 +25%，持续 3 秒"), 80,
         QStringLiteral("\"它有意识，有语言，能看穿本质——但它不是角色，它是道具。\"")},
        {QStringLiteral("小火苗"), 5, QStringLiteral("烬墨之裔"), false,
         200, 15, 3,
         QStringLiteral("最后一把火柴"),
         QStringLiteral("全屏敌方扣 120 血"), 140,
         QStringLiteral("\"她只是在寒冷里一根一根划火柴，然后消失了。\n她的故事从一开始就不是为了让她活下去写的。\"")},
        {QStringLiteral("无邀者"), 5, QStringLiteral("烬墨之裔"), false,
         350, 10, 3,
         QStringLiteral("空白结局"),
         QStringLiteral("目标 DOT 每帧 12 伤害，持续 4 秒"), 120,
         QStringLiteral("\"执笔者写到她时犹豫了，她的结局那页是空白的。\n她游荡了数个纪元，看着别的角色走完自己的故事。\"")},
    };

    // 随机选 3 个
    QList<int> indices;
    for (int i = 0; i < pool.size(); i++)
        indices.append(i);

    for (int i = 0; i < 3 && !indices.isEmpty(); i++) {
        int pick = QRandomGenerator::global()->bounded(indices.size());
        int idx = indices.takeAt(pick);
        m_goods.append(pool[idx]);
    }

    // 创建卡片
    auto* cardsLayout = qobject_cast<QHBoxLayout*>(m_cardsContainer->layout());
    QLayoutItem* oldStretch = cardsLayout->takeAt(cardsLayout->count() - 1);
    delete oldStretch;

    for (const ShopGoods& g : m_goods) {
        auto* card = new HeroCard(g, m_cardsContainer);
        connect(card, &HeroCard::clicked, this, &ShopPanel::onHeroClicked);
        connect(card, &HeroCard::detailClicked, this, &ShopPanel::onDetailClicked);
        cardsLayout->addWidget(card);
        m_heroCards.append(card);
    }

    cardsLayout->addStretch();
}

void ShopPanel::onHeroClicked(const QString& heroName)
{
    // 找到对应的 goods
    ShopGoods* goods = nullptr;
    for (ShopGoods& g : m_goods) {
        if (g.heroName == heroName && !g.purchased) {
            goods = &g;
            break;
        }
    }
    if (!goods) return;

    // 检查金币是否足够
    if (m_player->Gold() < goods->cost) return;

    // 检查 bench 或棋盘是否有空位
    if (!m_game->canPlaceUnit()) return;

    // 扣钱
    m_player->changeGold(-goods->cost);

    // 创建英雄
    Unit* hero = HeroFactory::createHero(heroName);
    if (!hero) return;

    hero->setOwner(Unit::Owner::PlayerCtrl);
    hero->setCost(goods->cost);
    m_game->addPlayerUnit(hero);
    SoundManager::instance()->play("buy");

    // 标记已购买
    goods->purchased = true;

    // 卡片变灰
    for (HeroCard* card : m_heroCards) {
        if (card->heroName() == heroName)
            card->setPurchased(true);
    }

    emit m_game->playerInfoChanged();
}

void ShopPanel::onDetailClicked(const ShopGoods& goods)
{
    auto* detail = new HeroDetailPanel(goods, this);
    // 显示在遮罩中央偏上
    QPoint center = mapToGlobal(rect().center());
    center.setY(center.y() - 80);
    detail->move(center - QPoint(detail->width() / 2, detail->height() / 2));
    detail->show();
}

void ShopPanel::onRefreshClicked()
{
    int cost = (m_refreshCount < 3) ? 1 : 2;  /* 前3次1金，之后2金 */

    if (m_player->Gold() < cost) return;       /* 不够钱不刷新 */

    m_player->changeGold(-cost);
    m_refreshCount++;
    refreshGoods();

    emit m_game->playerInfoChanged();          /* 通知金币面板更新 */
}

void ShopPanel::resetRefreshCount()
{
    m_refreshCount = 0;
}
