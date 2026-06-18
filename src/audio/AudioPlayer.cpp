#include "AudioPlayer.h"
#include "core/Logger.h"
#include "WavIO.h"
#include <QDir>
#include <QBuffer>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <algorithm>

namespace LAStudio {

class AudioPlaybackSession : public QObject {
    Q_OBJECT
public:
    AudioPlaybackSession(const QByteArray &pcmData, const QAudioFormat &fmt, QObject *parent = nullptr)
        : QObject(parent), m_pcmData(pcmData)
    {
        m_buffer.setData(m_pcmData);
        m_buffer.open(QIODevice::ReadOnly);

        QAudioDevice dev = QMediaDevices::defaultAudioOutput();
        if (!dev.isFormatSupported(fmt)) {
            Logger::warning("AudioPlaybackSession", "Format not supported by default output device");
        }

        m_sink = new QAudioSink(dev, fmt, this);
        connect(m_sink, &QAudioSink::stateChanged, this, &AudioPlaybackSession::onStateChanged);
    }

    void start() {
        m_sink->start(&m_buffer);
    }

    void stop() {
        m_sink->stop();
    }

signals:
    void finished();

private slots:
    void onStateChanged(QAudio::State state) {
        if (state == QAudio::IdleState) {
            emit finished();
        }
    }

private:
    QByteArray m_pcmData;
    QBuffer m_buffer;
    QAudioSink* m_sink = nullptr;
};

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
}

void AudioPlayer::playFile(const QString &path)
{
    stop();

    QString cleanPath = path;
    if (cleanPath.startsWith(QStringLiteral("file:///")))
        cleanPath = cleanPath.mid(8);
    else if (cleanPath.startsWith(QStringLiteral("file://")))
        cleanPath = cleanPath.mid(7);

    cleanPath = QDir::toNativeSeparators(cleanPath);

    WavIO::WavData data = WavIO::loadAsFloat(cleanPath);
    if (data.samples.isEmpty()) {
        Logger::error("AudioPlayer", "Failed to load audio file for playback: " + cleanPath);
        return;
    }

    QByteArray pcmData;
    pcmData.resize(data.samples.size() * 2);
    auto *out = reinterpret_cast<int16_t *>(pcmData.data());
    for (int i = 0; i < data.samples.size(); ++i) {
        float s = std::clamp(data.samples[i], -1.0f, 1.0f);
        out[i] = static_cast<int16_t>(s * 32767.0f);
    }

    QAudioFormat fmt;
    fmt.setSampleRate(data.sampleRate);
    fmt.setChannelCount(data.channels);
    fmt.setSampleFormat(QAudioFormat::Int16);

    auto *session = new AudioPlaybackSession(pcmData, fmt, this);
    m_session = session;

    connect(session, &AudioPlaybackSession::finished, this, [this, session]() {
        if (m_session == session) {
            m_session = nullptr;
            m_playing = false;
            emit playingChanged();
            emit playbackFinished();
        }
        session->deleteLater();
    });

    m_playing = true;
    emit playingChanged();
    session->start();
}

void AudioPlayer::playPcm(const QByteArray &pcm16Data, int sampleRate)
{
    stop();

    QAudioFormat fmt;
    fmt.setSampleRate(sampleRate);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    auto *session = new AudioPlaybackSession(pcm16Data, fmt, this);
    m_session = session;

    connect(session, &AudioPlaybackSession::finished, this, [this, session]() {
        if (m_session == session) {
            m_session = nullptr;
            m_playing = false;
            emit playingChanged();
            emit playbackFinished();
        }
        session->deleteLater();
    });

    m_playing = true;
    emit playingChanged();
    session->start();
}

void AudioPlayer::stop()
{
    if (m_session) {
        AudioPlaybackSession *session = m_session.data();
        m_session = nullptr;
        if (session) {
            session->stop();
            session->deleteLater();
        }
    }
    if (m_playing) {
        m_playing = false;
        emit playingChanged();
    }
}

} // namespace LAStudio

#include "AudioPlayer.moc"
