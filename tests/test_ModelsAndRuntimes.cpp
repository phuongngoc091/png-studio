#include "test_ModelsAndRuntimes.h"
#include <QtTest>
#include <QThreadPool>

#include "core/CapabilityFamilyModel.h"
#include "core/CatalogManager.h"
#include "core/RegistryManager.h"
#include "core/Settings.h"
#include "core/RuntimeManager.h"
#include "core/ModelManager.h"
#include "core/LogViewService.h"
#include "controllers/StudioConfigurationResolver.h"

#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSet>
#include <QTemporaryDir>

namespace LAStudio {

void TestModelsAndRuntimes::cleanupTestCase()
{
    QThreadPool::globalInstance()->waitForDone();
}

void TestModelsAndRuntimes::testModelManagerConcreteModelDir()
{
    qDebug() << "--- START: testModelManagerConcreteModelDir ---";
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString modelsRoot = tempDir.filePath(QStringLiteral("models"));
    ModelManager models;
    models.setModelsRoot(modelsRoot);

    QCOMPARE(QDir(models.concreteModelDir(QStringLiteral("cstr/example-GGUF"))).absolutePath(),
             QDir(modelsRoot + QStringLiteral("/cstr/example-GGUF")).absolutePath());

    QDir().mkpath(models.concreteModelDir(QStringLiteral("cstr/example-GGUF")));
    QFile directFile(QDir(models.concreteModelDir(QStringLiteral("cstr/example-GGUF")))
                         .absoluteFilePath(QStringLiteral("model.gguf")));
    QVERIFY(directFile.open(QIODevice::WriteOnly));
    directFile.write("direct");
    directFile.close();

    const QString legacyNestedDir = QDir(modelsRoot).absoluteFilePath(QStringLiteral("models/cstr/legacy-GGUF"));
    QDir().mkpath(legacyNestedDir);
    QFile legacyFile(QDir(legacyNestedDir).absoluteFilePath(QStringLiteral("legacy.gguf")));
    QVERIFY(legacyFile.open(QIODevice::WriteOnly));
    legacyFile.write("legacy");
    legacyFile.close();

    models.scanLocalModels();
    QVERIFY(models.hasFile(QStringLiteral("cstr/example-GGUF"), QStringLiteral("model.gguf")));
    QVERIFY(models.hasFile(QStringLiteral("cstr/legacy-GGUF"), QStringLiteral("legacy.gguf")));
}

void TestModelsAndRuntimes::testCapabilityFamilyModelSuitability()
{
    qDebug() << "--- START: testCapabilityFamilyModelSuitability ---";
    ModelManager models;
    Settings settings;
    RuntimeManager runtimes(nullptr, &settings);
    CapabilityFamilyModel model(&models, &runtimes, nullptr, &settings);

    QVariantMap family;
    family[QStringLiteral("file")] = QStringLiteral("default.gguf");
    family[QStringLiteral("size")] = QStringLiteral("2.0 GB");

    QVariantMap requirement;
    requirement[QStringLiteral("file")] = QStringLiteral("large.gguf");
    requirement[QStringLiteral("size")] = QStringLiteral("120.0 GB");

    // Suitability check should respect the requirement size (120 GB) rather than default family (2 GB)
    // and label it as unsuitable on typical test systems without 128GB VRAM/RAM.
    bool suitable = model.isModelSuitable(QStringLiteral("large.gguf"), family, requirement);
    QVERIFY(!suitable);
}

void TestModelsAndRuntimes::testVoiceDesignFamiliesExposeRuntimeOptions()
{
    qDebug() << "--- START: testVoiceDesignFamiliesExposeRuntimeOptions ---";
    const QString oldCurrentPath = QDir::currentPath();
    const auto restoreCurrentPath = qScopeGuard([oldCurrentPath]() {
        QDir::setCurrent(oldCurrentPath);
    });
    const QString repoRoot = QDir(QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QStringLiteral("/..")).absolutePath();
    QVERIFY2(QDir::setCurrent(repoRoot), "Test must run from the repository root to load catalog and schema files");

    CatalogManager catalog;
    RegistryManager registry;
    registry.initializeFromCatalog(&catalog);

    const QStringList expectedFamilyIds = {
        QStringLiteral("omnivoice"),
        QStringLiteral("qwen3-tts-1.7b-voicedesign")
    };

    for (const QString &familyId : expectedFamilyIds) {
        QVariantMap registryFamily;
        for (const QVariant &familyValue : registry.ttsFamilies()) {
            const QVariantMap family = familyValue.toMap();
            if (family.value(QStringLiteral("id")).toString() == familyId) {
                registryFamily = family;
                break;
            }
        }

        QVERIFY2(!registryFamily.isEmpty(), qPrintable(familyId + QStringLiteral(" should be available through the TTS-shared family pool")));
        QVERIFY2(!registryFamily.value(QStringLiteral("runtimes")).toList().isEmpty(),
                 qPrintable(familyId + QStringLiteral(" should keep its catalog runtime definitions")));
    }

    ModelManager models;
    Settings settings;
    RuntimeManager runtimes(&catalog, &settings);
    CapabilityFamilyModel familyModel(&models, &runtimes, &registry, &settings);
    familyModel.setCapability(QStringLiteral("voice-design"));

    for (const QString &familyId : expectedFamilyIds) {
        const QVariantMap modelItem = familyModel.itemForFamily(familyId);
        QVERIFY2(!modelItem.isEmpty(), qPrintable(QStringLiteral("CapabilityFamilyModel should expose ") + familyId));
        const QVariantList runtimeOptions = modelItem.value(QStringLiteral("runtimeOptions")).toList();
        QVERIFY2(!runtimeOptions.isEmpty(),
                 qPrintable(familyId + QStringLiteral(" should have runtime options to render for user selection")));

        QSet<QString> runtimeIds;
        for (const QVariant &runtimeValue : runtimeOptions) {
            const QString runtimeId = runtimeValue.toMap().value(QStringLiteral("id")).toString();
            QVERIFY2(!runtimeIds.contains(runtimeId),
                     qPrintable(familyId + QStringLiteral(" should expose one runtime row per runtime id")));
            runtimeIds.insert(runtimeId);
        }
    }
}

void TestModelsAndRuntimes::testLogViewServicePending()
{
    qDebug() << "--- START: testLogViewServicePending ---";
    // Verify that deleting LogViewService while thread pool task is pending does not crash the system.
    {
        LogViewService *service = new LogViewService();
        service->requestLoadLogs();
        delete service;
    }
    
    // Wait slightly to let any worker task run its course safely without referencing deleted service pointer
    QThreadPool::globalInstance()->waitForDone();
}

void TestModelsAndRuntimes::testStudioConfigurationResolver()
{
    qDebug() << "--- START: testStudioConfigurationResolver ---";
    StudioConfiguration config;
    config.capabilityId = QStringLiteral("stt");
    config.familyId = QStringLiteral("whisper-tiny");
    config.runtimeId = QStringLiteral("whisper.cpp");
    config.runtimeVersion = QStringLiteral("1.0");
    config.selectedFiles[QStringLiteral("model")] = QStringLiteral("whisper-tiny.gguf");

    auto resolved = StudioConfigurationResolver::resolve(config);
    QVERIFY(!resolved.isValid);
}

} // namespace LAStudio
