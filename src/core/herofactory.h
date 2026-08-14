#ifndef HEROFACTORY_H
#define HEROFACTORY_H

#include <QString>

class Unit;

class HeroFactory
{
public:
    static Unit* createHero(const QString& name);
};

#endif // HEROFACTORY_H
