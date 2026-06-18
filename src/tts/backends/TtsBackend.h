#pragma once
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QVector>

namespace LAStudio {

class TtsBackend {
public:
    virtual ~TtsBackend() = default;

    // Load model and return true if successful, along with the schema for QML UI.
    virtual bool load(const QVariantMap &config, QString &error, QVariantList &schema) = 0;
    
    // Unload resources related to this backend.
    virtual void unload() = 0;
    
    // Synthesize text to raw float PCM samples.
    virtual bool synthesize(const QString &text, float speed, const QVariantMap &settings, 
                            QVector<float> &samples, int &sampleRate, QString &error) = 0;
                            
    // Clone voice using reference audio and text.
    virtual bool cloneVoice(const QString &text, const QString &referencePath, const QVariantMap &settings, 
                            QVector<float> &samples, int &sampleRate, QString &error) = 0;
};

} // namespace LAStudio
