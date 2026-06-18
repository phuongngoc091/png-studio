#include "DownloadInstallService.h"

#include "core/DownloadManager.h"
#include "core/ModelManager.h"
#include "core/RuntimeManager.h"
#include "core/Logger.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QPointer>
#include <QMetaType>

namespace LAStudio {

namespace {
bool hasExpectedArchiveSignature(const QString &path, const QString &filename)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray header = file.read(4);
    if (filename.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
        return header.startsWith("PK");
    }
    if (filename.endsWith(QStringLiteral(".tar.gz"), Qt::CaseInsensitive) ||
        filename.endsWith(QStringLiteral(".tgz"), Qt::CaseInsensitive)) {
        return header.size() >= 2 &&
               static_cast<unsigned char>(header[0]) == 0x1f &&
               static_cast<unsigned char>(header[1]) == 0x8b;
    }
    return true;
}

QString yamlScalar(const QVariant &value)
{
    if (!value.isValid() || value.isNull()) return QStringLiteral("null");
    if (value.typeId() == QMetaType::Bool) return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.canConvert<int>() && value.typeId() != QMetaType::QString) return value.toString();

    QString text = value.toString();
    text.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    text.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    return QStringLiteral("\"%1\"").arg(text);
}

void appendYamlValue(QStringList &lines, const QVariant &value, int indent);

void appendYamlMap(QStringList &lines, const QVariantMap &map, int indent)
{
    const QString pad(indent, QLatin1Char(' '));
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        const QVariant value = it.value();
        if (value.typeId() == QMetaType::QVariantMap || value.typeId() == QMetaType::QVariantList) {
            lines << pad + it.key() + QStringLiteral(":");
            appendYamlValue(lines, value, indent + 2);
        } else {
            lines << pad + it.key() + QStringLiteral(": ") + yamlScalar(value);
        }
    }
}

void appendYamlList(QStringList &lines, const QVariantList &list, int indent)
{
    const QString pad(indent, QLatin1Char(' '));
    for (const QVariant &item : list) {
        if (item.typeId() == QMetaType::QVariantMap) {
            lines << pad + QStringLiteral("-");
            appendYamlMap(lines, item.toMap(), indent + 2);
        } else if (item.typeId() == QMetaType::QVariantList) {
            lines << pad + QStringLiteral("-");
            appendYamlList(lines, item.toList(), indent + 2);
        } else {
            lines << pad + QStringLiteral("- ") + yamlScalar(item);
        }
    }
}

void appendYamlValue(QStringList &lines, const QVariant &value, int indent)
{
    if (value.typeId() == QMetaType::QVariantMap) {
        appendYamlMap(lines, value.toMap(), indent);
    } else if (value.typeId() == QMetaType::QVariantList) {
        appendYamlList(lines, value.toList(), indent);
    } else {
        lines << QString(indent, QLatin1Char(' ')) + yamlScalar(value);
    }
}

QString modelYamlText(const QVariantMap &modelYaml)
{
    QStringList lines;
    QVariantMap remaining = modelYaml;
    const QStringList orderedKeys = {
        QStringLiteral("model"),
        QStringLiteral("base"),
        QStringLiteral("metadataOverrides"),
        QStringLiteral("config"),
        QStringLiteral("customFields"),
        QStringLiteral("suggestions")
    };

    for (const QString &key : orderedKeys) {
        if (!remaining.contains(key)) continue;
        const QVariant value = remaining.take(key);
        if (value.typeId() == QMetaType::QVariantMap || value.typeId() == QMetaType::QVariantList) {
            lines << key + QStringLiteral(":");
            appendYamlValue(lines, value, 2);
        } else {
            lines << key + QStringLiteral(": ") + yamlScalar(value);
        }
    }
    appendYamlMap(lines, remaining, 0);
    return lines.join(QStringLiteral("\n")) + QStringLiteral("\n");
}

QVariantMap virtualModelMetadata(const QVariantMap &family)
{
    return {
        {QStringLiteral("kind"), QStringLiteral("modelFile")},
        {QStringLiteral("familyId"), family.value(QStringLiteral("id")).toString()},
        {QStringLiteral("virtualModelId"), family.value(QStringLiteral("modelId")).toString()},
        {QStringLiteral("modelYaml"), family.value(QStringLiteral("modelYaml")).toMap()},
        {QStringLiteral("hubFiles"), family.value(QStringLiteral("hubFiles")).toMap()}
    };
}

