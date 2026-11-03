#include "pal4/thunder_visual_storage.h"

namespace pal4 {

std::uint32_t ThunderVisualStorage::Cleanup(
    ThunderObjectState& state, IThunderVisualStorageRuntime& runtime) {
    runtime.ResetProperties(state, 60u);
    (void)runtime.Free(state.words[51u]);
    state.words[51u] = 0u;
    state.words[52u] = 0u;
    state.words[53u] = 0u;
    const auto result = runtime.Free(state.words[47u]);
    state.words[47u] = 0u;
    state.words[48u] = 0u;
    state.words[49u] = 0u;
    return result;
}

}  // namespace pal4
