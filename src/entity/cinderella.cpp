#include "cinderella.h"
#include "skill/skill_crystalshoes.h"
#include "gui/castvisual/crystalshoes_castvisual.h"

cinderella::cinderella()
    : Unit(QStringLiteral("灰姑娘"))
    , m_skill(std::make_unique<CrystalShoesSkill>())
    , m_castVisual(std::make_unique<CrystalShoesCastVisual>(this, m_skill->castFrames()))
{
    //但是我这里不是针对法师，是针对灰姑娘的专属设计
    setMaxHp(250);//法师脆一点，初始的时候
    setHp(250);
    setMaxMana(100);
    setRange(3);//法师手长
    setAtk(8);//普攻伤害要低一点
    setmanaRecoveryAmount(8);
    setAttackSpeed(20);//法师普攻比较慢
    setMoveSpeed(100);//走地也慢
    addTrait(QStringLiteral("金冕盟约"));
    setStar1BaseHp(maxHp());
    setStar1BaseAtk(atk());
}

void cinderella::decideState()
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

void cinderella::tickCooldowns(Game* game)
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

void cinderella::cancelCasting() { m_skill->cancelCast(); }

void cinderella::resetToDefault()
{
    Unit::resetToDefault();
    m_skill->reset();
    m_castVisual->reset();
}
