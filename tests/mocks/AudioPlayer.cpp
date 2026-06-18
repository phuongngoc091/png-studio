#include "audio/AudioPlayer.h"

namespace LAStudio {

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
}

void AudioPlayer::playPcm(const QByteArray &, int)
{
}

void AudioPlayer::playFile(const QString &)
{
}

void AudioPlayer::stop()
{
}

} // namespace LAStudio
