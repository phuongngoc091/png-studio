#include "SpeechLmBackend.h"
#include "core/Logger.h"
#include "core/PathUtils.h"
#include <runtimes/SpeechLmTtsInterface.h>
#include <QThread>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace LAStudio {

static QString s_sessionSpeechLmRuntimePath;

static int recommendedThreadCount()
{
    const int ideal = QThread::idealThreadCount();
    if (ideal <= 0) return 8;
    return qBound(4, ideal, 16);
}

static bool isValidRuntimeAudio(const slm_audio &audio)
{
    constexpr int maxReasonableSamples = 24000 * 60 * 30; // 30 minutes at 24 kHz.
    return audio.samples != nullptr &&
           audio.n_samples > 0 &&
           audio.n_samples <= maxReasonableSamples &&
           audio.sample_rate > 0 &&
           audio.sample_rate <= 384000;
}

SpeechLmBackend::~SpeechLmBackend()
{
    unload();
}

bool SpeechLmBackend::load(const QVariantMap &config, QString &error, QVariantList &schema)
{
    QString modelPath = PathUtils::toNativeShortPath(config.value("model").toString());
    QString encoderPath = PathUtils::toNativeShortPath(config.value("encoder").toString());
    QString decoderPath = PathUtils::toNativeShortPath(config.value("decoder").toString());
    QString voicesPath = PathUtils::toNativeShortPath(config.value("voices").toString());
    QString runtimePath = PathUtils::toNativeShortPath(config.value("runtimePath").toString());
    const QString pipelineProfile = config.value(QStringLiteral("pipelineProfile")).toString();
    m_useAbiV2 = pipelineProfile == QStringLiteral("vieneu-v3-onnx");

    QString resolvedVoicesPath = voicesPath;
    if (resolvedVoicesPath.isEmpty() && !modelPath.isEmpty()) {
        QFileInfo modelInfo(modelPath);
        QString siblingVoices = modelInfo.dir().absoluteFilePath(QStringLiteral("voices.json"));
        if (QFileInfo::exists(siblingVoices)) {
            resolvedVoicesPath = siblingVoices;
        }
    }

    QVariantList speakerChoices;
    QString defaultVoiceId;
    if (!resolvedVoicesPath.isEmpty() && QFileInfo::exists(resolvedVoicesPath)) {
        QFile file(resolvedVoicesPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                defaultVoiceId = rootObj.value(QStringLiteral("default_voice")).toString();
                QJsonObject presets = rootObj.value(QStringLiteral("presets")).toObject();
                if (!presets.isEmpty()) {
                    for (auto it = presets.begin(); it != presets.end(); ++it) {
                        QString key = it.key();
                        QJsonObject voiceData = it.value().toObject();
                        QString displayText = voiceData.value(QStringLiteral("description")).toString();
                        if (displayText.isEmpty()) {
                            displayText = key;
                        }
                        QVariantMap choice;
                        choice[QStringLiteral("text")] = displayText;
                        choice[QStringLiteral("value")] = key;
                        speakerChoices.append(choice);
                    }
                } else {
                    for (auto it = rootObj.begin(); it != rootObj.end(); ++it) {
                        const QString key = it.key();
                        if (key == QStringLiteral("spec") || key == QStringLiteral("version") || key == QStringLiteral("default_voice")) {
                            continue;
                        }
                        if (!it.value().isObject()) {
                            continue;
                        }
                        QJsonObject voiceData = it.value().toObject();
                        if (!voiceData.contains(QStringLiteral("codes")) && !voiceData.contains(QStringLiteral("embedding"))) {
                            continue;
                        }
                        QString displayText = voiceData.value(QStringLiteral("description")).toString();
                        if (displayText.isEmpty()) {
                            displayText = key;
                        }
                        QVariantMap choice;
                        choice[QStringLiteral("text")] = displayText;
                        choice[QStringLiteral("value")] = key;
                        speakerChoices.append(choice);
                    }
                }
            }
        }
    }

    if (!speakerChoices.isEmpty()) {
        QVariantMap voiceParam;
        voiceParam["id"] = "voice";
        voiceParam["name"] = "Voice";
        voiceParam["type"] = "choice";
        voiceParam["choices"] = speakerChoices;
        if (defaultVoiceId.isEmpty()) {
            defaultVoiceId = speakerChoices.first().toMap().value(QStringLiteral("value")).toString();
        } else {
            bool defaultExists = false;
            for (const QVariant &choiceVar : speakerChoices) {
                if (choiceVar.toMap().value(QStringLiteral("value")).toString() == defaultVoiceId) {
                    defaultExists = true;
                    break;
                }
            }
            if (!defaultExists) {
                defaultVoiceId = speakerChoices.first().toMap().value(QStringLiteral("value")).toString();
            }
        }
        voiceParam["default"] = defaultVoiceId;
        voiceParam["description"] = "Select a voice preset from voices.json.";
        schema.append(voiceParam);
    } else {
        if (resolvedVoicesPath.isEmpty()) {
            Logger::warning("SpeechLmBackend", "SpeechLM preset voices unavailable: voices.json was not provided and no sibling voices.json was found.");
        } else {
            Logger::warning("SpeechLmBackend", QString("SpeechLM preset voices unavailable: could not parse voices from %1").arg(resolvedVoicesPath));
        }
    }

    auto& slm = SpeechLmTtsInterface::instance();
    if (!runtimePath.isEmpty()) {
        if (!s_sessionSpeechLmRuntimePath.isEmpty() &&
            s_sessionSpeechLmRuntimePath != runtimePath) {
            error = QStringLiteral("Switching SpeechLM TTS runtime backend (CPU/CUDA/Vulkan) in one running session is unstable. Please restart LA Studio after changing runtime.");
            Logger::error("SpeechLmBackend", error + QStringLiteral(" Previous: %1 | Requested: %2")
                          .arg(s_sessionSpeechLmRuntimePath, runtimePath));
            return false;
        }
        if (!slm.load(runtimePath)) {
            error = QString("Failed to load SpeechLM TTS runtime: ") + slm.errorString();
            Logger::error("SpeechLmBackend", error);
            return false;
        }
    }

    if (!slm.isLoaded()) {
        error = QStringLiteral("No SpeechLM TTS runtime loaded. Please select one in settings.");
        Logger::error("SpeechLmBackend", error);
        return false;
    }

    if (m_useAbiV2) {
        if (!slm.slm_init_v2_default_params || !slm.slm_init_v2 || !slm.slm_tts_v2_default_params || !slm.slm_synthesize_v2) {
            error = QStringLiteral("Selected SpeechLM runtime does not expose ABI v2 required for VieNeu-TTS v3 ONNX.");
            Logger::error("SpeechLmBackend", error);
            unload();
            return false;
        }

        QString configPath = PathUtils::toNativeShortPath(config.value(QStringLiteral("config")).toString());
        QString tokenizerPath = PathUtils::toNativeShortPath(config.value(QStringLiteral("tokenizer")).toString());
        QString prefillPath = PathUtils::toNativeShortPath(config.value(QStringLiteral("prefill")).toString());
        QString codecDecodePath = PathUtils::toNativeShortPath(config.value(QStringLiteral("codec_decode")).toString());

        const QString modelDir = !configPath.isEmpty()
            ? QFileInfo(configPath).absolutePath()
            : QFileInfo(modelPath).absolutePath();
        const QString onnxDir = !prefillPath.isEmpty()
            ? QFileInfo(prefillPath).absolutePath()
            : QFileInfo(modelPath).absolutePath();
        const QString codecDir = !codecDecodePath.isEmpty()
            ? QFileInfo(codecDecodePath).absolutePath()
            : QFileInfo(decoderPath).absolutePath();

        QByteArray profileBytes = QByteArrayLiteral("vieneu-v3-onnx");
        QByteArray modelDirBytes = PathUtils::toNativeShortPath(modelDir).toUtf8();
        QByteArray onnxDirBytes = PathUtils::toNativeShortPath(onnxDir).toUtf8();
        QByteArray codecDirBytes = PathUtils::toNativeShortPath(codecDir).toUtf8();
        QByteArray configPathBytes = configPath.toUtf8();
        QByteArray tokenizerPathBytes = tokenizerPath.toUtf8();
        QByteArray voicesPathBytes = resolvedVoicesPath.toUtf8();

        slm_init_params_v2 params;
        slm.slm_init_v2_default_params(&params);
        params.profile = profileBytes.constData();
        params.model_dir = modelDirBytes.isEmpty() ? nullptr : modelDirBytes.constData();
        params.onnx_dir = onnxDirBytes.isEmpty() ? nullptr : onnxDirBytes.constData();
        params.codec_dir = codecDirBytes.isEmpty() ? nullptr : codecDirBytes.constData();
        params.config_path = configPathBytes.isEmpty() ? nullptr : configPathBytes.constData();
        params.tokenizer_path = tokenizerPathBytes.isEmpty() ? nullptr : tokenizerPathBytes.constData();
        params.voices_json_path = voicesPathBytes.isEmpty() ? nullptr : voicesPathBytes.constData();
        params.n_threads = recommendedThreadCount();

        m_context = slm.slm_init_v2(&params);
    } else {
        QByteArray modelPathBytes = modelPath.toUtf8();
        QByteArray encoderPathBytes = encoderPath.toUtf8();
        QByteArray decoderPathBytes = decoderPath.toUtf8();
        QByteArray voicesPathBytes = resolvedVoicesPath.toUtf8();

        slm_init_params params;
        slm.slm_init_default_params(&params);
        params.model_path = modelPathBytes.constData();
        params.encoder_path = encoderPathBytes.isEmpty() ? nullptr : encoderPathBytes.constData();
        params.decoder_path = decoderPathBytes.isEmpty() ? nullptr : decoderPathBytes.constData();
        params.voices_json_path = voicesPathBytes.isEmpty() ? nullptr : voicesPathBytes.constData();
        params.n_ctx = 2048;
        params.n_threads = recommendedThreadCount();

        m_context = slm.slm_init(&params);
    }
    if (!m_context) {
        error = QString::fromUtf8(slm.slm_last_error());
        Logger::error("SpeechLmBackend", "Failed to initialize SpeechLM TTS model: " + error);
        unload();
        return false;
    }

    if (!runtimePath.isEmpty())
        s_sessionSpeechLmRuntimePath = runtimePath;

    Logger::info("SpeechLmBackend", QString("SpeechLM TTS profile loaded successfully"));
    return true;
}

