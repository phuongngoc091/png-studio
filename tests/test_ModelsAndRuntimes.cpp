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

void TestModelsAndRuntimes::testModelManagerResolvesSplitVirtualModelFiles()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString modelsRoot = tempDir.filePath(QStringLiteral("models"));
    const QString codecDir = QDir(modelsRoot).absoluteFilePath(QStringLiteral("pnnbao-ump/VieNeu-Codec"));
    const QString ggufDir = QDir(modelsRoot).absoluteFilePath(QStringLiteral("pnnbao-ump/VieNeu-TTS-v2-Turbo-GGUF"));
    QVERIFY(QDir().mkpath(codecDir));
    QVERIFY(QDir().mkpath(ggufDir));

    auto writeFile = [](const QString &path, const QByteArray &data) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write(data);
        return true;
    };

    QVERIFY(writeFile(QDir(codecDir).absoluteFilePath(QStringLiteral(".la-info.json")),
                      R"({"id":"vieneu-tts-v2-turbo","task":"tts"})"));
    QVERIFY(writeFile(QDir(codecDir).absoluteFilePath(QStringLiteral("vieneu_encoder.onnx")), "encoder"));
    QVERIFY(writeFile(QDir(codecDir).absoluteFilePath(QStringLiteral("vieneu_decoder.onnx")), "decoder"));
    QVERIFY(writeFile(QDir(ggufDir).absoluteFilePath(QStringLiteral(".la-info.json")),
                      R"({"id":"vieneu-tts-v2-turbo","task":"tts"})"));
    QVERIFY(writeFile(QDir(ggufDir).absoluteFilePath(QStringLiteral("vieneu-tts-v2-turbo.gguf")), "gguf"));
    QVERIFY(writeFile(QDir(ggufDir).absoluteFilePath(QStringLiteral("voices.json")), "{}"));

    ModelManager models;
    models.setModelsRoot(modelsRoot);
    models.scanLocalModels();

    QCOMPARE(QFileInfo(models.filePath(QStringLiteral("pnnbao-ump/VieNeu-TTS-v2-Turbo-GGUF"),
                                       QStringLiteral("vieneu-tts-v2-turbo.gguf"))).absoluteFilePath(),
             QFileInfo(QDir(ggufDir).absoluteFilePath(QStringLiteral("vieneu-tts-v2-turbo.gguf"))).absoluteFilePath());
    QCOMPARE(QFileInfo(models.filePath(QStringLiteral("pnnbao-ump/VieNeu-Codec"),
                                       QStringLiteral("vieneu_encoder.onnx"))).absoluteFilePath(),
             QFileInfo(QDir(codecDir).absoluteFilePath(QStringLiteral("vieneu_encoder.onnx"))).absoluteFilePath());
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

