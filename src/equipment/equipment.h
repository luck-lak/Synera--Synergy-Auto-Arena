#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QString>
#include <QList>

struct Equipment {
    int id;
    QString name;
    int hpBonus = 0;
    int atkBonus = 0;
    float atkSpeedPct = 0;   // 负值 = 加速 (AttackSpeedUp)
    float moveSpeedPct = 0;

    bool operator==(const Equipment& other) const { return id == other.id; }
};

static QList<Equipment> createEquipmentPool()
{
    return {
        {0, QStringLiteral("铁剑"),   0,  5,  0,     0},
        {1, QStringLiteral("锁子甲"), 100, 0,  0,     0},
        {2, QStringLiteral("短弓"),   0,   3, -0.15f, 0},
        {3, QStringLiteral("法杖"),   0,   8,  0,     0},
        {4, QStringLiteral("圆盾"),   50,  0, -0.10f, 0},
    };
}

#endif // EQUIPMENT_H
