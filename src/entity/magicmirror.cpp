#include "magicmirror.h"
#include "skill/skill_magicmirror.h"
#include "gui/castvisual/magicmirror_castvisual.h"

MagicMirror::MagicMirror()
    : Unit(QStringLiteral("魔镜"))
    , m_skill(std::make_unique<SkillMagicMirror>())
    , m_castVisual(std::make_unique<MagicMirrorCastVisual>(this, m_skill->castFrames()))
{
    // 远程辅助：中HP低攻，短蓝快充
    setMaxHp(350);
    setHp(350);
    setMaxMana(80);
    setRange(3);                     /* 远程 */
    setAtk(6);                       /* 低攻 */
    setmanaRecoveryAmount(8);
    setAttackSpeed(25);              /* 慢速普攻 */
    setMoveSpeed(90);
    addTrait(QStringLiteral("烬墨之裔"));
    addTrait(QStringLiteral("异种"));
    setStar1BaseHp(maxHp());
    setStar1BaseAtk(atk());
}

void MagicMirror::decideState()
{
    if (state() == State::Dead) return;

    // 沉睡或封印 → 不得施法、不行动
    if (hasEffect(EffectType::Sleeping) || hasEffect(EffectType::Sealed)) {
        Unit::decideState();
        return;
    }

    if (m_skill->isCasting()) {
        if (!target() || target()->isDead()) {
            m_skill->cancelCast();
            setState(State::Idle);
        }
        return;
    }

    if (mana() >= m_skill->manaCost() && m_skill->isReady()) {
        m_castVisual->reset();
        m_skill->startCast();
        setState(State::Casting);
        return;
    }

    Unit::decideState();
}

void MagicMirror::tickCooldowns(Game* game)
{
    tickEffects();

    if (!m_skill->isCasting())
        m_skill->Cast_tickCooldown();

    if (state() == State::Casting) {
        m_castVisual->tick();
        m_skill->tickCast();
        if (!m_skill->isCasting()) {
            m_skill->execute(this, game);
            m_skill->startCooldown();
            setMana(0);
            setState(State::Idle);
        }
        return;
    }

    Unit::tickCooldowns(game);
}

void MagicMirror::cancelCasting() { m_skill->cancelCast(); }

void MagicMirror::resetToDefault()
{
    Unit::resetToDefault();
    m_skill->reset();
    m_castVisual->reset();
}