void TestModelsAndRuntimes::testVieNeuV3CatalogIncludesMossExternalData()
{
    const QString oldCurrentPath = QDir::currentPath();
    const auto restoreCurrentPath = qScopeGuard([oldCurrentPath]() {
        QDir::setCurrent(oldCurrentPath);
    });
    const QString repoRoot = QDir(QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QStringLiteral("/..")).absolutePath();
    QVERIFY2(QDir::setCurrent(repoRoot), "Test must run from the repository root to load catalog and schema files");

    CatalogManager catalog;
    QVariantMap vieneuV3;
    for (const QVariant &familyValue : catalog.ttsFamilies()) {
        const QVariantMap family = familyValue.toMap();
        if (family.value(QStringLiteral("id")).toString() == QStringLiteral("vieneu-tts-v3-turbo")) {
            vieneuV3 = family;
            break;
        }
    }

    QVERIFY2(!vieneuV3.isEmpty(), "VieNeu-TTS v3 Turbo should be present in the catalog");
    QCOMPARE(vieneuV3.value(QStringLiteral("modelId")).toString(),
             QStringLiteral("lastudio-community/VieNeu-TTS-v3-Turbo-CPP"));

    QSet<QString> requiredFiles;
    QMap<QString, QVariantMap> requirementsByFile;
    for (const QVariant &reqValue : vieneuV3.value(QStringLiteral("requiredFiles")).toList()) {
        const QVariantMap req = reqValue.toMap();
        const QString file = req.value(QStringLiteral("file")).toString();
        requiredFiles.insert(file);
        requirementsByFile.insert(file, req);
    }

    QVERIFY2(requiredFiles.contains(QStringLiteral("backbone.gguf")),
             "VieNeu-TTS v3 native pipeline must require the GGUF semantic backbone");
    QVERIFY2(requiredFiles.contains(QStringLiteral("vieneu_v3_heads.npz")),
             "VieNeu-TTS v3 native pipeline must require native heads at the model root");
    QVERIFY2(requiredFiles.contains(QStringLiteral("acoustic/vieneu_acoustic_weights.npz")),
             "VieNeu-TTS v3 native pipeline must require acoustic native weights");
    QVERIFY2(requiredFiles.contains(QStringLiteral("voices_v3_turbo.json")),
             "VieNeu-TTS v3 native pipeline should install the published preset voice definitions");
    QVERIFY2(!requiredFiles.contains(QStringLiteral("onnx/vieneu_prefill.onnx")),
             "VieNeu-TTS v3 native catalog should not require the ONNX prefill graph");
    QVERIFY2(!requiredFiles.contains(QStringLiteral("onnx/vieneu_decode_step.onnx")),
             "VieNeu-TTS v3 native catalog should not require the ONNX decode-step graph");
    QVERIFY2(!requiredFiles.contains(QStringLiteral("onnx/vieneu_backbone_shared.data")),
             "VieNeu-TTS v3 native catalog should not require ONNX external backbone data");
    QVERIFY2(requiredFiles.contains(QStringLiteral("codec/moss_audio_tokenizer_decode_shared.data")),
             "MOSS decoder ONNX external data must be installed with the decoder graph");
    QVERIFY2(requiredFiles.contains(QStringLiteral("codec/moss_audio_tokenizer_encode.data")),
             "MOSS encoder ONNX external data must be installed with the encoder graph");

    const QVariantMap voiceCloningStudio =
        vieneuV3.value(QStringLiteral("studio")).toMap().value(QStringLiteral("voice-cloning")).toMap();
    const QVariantMap cloneDefaults = voiceCloningStudio.value(QStringLiteral("parameterDefaults")).toMap();
    QCOMPARE(cloneDefaults.value(QStringLiteral("temperature")).toDouble(), 0.8);
    QCOMPARE(cloneDefaults.value(QStringLiteral("top_k")).toInt(), 25);
    QCOMPARE(cloneDefaults.value(QStringLiteral("top_p")).toDouble(), 0.95);
    QCOMPARE(cloneDefaults.value(QStringLiteral("max_new_frames")).toInt(), 180);

    const QString cppRepo = QStringLiteral("lastudio-community/VieNeu-TTS-v3-Turbo-CPP");
    for (const QString &file : requiredFiles) {
        const QVariantMap req = requirementsByFile.value(file);
        const QString reqModelId = req.value(QStringLiteral("modelId")).toString();
        QVERIFY2(reqModelId.isEmpty() || reqModelId == cppRepo,
                 qPrintable(QStringLiteral("VieNeu-TTS v3 native file %1 must download from the CPP-ready repo").arg(file)));
    }

    const QVariantList runtimes = vieneuV3.value(QStringLiteral("runtimes")).toList();
    QVERIFY2(runtimes.size() >= 3, "VieNeu-TTS v3 should expose CPU, CUDA, and Vulkan runtime options");
    for (const QVariant &runtimeValue : runtimes) {
        const QVariantMap runtime = runtimeValue.toMap();
        QCOMPARE(runtime.value(QStringLiteral("engineFamily")).toString(), QStringLiteral("vieneu-tts"));
        QCOMPARE(runtime.value(QStringLiteral("backend")).toString(), QStringLiteral("vieneu-tts"));
        QCOMPARE(runtime.value(QStringLiteral("library")).toString(), QStringLiteral("vieneu-tts.dll"));
        QCOMPARE(runtime.value(QStringLiteral("version")).toString(), QStringLiteral("v0.1.1"));
        QCOMPARE(runtime.value(QStringLiteral("pipelineProfile")).toString(), QStringLiteral("vieneu-v3-native"));
        QVERIFY2(runtime.value(QStringLiteral("asset")).toString().startsWith(QStringLiteral("vieneu-tts-win-")),
                 "VieNeu-TTS v3 runtime assets should come from VieNeu-TTS.cpp release archives");
    }
}

