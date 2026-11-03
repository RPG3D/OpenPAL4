#pragma once

#include <array>
#include <cstdint>
#include "pal4/ui_navigation_list.h"

namespace pal4 {
class IRoleSelectorNavigationRuntime {
public:
    virtual ~IRoleSelectorNavigationRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual void CloseRoleSelector(std::uint32_t owner)=0;
    virtual std::uint8_t NavigateRoleSelector(std::uint32_t navigation,std::uint32_t event)=0;
};

class IRoleSelectorActionRuntime {
public:
    virtual ~IRoleSelectorActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void InvokeChoice(
        std::uint32_t callback,
        const std::array<std::uint32_t, 2>& arguments) = 0;
    virtual void LayoutButtons(std::uint32_t object) = 0;
    virtual void Close(std::uint32_t object,
                       std::uint32_t visible,
                       std::uint32_t parent) = 0;
    virtual void ForwardKeyEvent(std::uint32_t navigation,
                                 std::uint32_t event) = 0;
    virtual void RefreshNavigation(std::uint32_t navigation) = 0;
};

class RoleSelectorActions final {
public:
    // IDA 0x4b6de0, embedded navigation at owner+40.
    static std::uint8_t HandleNavigationKey(std::uint32_t navigation,std::uint32_t event,
                                           IRoleSelectorNavigationRuntime&);
    // IDA 0x4b6840.
    [[nodiscard]] static std::uint8_t HandleRoleClick(
        std::uint32_t object, std::uint32_t event,
        IRoleSelectorActionRuntime& runtime);
    // IDA 0x4b6890.
    [[nodiscard]] static std::uint8_t HandleCloseClick(
        std::uint32_t object,
        IRoleSelectorActionRuntime& runtime);
    // IDA 0x4b6e10.
    [[nodiscard]] static std::uint8_t HandleKeyEvent(
        std::uint32_t object, std::uint32_t event,
        IRoleSelectorActionRuntime& runtime);
};

}  // namespace pal4
