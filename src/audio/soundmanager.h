#ifndef AUDIO_SOUNDMANAGER_H
#define AUDIO_SOUNDMANAGER_H

#include <QString>
#include <map>
#include <memory>

class QSoundEffect;

class SoundManager
{
public:
    static SoundManager* instance();

    void play(const QString& name);
    void startBgm();
    void stopBgm();

private:
    SoundManager();
    void load(const QString& name, const QString& path);

    std::map<QString, std::unique_ptr<QSoundEffect>> m_sounds;
    bool m_bgmPlaying = false;
};

#endif // AUDIO_SOUNDMANAGER_H
