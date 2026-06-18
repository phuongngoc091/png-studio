#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <memory>

namespace LAStudio {

class TtsBackend;

class TtsWorker : public QObject {
    Q_OBJECT

public:
    explicit TtsWorker(QObject *parent = nullptr);
    ~TtsWorker() override;

public slots:
    void loadVoice(const QVariantMap &config);
    void unloadVoice();
    void synthesize(const QString &text, int speakerId = 0, float speed = 1.0f, const QVariantMap &settings = QVariantMap());
    void cloneVoice(const QString &text, const QString &referencePath, const QVariantMap &settings = QVariantMap());

signals:
    void modelLoaded(bool success, const QString &error, const QVariantList &schema = QVariantList());
    void finished(const QVector<float> &samples, int sampleRate);
    void errorOccurred(const QString &error);

private:
    std::unique_ptr<TtsBackend> m_backend;
    QString m_activeModelPath;
};

} // namespace LAStudio