bool writeTextFile(const QString &path, const QByteArray &content, QIODevice::OpenMode mode = QIODevice::Text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | mode)) {
        return false;
    }
    file.write(content);
    file.close();
    return true;
}

bool writeVirtualModelFilesToDisk(ModelManager *models, const QVariantMap &metadata, QString *errorMessage = nullptr)
{
    if (!models) {
        if (errorMessage) *errorMessage = QStringLiteral("Model manager is not available");
        return false;
    }

    const QString virtualModelId = metadata.value(QStringLiteral("virtualModelId")).toString();
    const QVariantMap modelYaml = metadata.value(QStringLiteral("modelYaml")).toMap();
    const QVariantMap hubFiles = metadata.value(QStringLiteral("hubFiles")).toMap();
    if (virtualModelId.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Virtual model id is empty");
        return false;
    }
    if (modelYaml.isEmpty() && hubFiles.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Virtual model metadata is empty");
        return false;
    }

    const QString virtualDir = models->virtualModelDir(virtualModelId);
    if (!QDir().mkpath(virtualDir)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to create virtual model directory: %1").arg(virtualDir);
        }
        return false;
    }

    bool ok = true;

    if (!modelYaml.isEmpty()) {
        if (!writeTextFile(QDir(virtualDir).absoluteFilePath(QStringLiteral("model.yaml")),
                           modelYamlText(modelYaml).toUtf8())) {
            ok = false;
            Logger::error(QStringLiteral("DownloadInstallService"),
                          QStringLiteral("Failed to write virtual model.yaml for %1").arg(virtualModelId));
        }
    }

    const QVariantMap manifest = hubFiles.value(QStringLiteral("manifest")).toMap();
    if (!manifest.isEmpty()) {
        const QJsonDocument doc(QJsonObject::fromVariantMap(manifest));
        if (!writeTextFile(QDir(virtualDir).absoluteFilePath(QStringLiteral("manifest.json")), doc.toJson(QJsonDocument::Indented))) {
            ok = false;
            Logger::error(QStringLiteral("DownloadInstallService"),
                          QStringLiteral("Failed to write virtual manifest.json for %1").arg(virtualModelId));
        }
    }

    const QVariantMap readme = hubFiles.value(QStringLiteral("readme")).toMap();
    const QString readmeContent = readme.value(QStringLiteral("content")).toString();
    if (!readmeContent.isEmpty()) {
        if (!writeTextFile(QDir(virtualDir).absoluteFilePath(QStringLiteral("README.md")), readmeContent.toUtf8())) {
            ok = false;
            Logger::error(QStringLiteral("DownloadInstallService"),
                          QStringLiteral("Failed to write virtual README.md for %1").arg(virtualModelId));
        }
    }

    const QVariantMap thumbnail = hubFiles.value(QStringLiteral("thumbnail")).toMap();
    const QByteArray thumbnailBytes = QByteArray::fromBase64(thumbnail.value(QStringLiteral("base64")).toString().toLatin1());
    if (!thumbnailBytes.isEmpty()) {
        QString thumbnailFilename = QFileInfo(thumbnail.value(QStringLiteral("filename")).toString()).fileName();
        if (thumbnailFilename.isEmpty()) {
            thumbnailFilename = QStringLiteral("thumbnail.png");
        }
        if (!writeTextFile(QDir(virtualDir).absoluteFilePath(thumbnailFilename),
                           thumbnailBytes,
                           QIODevice::OpenMode())) {
            ok = false;
            Logger::error(QStringLiteral("DownloadInstallService"),
                          QStringLiteral("Failed to write virtual %1 for %2").arg(thumbnailFilename, virtualModelId));
        }
    }

    if (!ok && errorMessage) {
        *errorMessage = QStringLiteral("Failed to write one or more virtual model files for %1").arg(virtualModelId);
    }
    return ok;
}
}

DownloadInstallService::DownloadInstallService(DownloadManager *downloads,
                                               ModelManager *models,
                                               RuntimeManager *runtimes,
                                               QObject *parent)
    : QObject(parent)
    , m_downloads(downloads)
    , m_models(models)
    , m_runtimes(runtimes)
{
    if (m_downloads) {
        connect(m_downloads, &DownloadManager::finished, this, &DownloadInstallService::onDownloadFinished);
    }
}

