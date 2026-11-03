#pragma once

#include <cstdint>
#include <array>
#include <string_view>

namespace pal4 {

class IWorldManagerEventRuntime {
public:
    virtual ~IWorldManagerEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EventKey(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t router, std::uint32_t mode,
        std::uint32_t key, std::uint32_t owner,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindEventGroup(
        std::uint32_t router, std::uint32_t key) = 0;
    virtual void ReleaseOwnedSlot(std::uint32_t slot) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class WorldManagerEventSubscriptions final {
public:
    static constexpr std::uint32_t kJoinCallback = 0x53f7c0u;
    static constexpr std::uint32_t kLeaveCallback = 0x53f810u;
    struct Binding {std::string_view name;std::uint32_t callback;};
    static constexpr std::array<Binding,4> kBindings{{
        {"re_join_scene",kJoinCallback},{"re_leave_scene",kLeaveCallback},
        {"me_join_scene",kJoinCallback},{"me_leave_scene",kLeaveCallback}}};

    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t manager, IWorldManagerEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t Unsubscribe(
        std::uint32_t manager, IWorldManagerEventRuntime& runtime);
};

}  // namespace pal4
