#include "SttModelSession.h"
#include "stt/SttEngine.h"
#include "controllers/StudioConfigurationResolver.h"
#include "core/Logger.h"
#include <QFileInfo>

namespace LAStudio {

SttModelSession::SttModelSession(SttEngine *engine, QObject *parent)
    : IModelSession(parent)
    , m_engine(engine)
{
    m_lifecycle = new ModelLifecycleController(
        [this](const StudioConfiguration &config) { return resolveConfig(config); },
        [this](const SessionConfiguration &config) { performLoad(config); },
        [this]() { performUnload(); },
        this
    );

    connect(m_lifecycle, &ModelLifecycleController::stateChanged, this, [this]() {
        // Forward stateChanged
        auto current = m_lifecycle->state();
        if (current != m_lastLifecycleState) {
            m_lastLifecycleState = current;
            emit stateChanged();
        }
    });
    connect(m_lifecycle, &ModelLifecycleController::activeConfigurationChanged, this, &SttModelSession::activeConfigurationChanged);
    connect(m_lifecycle, &ModelLifecycleController::pendingConfigurationChanged, this, &SttModelSession::pendingConfigurationChanged);
    connect(m_lifecycle, &ModelLifecycleController::activeSignatureChanged, this, &SttModelSession::activeSignatureChanged);
    connect(m_lifecycle, &ModelLifecycleController::errorOccurred, this, &SttModelSession::errorOccurred);

    if (m_engine) {
        connect(m_engine, &SttEngine::stateChanged, this, &SttModelSession::onEngineStateChanged);
        connect(m_engine, &SttEngine::errorOccurred, this, [this](const QString &err) {
            m_lifecycle->onLoadError(err);
        });
    }
}

ModelSessionState SttModelSession::state() const
{
    return m_lifecycle->state();
}

bool SttModelSession::modelActive() const
{
    return m_lifecycle->modelActive();
}

bool SttModelSession::canProcess() const
{
    return m_lifecycle->canProcess();
}

std::optional<SessionConfiguration> SttModelSession::activeConfiguration() const
{
    return m_lifecycle->activeConfiguration();
}

std::optional<SessionConfiguration> SttModelSession::pendingConfiguration() const
{
    return m_lifecycle->pendingConfiguration();
}

QString SttModelSession::activeSignature() const
{
    return m_lifecycle->activeSignature();
}

void SttModelSession::requestLoad(const QString &capabilityId, const StudioConfiguration &configuration)
{
    m_lifecycle->requestLoad(capabilityId, configuration);
}

void SttModelSession::requestUnload(const QString &capabilityId)
{
    m_lifecycle->requestUnload(capabilityId);
}

void SttModelSession::requestReload(const QString &capabilityId)
{
    m_lifecycle->requestReload(capabilityId);
}

bool SttModelSession::usesRuntime(const QString &runtimeId, const QString &runtimeVersion) const
{
    auto active = activeConfiguration();
    if (active && active->selection.runtimeId == runtimeId && active->selection.runtimeVersion == runtimeVersion) {
        return true;
    }
    auto pending = pendingConfiguration();
    if (pending && pending->selection.runtimeId == runtimeId && pending->selection.runtimeVersion == runtimeVersion) {
        return true;
    }
    return false;
}

bool SttModelSession::usesModelPath(const QString &modelPath) const
{
    auto cleanPath = [](const QString &p) {
        return QFileInfo(p).absoluteFilePath().replace(QStringLiteral("\\"), QStringLiteral("/"));
    };

    const QString target = cleanPath(modelPath);

    auto checkConfig = [&](const std::optional<SessionConfiguration> &conf) {
        if (!conf) return false;
        for (const QString &p : conf->resolvedModelPaths) {
            if (cleanPath(p).compare(target, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };

    return checkConfig(activeConfiguration()) || checkConfig(pendingConfiguration());
}

void SttModelSession::onEngineStateChanged()
{
    if (!m_engine) return;

    SttEngine::State engState = m_engine->state();
    
    // Map engine state directly to lifecycle controller triggers
    if (engState == SttEngine::Ready) {
        m_lifecycle->onLoadSuccess();
    } else if (engState == SttEngine::Unloaded) {
        m_lifecycle->onUnloadFinished();
    } else if (engState == SttEngine::Processing) {
        m_lifecycle->onProcessingStateChanged(true);
    } else if (engState == SttEngine::Error) {
        m_lifecycle->onEngineError(QStringLiteral("STT Engine entered error state"));
    } else {
        // If it transitions from Processing to Ready/Error/Unloaded
        if (m_lifecycle->state() == ModelSessionState::Processing) {
            m_lifecycle->onProcessingStateChanged(false);
        }
    }
}

std::optional<SessionConfiguration> SttModelSession::resolveConfig(const StudioConfiguration &config)
{
    auto resolved = StudioConfigurationResolver::resolve(config);
    if (!resolved.isValid) {
        return std::nullopt;
    }

    SessionConfiguration out;
    out.capabilityId = config.capabilityId;
    out.selection = config;
    out.runtimePath = resolved.runtimePath;
    out.signature = resolved.signature;

    // Resolve paths values
    for (const auto &val : resolved.resolvedPaths.values()) {
        out.resolvedModelPaths.append(val.toString());
    }

    return out;
}

void SttModelSession::performLoad(const SessionConfiguration &config)
{
    if (!m_engine) {
        m_lifecycle->onLoadError(QStringLiteral("STT Engine is null"));
        return;
    }

    // STT Engine needs model path (usually key "model") and runtime path
    // Let's resolve the specific model path from selection mapping
    auto resolved = StudioConfigurationResolver::resolve(config.selection);
    QString modelPath = resolved.resolvedPaths.value(QStringLiteral("model")).toString();

    Logger::info(QStringLiteral("SttModelSession"), QStringLiteral("performLoad modelPath: %1, runtimePath: %2").arg(modelPath, config.runtimePath));
    m_engine->loadModel(modelPath, false, config.runtimePath);
}

void SttModelSession::performUnload()
{
    if (m_engine) {
        Logger::info(QStringLiteral("SttModelSession"), QStringLiteral("performUnload"));
        m_engine->unloadModel();
    } else {
        m_lifecycle->onUnloadFinished();
    }
}

} // namespace LAStudio