bool DownloadInstallService::writeVirtualModelFiles(const QVariantMap &metadata)
{
    QString errorMessage;
    const bool ok = writeVirtualModelFilesToDisk(m_models, metadata, &errorMessage);
    if (!ok && errorMessage != QStringLiteral("Virtual model metadata is empty")) {
        emit errorOccurred(errorMessage);
    }
    return ok;
}

bool DownloadInstallService::enqueueModelFile(const QVariantMap &family, const QVariantMap &requirement)
{
    if (!m_downloads || !m_models) {
        emit errorOccurred(QStringLiteral("Download services are not available"));
        return false;
    }

    const QString selectedFile = requirement.value(QStringLiteral("selectedFile")).toString();
    if (family.isEmpty() || selectedFile.isEmpty()) {
        emit errorOccurred(QStringLiteral("Model download request is incomplete"));
        return false;
    }
    if (requirement.value(QStringLiteral("installed")).toBool()) {
        return true;
    }

    const QVariantMap metadata = virtualModelMetadata(family);
    if (!writeVirtualModelFiles(metadata))
        return false;

    QString sourceModelId = requirement.value(QStringLiteral("modelId")).toString();
    if (sourceModelId.isEmpty())
        sourceModelId = family.value(QStringLiteral("modelId")).toString();
    if (sourceModelId.isEmpty()) {
        emit errorOccurred(QStringLiteral("Model source id is empty"));
        return false;
    }

    m_downloads->enqueue(sourceModelId, selectedFile, m_models->concreteModelDir(sourceModelId), metadata);
    return true;
}

bool DownloadInstallService::enqueueRuntime(const QVariantMap &family,
                                            const QString &capability,
                                            const QString &familyId,
                                            const QVariantMap &runtime)
{
    if (!m_downloads || !m_runtimes) {
        emit errorOccurred(QStringLiteral("Runtime download services are not available"));
        return false;
    }
    if (runtime.isEmpty() || runtime.value(QStringLiteral("installed")).toBool())
        return true;

    const QString asset = runtime.value(QStringLiteral("asset")).toString();
    const QString source = runtime.value(QStringLiteral("source")).toString();
    const QString version = runtime.value(QStringLiteral("version")).toString();
    if (asset.isEmpty() || source.isEmpty()) {
        emit errorOccurred(QStringLiteral("Runtime download request is incomplete"));
        return false;
    }

    const QVariantMap virtualMetadata = virtualModelMetadata(family);
    if (!writeVirtualModelFiles(virtualMetadata))
        return false;

    const QString baseUrl = source + version + QStringLiteral("/");
    QVariantMap metadata;
    metadata.insert(QStringLiteral("id"), runtime.value(QStringLiteral("id")).toString());
    metadata.insert(QStringLiteral("version"), version);
    metadata.insert(QStringLiteral("engineName"),
                    !runtime.value(QStringLiteral("name")).toString().isEmpty()
                        ? runtime.value(QStringLiteral("name")).toString()
                        : (!runtime.value(QStringLiteral("label")).toString().isEmpty()
                            ? runtime.value(QStringLiteral("label")).toString()
                            : runtime.value(QStringLiteral("id")).toString()));
    metadata.insert(QStringLiteral("engineFamily"), runtime.value(QStringLiteral("engineFamily")).toString());
    metadata.insert(QStringLiteral("type"), capability == QStringLiteral("stt") ? QStringLiteral("stt") : QStringLiteral("tts"));
    metadata.insert(QStringLiteral("library"), runtime.value(QStringLiteral("library")).toString());
    metadata.insert(QStringLiteral("dependencyDownloads"), runtime.value(QStringLiteral("dependencyDownloads")).toList());

    QVariantMap runtimeMetadata;
    runtimeMetadata.insert(QStringLiteral("backend"), runtime.value(QStringLiteral("backend")).toString());
    runtimeMetadata.insert(QStringLiteral("modelFamily"), familyId);
    runtimeMetadata.insert(QStringLiteral("modelId"), family.value(QStringLiteral("modelId")).toString());
    runtimeMetadata.insert(QStringLiteral("modelVersion"), runtime.value(QStringLiteral("modelVersion")).toString());
    runtimeMetadata.insert(QStringLiteral("runtimeVersion"), version);
    runtimeMetadata.insert(QStringLiteral("asset"), asset);
    runtimeMetadata.insert(QStringLiteral("source"), baseUrl + asset);
    metadata.insert(QStringLiteral("metadata"), runtimeMetadata);

    m_downloads->enqueueUrl(baseUrl + asset, asset, m_runtimes->backendsPath(), metadata);
    return true;
}

