#include "traitmanager.h"
#include "entity/unit.h"
#include "skill/skilleffect.h"

void TraitManager::clearAll(const QList<Unit*>& allUnits)
{
    for (Unit* u : allUnits) {
        if (!u) continue;
        u->removeEffectsFromSource(this);
        auto it = m_applied.find(u->id());
        if (it != m_applied.end()) {
            if (it->hpBonus) {
                u->setMaxHp(u->maxHp() - it->hpBonus);
                if (u->hp() > u->maxHp()) u->setHp(u->maxHp());
            }
            if (it->manaRecoveryBonus)
                u->setmanaRecoveryAmount(u->manaRecoveryAmount() - it->manaRecoveryBonus);
            if (it->atkBonus)
                u->setAtk(u->atk() - it->atkBonus);
        }
    }
    m_applied.clear();
}

void TraitManager::recalculate(const QList<Unit*>& allUnits)
{
    clearAll(allUnits);

    QHash<QString, int> traitCounts;
    QHash<QString, QList<Unit*>> traitUnits;

    for (Unit* u : allUnits) {
        if (!u || u->isDead()) continue;
        if (u->position().y() < 0) continue;
        if (u->owner() != Unit::Owner::PlayerCtrl) continue;
        for (const QString& t : u->traits()) {
            traitCounts[t]++;
            traitUnits[t].append(u);
        }
    }

    for (auto it = traitCounts.begin(); it != traitCounts.end(); ++it) {
        const QString& trait = it.key();
        int count = it.value();

        int hpBonus = 0, manaRecoveryBonus = 0, atkBonus = 0;
        float atkSpeedPct = 0, moveSpeedPct = 0;

        if (trait == QStringLiteral("金冕盟约")) {
            if (count >= 3)      hpBonus = 300;
            else if (count >= 2) hpBonus = 150;
        } else if (trait == QStringLiteral("野血盟约")) {
            if (count >= 2) { atkSpeedPct = -0.25f; moveSpeedPct = -0.15f; }
        } else if (trait == QStringLiteral("烬墨之裔")) {
            if (count >= 3)      manaRecoveryBonus = 6;
            else if (count >= 2) manaRecoveryBonus = 3;
        } else if (trait == QStringLiteral("异种")) {
            if (count >= 3)      { atkBonus = 20; moveSpeedPct = -0.15f; }
            else if (count >= 2) { atkBonus = 10; }
        }

        for (Unit* u : traitUnits[it.key()]) {
            if (u->baseMaxHp() == 0) {
                u->setBaseMaxHp(u->maxHp());
                u->setBaseManaRecovery(u->manaRecoveryAmount());
                u->setBaseAtk(u->atk());
            }

            if (hpBonus) {
                u->setMaxHp(u->maxHp() + hpBonus);
                u->setHp(u->hp() + hpBonus);
            }
            if (manaRecoveryBonus)
                u->setmanaRecoveryAmount(u->baseManaRecovery() + manaRecoveryBonus);
            if (atkBonus)
                u->setAtk(u->baseAtk() + atkBonus);
            if (atkSpeedPct != 0)
                u->addEffect({EffectType::AttackSpeedUp, -1, atkSpeedPct, this});
            if (moveSpeedPct != 0)
                u->addEffect({EffectType::MoveSpeedUp, -1, moveSpeedPct, this});

            AppliedBonus& ab = m_applied[u->id()];
            ab.hpBonus = hpBonus;
            ab.manaRecoveryBonus = manaRecoveryBonus;
            ab.atkBonus = atkBonus;
        }
    }
}
