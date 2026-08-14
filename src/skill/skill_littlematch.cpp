#include "skill_littlematch.h"
#include "entity/unit.h"
#include "core/game.h"

SkillLittleMatch::SkillLittleMatch()
    : Skill(QStringLiteral("最后一把火柴"), 140, 50, 210)
{}

void SkillLittleMatch::execute(Unit* caster, Game* game)
{
    if (!caster || caster->isDead()) return;

    // 对全体敌方造成 120 伤害
    for (Unit* u : game->allUnits()) {
        if (!u || u == caster || u->isDead()) continue;
        if (u->position().y() == -1) continue;
        if (u->owner() == caster->owner()) continue;
        if (!u->isVulnerable()) continue;

        int dmg = static_cast<int>(120 * u->damageTakenMultiplier());
        u->setHp(u->hp() - dmg);
        u->triggerHitFlash();
        if (u->isDead())
            u->setState(Unit::State::Dead);
    }
}