void SpeechLmBackend::unload()
{
    if (m_context) {
        auto& slm = SpeechLmTtsInterface::instance();
        if (slm.isLoaded() && slm.slm_free) {
            slm.slm_free((struct slm_context*)m_context);
        }
        m_context = nullptr;
    }
    SpeechLmTtsInterface::instance().unload();
    m_useAbiV2 = false;
}

bool SpeechLmBackend::synthesize(const QString &text, float speed, const QVariantMap &settings, 
                               QVector<float> &samples, int &sampleRate, QString &error)
{
    Q_UNUSED(speed);
    auto& slm = SpeechLmTtsInterface::instance();
    if (!slm.isLoaded() || !slm.slm_synthesize || !slm.slm_last_error || !slm.slm_audio_free || !m_context) {
        error = QStringLiteral("SpeechLM TTS runtime was unloaded unexpectedly.");
        return false;
    }

    QByteArray textBytes = text.toUtf8();
    QByteArray voiceIdBytes;
    slm_audio out;
    memset(&out, 0, sizeof(out));
    int status = -1;
    if (m_useAbiV2) {
        slm_tts_params_v2 params;
        slm.slm_tts_v2_default_params(&params);
        params.text = textBytes.constData();
        if (settings.contains(QStringLiteral("voice"))) {
            voiceIdBytes = settings.value(QStringLiteral("voice")).toString().toUtf8();
            params.voice_id = voiceIdBytes.constData();
        }
        if (settings.contains(QStringLiteral("temperature"))) params.temperature = settings.value(QStringLiteral("temperature")).toFloat();
        if (settings.contains(QStringLiteral("top_k"))) params.top_k = settings.value(QStringLiteral("top_k")).toInt();
        if (settings.contains(QStringLiteral("top_p"))) params.top_p = settings.value(QStringLiteral("top_p")).toFloat();
        if (settings.contains(QStringLiteral("max_new_frames"))) params.max_new_frames = settings.value(QStringLiteral("max_new_frames")).toInt();
        if (settings.contains(QStringLiteral("repetition_penalty"))) params.repetition_penalty = settings.value(QStringLiteral("repetition_penalty")).toFloat();
        if (settings.contains(QStringLiteral("max_chars"))) params.max_chars = settings.value(QStringLiteral("max_chars")).toInt();
        if (settings.contains(QStringLiteral("apply_watermark"))) params.apply_watermark = settings.value(QStringLiteral("apply_watermark")).toBool();
        status = slm.slm_synthesize_v2((struct slm_context*)m_context, &params, &out);
    } else {
        slm_tts_params params;
        slm.slm_tts_default_params(&params);
        params.text = textBytes.constData();
        if (settings.contains(QStringLiteral("voice"))) {
            voiceIdBytes = settings.value(QStringLiteral("voice")).toString().toUtf8();
            params.voice_id = voiceIdBytes.constData();
        }
        if (settings.contains(QStringLiteral("temperature"))) params.temperature = settings.value(QStringLiteral("temperature")).toFloat();
        if (settings.contains(QStringLiteral("top_k"))) params.top_k = settings.value(QStringLiteral("top_k")).toInt();
        if (settings.contains(QStringLiteral("max_chars"))) params.max_chars = settings.value(QStringLiteral("max_chars")).toInt();
        if (settings.contains(QStringLiteral("max_tokens"))) params.max_tokens = settings.value(QStringLiteral("max_tokens")).toInt();
        if (settings.contains(QStringLiteral("skip_normalize"))) params.skip_normalize = settings.value(QStringLiteral("skip_normalize")).toBool();
        if (settings.contains(QStringLiteral("skip_phonemize"))) params.skip_phonemize = settings.value(QStringLiteral("skip_phonemize")).toBool();
        if (settings.contains(QStringLiteral("apply_watermark"))) params.apply_watermark = settings.value(QStringLiteral("apply_watermark")).toBool();
        status = slm.slm_synthesize((struct slm_context*)m_context, &params, &out);
    }
    Logger::info("SpeechLmBackend", QString("SpeechLM TTS synthesize returned: status=%1, samples=%2, sampleRate=%3")
                 .arg(status)
                 .arg(out.n_samples)
                 .arg(out.sample_rate));

    if (status != 0) {
        error = QString::fromUtf8(slm.slm_last_error());
        Logger::error("SpeechLmBackend", "SpeechLM TTS synthesis failed: " + error);
        return false;
    }

    if (!isValidRuntimeAudio(out)) {
        Logger::error("SpeechLmBackend", QString("SpeechLM TTS returned invalid audio buffer: samples=%1, sampleRate=%2, ptr=%3")
                      .arg(out.n_samples)
                      .arg(out.sample_rate)
                      .arg(QString::number(reinterpret_cast<quintptr>(out.samples), 16)));
        slm.slm_audio_free(&out);
        error = QStringLiteral("SpeechLM TTS returned an invalid or empty audio buffer.");
        return false;
    }

    samples.resize(out.n_samples);
    memcpy(samples.data(), out.samples, sizeof(float) * out.n_samples);
    sampleRate = out.sample_rate;

    slm.slm_audio_free(&out);
    return true;
}

