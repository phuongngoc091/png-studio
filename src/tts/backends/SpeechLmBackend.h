#pragma once
#include "TtsBackend.h"

namespace LAStudio {

class SpeechLmBackend : public TtsBackend {
public:
    SpeechLmBackend() = default;
    ~SpeechLmBackend() override;

    bool load(const QVariantMap &config, QString &error, QVariantList &schema) override;
    void unload() override;
    bool synthesize(const QString &text, float speed, const QVariantMap &settings, 
                    QVector<float> &samples, int &sampleRate, QString &error) override;
    bool cloneVoice(const QString &text, const QString &referencePath, const QVariantMap &settings, 
                    QVector<float> &samples, int &sampleRate, QString &error) override;

private:
    void *m_context = nullptr;
};

} // namespace LAStudio
