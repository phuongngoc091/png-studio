#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqml.h>

namespace LAStudio {

class AudioPlaybackSession;

class AudioPlayer : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("AudioPlayer is managed by AppController")

    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)

public:
    explicit AudioPlayer(QObject *parent = nullptr);

    bool isPlaying() const { return m_playing; }

    Q_INVOKABLE void playPcm(const QByteArray &pcm16Data, int sampleRate);
    Q_INVOKABLE void playFile(const QString &path);
    Q_INVOKABLE void stop();

signals:
    void playingChanged();
    void playbackFinished();

private:
    QPointer<AudioPlaybackSession> m_session;
    bool m_playing = false;
};

} // namespace LAStudio
