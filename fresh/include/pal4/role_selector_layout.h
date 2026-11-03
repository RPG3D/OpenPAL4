#pragma once

#include "pal4/role_static_elements.h"

#include <array>
#include <cstdint>

namespace pal4 {

enum class RoleSelectorScratch {
    Players,
    Displayed,
};

class IRoleSelectorLayoutRuntime : public IRoleStaticCharacterRuntime {
public:
    [[nodiscard]] virtual std::int32_t GetSortKey(
        std::uint32_t player) = 0;
    virtual void SetVisible(std::uint32_t window,
                            bool visible) = 0;
    virtual void AppendNavigation(std::uint32_t navigation,
                                  std::uint32_t button) = 0;
    virtual void SetPosition(
        std::uint32_t window,
        const std::array<std::uint32_t, 2>& position) = 0;
    [[nodiscard]] virtual float GetX(std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window) = 0;
    virtual void SetX(std::uint32_t window, float x) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseScratch(
        RoleSelectorScratch scratch) = 0;
};

class RoleSelectorLayout final {
public:
    // IDA 0x4b68a0.
    [[nodiscard]] static std::uint32_t Layout(
        std::uint32_t object,
        IRoleSelectorLayoutRuntime& runtime);
};

}  // namespace pal4
