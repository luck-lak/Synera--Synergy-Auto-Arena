#include "herodetailpanel.h"
#include "shopgoods.h"
#include <QLabel>
#include <QVBoxLayout>

HeroDetailPanel::HeroDetailPanel(const ShopGoods& goods, QWidget* parent)
    : QWidget(parent, Qt::Popup)
{
    setFixedSize(240, 320);
    setStyleSheet(R"(
        HeroDetailPanel {
            background-color: #2a2a2a;
            border: 1px solid #666;
            border-radius: 6px;
        }
    )");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 14);
    layout->setSpacing(8);

    // 名字
    auto* nameLabel = new QLabel(goods.heroName, this);
    nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f1c40f;");
    nameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(nameLabel);

    // 派系
    auto* factionLabel = new QLabel(goods.faction, this);
    factionLabel->setStyleSheet("font-size: 12px; color: #aaa;");
    factionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(factionLabel);

    // 分隔线
    auto* sep1 = new QLabel(this);
    sep1->setFixedHeight(1);
    sep1->setStyleSheet("background-color: #444;");
    layout->addWidget(sep1);

    // 属性
    auto* statsLabel = new QLabel(
        QString("HP: %1   ATK: %2\n射程: %3   费用: %4 G")
            .arg(goods.hp).arg(goods.atk).arg(goods.range).arg(goods.cost),
        this);
    statsLabel->setStyleSheet("font-size: 12px; color: #ddd;");
    layout->addWidget(statsLabel);

    // 分隔线
    auto* sep2 = new QLabel(this);
    sep2->setFixedHeight(1);
    sep2->setStyleSheet("background-color: #444;");
    layout->addWidget(sep2);

    // 技能
    if (!goods.skillName.isEmpty()) {
        auto* skillTitle = new QLabel(
            QString("技能: %1  (蓝耗 %2)").arg(goods.skillName).arg(goods.manaCost), this);
        skillTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #4fc3f7;");
        layout->addWidget(skillTitle);

        auto* skillDesc = new QLabel(goods.skillDesc, this);
        skillDesc->setWordWrap(true);
        skillDesc->setStyleSheet("font-size: 11px; color: #bbb;");
        layout->addWidget(skillDesc);
    }

    // 分隔线
    auto* sep3 = new QLabel(this);
    sep3->setFixedHeight(1);
    sep3->setStyleSheet("background-color: #444;");
    layout->addWidget(sep3);

    // 故事
    auto* storyLabel = new QLabel(goods.story, this);
    storyLabel->setWordWrap(true);
    storyLabel->setStyleSheet("font-size: 11px; color: #999; font-style: italic;");
    layout->addWidget(storyLabel);

    layout->addStretch();
}
