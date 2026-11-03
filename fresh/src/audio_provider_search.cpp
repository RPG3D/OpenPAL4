#include "pal4/audio_manager.h"
#include <cstring>

namespace pal4 {
std::uint32_t AudioSystem::Find3DProvider(
    const char* requested_name, IAudioProviderSearchRuntime& runtime) {
    std::uint32_t cursor = 0u;
    std::uint32_t provider = 0u;
    const char* name = nullptr;
    while (runtime.Enumerate(cursor, provider, name)) {
        if (std::strstr(name, requested_name) != nullptr &&
            runtime.Open(provider) == 0) {
            runtime.Close(provider);
            runtime.Log(93u);
            return provider;
        }
    }
    if (runtime.HasLastError())
        runtime.ReadLastError();
    runtime.Log(94u);

    cursor = 0u;
    while (runtime.Enumerate(cursor, provider, name)) {
        if (runtime.Open(provider) == 0) {
            runtime.Close(provider);
            runtime.Log(95u);
            return provider;
        }
    }
    if (runtime.HasLastError())
        runtime.ReadLastError();
    runtime.Log(96u);
    return 0u;
}

}  // namespace pal4
