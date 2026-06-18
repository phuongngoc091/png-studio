#include "tts/TtsEngine.h"

namespace LAStudio {

TtsEngine::TtsEngine(QObject *parent)
    : QObject(parent)
{
    m_lastSamples = {0.1f, -0.1f, 0.2f, -0.2f};
    m_sampleRate = 16000;
}

TtsEngine::~TtsEngine()
{
}

TtsEngine::State TtsEngine::state() const
{
    return Unloaded;
}

void TtsEngine::setFamilyConfig(const QVariantMap &)
{
}

QString TtsEngine::estimatedRamUsage() const
{
    return QStringLiteral("0 GB");
}

QString TtsEngine::estimatedVramUsage() const
{
    return QStringLiteral("0 GB");
}

QVariantList TtsEngine::schemaForCapability(const QString &) const
{
    return {};
}

QVariantMap TtsEngine::studioConfigForCapability(const QString &) const
{
    return {};
}

void TtsEngine::loadVoice(const QVariantMap &)
{
}

void TtsEngine::loadModel(const QString &, const QString &, const QString &)
{
}

void TtsEngine::loadModel(const QVariantMap &, const QString &)
{
}

void TtsEngine::setRuntimePath(const QString &)
{
}

void TtsEngine::unloadVoice()
{
}

void TtsEngine::unloadVoiceSync()
{
}

void TtsEngine::clearLastSamples()
{
    m_lastSamples.clear();
}

void TtsEngine::synthesize(const QString &, int, float, const QVariantMap &)
{
}

void TtsEngine::cloneVoice(const QString &, const QString &, const QVariantMap &)
{
}

void TtsEngine::designVoice(const QString &, const QVariantMap &)
{
}

QVariantList TtsEngine::buildSchemaForRuntime(const QString &) const { return {}; }
void TtsEngine::updateMemoryUsageEstimates() {}
QString TtsEngine::formatBytes(qint64) { return QString(); }
void TtsEngine::dispatch(const EngineEvent &) {}
void TtsEngine::applyState(const EngineState &) {}
QString TtsEngine::voiceConfigSignature(const QVariantMap &) const { return QString(); }
void TtsEngine::rememberLoadingVoiceConfig(const QVariantMap &) {}
void TtsEngine::rememberLoadedVoiceConfig() {}
void TtsEngine::clearVoiceConfigTracking() {}

void TtsEngine::onWorkerModelLoaded(bool, const QString &, const QVariantList &) {}
void TtsEngine::onWorkerFinished(const QVector<float> &, int) {}
void TtsEngine::onWorkerError(const QString &) {}
void TtsEngine::updateCpuUsage() {}

} // namespace LAStudio
