#include "ModelSessionRegistry.h"
#include "SttModelSession.h"
#include "TtsSharedModelSession.h"
#include "core/StudioCapabilityRegistry.h"
#include "core/Logger.h"

#include <QFileInfo>

namespace LAStudio {

ModelSessionRegistry::ModelSessionRegistry(SttEngine *sttEngine, TtsEngine *ttsEngine, QObject *parent)
    : QObject(parent)
{
    m_sttSession = new SttModelSession(sttEngine, this);
    m_ttsSession = new TtsSharedModelSession(ttsEngine, this);
}

IModelSession *ModelSessionRegistry::sessionForCapability(const QString &capabilityId) const
{
    StudioCapabilityDescriptor desc = StudioCapabilityRegistry::instance()->getCapability(capabilityId);
    if (desc.sharedEngineGroup == QStringLiteral("stt")) {
        return m_sttSession;
    }
    if (desc.sharedEngineGroup == QStringLiteral("tts-shared")) {
        return m_ttsSession;
    }
    return nullptr;
}

QList<IModelSession *> ModelSessionRegistry::sessions() const
{
    QList<IModelSession *> out;
    if (m_sttSession) {
        out.append(m_sttSession);
    }
    if (m_ttsSession) {
        out.append(m_ttsSession);
    }
    return out;
}

ResourceReleaseResult ModelSessionRegistry::prepareRuntimeRemoval(const QString &runtimeId,
                                                                   const QString &runtimeVersion)
{
    Logger::info(QStringLiteral("ModelSessionRegistry"),
                 QStringLiteral("prepareRuntimeRemoval: %1 %2").arg(runtimeId, runtimeVersion));

    QList<IModelSession*> sessions = { m_sttSession, m_ttsSession };
    ResourceReleaseResult overallResult = ResourceReleaseResult::NotInUse;

    for (IModelSession *session : sessions) {
        if (session && session->usesRuntime(runtimeId, runtimeVersion)) {
            ModelSessionState state = session->state();
            if (state == ModelSessionState::Processing) {
                return ResourceReleaseResult::BusyProcessing;
            }
            
            Logger::info(QStringLiteral("ModelSessionRegistry"),
                         QStringLiteral("Requesting unload of session due to runtime removal. State: %1")
                         .arg(static_cast<int>(state)));
            for (const SessionConfiguration &config : session->loadedConfigurations()) {
                if (config.selection.runtimeId != runtimeId) {
                    continue;
                }
                if (!runtimeVersion.isEmpty() && config.selection.runtimeVersion != runtimeVersion) {
                    continue;
                }
                session->requestUnloadConfiguration(config.signature);
            }
            overallResult = ResourceReleaseResult::Pending;
        }
    }

    return overallResult;
}

ResourceReleaseResult ModelSessionRegistry::prepareModelRemoval(const QString &modelPath)
{
    Logger::info(QStringLiteral("ModelSessionRegistry"),
                 QStringLiteral("prepareModelRemoval: %1").arg(modelPath));

    QList<IModelSession*> sessions = { m_sttSession, m_ttsSession };
    ResourceReleaseResult overallResult = ResourceReleaseResult::NotInUse;

    for (IModelSession *session : sessions) {
        if (session && session->usesModelPath(modelPath)) {
            ModelSessionState state = session->state();
            if (state == ModelSessionState::Processing) {
                return ResourceReleaseResult::BusyProcessing;
            }

            Logger::info(QStringLiteral("ModelSessionRegistry"),
                         QStringLiteral("Requesting unload of session due to model removal. State: %1")
                         .arg(static_cast<int>(state)));
            const QString normalizedPath = QFileInfo(modelPath).canonicalFilePath();
            for (const SessionConfiguration &config : session->loadedConfigurations()) {
                bool usesPath = false;
                for (const QString &path : config.resolvedModelPaths) {
                    const QFileInfo info(path);
                    if (info.canonicalFilePath() == normalizedPath || info.absoluteFilePath() == modelPath) {
                        usesPath = true;
                        break;
                    }
                }
                if (usesPath) {
                    session->requestUnloadConfiguration(config.signature);
                }
            }
            overallResult = ResourceReleaseResult::Pending;
        }
    }

    return overallResult;
}

} // namespace LAStudio
