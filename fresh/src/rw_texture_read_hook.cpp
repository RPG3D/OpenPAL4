#include "pal4/rw_texture_read_hook.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace pal4 {

std::uint32_t RwTextureReadHook::Install(
    RwTextureReadHookState& state,
    IRwTextureReadHookRuntime& runtime) {
    state.previous_callback = runtime.GetReadCallback();
    return runtime.SetReadCallback(kCustomCallbackAddress);
}

std::uint32_t RwTextureReadHook::Read(
    const RwTextureReadHookState& state,
    const char* name, const std::uint32_t mask_address,
    IRwTextureReadHookRuntime& runtime) {
    // Existing host input policy: original strlen/strncpy has no null or length
    // guard. These checks are not evidence of original malformed-input behavior.
    if (name == nullptr) return 0;
    std::array<char, 256> base{};
    const auto length = std::strlen(name);
    const auto* extension = std::strrchr(name, '.');
    if (extension == nullptr) {
        const auto copied = std::min(length, base.size() - 1u);
        std::memcpy(base.data(), name, copied);
    } else if (length > 5u) {
        const auto copied = std::min(
            static_cast<std::size_t>(extension - name), base.size() - 1u);
        std::memcpy(base.data(), name, copied);
    }

    const auto texture = runtime.ReadDds(base.data(), mask_address);
    if (texture != 0 || state.previous_callback == 0) return texture;
    return runtime.ReadOriginal(state.previous_callback, name, mask_address);
}

}  // namespace pal4