void DownloadInstallService::onDownloadFinished(const QString &modelId,
                                                const QString &filename,
                                                const QString &localPath,
                                                const QVariantMap &metadata)
{
    QString task = QStringLiteral("stt");
    QString format = QStringLiteral("bin");
    if (filename.endsWith(QStringLiteral(".gguf"))) format = QStringLiteral("gguf");
    else if (filename.endsWith(QStringLiteral(".onnx"))) format = QStringLiteral("onnx");
    
    if (modelId.contains(QStringLiteral("tts"), Qt::CaseInsensitive) || 
        modelId.contains(QStringLiteral("parler"), Qt::CaseInsensitive) || 
        modelId.contains(QStringLiteral("omnivoice"), Qt::CaseInsensitive) || 
        modelId.contains(QStringLiteral("vibevoice"), Qt::CaseInsensitive) || 
        modelId.contains(QStringLiteral("kokoro"), Qt::CaseInsensitive) ||
        modelId.contains(QStringLiteral("qwen3-tts"), Qt::CaseInsensitive) ||
        filename.contains(QStringLiteral("tts"), Qt::CaseInsensitive) || 
        filename.contains(QStringLiteral("omnivoice"), Qt::CaseInsensitive) ||
        filename.contains(QStringLiteral("vibevoice"), Qt::CaseInsensitive) ||
        filename.contains(QStringLiteral("kokoro"), Qt::CaseInsensitive) ||
        filename.contains(QStringLiteral("qwen3-tts"), Qt::CaseInsensitive)) {
        task = QStringLiteral("tts");
    }

    if (filename.contains(QStringLiteral("asr"), Qt::CaseInsensitive) || 
        filename.contains(QStringLiteral("stt"), Qt::CaseInsensitive) ||
        modelId.contains(QStringLiteral("asr"), Qt::CaseInsensitive) || 
        modelId.contains(QStringLiteral("stt"), Qt::CaseInsensitive)) {
        task = QStringLiteral("stt");
    }

    QFileInfo fi(localPath);
    QString dirPath = fi.absolutePath();
    bool isRuntime = dirPath.contains(QStringLiteral("backends"));

    if (metadata.value(QStringLiteral("kind")).toString() == QStringLiteral("runtimeDependency")) {
        const QString dependency = metadata.value(QStringLiteral("dependency")).toString();
        const QString runtimeDir = metadata.value(QStringLiteral("runtimeDir")).toString();
        if (dependency == QStringLiteral("espeak-ng") &&
            filename.endsWith(QStringLiteral(".msi"), Qt::CaseInsensitive) &&
            !runtimeDir.isEmpty()) {
            const QString targetDir = QDir(runtimeDir).absoluteFilePath(QStringLiteral("espeak-ng"));
            QDir().mkpath(targetDir);

            QProcess *process = new QProcess(this);
            process->setProgram(QStringLiteral("msiexec"));
            process->setArguments({
                QStringLiteral("/a"),
                QDir::toNativeSeparators(localPath),
                QStringLiteral("/qn"),
                QStringLiteral("TARGETDIR=%1").arg(QDir::toNativeSeparators(targetDir))
            });

            QPointer<DownloadInstallService> weakThis(this);
            connect(process, &QProcess::finished, this,
                    [weakThis, process, filename, targetDir, localPath](int exitCode, QProcess::ExitStatus status) {
                process->deleteLater();
                if (!weakThis) return;
                if (exitCode == 0 && status == QProcess::NormalExit) {
                    Logger::info(QStringLiteral("DownloadInstallService"),
                                 QStringLiteral("Extracted %1 dependency to %2").arg(filename, targetDir));
                    QFile::remove(localPath);
                    weakThis->m_runtimes->scanRuntimes();
                } else {
                    Logger::error(QStringLiteral("DownloadInstallService"),
                                  QStringLiteral("Failed to extract runtime dependency %1").arg(filename));
                    emit weakThis->errorOccurred(QStringLiteral("Failed to extract runtime dependency: ") + filename);
                }
            });

            process->start();
            return;
        }
    }

    if (filename.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive) ||
        filename.endsWith(QStringLiteral(".tar.gz"), Qt::CaseInsensitive) ||
        filename.endsWith(QStringLiteral(".tgz"), Qt::CaseInsensitive)) {
        if (QFileInfo(localPath).size() == 0) {
            QFile::remove(localPath);
            Logger::error(QStringLiteral("DownloadInstallService"),
                          QStringLiteral("Refusing to extract empty archive: %1").arg(filename));
            emit errorOccurred(QStringLiteral("Refusing to extract empty archive: ") + filename);
            return;
        }
        if (!hasExpectedArchiveSignature(localPath, filename)) {
            QFile::remove(localPath);
            Logger::error(QStringLiteral("DownloadInstallService"),
                          QStringLiteral("Refusing to extract invalid archive: %1").arg(filename));
            emit errorOccurred(QStringLiteral("Refusing to extract invalid archive: ") + filename);
            return;
        }

        QString extractName = fi.completeBaseName();
        if (filename.endsWith(QStringLiteral(".tar.gz"), Qt::CaseInsensitive)) {
            QFileInfo tarFi(fi.completeBaseName());
            extractName = tarFi.completeBaseName();
        }
        if (isRuntime && metadata.contains("id") && metadata.contains("version")) {
            QString runtimeId = metadata.value("id").toString();
            QString runtimeVersion = metadata.value("version").toString();
            QString engineFamily = metadata.value("engineFamily").toString();

            if (engineFamily.isEmpty()) {
                for (const auto &plat : {QStringLiteral("-win-"), QStringLiteral("-linux-"), QStringLiteral("-macos-")}) {
                    int idx = runtimeId.indexOf(plat);
                    if (idx > 0) { engineFamily = runtimeId.left(idx); break; }
                }
                if (engineFamily.isEmpty()) engineFamily = runtimeId;
            }

            QString variant = runtimeId;
            if (runtimeId.startsWith(engineFamily + QStringLiteral("-"))) {
                variant = runtimeId.mid(engineFamily.length() + 1);
            }

            QString familyDir = dirPath + QStringLiteral("/") + engineFamily;
            QDir().mkpath(familyDir);
            extractName = variant + QStringLiteral("-") + runtimeVersion;
            dirPath = familyDir;
        }
        QString extractDir = dirPath + QStringLiteral("/") + extractName;
        QDir().mkpath(extractDir);

        if (isRuntime) {
            QString runtimeId = metadata.value("id").toString();
            QString runtimeVersion = metadata.value("version").toString();
            if (!runtimeId.isEmpty()) {
                m_activeExtractions.insert(runtimeId + QStringLiteral("::") + runtimeVersion);
                emit installStatesChanged();
            }
        }

        QProcess *process = new QProcess(this);
        process->setProgram(QStringLiteral("tar"));
        process->setArguments({QStringLiteral("-xf"), localPath, QStringLiteral("-C"), extractDir});
        process->setProcessChannelMode(QProcess::MergedChannels);
        
        QPointer<DownloadInstallService> weakThis(this);
        connect(process, &QProcess::finished, this, [weakThis, process, isRuntime, task, format, dirPath, filename, fi, extractDir, metadata, localPath](int exitCode, QProcess::ExitStatus status) {
            const QString output = QString::fromLocal8Bit(process->readAll()).trimmed();
            process->deleteLater();
            if (!weakThis) return;

            if (isRuntime) {
                QString runtimeId = metadata.value("id").toString();
                QString runtimeVersion = metadata.value("version").toString();
                if (!runtimeId.isEmpty()) {
                    weakThis->m_activeExtractions.remove(runtimeId + QStringLiteral("::") + runtimeVersion);
                    emit weakThis->installStatesChanged();
                }
            }

            if (exitCode == 0 && status == QProcess::NormalExit) {
                Logger::info(QStringLiteral("DownloadInstallService"), QStringLiteral("Extracted %1 to %2").arg(filename, extractDir));
                
                QFile::remove(localPath);

                if (isRuntime) {
                    QDir dir(extractDir);
                    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    if (subdirs.size() == 1 && dir.entryList(QDir::Files).isEmpty()) {
                        QString subName = subdirs.first();
                        QDir subDir(dir.absoluteFilePath(subName));
                        QStringList entries = subDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                        for (const auto &entry : entries) {
                            QFile::rename(subDir.absoluteFilePath(entry), dir.absoluteFilePath(entry));
                        }
                        dir.rmdir(subName);
                    }

                    QFile manifestFile(dir.absoluteFilePath(QStringLiteral("backend-manifest.json")));
                    QJsonObject manifest;
                    if (manifestFile.exists()) {
                        if (manifestFile.open(QIODevice::ReadOnly)) {
                            auto doc = QJsonDocument::fromJson(manifestFile.readAll());
                            manifestFile.close();
                            if (doc.isObject()) {
                                manifest = doc.object();
                            }
                        }
                    }

                    QString runtimeId = metadata.value("id").toString();
                    manifest["id"] = runtimeId;
                    manifest["name"] = metadata.value("engineName").toString();
                    manifest["version"] = metadata.value("version").toString();
                    manifest["type"] = metadata.value("type").toString().isEmpty() ? QStringLiteral("stt") : metadata.value("type").toString();

                    QString ef = metadata.value("engineFamily").toString();
                    if (ef.isEmpty()) {
                        for (const auto &plat : {QStringLiteral("-win-"), QStringLiteral("-linux-"), QStringLiteral("-macos-")}) {
                            int idx = runtimeId.indexOf(plat);
                            if (idx > 0) { ef = runtimeId.left(idx); break; }
                        }
                        if (ef.isEmpty()) ef = runtimeId;
                    }
                    manifest["engineFamily"] = ef;
                    QString vr = runtimeId;
                    if (runtimeId.startsWith(ef + QStringLiteral("-")))
                        vr = runtimeId.mid(ef.length() + 1);
                    manifest["variant"] = vr;

                    if (metadata.contains(QStringLiteral("library"))) {
                        manifest["library"] = metadata.value(QStringLiteral("library")).toString();
                    }
                    if (metadata.contains(QStringLiteral("metadata"))) {
                        QJsonObject newMeta = QJsonObject::fromVariantMap(
                            metadata.value(QStringLiteral("metadata")).toMap());
                        QJsonObject existingMeta = manifest.value(QStringLiteral("metadata")).toObject();
                        for (auto it = newMeta.begin(); it != newMeta.end(); ++it) {
                            existingMeta[it.key()] = it.value();
                        }
                        manifest["metadata"] = existingMeta;
                    }
                    
                    QJsonDocument doc(manifest);
                    if (manifestFile.open(QIODevice::WriteOnly)) {
                        manifestFile.write(doc.toJson(QJsonDocument::Indented));
                        manifestFile.close();
                    }

                    weakThis->m_runtimes->scanRuntimes();

                    const QVariantList dependencyDownloads = metadata.value(QStringLiteral("dependencyDownloads")).toList();
                    for (const QVariant &depValue : dependencyDownloads) {
                        const QVariantMap dep = depValue.toMap();
                        const QString url = dep.value(QStringLiteral("url")).toString();
                        const QString depFilename = dep.value(QStringLiteral("filename")).toString();
                        const QString dependency = dep.value(QStringLiteral("dependency")).toString();
                        if (url.isEmpty() || depFilename.isEmpty() || dependency.isEmpty()) continue;

                        QVariantMap depMetadata;
                        depMetadata[QStringLiteral("kind")] = QStringLiteral("runtimeDependency");
                        depMetadata[QStringLiteral("dependency")] = dependency;
                        depMetadata[QStringLiteral("runtimeDir")] = extractDir;
                        weakThis->m_downloads->enqueueUrl(url, depFilename, extractDir, depMetadata);
                    }
                } else {
                    QString sizeStr = QString::number(fi.size() / (1024.0 * 1024.0), 'f', 1) + " MB";
                    weakThis->m_models->addModel(fi.completeBaseName(), task, format, dirPath, {filename}, sizeStr);
                }
            } else {
                Logger::error(QStringLiteral("DownloadInstallService"),
                              QStringLiteral("Failed to extract %1 (exit=%2, status=%3)%4")
                                  .arg(filename)
                                  .arg(exitCode)
                                  .arg(status == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crashed"))
                                  .arg(output.isEmpty() ? QString() : QStringLiteral(": %1").arg(output)));
                emit weakThis->errorOccurred(QStringLiteral("Failed to extract: ") + filename);
            }
        });
        
        process->start();
        return;
    }

    if (isRuntime) {
        m_runtimes->scanRuntimes();
    } else {
        QString virtualWriteError;
        if (!writeVirtualModelFilesToDisk(m_models, metadata, &virtualWriteError) &&
            virtualWriteError != QStringLiteral("Virtual model metadata is empty") &&
            virtualWriteError != QStringLiteral("Virtual model id is empty")) {
            emit errorOccurred(virtualWriteError);
        }

        QDir modelDir(dirPath);
        QFile infoFile(modelDir.absoluteFilePath(QStringLiteral(".la-info.json")));
        QJsonObject info;
        if (infoFile.open(QIODevice::ReadOnly)) {
            const QJsonDocument existingDoc = QJsonDocument::fromJson(infoFile.readAll());
            if (existingDoc.isObject()) {
                info = existingDoc.object();
            }
            infoFile.close();
        }

        QString resolvedId = metadata.value(QStringLiteral("familyId")).toString();
        if (resolvedId.isEmpty()) {
            resolvedId = metadata.value(QStringLiteral("virtualModelId")).toString();
        }
        if (resolvedId.isEmpty()) {
            resolvedId = modelId;
        }

        if (infoFile.open(QIODevice::WriteOnly)) {
            QJsonArray ids = info.value(QStringLiteral("ids")).toArray();
            const QString existingId = info.value(QStringLiteral("id")).toString();
            if (!existingId.isEmpty() && !ids.contains(QJsonValue(existingId))) {
                ids.append(existingId);
            }
            if (!resolvedId.isEmpty() && !ids.contains(QJsonValue(resolvedId))) {
                ids.append(resolvedId);
            }
            if (!modelId.isEmpty() && !ids.contains(QJsonValue(modelId))) {
                ids.append(modelId);
            }
            info["id"] = resolvedId;
            info["ids"] = ids;
            info["task"] = task;
            QJsonDocument doc(info);
            infoFile.write(doc.toJson());
            infoFile.close();
        }

        QString sizeStr = QString::number(fi.size() / (1024.0 * 1024.0), 'f', 1) + " MB";
        m_models->addModel(resolvedId, task, format, dirPath, {filename}, sizeStr);
    }
}

