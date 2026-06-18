#include "stt/SttEngine.h"
#include <QTimer>

namespace LAStudio {

// Helper for std::visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

SttEngine::SttEngine(QObject *parent)
    : QObject(parent)
{
}

SttEngine::~SttEngine()
{
}

SttEngine::State SttEngine::state() const
{
    return std::visit(overloaded{
        [](StateUnloaded) { return Unloaded; },
        [](StateLoading)  { return Loading; },
        [](StateReady)    { return Ready; },
        [](StateProcessing) { return Processing; },
        [](StateError)    { return Error; }
    }, m_state);
}

void SttEngine::loadModel(const QString &modelPath, bool useGpu, const QString &runtimePath)
{
    Q_UNUSED(modelPath);
    Q_UNUSED(useGpu);
    Q_UNUSED(runtimePath);

    m_state = StateLoading{};
    emit stateChanged();
    emit modelLoadedChanged();

    // Simulate model loading asynchronously
    QTimer::singleShot(50, this, [this]() {
        m_state = StateReady{};
        emit stateChanged();
        emit modelLoadedChanged();
    });
}

void SttEngine::unloadModel()
{
    m_state = StateUnloaded{};
    emit stateChanged();
    emit modelLoadedChanged();
}

void SttEngine::unloadModelSync()
{
    unloadModel();
}

void SttEngine::transcribeSamples(const QVector<float> &samples, const QString &language, int threads, bool translate, const QVariantMap &settings)
{
    Q_UNUSED(samples);
    Q_UNUSED(language);
    Q_UNUSED(threads);
    Q_UNUSED(translate);
    Q_UNUSED(settings);

    m_state = StateProcessing{};
    emit stateChanged();
    emit processingChanged();

    // Simulate transcription asynchronously
    QTimer::singleShot(50, this, [this]() {
        m_progress = 100;
        m_transcript = QStringLiteral("Mock transcribed text");
        emit progressChanged();
        emit transcriptChanged();

        m_state = StateReady{};
        emit stateChanged();
        emit processingChanged();

        emit transcriptionFinished(m_transcript, QVariantList());
    });
}

void SttEngine::onWorkerModelLoaded(bool, const QString &) {}
void SttEngine::onWorkerProgress(int) {}
void SttEngine::onWorkerFinished(const QString &, const QVariantList &) {}
void SttEngine::onWorkerError(const QString &) {}
void SttEngine::dispatch(const EngineEvent &) {}
void SttEngine::applyState(const EngineState &) {}

} // namespace LAStudio
