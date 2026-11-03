#include "pal4/ui_exit_game_callback.h"

namespace pal4 {

std::uint32_t UiExitGameCallback::Construct(
    const std::uint32_t self, const std::uint32_t command,
    const std::uint32_t first, const std::uint32_t second,
    IUiExitGameCallbackRuntime& runtime) {
    runtime.WriteWord(self + 8u, command);
    runtime.WriteWord(self + 12u, first);
    runtime.WriteWord(self + 16u, second);
    runtime.WriteWord(self, kVtable);
    return self;
}

std::uint32_t UiExitGameCallback::Clone(
    const std::uint32_t self,
    IUiExitGameCallbackRuntime& runtime) {
    const auto clone = runtime.AllocateMemory(0x18u);
    if (clone == 0u) return 0u;
    runtime.WriteWord(clone, kBaseVtable);
    runtime.WriteWord(clone + 8u, runtime.ReadWord(self + 8u));
    runtime.WriteWord(clone + 12u, runtime.ReadWord(self + 12u));
    runtime.WriteWord(clone + 16u, runtime.ReadWord(self + 16u));
    runtime.WriteWord(clone, kVtable);
    return clone;
}

}  // namespace pal4
