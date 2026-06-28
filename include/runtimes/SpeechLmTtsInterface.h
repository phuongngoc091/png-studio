#pragma once

#include <QLibrary>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <stdbool.h>
#include <stdint.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace LAStudio {

struct slm_init_params {
    int          abi_version;
    const char * model_path;
    const char * encoder_path;
    const char * decoder_path;
    const char * voices_json_path;
    int          n_ctx;
    int          n_threads;
    int          n_gpu_layers;
    bool         flash_attn;
    bool         mlock;
};

struct slm_audio {
    float * samples;
    int     n_samples;
    int     sample_rate;
    int     channels;
};

struct slm_context;

struct slm_tts_params {
    int           abi_version;
    const char *  text;
    const char *  voice_id;
    const float * voice_embedding;
    float         temperature;
    int           top_k;
    int           max_chars;
    int           max_tokens;
    bool          skip_normalize;
    bool          skip_phonemize;
    bool          apply_watermark;
};

struct slm_init_params_v2 {
    int          abi_version;
    const char * profile;
    const char * model_dir;
    const char * onnx_dir;
    const char * codec_dir;
    const char * config_path;
    const char * tokenizer_path;
    const char * voices_json_path;
    int          n_threads;
};

struct slm_tts_params_v2 {
    int          abi_version;
    const char * text;
    const char * voice_id;
    const char * ref_audio_path;
    float        temperature;
    int          top_k;
    float        top_p;
    int          max_new_frames;
    float        repetition_penalty;
    int          max_chars;
    bool         apply_watermark;
};

typedef const char * (*slm_version_fn)(void);
typedef const char * (*slm_last_error_fn)(void);
typedef void (*slm_audio_free_fn)(struct slm_audio * a);
typedef void (*slm_init_default_params_fn)(struct slm_init_params * p);
typedef struct slm_context * (*slm_init_fn)(const struct slm_init_params * params);
typedef void (*slm_free_fn)(struct slm_context * slm);
typedef void (*slm_tts_default_params_fn)(struct slm_tts_params * p);
typedef int (*slm_synthesize_fn)(struct slm_context * slm, const struct slm_tts_params * params, struct slm_audio * out);
typedef void (*slm_init_v2_default_params_fn)(struct slm_init_params_v2 * p);
typedef struct slm_context * (*slm_init_v2_fn)(const struct slm_init_params_v2 * params);
typedef void (*slm_tts_v2_default_params_fn)(struct slm_tts_params_v2 * p);
typedef int (*slm_synthesize_v2_fn)(struct slm_context * slm, const struct slm_tts_params_v2 * params, struct slm_audio * out);
typedef int (*slm_encode_reference_fn)(struct slm_context * slm, const char * ref_audio_path, float * out_embedding_128);
typedef int (*slm_list_preset_voices_fn)(struct slm_context * slm, char * out_json, int max_len);
typedef int (*slm_set_preset_voice_fn)(struct slm_context * slm, const char * voice_id);

class SpeechLmTtsInterface {
public:
    static SpeechLmTtsInterface& instance() {
        static SpeechLmTtsInterface inst;
        return inst;
    }

    void unload() {
        if (m_lib.isLoaded()) {
            m_lib.unload();
        }
        slm_version = nullptr;
        slm_last_error = nullptr;
        slm_audio_free = nullptr;
        slm_init_default_params = nullptr;
        slm_init = nullptr;
        slm_free = nullptr;
        slm_tts_default_params = nullptr;
        slm_synthesize = nullptr;
        slm_init_v2_default_params = nullptr;
        slm_init_v2 = nullptr;
        slm_tts_v2_default_params = nullptr;
        slm_synthesize_v2 = nullptr;
        slm_encode_reference = nullptr;
        slm_list_preset_voices = nullptr;
        slm_set_preset_voice = nullptr;
        m_loadedPath.clear();
        m_lastError.clear();
    }

