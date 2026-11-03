#include "pal4/native_movie_flag_runtime.h"

#include <cstdint>
#include <stdexcept>

static_assert(sizeof(void*) == 4u);
static_assert(sizeof(pal4::DialogManagerState) ==
              pal4::DialogManagerState::kImageSize);

namespace pal4 {

std::uint32_t NativeMovieFlagRuntime::GlobalData() {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&state_));
}

void NativeMovieFlagRuntime::CheckMovieFlagAddress(
    const std::uint32_t address) const {
    const auto base = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&state_));
    if (address != base + 0x18u)
        throw std::out_of_range("PAL4 movie flag address");
}

std::uint32_t NativeMovieFlagRuntime::ReadWord(
    const std::uint32_t address) {
    CheckMovieFlagAddress(address);
    return state_.Word(0x18u);
}

void NativeMovieFlagRuntime::WriteWord(
    const std::uint32_t address, const std::uint32_t value) {
    CheckMovieFlagAddress(address);
    state_.SetWord(0x18u, value);
}

void NativeMovieFlagRuntime::SaveDialogSettings(
    const std::uint32_t global_data) {
    if (global_data != GlobalData())
        throw std::invalid_argument("PAL4 dialog settings object mismatch");
    DialogManagerSettings::Save(state_, settings_);
}

}  // namespace pal4
