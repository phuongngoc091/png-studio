#include "HFHubClient.h"
#include "Logger.h"

#include <curl/curl.h>

#include <QThreadPool>
#include <QRunnable>
#include <QFile>
#include <QDir>
#include <QMetaObject>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QFileInfo>

namespace LAStudio {

struct DownloadContext {
    HFHubClient *client;
    QString identifier;
    QString filename;
    QFile *file;
    qint64 lastUpdate = 0;
};

HFHubClient::HFHubClient(QObject *parent)
    : QObject(parent)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

size_t HFHubClient::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *buffer = static_cast<QByteArray *>(userdata);
    buffer->append(ptr, static_cast<qsizetype>(size * nmemb));
    return size * nmemb;
}

size_t HFHubClient::headerCallback(char *buffer, size_t size, size_t nitems, void *)
{
    Q_UNUSED(buffer);
    return size * nitems;
}

int HFHubClient::progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                                   curl_off_t /*ultotal*/, curl_off_t /*ulnow*/)
{
    auto *ctx = static_cast<DownloadContext *>(clientp);
    
    // Throttle updates to ~10Hz (every 100ms) to prevent UI thread saturation
    // but always allow the final update (dlnow == dltotal)
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - ctx->lastUpdate < 100 && dlnow < dltotal && dltotal > 0) {
        return 0;
    }
    ctx->lastUpdate = now;

    QString id = ctx->identifier;
    QString fn = ctx->filename;
    HFHubClient *client = ctx->client;
    QMetaObject::invokeMethod(client, [=]() {
        emit client->downloadProgress(
            id, fn,
            static_cast<qint64>(dlnow),
            static_cast<qint64>(dltotal));
    }, Qt::QueuedConnection);
    return 0;
}

void HFHubClient::searchModels(const QString &query, const QString &task, bool whisperOnly)
{
    m_searching = true;
    emit searchingChanged();

    QThreadPool::globalInstance()->start([this, query, task, whisperOnly]() {
        CURL *curl = curl_easy_init();
        if (!curl) {
            QMetaObject::invokeMethod(this, [this]() {
                m_searching = false;
                emit searchingChanged();
                emit searchError(QStringLiteral("Failed to initialize curl"));
            }, Qt::QueuedConnection);
            return;
        }

        QString url = QStringLiteral("https://huggingface.co/api/models?search=%1&limit=30&full=true&config=true")
                          .arg(QString::fromUtf8(QUrl::toPercentEncoding(query)));
        if (!task.isEmpty()) {
            url += QStringLiteral("&filter=%1").arg(task);
        }

        QByteArray urlBytes = url.toUtf8();
        QByteArray responseData;
        curl_easy_setopt(curl, CURLOPT_URL, urlBytes.constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LAStudio/0.1");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            QString err = QString::fromUtf8(curl_easy_strerror(res));
            QMetaObject::invokeMethod(this, [this, err]() {
                m_searching = false;
                emit searchingChanged();
                emit searchError(err);
            }, Qt::QueuedConnection);
            return;
        }

        QVariantList results;
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            for (const QJsonValue &val : arr) {
                if (!val.isObject()) continue;
                QJsonObject item = val.toObject();
                
                QVariantMap m;
                
                QString id;
                if (item.contains("id")) id = item.value("id").toString();
                else if (item.contains("modelId")) id = item.value("modelId").toString();
                
                m[QStringLiteral("id")] = id;
                m[QStringLiteral("modelId")] = id;
                m[QStringLiteral("author")] = item.value("author").toString();
                m[QStringLiteral("downloads")] = item.value("downloads").toVariant().toLongLong();
                m[QStringLiteral("likes")] = item.value("likes").toVariant().toLongLong();
                m[QStringLiteral("lastModified")] = item.value("lastModified").toString();

                QString pipelineTag = item.value("pipeline_tag").toString();
                m[QStringLiteral("task")] = pipelineTag;

                QStringList tags;
                if (item.contains("tags") && item.value("tags").isArray()) {
                    QJsonArray tagsArr = item.value("tags").toArray();
                    for (const QJsonValue &t : tagsArr) {
                        if (t.isString()) tags << t.toString();
                    }
                    m[QStringLiteral("tags")] = tags;
                }

                bool hasBinFile = false;
                if (item.contains("siblings") && item.value("siblings").isArray()) {
                    QVariantList files;
                    QJsonArray sibArr = item.value("siblings").toArray();
                    for (const QJsonValue &sVal : sibArr) {
                        QJsonObject s = sVal.toObject();
                        QVariantMap fm;
                        QString filename = s.value("rfilename").toString();
                        fm[QStringLiteral("rfilename")] = filename;
                        if (s.contains("size"))
                            fm[QStringLiteral("size")] = s.value("size").toVariant().toLongLong();
                        files.append(fm);

                        if (filename.endsWith(QStringLiteral(".bin"), Qt::CaseInsensitive)) {
                            hasBinFile = true;
                        }
                    }
                    m[QStringLiteral("files")] = files;
                }

                if (whisperOnly) {
                    bool isWhisper = (pipelineTag == QStringLiteral("automatic-speech-recognition") || 
                                      tags.contains(QStringLiteral("whisper"), Qt::CaseInsensitive));
                    bool isGGML = (tags.contains(QStringLiteral("ggml"), Qt::CaseInsensitive) || hasBinFile);
                    
                    if (!isWhisper || !isGGML) {
                        continue;
                    }
                }

                results.append(m);
            }
        }

        QMetaObject::invokeMethod(this, [this, results]() {
            m_searching = false;
            emit searchingChanged();
            emit searchFinished(results);
        }, Qt::QueuedConnection);
    });
}

