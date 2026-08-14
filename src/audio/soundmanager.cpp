#include "soundmanager.h"
#include <QSoundEffect>
#include <QCoreApplication>
#include <QDir>

SoundManager* SoundManager::instance()
{
    static SoundManager s;
    return &s;
}

SoundManager::SoundManager()
{
    QString dir = QCoreApplication::applicationDirPath();
    // 试两个常见位置
    QStringList roots = { dir, dir + "/..", dir + "/../.." };
    QString base;
    for (const QString& r : roots) {
        if (QDir(r + "/assets/sounds").exists()) { base = r; break; }
    }
    if (base.isEmpty()) base = dir;  /* fallback */

    load("attack",       base + "/assets/sounds/attack.wav");
    load("skill",        base + "/assets/sounds/skill.wav");
    load("buy",          base + "/assets/sounds/buy.wav");
    load("starup",       base + "/assets/sounds/starup.wav");
    load("equip",        base + "/assets/sounds/equip.wav");
    load("victory",      base + "/assets/sounds/victory.wav");
    load("defeat",       base + "/assets/sounds/defeat.wav");
    load("round_start",  base + "/assets/sounds/round_start.wav");
    load("bgm",           base + "/assets/sounds/bgm.wav");
}

void SoundManager::load(const QString& name, const QString& path)
{
    auto fx = std::make_unique<QSoundEffect>();
    fx->setSource(QUrl::fromLocalFile(path));
    fx->setVolume(0.4f);
    m_sounds[name] = std::move(fx);
}

void SoundManager::play(const QString& name)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
        it->second->play();
}

void SoundManager::startBgm()
{
    if (m_bgmPlaying) return;
    auto it = m_sounds.find("bgm");
    if (it != m_sounds.end()) {
        it->second->setLoopCount(QSoundEffect::Infinite);
        it->second->setVolume(0.25f);
        it->second->play();
        m_bgmPlaying = true;
    }
}

void SoundManager::stopBgm()
{
    auto it = m_sounds.find("bgm");
    if (it != m_sounds.end())
        it->second->stop();
    m_bgmPlaying = false;
}
