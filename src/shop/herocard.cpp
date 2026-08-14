#include "herocard.h"
#include "shopgoods.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QCoreApplication>
#include <QFileInfo>
#include <QPixmap>

HeroCard::HeroCard(const ShopGoods& goods, QWidget* parent)
    : QWidget(parent)
    , m_goods(goods)
{
    setFixedSize(160, 220);
    setCursor(Qt::PointingHandCursor);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 10, 8, 10);
    layout->setSpacing(6);

    // 头像占位
    m_imageLabel = new QLabel(this);
    m_imageLabel->setFixedSize(80, 80);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet(R"(
        background-color: #1e1e1e;   /* 深黑底 */
        border: 1px solid #555;      /* 灰色边框 */
        border-radius: 4px;
        color: #888;                 /* 灰色文字 */
        font-size: 12px;
    )");
    m_imageLabel->setText(goods.heroName.left(1));
    layout->addWidget(m_imageLabel, 0, Qt::AlignCenter);

    loadSprite();

    // 名字
    m_nameLabel = new QLabel(goods.heroName, this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0e0;");  /* 浅白字 */
    layout->addWidget(m_nameLabel);

    // 派系
    m_factionLabel = new QLabel(goods.faction, this);
    m_factionLabel->setAlignment(Qt::AlignCenter);
    m_factionLabel->setStyleSheet("font-size: 11px; color: #999;");  /* 灰色小字 */
    layout->addWidget(m_factionLabel);

    layout->addStretch();

    // 底部按钮栏：左"详情" + 右费用
    auto* bottomBar = new QWidget(this);
    auto* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(6);

    auto* detailBtn = new QPushButton(QStringLiteral("详情"), bottomBar);
    detailBtn->setFixedHeight(28);
    detailBtn->setCursor(Qt::PointingHandCursor);
    detailBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;  /* 深灰底 */
            border: 1px solid #666;      /* 灰色边框 */
            border-radius: 3px;
            color: #ccc;                 /* 浅灰字 */
            font-size: 11px;
            padding: 2px 8px;
        }
        QPushButton:hover {
            background-color: #555;
            color: #fff;
        }
    )");
    connect(detailBtn, &QPushButton::clicked, this, [this]() {
        emit detailClicked(m_goods);
    });

    m_costLabel = new QLabel(QString("%1 G").arg(goods.cost), bottomBar);
    m_costLabel->setAlignment(Qt::AlignCenter);
    m_costLabel->setStyleSheet(R"(
        font-size: 16px;
        font-weight: bold;
        color: #f1c40f;             /* 金色 */
        background-color: #2a2a2a;  /* 深灰底 */
        border-radius: 4px;
        padding: 4px 8px;
    )");

    bottomLayout->addWidget(detailBtn);
    bottomLayout->addWidget(m_costLabel);

    layout->addWidget(bottomBar);

    // 默认样式
    setStyleSheet(R"(
        HeroCard {
            background-color: #333;   /* 深灰底 */
            border: 1px solid #555;   /* 灰色边框 */
            border-radius: 6px;
        }
        HeroCard:hover {
            background-color: #3a3a3a;
            border: 1px solid #888;
        }
    )");
}

void HeroCard::setPurchased(bool purchased)
{
    m_purchased = purchased;
    if (purchased) {
        setStyleSheet(R"(
            HeroCard {
                background-color: #222;   /* 更暗底 */
                border: 1px solid #444;   /* 暗边框 */
                border-radius: 6px;
            }
        )");
        m_costLabel->setText(QStringLiteral("已购买"));
        m_costLabel->setStyleSheet(R"(
            font-size: 13px;
            color: #666;                  /* 灰色字 */
            background-color: #1e1e1e;
            border-radius: 4px;
            padding: 4px 0px;
        )");
        setCursor(Qt::ArrowCursor);
    }
}

QString HeroCard::heroName() const
{
    return m_goods.heroName;
}

void HeroCard::mousePressEvent(QMouseEvent* event)
{
    if (m_purchased) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (event->button() == Qt::LeftButton)
        emit clicked(m_goods.heroName);
    QWidget::mousePressEvent(event);
}

QString HeroCard::spritePath() const
{
    const QString& name = m_goods.heroName;
    if (name == QString::fromUtf8("灰姑娘"))
        return QStringLiteral("assets/cinderella/7dfcca7d-7c9c-4564-8bf3-fa5bb997bf56.png");
    if (name == QString::fromUtf8("大灰狼"))
        return QStringLiteral("assets/silvrmane/1e8f9073-1be0-42cf-b88a-0f8cd218901b.png");
    if (name == QString::fromUtf8("魔镜"))
        return QStringLiteral("assets/magicmirror/magicmirror_2.png");
    if (name == QString::fromUtf8("人鱼"))
        return QStringLiteral("assets/mermaid/mermaid_2.png");
    if (name == QString::fromUtf8("白雪"))
        return QStringLiteral("assets/snowwhite/snowwhite_1.png");
    if (name == QString::fromUtf8("小火苗"))
        return QStringLiteral("assets/littlematch/littlematch_1.png");
    if (name == QString::fromUtf8("无邀者"))
        return QStringLiteral("assets/Uninvited/Uninvited_1.png");
    if (name == QString::fromUtf8("睡美人"))
        return QStringLiteral("assets/sleepbeauty/sleepbeauty_2.png");
    return QString();
}

void HeroCard::loadSprite()
{
    QString path = spritePath();
    if (path.isEmpty()) return;

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString roots[] = {
        appDir,
        QFileInfo(appDir + "/..").canonicalFilePath(),
        QFileInfo(appDir + "/../..").canonicalFilePath()
    };

    QPixmap pix;
    for (const QString& root : roots) {
        if (root.isEmpty()) continue;
        pix.load(root + "/" + path);
        if (!pix.isNull()) break;
    }

    if (!pix.isNull()) {
        m_imageLabel->setPixmap(pix.scaled(76, 76, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imageLabel->setText(QString());
    }
}