int DownloadInstallService::modelFileState(const QString &modelId, const QString &filename) const
{
    if (m_models && m_models->hasFile(modelId, filename)) {
        return Installed;
    }
    if (m_downloads && m_downloads->isDownloading(modelId, filename)) {
        return Downloading;
    }
    return NotInstalled;
}

int DownloadInstallService::runtimeState(const QString &runtimeId, const QString &version, const QString &libraryPath, const QString &assetName) const
{
    QString key = runtimeId + QStringLiteral("::") + version;
    if (m_activeExtractions.contains(key)) {
        return Installing;
    }

    bool installed = false;
    if (m_runtimes) {
        QVariantList installedVers = m_runtimes->runtimeVersions(runtimeId);
        if (version.isEmpty()) {
            installed = !installedVers.isEmpty();
        } else {
            for (const QVariant &inst : installedVers) {
                if (inst.toMap().value(QStringLiteral("version")).toString() == version) {
                    installed = true;
                    break;
                }
            }
        }
    }

    if (!installed && !libraryPath.isEmpty()) {
        installed = QFileInfo::exists(libraryPath);
    }

    if (installed) {
        return Installed;
    }

    if (m_downloads && !assetName.isEmpty()) {
        QVariantList active = m_downloads->activeDownloads();
        for (const QVariant &val : active) {
            const QVariantMap activeDownload = val.toMap();
            if (activeDownload.value(QStringLiteral("filename")).toString() != assetName) {
                continue;
            }
            const QVariantMap metadata = activeDownload.value(QStringLiteral("metadata")).toMap();
            const QString activeRuntimeId = metadata.value(QStringLiteral("id")).toString();
            const QString activeVersion = metadata.value(QStringLiteral("version")).toString();
            if ((!activeRuntimeId.isEmpty() || !activeVersion.isEmpty()) &&
                (activeRuntimeId != runtimeId || activeVersion != version)) {
                continue;
            }
            if (activeRuntimeId.isEmpty() || activeRuntimeId == runtimeId) {
                return Downloading;
            }
        }
    }

    return NotInstalled;
}

} // namespace LAStudio