void HFHubClient::downloadUrl(const QString &url,
                               const QString &filename,
                               const QString &destDir)
{
    // Use URL as identifier for signals
    internalDownload(url, url, filename, destDir);
}

void HFHubClient::downloadFile(const QString &modelId,
                                const QString &filename,
                                const QString &destDir)
{
    QString url = QStringLiteral("https://huggingface.co/%1/resolve/main/%2")
                      .arg(modelId, filename);
    internalDownload(url, modelId, filename, destDir);
}

void HFHubClient::internalDownload(const QString &url,
                                   const QString &identifier,
                                   const QString &filename,
                                   const QString &destDir)
{
    QThreadPool::globalInstance()->start([this, url, identifier, filename, destDir]() {
        QDir().mkpath(destDir);
        const QString localPath = destDir + QStringLiteral("/") + filename;
        const QString tempPath = localPath + QStringLiteral(".download");

        QFile *file = new QFile(tempPath);
        if (!file->open(QIODevice::WriteOnly)) {
            delete file;
            QMetaObject::invokeMethod(this, [=]() {
                emit downloadError(identifier, filename,
                                   QStringLiteral("Cannot open file for writing: ") + tempPath);
            }, Qt::QueuedConnection);
            return;
        }

        CURL *curl = curl_easy_init();
        if (!curl) {
            file->close();
            delete file;
            QMetaObject::invokeMethod(this, [=]() {
                emit downloadError(identifier, filename,
                                   QStringLiteral("Failed to initialize curl"));
            }, Qt::QueuedConnection);
            return;
        }

        DownloadContext ctx{this, identifier, filename, file};

        auto fileWriter = [](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
            auto *f = static_cast<QFile *>(userdata);
            qint64 written = f->write(ptr, static_cast<qint64>(size * nmemb));
            return written < 0 ? 0 : static_cast<size_t>(written);
        };

        QByteArray urlBytes = url.toUtf8();
        curl_easy_setopt(curl, CURLOPT_URL, urlBytes.constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         static_cast<size_t(*)(char*,size_t,size_t,void*)>(fileWriter));
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LAStudio/0.1");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

        CURLcode res = curl_easy_perform(curl);
        long responseCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        curl_easy_cleanup(curl);
        file->close();

        if (res != CURLE_OK || responseCode >= 400 || QFileInfo(tempPath).size() == 0) {
            QFile::remove(tempPath);
            QString err;
            if (responseCode >= 400) {
                err = QStringLiteral("HTTP %1 while downloading %2").arg(responseCode).arg(url);
            } else if (res != CURLE_OK) {
                err = QString::fromUtf8(curl_easy_strerror(res));
            } else {
                err = QStringLiteral("Downloaded file is empty: %1").arg(url);
            }
            QMetaObject::invokeMethod(this, [=]() {
                emit downloadError(identifier, filename, err);
            }, Qt::QueuedConnection);
        } else {
            QFile::remove(localPath);
            if (QFile::rename(tempPath, localPath)) {
                QMetaObject::invokeMethod(this, [=]() {
                    emit downloadFinished(identifier, filename, localPath);
                }, Qt::QueuedConnection);
            } else {
                QFile::remove(tempPath);
                QMetaObject::invokeMethod(this, [=]() {
                    emit downloadError(identifier, filename,
                                       QStringLiteral("Failed to rename temporary file to destination: ") + localPath);
                }, Qt::QueuedConnection);
            }
        }
        delete file;
    });
}

} // namespace LAStudio
