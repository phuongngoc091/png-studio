#pragma once

namespace LAStudio {

struct crispasr_session;

struct crispasr_open_params_v1 {
    int abi_version;
    int n_threads;
    int use_gpu;
    int verbosity;
    int flash_attn;
    int n_gpu_layers;
    int reserved[6];
};

} // namespace LAStudio
