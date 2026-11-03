#pragma once

#include <cstdint>

namespace pal4 {

struct RwTextureReadHookState final {
    std::uint32_t previous_callback = 0;
};

class IRwTextureReadHookRuntime {
public:
    virtual ~IRwTextureReadHookRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetReadCallback() = 0;
    [[nodiscard]] virtual std::uint32_t SetReadCallback(
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDds(
        const char* base_name, std::uint32_t mask_address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadOriginal(
        std::uint32_t callback, const char* name,
        std::uint32_t mask_address) = 0;
};

class RwTextureReadHook final {
public:
    static constexpr std::uint32_t kPreviousCallbackAddress = 0x94d488u;
    static constexpr std::uint32_t kCustomCallbackAddress = 0x66cac0u;

    [[nodiscard]] static std::uint32_t Install(
        RwTextureReadHookState& state,
        IRwTextureReadHookRuntime& runtime);
    [[nodiscard]] static std::uint32_t Read(
        const RwTextureReadHookState& state,
        const char* name, std::uint32_t mask_address,
        IRwTextureReadHookRuntime& runtime);
};

}  // namespace pal4
