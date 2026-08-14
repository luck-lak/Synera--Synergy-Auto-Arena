#include "mermaid.h"
#include "skill/skill_mermaid.h"
#include "gui/castvisual/mermaid_castvisual.h"

Mermaid::Mermaid()
    : Unit(QStringLiteral("人鱼"))
    , m_skill(std::make_unique<SkillMermaid>())
    , m_castVisual(std::make_unique<MermaidCastVisual>(this, m_skill->castFrames()))
{
    // 远程法师：中HP中攻，长读条大招
    setMaxHp(280);
    setHp(280);
    setMaxMana(120);
    setRange(3);                          /* 远程 */
    setAtk(10);
    setmanaRecoveryAmount(12);
    setAttackSpeed(25);
    setMoveSpeed(90);
    addTrait(QStringLiteral("野血盟约"));
    addTrait(QStringLiteral("异种"));
    setStar1BaseHp(maxHp());
    setStar1BaseAtk(atk());
}

void Mermaid::decideState()
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

void Mermaid::tickCooldowns(Game* game)
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

void Mermaid::cancelCasting() { m_skill->cancelCast(); }

void Mermaid::resetToDefault()
{
    Unit::resetToDefault();
    m_skill->reset();
    m_castVisual->reset();
}