bool SpeechLmBackend::cloneVoice(const QString &text, const QString &referencePath, const QVariantMap &settings, 
                               QVector<float> &samples, int &sampleRate, QString &error)
{
    auto& slm = SpeechLmTtsInterface::instance();
    if (!slm.isLoaded() || !m_context) {
        error = QStringLiteral("SpeechLM TTS runtime was unloaded unexpectedly.");
        return false;
    }

    if (m_useAbiV2) {
        QByteArray textBytes = text.toUtf8();
        QByteArray refPathBytes = QDir::toNativeSeparators(PathUtils::toNativeShortPath(referencePath)).toUtf8();

        slm_tts_params_v2 params;
        slm.slm_tts_v2_default_params(&params);
        params.text = textBytes.constData();
        params.ref_audio_path = refPathBytes.constData();
        if (settings.contains(QStringLiteral("temperature"))) params.temperature = settings.value(QStringLiteral("temperature")).toFloat();
        if (settings.contains(QStringLiteral("top_k"))) params.top_k = settings.value(QStringLiteral("top_k")).toInt();
        if (settings.contains(QStringLiteral("top_p"))) params.top_p = settings.value(QStringLiteral("top_p")).toFloat();
        if (settings.contains(QStringLiteral("max_new_frames"))) params.max_new_frames = settings.value(QStringLiteral("max_new_frames")).toInt();
        if (settings.contains(QStringLiteral("repetition_penalty"))) params.repetition_penalty = settings.value(QStringLiteral("repetition_penalty")).toFloat();
        if (settings.contains(QStringLiteral("max_chars"))) params.max_chars = settings.value(QStringLiteral("max_chars")).toInt();
        if (settings.contains(QStringLiteral("apply_watermark"))) params.apply_watermark = settings.value(QStringLiteral("apply_watermark")).toBool();

        slm_audio out;
        memset(&out, 0, sizeof(out));
        int status = slm.slm_synthesize_v2((struct slm_context*)m_context, &params, &out);
        if (status != 0) {
            error = QString::fromUtf8(slm.slm_last_error());
            Logger::error("SpeechLmBackend", "Failed to synthesize VieNeu v3 cloned voice: " + error);
            return false;
        }
        if (!isValidRuntimeAudio(out)) {
            slm.slm_audio_free(&out);
            error = QStringLiteral("SpeechLM TTS returned an invalid or empty audio buffer.");
            return false;
        }
        samples.resize(out.n_samples);
        memcpy(samples.data(), out.samples, sizeof(float) * out.n_samples);
        sampleRate = out.sample_rate;
        slm.slm_audio_free(&out);
        return true;
    }

    QByteArray refPathBytes = QDir::toNativeSeparators(PathUtils::toNativeShortPath(referencePath)).toUtf8();
    float embedding[128];
    memset(embedding, 0, sizeof(embedding));

    int rc = slm.slm_encode_reference((struct slm_context*)m_context, refPathBytes.constData(), embedding);
    if (rc != 0) {
        error = QString::fromUtf8(slm.slm_last_error());
        Logger::error("SpeechLmBackend", "Failed to encode reference audio: " + error);
        return false;
    }

    QByteArray textBytes = text.toUtf8();
    slm_tts_params params;
    slm.slm_tts_default_params(&params);
    params.text = textBytes.constData();
    params.voice_embedding = embedding;

    if (settings.contains(QStringLiteral("temperature"))) {
        params.temperature = settings.value(QStringLiteral("temperature")).toFloat();
    }
    if (settings.contains(QStringLiteral("top_k"))) {
        params.top_k = settings.value(QStringLiteral("top_k")).toInt();
    }
    if (settings.contains(QStringLiteral("max_chars"))) {
        params.max_chars = settings.value(QStringLiteral("max_chars")).toInt();
    }
    if (settings.contains(QStringLiteral("max_tokens"))) {
        params.max_tokens = settings.value(QStringLiteral("max_tokens")).toInt();
    }
    if (settings.contains(QStringLiteral("skip_normalize"))) {
        params.skip_normalize = settings.value(QStringLiteral("skip_normalize")).toBool();
    }
    if (settings.contains(QStringLiteral("skip_phonemize"))) {
        params.skip_phonemize = settings.value(QStringLiteral("skip_phonemize")).toBool();
    }
    if (settings.contains(QStringLiteral("apply_watermark"))) {
        params.apply_watermark = settings.value(QStringLiteral("apply_watermark")).toBool();
    }

    slm_audio out;
    memset(&out, 0, sizeof(out));
    int status = slm.slm_synthesize((struct slm_context*)m_context, &params, &out);

    if (status != 0) {
        error = QString::fromUtf8(slm.slm_last_error());
        Logger::error("SpeechLmBackend", "Failed to synthesize cloned voice: " + error);
        return false;
    }

    if (!isValidRuntimeAudio(out)) {
        Logger::error("SpeechLmBackend", QString("SpeechLM TTS voice cloning returned invalid audio buffer: samples=%1, sampleRate=%2, ptr=%3")
                      .arg(out.n_samples)
                      .arg(out.sample_rate)
                      .arg(QString::number(reinterpret_cast<quintptr>(out.samples), 16)));
        slm.slm_audio_free(&out);
        error = QStringLiteral("SpeechLM TTS returned an invalid or empty audio buffer.");
        return false;
    }

    samples.resize(out.n_samples);
    memcpy(samples.data(), out.samples, sizeof(float) * out.n_samples);
    sampleRate = out.sample_rate;

    slm.slm_audio_free(&out);
    Logger::info("SpeechLmBackend", QString("SpeechLM TTS voice cloning successful"));
    return true;
}

} // namespace LAStudio
