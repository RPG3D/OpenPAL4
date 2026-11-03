#pragma once

#include <cstdint>

namespace pal4 {

class ISystemToolbarVisibilityRuntime {
public:
    virtual ~ISystemToolbarVisibilityRuntime() = default;
    virtual void SetParent(std::uint32_t frame,
                           std::uint8_t visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void DispatchEvent(std::uint32_t tree, std::uint32_t key,
                               std::uint32_t first,
                               std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    [[nodiscard]] virtual std::int32_t GameEntry(
        std::uint32_t game, std::uint32_t index) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t LastActiveMember(
        std::uint32_t members) = 0;
    [[nodiscard]] virtual std::uint32_t InsertMember(
        std::uint32_t members, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveMember(
        std::uint32_t members, std::uint32_t member) = 0;
};

class SystemToolbarVisibility final {
public:
    static constexpr std::uint32_t kParentEvent = 0x8b1394u;
    static constexpr std::uint32_t kWindowEvent = 0x8b13a0u;
    static constexpr std::uint32_t kShowEvent = 0x8b13a4u;

    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t frame, std::uint8_t visible,
        std::uint32_t parent, ISystemToolbarVisibilityRuntime& runtime);
};

}  // namespace pal4
