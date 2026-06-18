#include "TtsSharedModelSession.h"
#include "tts/TtsEngine.h"
#include "controllers/StudioConfigurationResolver.h"
#include "core/Logger.h"
#include <QFileInfo>

namespace LAStudio {

TtsSharedModelSession::TtsSharedModelSession(TtsEngine *engine, QObject *parent)
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
        auto current = m_lifecycle->state();
        if (current != m_lastLifecycleState) {
            m_lastLifecycleState = current;
            emit stateChanged();
        }
    });
    connect(m_lifecycle, &ModelLifecycleController::activeConfigurationChanged, this, &TtsSharedModelSession::activeConfigurationChanged);
    connect(m_lifecycle, &ModelLifecycleController::pendingConfigurationChanged, this, &TtsSharedModelSession::pendingConfigurationChanged);
    connect(m_lifecycle, &ModelLifecycleController::activeSignatureChanged, this, &TtsSharedModelSession::activeSignatureChanged);
    connect(m_lifecycle, &ModelLifecycleController::errorOccurred, this, &TtsSharedModelSession::errorOccurred);

    if (m_engine) {
        connect(m_engine, &TtsEngine::stateChanged, this, &TtsSharedModelSession::onEngineStateChanged);
        connect(m_engine, &TtsEngine::errorOccurred, this, [this](const QString &err) {
            m_lifecycle->onLoadError(err);
        });
    }
}

ModelSessionState TtsSharedModelSession::state() const
{
    return m_lifecycle->state();
}

bool TtsSharedModelSession::modelActive() const
{
    return m_lifecycle->modelActive();
}

bool TtsSharedModelSession::canProcess() const
{
    return m_lifecycle->canProcess();
}

std::optional<SessionConfiguration> TtsSharedModelSession::activeConfiguration() const
{
    auto conf = m_lifecycle->activeConfiguration();
    if (conf) {
        conf->capabilityId = m_activeOwner;
    }
    return conf;
}

std::optional<SessionConfiguration> TtsSharedModelSession::pendingConfiguration() const
{
    auto conf = m_lifecycle->pendingConfiguration();
    if (conf) {
        conf->capabilityId = m_pendingOwner.isEmpty() ? m_activeOwner : m_pendingOwner;
    }
    return conf;
}

QString TtsSharedModelSession::activeSignature() const
{
    return m_lifecycle->activeSignature();
}

bool TtsSharedModelSession::isOwnerActive(const QString &capabilityId) const
{
    return !capabilityId.isEmpty() && capabilityId == m_activeOwner;
}

bool TtsSharedModelSession::isOwnerPending(const QString &capabilityId) const
{
    return !capabilityId.isEmpty() && capabilityId == m_pendingOwner;
}

void TtsSharedModelSession::requestLoad(const QString &capabilityId, const StudioConfiguration &configuration)
{
    Logger::info(QStringLiteral("TtsSharedModelSession"),
                 QStringLiteral("requestLoad from capability: %1").arg(capabilityId));

    auto resolvedOpt = resolveConfig(configuration);
    if (!resolvedOpt) {
        m_lifecycle->onLoadError(QStringLiteral("Failed to resolve configuration"));
        return;
    }

    SessionConfiguration resolved = *resolvedOpt;

    // Check if signature matches current active signature
    if (m_lifecycle->state() == ModelSessionState::Ready && m_lifecycle->activeSignature() == resolved.signature) {
        if (m_engine) {
            m_engine->setFamilyConfig(resolved.familyConfig);
            m_engine->setRuntimePath(resolved.runtimePath);
        }
        if (m_activeOwner != capabilityId) {
            Logger::info(QStringLiteral("TtsSharedModelSession"),
                         QStringLiteral("Signature matches. Switching owner from %1 to %2 without reloading engine.")
                         .arg(m_activeOwner, capabilityId));
            m_activeOwner = capabilityId;
            emit activeConfigurationChanged();
            emit activeSignatureChanged();
        }
        return;
    }

    m_pendingOwner = capabilityId;
    m_lifecycle->requestLoad(capabilityId, configuration);
}

void TtsSharedModelSession::requestUnload(const QString &capabilityId)
{
    if (m_activeOwner == capabilityId || m_pendingOwner == capabilityId) {
        m_lifecycle->requestUnload(capabilityId);
    }
}

void TtsSharedModelSession::requestReload(const QString &capabilityId)
{
    if (m_activeOwner == capabilityId) {
        m_pendingOwner = capabilityId;
        m_lifecycle->requestReload(capabilityId);
    }
}

bool TtsSharedModelSession::usesRuntime(const QString &runtimeId, const QString &runtimeVersion) const
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

bool TtsSharedModelSession::usesModelPath(const QString &modelPath) const
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

void TtsSharedModelSession::onEngineStateChanged()
{
    if (!m_engine) return;

    TtsEngine::State engState = m_engine->state();

    if (engState == TtsEngine::Ready) {
        if (!m_pendingOwner.isEmpty()) {
            m_activeOwner = m_pendingOwner;
            m_pendingOwner.clear();
        }
        m_lifecycle->onLoadSuccess();
    } else if (engState == TtsEngine::Unloaded) {
        m_activeOwner.clear();
        m_pendingOwner.clear();
        m_lifecycle->onUnloadFinished();
    } else if (engState == TtsEngine::Processing) {
        m_lifecycle->onProcessingStateChanged(true);
    } else if (engState == TtsEngine::Error) {
        m_activeOwner.clear();
        m_pendingOwner.clear();
        m_lifecycle->onEngineError(QStringLiteral("TTS Engine entered error state"));
    } else {
        if (m_lifecycle->state() == ModelSessionState::Processing) {
            m_lifecycle->onProcessingStateChanged(false);
        }
    }
}

std::optional<SessionConfiguration> TtsSharedModelSession::resolveConfig(const StudioConfiguration &config)
{
    auto resolved = StudioConfigurationResolver::resolve(config);
    if (!resolved.isValid) {
        return std::nullopt;
    }

    SessionConfiguration out;
    out.capabilityId = config.capabilityId;
    out.selection = config;
    out.runtimePath = resolved.runtimePath;
    out.familyConfig = resolved.family;
    out.signature = resolved.signature;

    for (const auto &val : resolved.resolvedPaths.values()) {
        out.resolvedModelPaths.append(val.toString());
    }

    return out;
}

void TtsSharedModelSession::performLoad(const SessionConfiguration &config)
{
    if (!m_engine) {
        m_lifecycle->onLoadError(QStringLiteral("TTS Engine is null"));
        return;
    }

    auto resolved = StudioConfigurationResolver::resolve(config.selection);
    if (!resolved.isValid) {
        m_lifecycle->onLoadError(QStringLiteral("Failed to resolve configuration during load"));
        return;
    }

    m_engine->clearLastSamples();
    m_engine->setFamilyConfig(resolved.family);
    m_engine->setRuntimePath(config.runtimePath);
    m_engine->loadModel(resolved.resolvedPaths, config.runtimePath);
}

void TtsSharedModelSession::performUnload()
{
    if (m_engine) {
        m_engine->unloadVoice();
    } else {
        m_lifecycle->onUnloadFinished();
    }
}

} // namespace LAStudio
