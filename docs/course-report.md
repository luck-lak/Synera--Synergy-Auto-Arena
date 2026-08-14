# Synera: Synergy Auto-Arena

## 一、基本信息

- **项目名称**：Synera — 单机轻量级自走棋 (PVE)
- **开发环境**：Windows 11 / Qt 6.11.1 / CMake 3.16+ / MinGW 64-bit / C++17
- **代码规模**：约 6800 行 (含 `src/` 下 80+ 源文件)
- **构建方式**：Qt Creator 打开 `CMakeLists.txt` → Run CMake → Build → 将 `assets/` 复制到构建目录

---

## 二、项目各阶段完成度

### 阶段一：棋盘、单位与拖拽 (100%)

- [x] 8×8 棋盘，上半场敌方（rows 0-3）、下半场我方（rows 4-7）
- [x] 8 槽备战区（Bench）
- [x] Unit 基类（HP / ATK / Range / Max Mana / Mana / 状态机），我方与敌方同构，仅用 `Owner` 枚举区分
- [x] 鼠标拖拽摆放，非法放置弹回原位或交换位置
- [x] GUI 展示棋盘、备战区、血条、蓝条、单位头像

### 阶段二：战斗、寻路与技能 (100%)

- [x] 准备 → 战斗 → 结算三阶段循环，轮次推进
- [x] 敌方按关卡配置自动生成，5 种敌人模板随轮次增强
- [x] 单位状态机：Idle / Moving / Attacking / Casting / Dead
- [x] 索敌规则：欧氏距离最近 → 生命值低优先 → 从左到右 → 从下到上
- [x] BFS 寻路（防碰撞、可绕行）
- [x] 普攻回蓝 + 远程飞弹系统 + 8 个英雄各具独特技能（多态 `Skill::execute`）
- [x] 胜负结算：Player HP 归零失败，敌方全灭胜利

### 阶段三：商店、羁绊、升星、装备与存档 (100%)

- [x] 金币系统：基础收入 8 金/轮 + 利息（金币/10，封顶 3）+ 胜负奖励
- [x] 商店：随机 3 名英雄上架，购买后放备战区 / 棋盘，可花费金币刷新
- [x] 人口系统：初始 3 人，花费金币升级（最多 6 级 / 8 人口）
- [x] 4 种羁绊（职业/种族）：金冕盟约（HP 光环）、野血盟约（攻速移速光环）、烬墨之裔（回蓝机制改变）、异种（ATK+移速）
- [x] 升星：3 合 1 自动合成，2 星属性 ×1.8，3 星属性 ×3.24，装备自动重新分配
- [x] 装备：5 种基础装备，战斗中击杀掉落，拖拽穿戴/转移/卸下，1 星 1 件 / 2 星 2 件
- [x] 存档/读档：JSON 完整存档，每轮自动保存，支持保存退出 & 继续游戏
- [x] GUI 完整展示经济、商店、羁绊、星级、装备、轮次与阶段信息
- [x] 开始界面：新游戏 / 继续游戏

### 阶段四：扩展功能 (100%)

- [x] 视听结合：背景音乐（75 秒舒缓循环）+ 战斗事件音效（升星/购买/装备/胜负），使用 Qt Multimedia + QSoundEffect
- [x] 利息系统 — "每 10 金币产生 1 金币的利息机制"（roundIncome 里qMin(p_Gold / 10, 3)）
- [x] 敌方站位策略 — 给 8 轮设计了 8 种不同阵型（一字/四角/V形/双翼/层阵），每轮敌人站位完全不同
- [x] UnitInfoPanel 属性分解 — 左侧角色面板把 ATK/HP 拆成"基础 + 羁绊 +装备"，不是简单显示数值，而是算出来的三层分解。
---

## 三、文件树结构

