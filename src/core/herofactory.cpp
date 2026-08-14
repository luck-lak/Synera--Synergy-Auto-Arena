#include "herofactory.h"
#include "entity/unit.h"
#include "entity/cinderella.h"
#include "entity/silvermane.h"
#include "entity/magicmirror.h"
#include "entity/mermaid.h"
#include "entity/snowwhite.h"
#include "entity/littlematch.h"
#include "entity/uninvited.h"
#include "entity/sleepingbeauty.h"

Unit* HeroFactory::createHero(const QString& name)
{
    if (name == QStringLiteral("灰姑娘"))
        return new cinderella();
    if (name == QStringLiteral("大灰狼"))
        return new silvermane();
    if (name == QStringLiteral("魔镜"))
        return new MagicMirror();
    if (name == QStringLiteral("人鱼"))
        return new Mermaid();
    if (name == QStringLiteral("白雪"))
        return new SnowWhite();
    if (name == QStringLiteral("小火苗"))
        return new LittleMatch();
    if (name == QStringLiteral("无邀者"))
        return new Uninvited();
    if (name == QStringLiteral("睡美人"))
        return new SleepingBeauty();
    return nullptr;
}
