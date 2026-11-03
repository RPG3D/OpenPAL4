#include "pal4/fog_config.h"

#include <bit>

namespace pal4 {

FogConfigState& FogConfig::Initialize(FogConfigState& state) {
    state.active = 0;
    state.enabled = 0;
    state.word4 = 0xffffffffu;
    state.word8 = 1;
    state.float12 = std::bit_cast<float>(0x42480000u);
    state.float16 = std::bit_cast<float>(0x43fa0ccdu);
    state.float20 = std::bit_cast<float>(0x3a83126fu);
    return state;
}

void FogConfig::ResetWrapper(FogConfigState& state) {
    Reset(state);
}

std::uint8_t FogConfig::EnableWithDefaults(FogConfigState& state) {
    Reset(state);
    state.active = 1;
    return 1;
}

void FogConfig::Reset(FogConfigState& state) {
    if (!state.active)
        return;
    state.active = 0;
    state.enabled = 0;
    state.word4 = 0xffffffffu;
    state.word8 = 1;
    state.float12 = std::bit_cast<float>(0x42480000u);
    state.float16 = std::bit_cast<float>(0x43fa0000u);
    state.float20 = std::bit_cast<float>(0x3a83126fu);
}

bool FogConfig::Load(FogConfigState& state, const std::string_view path,
                     IFogConfigFileRuntime& runtime) {
    Reset(state);
    std::uint8_t enabled = 0;
    std::uint32_t word4 = 0xffffffffu;
    std::uint32_t word8 = 1;
    float float12 = 0.0f;
    float float16 = 1.0f;
    float float20 = std::bit_cast<float>(0x3a83126fu);

    const std::uintptr_t file = runtime.Open(path, "r");
    if (!file)
        return false;
    runtime.Read(file, &enabled, 1, 1);
    runtime.Read(file, &word4, 4, 1);
    runtime.Read(file, &word8, 4, 1);
    runtime.Read(file, &float12, 4, 1);
    runtime.Read(file, &float16, 4, 1);
    runtime.Read(file, &float20, 4, 1);
    runtime.Close(file);

    if (!EnableWithDefaults(state))
        return false;
    state.enabled = enabled;
    state.word4 = word4;
    state.word8 = word8;
    state.float12 = float12;
    state.float16 = float16;
    state.float20 = float20;
    return true;
}

}  // namespace pal4