void TestModelsAndRuntimes::testCapabilityFamilyModelRejectsIncompleteModelFiles()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString modelId = QStringLiteral("lastudio-community/VieNeu-TTS-v3-Turbo-CPP");
    const QString fileName = QStringLiteral("vieneu_v3_heads.npz");
    ModelManager models;
    models.setModelsRoot(tempDir.path());

    const QString modelDir = models.concreteModelDir(modelId);
    QVERIFY(QDir().mkpath(modelDir));
    QFile headsFile(QDir(modelDir).absoluteFilePath(fileName));
    QVERIFY(headsFile.open(QIODevice::WriteOnly));
    headsFile.write("partial");
    headsFile.close();

    models.scanLocalModels();

    CapabilityFamilyModel familyModel(&models, nullptr, nullptr, nullptr);
    QVariantMap family;
    family.insert(QStringLiteral("modelId"), modelId);
    family.insert(QStringLiteral("localDir"), modelId);

    QVariantMap requirement;
    requirement.insert(QStringLiteral("file"), fileName);
    requirement.insert(QStringLiteral("size"), QStringLiteral("25 MB"));

    QVERIFY2(!familyModel.isFileInstalled(family, fileName, requirement),
             "Tiny interrupted model files must not count as installed");

    QVERIFY(headsFile.open(QIODevice::WriteOnly));
    QVERIFY(headsFile.resize(20 * 1024 * 1024));
    headsFile.close();

    QVERIFY2(familyModel.isFileInstalled(family, fileName, requirement),
             "Runtime-normalized NPZ files can be smaller than the original stored archive");
}

void TestModelsAndRuntimes::testQwen3TtsUsesAutomaticFrameLimit()
{
    const QString oldCurrentPath = QDir::currentPath();
    const auto restoreCurrentPath = qScopeGuard([oldCurrentPath]() {
        QDir::setCurrent(oldCurrentPath);
    });
    const QString repoRoot = QDir(QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QStringLiteral("/..")).absolutePath();
    QVERIFY2(QDir::setCurrent(repoRoot), "Test must run from the repository root to load catalog and schema files");

    CatalogManager catalog;
    QSet<QString> qwen3FamilyIds = {
        QStringLiteral("qwen3-tts-0.6b-base"),
        QStringLiteral("qwen3-tts-1.7b-base"),
        QStringLiteral("qwen3-tts-1.7b-customvoice"),
        QStringLiteral("qwen3-tts-1.7b-voicedesign")
    };

    for (const QVariant &familyValue : catalog.ttsFamilies()) {
        const QVariantMap family = familyValue.toMap();
        const QString familyId = family.value(QStringLiteral("id")).toString();
        if (!qwen3FamilyIds.contains(familyId)) {
            continue;
        }

        const QVariantMap parameter = family.value(QStringLiteral("parameterDefinitions")).toMap()
                                         .value(QStringLiteral("max_codec_steps")).toMap();
        QVERIFY2(!parameter.isEmpty(), qPrintable(familyId + QStringLiteral(" should expose max_codec_steps")));
        QCOMPARE(parameter.value(QStringLiteral("min")).toInt(), 0);
        QCOMPARE(parameter.value(QStringLiteral("default")).toInt(), 0);
        qwen3FamilyIds.remove(familyId);
    }

    QVERIFY2(qwen3FamilyIds.isEmpty(), "Every Qwen3-TTS family should be present in the catalog");
}

void TestModelsAndRuntimes::testQwen3TtsDoesNotExposeUnsupportedLengthScale()
{
    const QString oldCurrentPath = QDir::currentPath();
    const auto restoreCurrentPath = qScopeGuard([oldCurrentPath]() {
        QDir::setCurrent(oldCurrentPath);
    });
    const QString repoRoot = QDir(QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QStringLiteral("/..")).absolutePath();
    QVERIFY2(QDir::setCurrent(repoRoot), "Test must run from the repository root to load catalog and schema files");

    CatalogManager catalog;
    QSet<QString> qwen3FamilyIds = {
        QStringLiteral("qwen3-tts-0.6b-base"),
        QStringLiteral("qwen3-tts-1.7b-base"),
        QStringLiteral("qwen3-tts-1.7b-customvoice"),
        QStringLiteral("qwen3-tts-1.7b-voicedesign")
    };

    for (const QVariant &familyValue : catalog.ttsFamilies()) {
        const QVariantMap family = familyValue.toMap();
        const QString familyId = family.value(QStringLiteral("id")).toString();
        if (!qwen3FamilyIds.contains(familyId)) {
            continue;
        }

        const QVariantMap definitions = family.value(QStringLiteral("parameterDefinitions")).toMap();
        QVERIFY2(!definitions.contains(QStringLiteral("length_scale")),
                 qPrintable(familyId + QStringLiteral(" should not expose unsupported length_scale")));

        const QVariantMap studio = family.value(QStringLiteral("studio")).toMap();
        for (const QVariant &capabilityValue : studio) {
            const QVariantMap capability = capabilityValue.toMap();
            const QVariantList parameters = capability.value(QStringLiteral("parameters")).toList();
            QVERIFY2(!parameters.contains(QStringLiteral("length_scale")),
                     qPrintable(familyId + QStringLiteral(" should not send unsupported length_scale")));
        }

        qwen3FamilyIds.remove(familyId);
    }

    QVERIFY2(qwen3FamilyIds.isEmpty(), "Every Qwen3-TTS family should be present in the catalog");
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
