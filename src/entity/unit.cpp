#include "unit.h"
#include "projectile.h"
#include "core/boardgeometry.h"
#include "equipment/equipment.h"

int Unit::s_nextId = 0;

Unit::Unit(const QString& name)
    : m_id(s_nextId++)
    , m_name(name)
    , m_position(0, 0)
{}
Unit::~Unit()
{
    clearProjectiles();
    delete m_equipment[0];
    delete m_equipment[1];
}

void Unit::updateTarget(const QList<Unit*>& allUnits){
    if(m_state == State::Dead)return;

    // 目标死了 → 清除
    if (m_target && m_target->isDead()) {
        m_target = nullptr;
        if (m_state != State::Casting)
            m_state = State::Idle;
    }

    //原来目标活着也重新评估，因为在默认寻路逻辑（找设定逻辑下最优）下符合条件的对象可能会发生改变
    Unit* newTarget = findTarget(allUnits);
    if (newTarget && newTarget != m_target) {
        m_target = newTarget;
        //m_state = State::Moving;这里不应该耦合状态切换逻辑，从函数名字看来这就是用来找目标的
    }

}

void Unit::tickCooldowns(Game* /*game*/)
{
    tickEffects();
    tickHitFlash();

    //这种是默认不会放技能的角色，只有四种状态，没有Casting状态
    if (m_state == State::Moving && m_moveCooldown > 0)
        m_moveCooldown--;
    if (m_state == State::Attacking && m_attackCooldown > 0)
        m_attackCooldown--;
}

void Unit::decideState()
{
    if (m_state == State::Dead) return;

    // 沉睡或封印期间不行动
    if (hasEffect(EffectType::Sleeping) || hasEffect(EffectType::Sealed)) {
        m_state = State::Idle;
        return;
    }

    if (!m_target) return;

    bool inRange = isInRangeOf(m_target);

    if (m_state == State::Idle) {
        if (inRange) {
            m_state = State::Attacking;
            m_attackCooldown = m_attackSpeed;   // 攻击前摇
        } else {
            m_state = State::Moving;
        }
    } else if (m_state == State::Moving) {
        if (inRange) {
            m_state = State::Attacking;
            m_attackCooldown = m_attackSpeed;
        }
    } else if (m_state == State::Attacking) {
        if (!inRange) m_state = State::Moving;
    }
}

void Unit::resetToDefault()
{
    m_hp = m_maxHp;
    m_mana = 0;
    m_state = State::Idle;
    m_target = nullptr;
    m_path.clear();
    m_moveCooldown = 0;
    m_attackCooldown = 0;
    m_hitFlashFrames = 0;
    m_statusEffects.clear();
    if (m_baseAttackSpeed != -1)
        m_attackSpeed = m_baseAttackSpeed;
    if (m_baseMoveSpeed != -1)
        m_moveSpeed = m_baseMoveSpeed;
}

Unit* Unit::findTarget(const QList<Unit*>& allUnits){
    Unit* best = nullptr;
    qreal bestDist = 1e18;
    int bestHp = 999999;

    for(Unit* other : allUnits){
        if(!other || other == this)continue;
        if(other->isDead())continue;
        if(other->owner() == m_owner)continue;
        if (other->position().y() == -1) continue; //Bench上的不参战，不要锁定到bench上的单位
        if (other->hasEffect(EffectType::Sealed)) continue;  /* 封印目标不可被锁定 */

        int dx = m_position.x() - other->position().x();
        int dy = m_position.y() - other->position().y();
        qreal dist = dx * dx + dy * dy;

        bool better = false;
        if (dist < bestDist) {
            better = true;
        } else if (dist == bestDist && best) {
            if (other->hp() < bestHp) {        //血量低优先
                    better = true;
            } else if (other->hp() == bestHp) {
                if (other->position().x() <
                    best->position().x()) {   // 从左到右
                    better = true;
                } else if (other->position().x() ==
                               best->position().x() &&
                           other->position().y() <
                               best->position().y()) {  // 从下到上
                    better = true;
                }
            }
        }

        if (better) {
            best = other;
            bestDist = dist;
            bestHp = other->hp();
        }

    }
    return best;
}

void Unit::updateProjectiles()
{
    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        Projectile* p = m_projectiles[i];

        if (p->tick()) {
            // 到达：目标还活着就扣血
            if (p->target() && !p->target()->isDead() && p->target()->isVulnerable()) {
                int dmg = static_cast<int>(p->damage() * p->target()->damageTakenMultiplier());
                p->target()->setHp(p->target()->hp() - dmg);
                p->target()->triggerHitFlash();
                if (p->target()->isDead()) {
                    p->target()->setState(State::Dead);
                }
            }
            delete p;
            m_projectiles.removeAt(i);
        }
    }
}

void Unit::clearProjectiles()
{
    qDeleteAll(m_projectiles);
    m_projectiles.clear();
}

