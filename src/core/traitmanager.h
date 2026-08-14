#ifndef TRAITMANAGER_H
#define TRAITMANAGER_H

#include <QHash>
#include <QList>
#include <QString>

class Unit;

class TraitManager
{
public:
    void recalculate(const QList<Unit*>& allUnits);
    void clearAll(const QList<Unit*>& allUnits);

private:
    struct AppliedBonus {
        int hpBonus = 0;
        int manaRecoveryBonus = 0;
        int atkBonus = 0;
    };
    QHash<int, AppliedBonus> m_applied;
};

#endif // TRAITMANAGER_H
