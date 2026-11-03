#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct FogConfigState final {
    std::uint8_t active = 0;
    std::uint8_t enabled = 0;
    std::uint8_t reserved[2]{};
    std::uint32_t word4 = 0;
    std::uint32_t word8 = 0;
    float float12 = 0.0f;
    float float16 = 0.0f;
    float float20 = 0.0f;
};

static_assert(sizeof(FogConfigState) == 24);
static_assert(offsetof(FogConfigState, word4) == 4);
static_assert(offsetof(FogConfigState, float12) == 12);

class IFogConfigFileRuntime {
public:
    virtual ~IFogConfigFileRuntime() = default;
    [[nodiscard]] virtual std::uintptr_t Open(
        std::string_view path, std::string_view mode) = 0;
    virtual void Read(std::uintptr_t file, void* destination,
                      std::uint32_t item_size, std::uint32_t item_count) = 0;
    virtual void Close(std::uintptr_t file) = 0;
};

class FogConfig final {
public:
    [[nodiscard]] static FogConfigState& Initialize(FogConfigState& state);
    static void ResetWrapper(FogConfigState& state);
    [[nodiscard]] static std::uint8_t EnableWithDefaults(FogConfigState& state);
    static void Reset(FogConfigState& state);
    [[nodiscard]] static bool Load(FogConfigState& state,
                                   std::string_view path,
                                   IFogConfigFileRuntime& runtime);
};

}  // namespace pal4