```
Synera_starter/
├── CMakeLists.txt              # CMake 构建配置（Qt6 + GLOB_RECURSE 自动收录源文件）
├── README.md                   # 本文件
├── assets/
│   ├── sounds/                 # 音效与背景音乐（WAV）
│   │   ├── bgm.wav             # 75 秒舒缓循环 BGM
│   │   ├── buy.wav             # 购买英雄
│   │   ├── equip.wav           # 穿戴装备
│   │   ├── starup.wav          # 升星
│   │   ├── victory.wav         # 胜利
│   │   ├── defeat.wav          # 战败
│   │   └── round_start.wav     # 轮次开始号角
│   └── (英雄精灵图等)
└── src/
    ├── main.cpp                # 入口：MainMenu → GameWindow
    ├── audio/
    │   └── soundmanager.h/.cpp # 音效管理器单例（预加载 + 播放 / BGM 控制）
    ├── core/
    │   ├── game.h/.cpp         # 核心控制器（回合循环、战斗逻辑、寻路、存档、装备掉落）
    │   ├── board.h/.cpp        # 8×8 棋盘（单元格占用 + 反向索引）
    │   ├── bench.h/.cpp        # 8 槽备战区
    │   ├── boardgeometry.h/.cpp# 网格坐标 ↔ 世界坐标转换
    │   ├── herofactory.h/.cpp  # 英雄工厂（字符串 → Unit*）
    │   └── traitmanager.h/.cpp # 羁绊统计与加成应用
    ├── entity/
    │   ├── unit.h/.cpp         # 单位基类（属性、状态机、效果系统、装备槽、寻路索敌）
    │   ├── player.h            # 玩家（HP / 金币 / 等级 / 人口）
    │   ├── projectile.h/.cpp   # 投射物基类（飞弹移动 + 到达检测）
    │   └── cinderella.h/.cpp ... # 8 个英雄子类（继承 Unit + 组合 Skill + CastVisual）
    ├── equipment/
    │   └── equipment.h         # 装备数据结构 + 5 种装备池
    ├── gui/
    │   ├── gamewindow.h/.cpp   # 主游戏窗口（面板布局 + 信号连接）
    │   ├── mainmenu.h/.cpp     # 开始界面（新游戏 / 继续游戏）
    │   ├── griditem.h/.cpp     # 棋盘格子图形项
    │   ├── unititem.h/.cpp     # 单位图形项（精灵图 + 血条蓝条 + 星标 + 拖拽）
    │   ├── equipmentitem.h/.cpp# 可拖拽装备图标
    │   ├── playerinfopanel.h/.cpp # 右侧玩家面板（HP / 金币 / 等级 / 羁绊 / 升级按钮）
    │   ├── unitinfopanel.h/.cpp   # 左侧单位详情面板（属性分解 + 装备信息 + 点击卸下）
    │   └── castvisual/         # 8 个英雄的施法蓄力特效子类
    ├── shop/
    │   ├── shopgoods.h         # 商店商品数据结构
    │   ├── shoppanel.h/.cpp    # 商店覆盖层（刷新 / 购买 / 随机上架）
    │   ├── herocard.h/.cpp     # 英雄卡片（头像 + 详情按钮）
    │   └── herodetailpanel.h/.cpp # 英雄详情浮窗（属性 + 技能蓝耗 + 故事）
    └── skill/
        ├── skill.h/.cpp        # 技能基类（蓝耗 / 读条帧 / 冷却帧）
        ├── skilleffect.h       # 效果类型枚举 + SkillEffect 结构体
        ├── crystalshoes_projectile.h/.cpp # 水晶鞋飞弹（图片 + 旋转）
        └── skill_*.h/.cpp      # 8 个英雄各自的技能子类
```

---

## 四、核心类和数据结构

| 类名 | 文件 | 主要功能 |
|------|------|------|
| `Game` | `core/game.h` | 游戏总控制器：阶段循环、战斗 Tick、BFS 寻路、装备掉落、存档序列化、升星合成 |
| `Unit` | `entity/unit.h` | 单位基类：属性 (HP/ATK/Range/Mana)、5 态状态机、索敌、效果系统、双装备槽、飞弹管理 |
| `Board` | `core/board.h` | 8×8 棋盘：`QVector<Unit*>` 64 格 + `QHash<Unit*, QPoint>` 反向索引 |
| `Bench` | `core/bench.h` | 8 槽备战区：`QVector<Unit*>` + `QHash<Unit*, int>` 反向索引 |
| `Player` | `entity/player.h` | 玩家资源：HP / 金币 / 等级 / 人口上限 / 升级花费 / 利息计算 |
| `Skill` | `skill/skill.h` | 技能抽象基类：`manaCost` / `castFrames` / `cooldownFrames` + `virtual execute()` |
| `CastVisual` | `gui/castvisual/castvisual.h` | 施法蓄力视觉效果基类（progress / paint），8 个英雄各有一子类 |
| `Projectile` | `entity/projectile.h` | 飞弹基类：tick 移动、到达判定；`CrystalShoesProjectile` 子类带图片旋转 |
| `TraitManager` | `core/traitmanager.h` | 羁绊统计器：扫描棋盘 → 计数 → 按阈值应用 HP / ATK / 回蓝 / 攻速加成 |
| `Equipment` | `equipment/equipment.h` | 装备数据结构（id / name / hpBonus / atkBonus / atkSpeedPct） + 5 种装备池 |
| `SoundManager` | `audio/soundmanager.h` | 音效管理器单例：预加载 WAV → `play(name)` / `startBgm()` / `stopBgm()` |
| `HeroFactory` | `core/herofactory.h` | 工厂函数 `createHero(QString) → Unit*`，按字符串创建英雄子类实例 |

所有英雄类（`cinderella`, `silvermane`, `MagicMirror`, `Mermaid`, `SnowWhite`, `LittleMatch`, `Uninvited`, `SleepingBeauty`）均继承 `Unit`，组合 `Skill` + `CastVisual`，复写 `decideState()` 和 `tickCooldowns()` 实现各自技能触发逻辑。

