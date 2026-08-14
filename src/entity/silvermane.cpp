#include "silvermane.h"
#include "skill/skill_silvermane.h"
#include "gui/castvisual/silvermane_castvisual.h"

silvermane::silvermane()
    : Unit(QStringLiteral("大灰狼"))
    , m_skill(std::make_unique<SkillSilvermane>())
    , m_castVisual(std::make_unique<SilvermaneCastVisual>(this, m_skill->castFrames()))
{
    // 近战刺客：高攻高速脆皮
    setMaxHp(300);                    /* 脆皮 */
    setHp(300);
    setMaxMana(100);
    setRange(1);                      /* 近战 */
    setAtk(15);                       /* 高攻 */
    setmanaRecoveryAmount(5);
    setAttackSpeed(12);               /* 攻速快 */
    setMoveSpeed(70);                 /* 移速快 */
    addTrait(QStringLiteral("野血盟约"));
    addTrait(QStringLiteral("异种"));
    setStar1BaseHp(maxHp());
    setStar1BaseAtk(atk());
}

void silvermane::decideState()
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

void silvermane::tickCooldowns(Game* game)
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

void silvermane::cancelCasting() { m_skill->cancelCast(); }

void silvermane::resetToDefault()
{
    Unit::resetToDefault();
    m_skill->reset();
    m_castVisual->reset();
}
