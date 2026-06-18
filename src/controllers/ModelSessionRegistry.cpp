#include "ModelSessionRegistry.h"
#include "SttModelSession.h"
#include "TtsSharedModelSession.h"
#include "core/StudioCapabilityRegistry.h"
#include "core/Logger.h"

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
            
            // For Ready, Loading, Unloading, Error: trigger unload
            QString capId;
            auto active = session->activeConfiguration();
            if (active) {
                capId = active->capabilityId;
            } else {
                auto pending = session->pendingConfiguration();
                if (pending) {
                    capId = pending->capabilityId;
                }
            }

            if (capId.isEmpty()) {
                // Default fallback capId
                if (session == m_sttSession) capId = QStringLiteral("stt");
                else capId = QStringLiteral("tts");
            }

            Logger::info(QStringLiteral("ModelSessionRegistry"),
                         QStringLiteral("Requesting unload of session due to runtime removal. State: %1")
                         .arg(static_cast<int>(state)));
            session->requestUnload(capId);
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

            QString capId;
            auto active = session->activeConfiguration();
            if (active) {
                capId = active->capabilityId;
            } else {
                auto pending = session->pendingConfiguration();
                if (pending) {
                    capId = pending->capabilityId;
                }
            }

            if (capId.isEmpty()) {
                if (session == m_sttSession) capId = QStringLiteral("stt");
                else capId = QStringLiteral("tts");
            }

            Logger::info(QStringLiteral("ModelSessionRegistry"),
                         QStringLiteral("Requesting unload of session due to model removal. State: %1")
                         .arg(static_cast<int>(state)));
            session->requestUnload(capId);
            overallResult = ResourceReleaseResult::Pending;
        }
    }

    return overallResult;
}

} // namespace LAStudio
