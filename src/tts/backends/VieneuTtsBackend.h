#pragma once

#include "SpeechLmBackend.h"

namespace LAStudio {

class VieneuTtsBackend : public SpeechLmBackend {
public:
    VieneuTtsBackend() = default;
    ~VieneuTtsBackend() override = default;
};

} // namespace LAStudio
