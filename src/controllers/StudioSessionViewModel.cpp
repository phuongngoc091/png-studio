#include "StudioSessionViewModel.h"
#include "controllers/AppController.h"
#include "controllers/ModelSessionRegistry.h"
#include "controllers/IModelSession.h"
#include "core/StudioCapabilityRegistry.h"
#include "controllers/StudioConfigurationResolver.h"
#include "core/Logger.h"
#include "core/RegistryManager.h"
#include "core/HardwareManager.h"
#include <QTimer>

namespace LAStudio {

StudioSessionViewModel::StudioSessionViewModel(QObject *parent)
    : QObject(parent)
{
    AppController *app = AppController::instance();
    if (app) {
        m_familiesModel = new CapabilityFamilyModel(app->models(), app->runtimes(), app->registry(), app->settings(), this);
        if (app->registry()) {
            m_repository = new StudioSelectionRepository(app->registry()->connectionName(), this);
            m_repository->migrateLegacySelectionsIfNeeded(app->settings());
        }
    } else {
        m_familiesModel = new CapabilityFamilyModel(nullptr, nullptr, nullptr, nullptr, this);
    }
    if (app && app->tts()) {
        connect(app->tts(), &TtsEngine::cpuUsageChanged, this, &StudioSessionViewModel::stateChanged);
        connect(app->tts(), &TtsEngine::memoryUsageChanged, this, &StudioSessionViewModel::stateChanged);
    }
}

void StudioSessionViewModel::setCapabilityId(const QString &id)
{
    if (m_capabilityId == id) return;
    m_capabilityId = id;
    emit capabilityIdChanged();

    m_familiesModel->setCapability(m_capabilityId);
    initializeAction();
    syncSelectionFromSettings();
}

void StudioSessionViewModel::initializeAction()
{
    if (m_action) {
        m_action->disconnect(this);
        m_action->deleteLater();
        m_action = nullptr;
    }

    m_action = StudioCapabilityRegistry::instance()->createAction(m_capabilityId, this);

    if (m_action) {
        connect(m_action, &IStudioAction::stateChanged, this, &StudioSessionViewModel::onActionStateChanged);
        connect(m_action, &IStudioAction::activeConfigurationChanged, this, &StudioSessionViewModel::onActionConfigurationChanged);
    }
}

QString StudioSessionViewModel::selectedFamilyId() const
{
    if (m_selectionCommitted && m_repository) {
        return m_repository->selectionFor(m_capabilityId).familyId;
    }
    return m_pendingFamilyId;
}

QString StudioSessionViewModel::runtimeId() const
{
    if (m_selectionCommitted && m_repository) {
        return m_repository->selectionFor(m_capabilityId).runtimeId;
    }
    return m_pendingRuntimeId;
}

QString StudioSessionViewModel::runtimeVersion() const
{
    if (m_selectionCommitted && m_repository) {
        return m_repository->selectionFor(m_capabilityId).runtimeVersion;
    }
    return m_pendingRuntimeVersion;
}

QVariantMap StudioSessionViewModel::selectedFiles() const
{
    if (m_selectionCommitted && m_repository) {
        const StudioConfiguration committed = m_repository->selectionFor(m_capabilityId);
        if (m_pendingFamilyId.isEmpty() || m_pendingFamilyId == committed.familyId) {
            return committed.selectedFiles;
        }
    }
    return m_pendingFiles;
}

bool StudioSessionViewModel::studioReady() const
{
    return modelActive();
}

bool StudioSessionViewModel::modelActive() const
{
    if (!m_action) return false;
    StudioState s = m_action->state();
    return s == StudioState::Ready || s == StudioState::Processing;
}

bool StudioSessionViewModel::canProcess() const
{
    return m_action && m_action->state() == StudioState::Ready;
}

QString StudioSessionViewModel::activeSignature() const
{
    return m_action ? m_action->activeConfigurationSignature() : QString();
}

QString StudioSessionViewModel::pendingSignature() const
{
    AppController *app = AppController::instance();
    if (app && app->sessionRegistry()) {
        IModelSession *session = app->sessionRegistry()->sessionForCapability(m_capabilityId);
        if (session && session->pendingConfiguration()) {
            return session->pendingConfiguration()->signature;
        }
    }
    return QString();
}

QString StudioSessionViewModel::lastError() const
{
    return m_action ? m_action->errorDetail() : QString();
}

StudioState StudioSessionViewModel::state() const
{
    return m_action ? m_action->state() : StudioState::Unloaded;
}

QString StudioSessionViewModel::statusText() const
{
    switch (state()) {
    case StudioState::Unloaded: return QStringLiteral("Unloaded");
    case StudioState::Loading: return QStringLiteral("Loading");
    case StudioState::Ready: return QStringLiteral("Ready");
    case StudioState::Processing: return QStringLiteral("Processing");
    case StudioState::Error: return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

QString StudioSessionViewModel::statusDetail() const
{
    if (m_action && !m_action->errorDetail().isEmpty()) {
        return m_action->errorDetail();
    }
    if (m_selectionCommitted) {
        return studioReady() ? QStringLiteral("Loaded and ready") : QStringLiteral("Configuration saved (unloaded)");
    }
    return QStringLiteral("Configure a model family to start");
}

double StudioSessionViewModel::cpuUsage() const
{
    AppController *app = AppController::instance();
    if (!app || m_capabilityId == QStringLiteral("stt")) return 0.0;
    return app->tts()->cpuUsage();
}

QString StudioSessionViewModel::estimatedRamUsage() const
{
    AppController *app = AppController::instance();
    if (!app || m_capabilityId == QStringLiteral("stt")) return {};
    return app->tts()->estimatedRamUsage();
}

QString StudioSessionViewModel::estimatedVramUsage() const
{
    AppController *app = AppController::instance();
    if (!app || m_capabilityId == QStringLiteral("stt")) return {};
    return app->tts()->estimatedVramUsage();
}

QVariantMap StudioSessionViewModel::getFamilyConfig(const QString &familyId) const
{
    AppController* app = AppController::instance();
    if (!app) return {};

    const QString domain = StudioCapabilityRegistry::instance()->familyDomain(m_capabilityId);
    const QVariantList all = domain == QStringLiteral("stt")
        ? app->registry()->sttFamilies()
        : app->registry()->ttsFamilies();
    for (const QVariant &item : all) {
        QVariantMap fam = item.toMap();
        if (fam.value(QStringLiteral("id")).toString() == familyId) {
            return fam;
        }
    }
    return {};
}

QVariantList StudioSessionViewModel::families() const
{
    AppController* app = AppController::instance();
    if (!app) return {};

    const QString domain = StudioCapabilityRegistry::instance()->familyDomain(m_capabilityId);
    const QVariantList all = domain == QStringLiteral("stt")
        ? app->registry()->sttFamilies()
        : app->registry()->ttsFamilies();
    QVariantList out;
    for (const QVariant &item : all) {
        QVariantMap fam = item.toMap();
        if (StudioCapabilityRegistry::instance()->familySupportsCapability(fam, m_capabilityId)) {
            out.append(item);
        }
    }
    return out;
}

void StudioSessionViewModel::openConfiguration(const QString &familyId)
{
    selectFamily(familyId);
    emit configurationGalleryRequestReset();
}

void StudioSessionViewModel::selectFamily(const QString &familyId)
{
    m_pendingFamilyId = familyId;
    m_pendingFiles.clear();
    m_familiesModel->setSelectedFamilyId(m_pendingFamilyId);

    // Resolve compatible runtime default
    QVariantMap family = getFamilyConfig(familyId);
    QVariantList runtimes = family.value(QStringLiteral("runtimes")).toList();
    
    m_pendingRuntimeId.clear();
    m_pendingRuntimeVersion.clear();

    for (const QVariant &rtVal : runtimes) {
        QVariantMap rt = rtVal.toMap();
        QVariantMap comp = HardwareManager::instance()->runtimeCompatibility(rt);
        if (comp.value(QStringLiteral("compatible")).toBool()) {
            m_pendingRuntimeId = rt.value(QStringLiteral("id")).toString();
            m_pendingRuntimeVersion = rt.value(QStringLiteral("version")).toString();
            break;
        }
    }

    emit selectionChanged();
}

void StudioSessionViewModel::selectRuntime(const QString &runtimeId, const QString &version)
{
    m_pendingRuntimeId = runtimeId;
    m_pendingRuntimeVersion = version;
    emit selectionChanged();
}

void StudioSessionViewModel::commitSelection()
{
    if (m_pendingFamilyId.isEmpty()) return;

    StudioConfiguration config;
    config.capabilityId = m_capabilityId;
    config.familyId = m_pendingFamilyId;
    config.runtimeId = m_pendingRuntimeId;
    config.runtimeVersion = m_pendingRuntimeVersion;

    // Build requirement selections
    QVariantMap family = getFamilyConfig(m_pendingFamilyId);
    QVariantList reqFiles = family.value(QStringLiteral("requiredFiles")).toList();
    AppController* app = AppController::instance();
    
    for (const QVariant &reqVal : reqFiles) {
        QVariantMap req = reqVal.toMap();
        QString role = req.value(QStringLiteral("role")).toString();
        
        QString selected = m_pendingFiles.value(role).toString();
        if (selected.isEmpty()) {
            // Find the best installed candidate when the user did not choose one.
            selected = req.value(QStringLiteral("file")).toString();
            QVariantList candidates = req.value(QStringLiteral("candidates")).toList();
            for (const QVariant &cand : candidates) {
                QString modelId = req.value(QStringLiteral("modelId")).toString();
                if (modelId.isEmpty()) modelId = family.value(QStringLiteral("modelId")).toString();
                if (app->models()->hasFile(modelId, cand.toString())) {
                    selected = cand.toString();
                    break;
                }
            }
        }
        config.selectedFiles.insert(role, selected);
    }

    if (m_repository) {
        m_repository->saveActiveSelection(config);
    }

    m_selectionCommitted = true;
    emit selectionChanged();
    emit studioContextChanged(config.familyId, config.runtimeId, config.runtimeVersion);
    emit configurationDialogClosed();

    loadSelectedConfiguration();
}

void StudioSessionViewModel::loadSelectedConfiguration()
{
    if (!m_action || !m_repository) return;
    StudioConfiguration config = m_repository->selectionFor(m_capabilityId);
    if (config.isValid()) {
        m_action->load(config);
    }
}

void StudioSessionViewModel::unload()
{
    if (m_action) {
        m_action->unload();
    }
}

void StudioSessionViewModel::reload()
{
    if (m_action) {
        if (state() == StudioState::Unloaded || state() == StudioState::Error) {
            loadSelectedConfiguration();
        } else {
            m_action->reload();
        }
    }
}

void StudioSessionViewModel::syncSelectionFromSettings()
{
    if (!m_repository) return;
    StudioConfiguration config = m_repository->selectionFor(m_capabilityId);
    if (config.isValid()) {
        m_pendingFamilyId = config.familyId;
        m_pendingRuntimeId = config.runtimeId;
        m_pendingRuntimeVersion = config.runtimeVersion;
        m_pendingFiles = config.selectedFiles;
        m_familiesModel->setSelectedFamilyId(m_pendingFamilyId);
        m_selectionCommitted = true;
        
        emit selectionChanged();
        emit studioContextChanged(m_pendingFamilyId, m_pendingRuntimeId, m_pendingRuntimeVersion);

        // Keep STT unloaded on startup to avoid unnecessary memory usage.
        if (m_capabilityId != QStringLiteral("stt")) {
            QTimer::singleShot(100, this, &StudioSessionViewModel::loadSelectedConfiguration);
        }
    } else {
        m_selectionCommitted = false;
        m_pendingFamilyId.clear();
        m_pendingRuntimeId.clear();
        m_pendingRuntimeVersion.clear();
        m_pendingFiles.clear();
        m_familiesModel->setSelectedFamilyId(QString());
        emit selectionChanged();
    }
}

void StudioSessionViewModel::commitConfigurationSelection(const QString &familyId, const QString &runtimeId, const QString &runtimeVersion, const QVariantMap &selectedFiles)
{
    m_pendingFamilyId = familyId;
    m_pendingRuntimeId = runtimeId;
    m_pendingRuntimeVersion = runtimeVersion;
    m_pendingFiles = selectedFiles;
    commitSelection();
}

void StudioSessionViewModel::autoLoadIfReady()
{
    loadSelectedConfiguration();
}

void StudioSessionViewModel::onActionStateChanged()
{
    emit stateChanged();
}

void StudioSessionViewModel::onActionConfigurationChanged()
{
    emit stateChanged();
}

// Presentational helper implementations
QString StudioSessionViewModel::studioHeaderTitle() const
{
    StudioCapabilityDescriptor desc = StudioCapabilityRegistry::instance()->getCapability(m_capabilityId);
    return desc.pageTitle;
}

QString StudioSessionViewModel::modalSelectionTitle() const
{
    QVariantMap family = getFamilyConfig(selectedFamilyId());
    return family.value(QStringLiteral("title"), QStringLiteral("Model + Runtime")).toString();
}

QString StudioSessionViewModel::modalSelectionValue() const
{
    return runtimeDisplayText();
}

QString StudioSessionViewModel::modalSelectionDetail() const
{
    return statusDetail();
}

QString StudioSessionViewModel::runtimeDisplayText() const
{
    QVariantMap family = getFamilyConfig(selectedFamilyId());
    QString rtId = runtimeId();
    QString rtVer = runtimeVersion();
    if (rtId.isEmpty()) return QStringLiteral("Select model and runtime");

    QVariantList runtimes = family.value(QStringLiteral("runtimes")).toList();
    for (const QVariant &rtVal : runtimes) {
        QVariantMap rt = rtVal.toMap();
        if (rt.value(QStringLiteral("id")).toString() == rtId) {
            QString name = rt.value(QStringLiteral("name")).toString();
            return rtVer.isEmpty() ? name : name + QStringLiteral("  ") + rtVer;
        }
    }
    return rtId;
}

} // namespace LAStudio
