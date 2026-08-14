#ifndef PLAYER_H
#define PLAYER_H
#include <QtMath>
class Player{
public:
    static constexpr int kMaxLevel = 6;
private:
    int p_Hp = 100;
    int p_Gold = 25;
    int p_Level = 1;
    int p_PopulationCap = 3;
    int p_PopulationUsed = 0;
    //如果是PVP，还要有对敌方造成伤害，现在先不管
public:
    int Hp() const {return p_Hp;}
    int Gold() const{return p_Gold;}
    int Level() const{return p_Level;}
    int PopulationCap() const{return p_PopulationCap;}
    int PopulationUsed() const{return p_PopulationUsed;}
    void setHp(int hp){ p_Hp = hp;}
    void setGold(int Gold){p_Gold =Gold;}
    void changeGold(int changeValue){p_Gold +=changeValue;}//可以是负值
    void setLevel(int level){p_Level = level;}
    void setPopulationCap(int PopulationCap){p_PopulationCap = PopulationCap;}
    void changePopulationCap(int changeValue){p_PopulationCap += changeValue;}
    void setPopulationUsed(int PopulationUsed){p_PopulationUsed = PopulationUsed;}
    void changePopulationUsed(int changeValue){p_PopulationUsed += changeValue;}
    //一些便利方法
    bool isDead() const { return p_Hp <= 0; }
    bool canDeploy() const { return p_PopulationUsed < p_PopulationCap; }
    void roundIncome(bool won) {
        int interest = qMin(p_Gold / 10, 3);  /* 利息：当前金币/10，封顶 3 */
        p_Gold += 8;                           /* 基础收入 */
        p_Gold += (won ? 3 : 1);               /* 战斗奖励 */
        p_Gold += interest;                    /* 利息 */
    }
    int levelUpCost() const { return p_Level * 6 + 4; }
    bool tryLevelUp() {
        if (p_Level >= kMaxLevel) return false;
        int cost = levelUpCost();
        if (p_Gold < cost) return false;
        p_Gold -= cost;
        p_Level++;
        p_PopulationCap++;
        return true;
    }
};
#endif // PLAYER_H