Projectile* Unit::createProjectile(Unit* target, BoardGeometry* geo)
{
    return new SimpleProjectile(target, m_atk, geo);
}

void Unit::addEffect(const SkillEffect& e)
{
    m_statusEffects.append(e);
}

void Unit::removeEffectsFromSource(void* source)
{
    for (int i = m_statusEffects.size() - 1; i >= 0; --i) {
        if (m_statusEffects[i].source == source)
            m_statusEffects.removeAt(i);
    }
}

void Unit::tickEffects()
{
    // 首次记录初始基准值
    if (m_baseAttackSpeed == -1)
        m_baseAttackSpeed = m_attackSpeed;
    if (m_baseMoveSpeed == -1)
        m_baseMoveSpeed = m_moveSpeed;

    // 每帧从基准值开始恢复
    m_attackSpeed = m_baseAttackSpeed;
    m_moveSpeed = m_baseMoveSpeed;

    bool dead = false;

    for (int i = m_statusEffects.size() - 1; i >= 0; --i) {
        SkillEffect& e = m_statusEffects[i];
        if (e.remainingFrames > 0) {
            e.remainingFrames--;
            if (e.remainingFrames <= 0) {
                m_statusEffects.removeAt(i);
                continue;
            }
        }

        switch (e.type) {
        case EffectType::AttackSpeedSlow:
            m_attackSpeed = static_cast<int>(m_attackSpeed * (1.0f + e.value));
            break;
        case EffectType::AttackSpeedUp:
            m_attackSpeed = static_cast<int>(m_attackSpeed * (1.0f - e.value));
            break;
        case EffectType::MoveSpeedUp:
            m_moveSpeed = static_cast<int>(m_moveSpeed * (1.0f - e.value));
            break;
        case EffectType::DamageOverTime:
            if (isVulnerable()) {
                int dmg = static_cast<int>(e.value * damageTakenMultiplier());
                setHp(hp() - dmg);
            }
            if (isDead())
                dead = true;
            break;
        case EffectType::HealOverTime:
            setHp(hp() + static_cast<int>(e.value));
            break;
        }
    }

    if (dead)
        setState(State::Dead);
}

bool Unit::hasEffect(EffectType type) const
{
    for (const SkillEffect& e : m_statusEffects) {
        if (e.type == type) return true;
    }
    return false;
}

bool Unit::isVulnerable() const
{
    return !hasEffect(EffectType::Sealed) && !hasEffect(EffectType::Invincible);
}

float Unit::damageTakenMultiplier() const
{
    float mult = 1.0f;
    for (const SkillEffect& e : m_statusEffects) {
        if (e.type == EffectType::DamageTakenUp)
            mult += e.value;
    }
    return mult;
}

bool Unit::canEquip() const
{
    int maxSlots = (m_starLevel >= 2) ? 2 : 1;
    return equipCount() < maxSlots;
}

void Unit::equip(const Equipment* eq)
{
    if (!eq || !canEquip()) return;

    // 找第一个空槽
    int slot = m_equipment[0] ? 1 : 0;
    m_equipment[slot] = new Equipment(*eq);    /* 拷贝，Unit 自己管理生命周期 */
    setMaxHp(maxHp() + eq->hpBonus);
    setHp(hp() + eq->hpBonus);
    setAtk(atk() + eq->atkBonus);
    if (eq->atkSpeedPct != 0)
        setAttackSpeed(static_cast<int>(attackSpeed() * (1.0f + eq->atkSpeedPct)));
    if (eq->moveSpeedPct != 0)
        setMoveSpeed(static_cast<int>(moveSpeed() * (1.0f + eq->moveSpeedPct)));
    // 重置 tickEffects 基准，让它下一帧重新记录
    m_baseAttackSpeed = -1;
    m_baseMoveSpeed = -1;
}

void Unit::unequip(int slot)
{
    if (slot < -1 || slot > 1) return;

    // unequip all
    if (slot == -1) {
        unequip(1);   /* 先卸 1 再卸 0，倒序避免 re-index */
        unequip(0);
        return;
    }

    Equipment* eq = m_equipment[slot];
    if (!eq) return;

    setMaxHp(maxHp() - eq->hpBonus);
    if (hp() > maxHp()) setHp(maxHp());
    setAtk(atk() - eq->atkBonus);
    if (eq->atkSpeedPct != 0)
        setAttackSpeed(static_cast<int>(attackSpeed() / (1.0f + eq->atkSpeedPct)));
    if (eq->moveSpeedPct != 0)
        setMoveSpeed(static_cast<int>(moveSpeed() / (1.0f + eq->moveSpeedPct)));

    delete m_equipment[slot];
    m_equipment[slot] = nullptr;
    m_baseAttackSpeed = -1;
    m_baseMoveSpeed = -1;
}