---

## 五、算法描述

### 5.1 BFS 寻路

**文件**：`core/game.cpp → Game::findpath()`

BFS 在 8×8 网格上搜索 start → end 的最短路径。四个方向（上下左右）按序扩展。遇到格子被其他单位占据时跳过（碰撞阻塞）。不包含终点格（防止站在已方单位上方）。寻路失败时返回空路径，单位回到 Idle 状态等待下次 Tick。

### 5.2 索敌

**文件**：`entity/unit.cpp → Unit::findTarget()`

遍历 `allUnits`，排除自己、死亡、同阵营、Bench 上、被封印的单位。计算欧氏距离平方，按优先级排序：① 距离最近 ② 生命值最低 ③ x 坐标最小（从左到右）④ y 坐标最小（从下到上）。选择第一个满足条件的作为攻击目标。

### 5.3 羁绊计算

**文件**：`core/traitmanager.cpp → TraitManager::recalculate()`

战斗开始时调用一次。扫描所有棋盘上存活的我方单位，按 `traits()` 标签分组计数。每个派系判断是否达到 2/3 人阈值，将 HP / ATK 加成直接写入单位属性，攻速/移速通过现有 `SkillEffect` 系统施加。`clearAll()` 负责撤回加成——通过 `AppliedBonus` 结构体记录加了多少，不依赖单位自身状态，避免耦合。

### 5.4 升星合并

**文件**：`core/game.cpp → Game::tryMergeStarUp()`

检测同英雄同星级是否凑满 3 个。收集所有单位的装备 → 全员卸装 → 删除两个牺牲单位 → 升级 keeper → 按 `star1BaseHp/Atk × 倍率` 重置属性 → 重新分配装备（填满槽位，多余放回装备栏）。Combat 阶段不触发。合并后递归调用自身检测下一级。

### 5.5 存档序列化

**文件**：`core/game.cpp → Game::saveToFile() / loadFromFile()`

保存：遍历所有玩家单位 → 记录 `type`(英雄名) + 位置 + 星数 + HP/Mana + 装备 ID + 装备栏 ID → JSON → `QFile` 写入磁盘。读取：`HeroFactory::createHero(type)` 重建各单位 → 恢复星级/属性/装备 → `buildScene()` 重建图形 → `reset()` 布阵。

---

## 六、辅助函数

| 函数 | 位置 | 作用 |
|------|------|------|
| `createEquipmentPool()` | `equipment/equipment.h` | 返回 5 种基础装备的静态列表（硬编码 ID），供存档匹配与装备查找 |
| `createEnemyFromTemplate()` | `game.cpp` | 按 EnemyTemplate 结构体创建敌方单位并应用永久被动效果 |
| `saveFilePath()` | `game.cpp` | 返回 `Documents/Synera/save.json` 路径，跨平台兼容 |
| `starMult()` | `unitinfopanel.cpp` | 升星属性倍率映射（与 `tryMergeStarUp` 公式一致） |

---

## 七、AI 使用说明

### 合作状态

本项目开发全程在 **Claude Code (Anthropic)** 的辅助下完成。AI 的角色定位为**协作伙伴**——提供设计方案、编写代码、排查 Bug、解释原理，最终决策与代码质量把控由我本人负责。

### 工作流程

1. **方案讨论**：向 AI 描述需求，AI 先给出多个可选方案并分析优劣。选定方向后开始实现。
2. **小步迭代**：每个大功能拆成 3-6 个小步骤，每步 AI 写代码 → 我审阅 → 编译测试 → 确认无误后继续下一步。
3. **Bug 修复**：运行时遇到崩溃或逻辑异常，AI 通过阅读源码和分析调用链定位根因并修复。
4. **Review 与解释**：对于不理解的代码，要求 AI 逐行解释其作用，在确认理解后才接受写入。

### 核心模块解析

**模块一：羁绊系统 (TraitManager)**

`TraitManager::recalculate()` 在战斗开始时统计棋盘上同派系存活单位数量，按 2/3 阈值分级应用加成。`AppliedBonus` 结构体记录每次应用的加成了多少，`clearAll()` 据此精确撤回，避免羁绊效果污染基础属性复原逻辑。HP/ATK 直接修改属性值，攻速/移速通过已有的 `SkillEffect` 系统施加（永久效果 `remainingFrames = -1`），复用现有系统。

**模块二：升星合并 (tryMergeStarUp)**

`tryMergeStarUp()` 实现自走棋的 3 合 1 合成机制。用 `star1BaseHp/Atk` 惰性记录 1 星原始值，升星时从该值 × 倍率计算新属性，避免其他加成干扰倍率。合成前收集全部装备 → 全员卸装 → 删牺牲单位 → 升级 keeper → 重新分配装备，多余放回装备栏，保证装备零丢失。合成后递归调用自身，实现多级连锁合并。