    bool load(const QString& libPath) {
        if (m_lib.isLoaded()) {
            if (m_lib.fileName() == libPath) return true;
            m_lastError = QStringLiteral("Hot-switch between SpeechLM TTS runtimes is not supported safely in one session. Please restart app after changing runtime.");
            return false;
        }

        QFileInfo fi(libPath);
        QString dir = QDir::toNativeSeparators(fi.absolutePath());

#ifdef Q_OS_WIN
        SetDllDirectoryW((LPCWSTR)dir.utf16());
#endif

        m_lib.setFileName(libPath);
        bool ok = m_lib.load();

#ifdef Q_OS_WIN
        SetDllDirectoryW(NULL);
#endif

        if (!ok) {
            m_lastError = m_lib.errorString();
            return false;
        }

        slm_version = resolve<slm_version_fn>("slm_version", "vn_version");
        slm_last_error = resolve<slm_last_error_fn>("slm_last_error", "vn_last_error");
        slm_audio_free = resolve<slm_audio_free_fn>("slm_audio_free", "vn_audio_free");
        slm_init_default_params = resolve<slm_init_default_params_fn>("slm_init_default_params", "vn_init_default_params");
        slm_init = resolve<slm_init_fn>("slm_init", "vn_init");
        slm_free = resolve<slm_free_fn>("slm_free", "vn_free");
        slm_tts_default_params = resolve<slm_tts_default_params_fn>("slm_tts_default_params", "vn_tts_default_params");
        slm_synthesize = resolve<slm_synthesize_fn>("slm_synthesize", "vn_synthesize");
        slm_init_v2_default_params = reinterpret_cast<slm_init_v2_default_params_fn>(m_lib.resolve("slm_init_v2_default_params"));
        slm_init_v2 = reinterpret_cast<slm_init_v2_fn>(m_lib.resolve("slm_init_v2"));
        slm_tts_v2_default_params = reinterpret_cast<slm_tts_v2_default_params_fn>(m_lib.resolve("slm_tts_v2_default_params"));
        slm_synthesize_v2 = reinterpret_cast<slm_synthesize_v2_fn>(m_lib.resolve("slm_synthesize_v2"));
        slm_encode_reference = resolve<slm_encode_reference_fn>("slm_encode_reference", "vn_encode_reference");
        slm_list_preset_voices = resolve<slm_list_preset_voices_fn>("slm_list_preset_voices", "vn_list_preset_voices");
        slm_set_preset_voice = resolve<slm_set_preset_voice_fn>("slm_set_preset_voice", "vn_set_preset_voice");

        ok = slm_version && slm_last_error && slm_audio_free &&
             slm_init_default_params && slm_init && slm_free &&
             slm_tts_default_params && slm_synthesize &&
             slm_encode_reference && slm_list_preset_voices && slm_set_preset_voice;

        if (!ok) {
            m_lastError = QStringLiteral("Failed to resolve required SpeechLM TTS symbols from runtime library.");
            m_lib.unload();
        } else {
            m_loadedPath = libPath;
            m_lastError.clear();
        }
        return ok;
    }

    bool isLoaded() const { return m_lib.isLoaded(); }
    QString errorString() const { return m_lastError.isEmpty() ? m_lib.errorString() : m_lastError; }

    slm_version_fn slm_version = nullptr;
    slm_last_error_fn slm_last_error = nullptr;
    slm_audio_free_fn slm_audio_free = nullptr;
    slm_init_default_params_fn slm_init_default_params = nullptr;
    slm_init_fn slm_init = nullptr;
    slm_free_fn slm_free = nullptr;
    slm_tts_default_params_fn slm_tts_default_params = nullptr;
    slm_synthesize_fn slm_synthesize = nullptr;
    slm_init_v2_default_params_fn slm_init_v2_default_params = nullptr;
    slm_init_v2_fn slm_init_v2 = nullptr;
    slm_tts_v2_default_params_fn slm_tts_v2_default_params = nullptr;
    slm_synthesize_v2_fn slm_synthesize_v2 = nullptr;
    slm_encode_reference_fn slm_encode_reference = nullptr;
    slm_list_preset_voices_fn slm_list_preset_voices = nullptr;
    slm_set_preset_voice_fn slm_set_preset_voice = nullptr;

private:
    SpeechLmTtsInterface() = default;

    template<typename Fn>
    Fn resolve(const char *primary, const char *legacy) {
        if (auto fn = reinterpret_cast<Fn>(m_lib.resolve(primary))) {
            return fn;
        }
        return reinterpret_cast<Fn>(m_lib.resolve(legacy));
    }

    QLibrary m_lib;
    QString m_loadedPath;
    QString m_lastError;
};

} // namespace LAStudio
