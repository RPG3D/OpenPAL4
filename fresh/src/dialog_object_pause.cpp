#include "pal4/dialog_object.h"

#include <cstddef>

namespace pal4 {
namespace {
std::uint8_t ReadByte(const DialogObjectState& state, std::size_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}
void WriteByte(DialogObjectState& state, std::size_t offset,
               std::uint8_t value) {
    const auto shift = static_cast<unsigned>(8u * (offset % 4u));
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
}  // namespace

bool DialogObject::PausePlayback(
    DialogObjectState& state, IDialogObjectPauseRuntime& runtime) {
    if (state.words[2u] == 0u || ReadByte(state, 80u) == 0u)
        return false;
    const auto was_waiting = ReadByte(state, 81u);
    WriteByte(state, 80u, 0u);
    if (was_waiting != 0u)
        return true;
    for (auto record = state.words[28u]; record != state.words[29u];
         record += 20u)
        runtime.PauseEffect(runtime.ReadEffectHandle(record));
    WriteByte(state, 81u, 1u);
    WriteByte(state, 82u, 0u);
    WriteByte(state, 80u, 0u);
    runtime.RefreshFrame(state);
    runtime.BeginWaiting(state.words[2u]);
    return true;
}

}  // namespace pal4
