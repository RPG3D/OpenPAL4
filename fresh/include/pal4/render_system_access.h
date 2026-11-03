#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct RenderSystemAccessState {
    std::uint8_t guard = 0u;
    std::uint32_t instance = 0u;
};

class IRenderSystemAccessRuntime {
public:
    virtual ~IRenderSystemAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view type_name,
        std::uint32_t singleton_id, std::uint32_t factory_address,
        std::uint32_t destructor_address) = 0;
};

class RenderSystemAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e3de0u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e3ddcu;
    static constexpr std::uint32_t kFactoryAddress = 0x409950u;
    static constexpr std::uint32_t kDestructorAddress = 0x4099b0u;

    [[nodiscard]] static std::uint32_t Get(
        RenderSystemAccessState& state,
        IRenderSystemAccessRuntime& runtime);
};

}  // namespace pal4
