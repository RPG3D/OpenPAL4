#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct RoleStateWindowState {
    std::array<std::uint32_t, 0x258u / 4u> words{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        return words[offset / 4u];
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        words[offset / 4u] = value;
    }
};

enum class RoleMouseEvent { Move, Leave, ButtonUp };

class IRoleStateWindowInitRuntime {
public:
    virtual ~IRoleStateWindowInitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindImage(
        std::string_view imageset, std::string_view image) = 0;
    virtual void InitializeNameBuffer() = 0;
    [[nodiscard]] virtual std::string FormatName(
        std::string_view pattern, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    virtual void SetRoleIndex(std::uint32_t window,
                              std::uint32_t index) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetFont(std::uint32_t window,
                         std::string_view font) = 0;
    virtual void SetTextFormatting(std::uint32_t window,
                                   std::uint32_t horizontal,
                                   std::uint32_t vertical) = 0;
    [[nodiscard]] virtual std::uint32_t WidthBits(
        std::uint32_t window) = 0;
    virtual void Subscribe(std::uint32_t window, RoleMouseEvent event,
                           std::uint32_t handler,
                           std::uint32_t owner,
                           bool use_button_state_factory) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer() = 0;
};

class RoleStateWindowInitializer final {
public:
    static constexpr std::uint32_t kMouseMoveHandler = 0x476910u;
    static constexpr std::uint32_t kMouseLeaveHandler = 0x476970u;

    [[nodiscard]] static std::uint32_t Initialize(
        RoleStateWindowState& state, std::uint32_t owner,
        std::uint32_t role_index, std::uint32_t unused,
        std::uint32_t argument4, IRoleStateWindowInitRuntime& runtime);
};

}  // namespace pal4
