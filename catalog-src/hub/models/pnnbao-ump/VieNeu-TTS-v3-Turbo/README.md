# VieNeu-TTS v3 Turbo

VieNeu-TTS v3 Turbo is a 48 kHz Vietnamese-English text-to-speech model from Pham Nguyen Ngoc Bao. The upstream SDK runs v3 Turbo on CPU through ONNX Runtime and on GPU through PyTorch.

## LA Studio Notes

LA Studio targets the CPU ONNX path through `speech-lm-tts.cpp` with `pipelineProfile: vieneu-v3-onnx`.

Required upstream components:

- VieNeu v3 model ONNX graphs and metadata from `pnnbao-ump/VieNeu-TTS-v3-Turbo`
- MOSS Audio Tokenizer Nano ONNX codec files from `OpenMOSS-Team/MOSS-Audio-Tokenizer-Nano-ONNX`

The native C++ runtime consumes local files only. It does not download model files at runtime.
