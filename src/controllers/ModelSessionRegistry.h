#pragma once
#include <QObject>
#include <QHash>
#include "IModelSession.h"

namespace LAStudio {

enum class ResourceReleaseResult {
    NotInUse,
    Released,
    Pending,
    BusyProcessing,
    Failed
};

class SttEngine;
class TtsEngine;

class ModelSessionRegistry : public QObject {
    Q_OBJECT
public:
    explicit ModelSessionRegistry(SttEngine *sttEngine, TtsEngine *ttsEngine, QObject *parent = nullptr);
    ~ModelSessionRegistry() override = default;

    IModelSession *sessionForCapability(const QString &capabilityId) const;

    ResourceReleaseResult prepareRuntimeRemoval(const QString &runtimeId,
                                                 const QString &runtimeVersion);

    ResourceReleaseResult prepareModelRemoval(const QString &modelPath);

private:
    IModelSession *m_sttSession = nullptr;
    IModelSession *m_ttsSession = nullptr;
};

} // namespace LAStudio
