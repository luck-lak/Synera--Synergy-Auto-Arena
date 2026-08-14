#ifndef SHOPGOODS_H
#define SHOPGOODS_H

#include <QString>

struct ShopGoods {
    QString heroName;     // 中文名
    int cost;             // 购买价格（金币）
    QString faction;      // 派系名
    bool purchased;       // 本轮是否已购买
    int hp;               // 生命值
    int atk;              // 攻击力
    int range;            // 射程
    QString skillName;    // 技能名
    QString skillDesc;    // 技能描述
    int manaCost = 0;     // 技能蓝耗
    QString story;        // 人物故事
};

#endif // SHOPGOODS_H
