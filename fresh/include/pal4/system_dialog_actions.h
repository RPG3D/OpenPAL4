#pragma once

#include <cstdint>

namespace pal4 {

class ISystemDialogVisibilityRuntime;

class ISystemDialogActionsRuntime {
public:
    virtual ~ISystemDialogActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void UpdateVisibility(std::uint32_t frame,
                                  std::uint8_t visible,
                                  std::uint32_t variable,
                                  std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t CallVirtual(
        std::uint32_t object, std::uint32_t function) = 0;
    virtual void SetScrollPosition(std::uint32_t scrollbar,
                                   float position) = 0;
};

class SystemDialogActions final {
public:
    [[nodiscard]] static std::uint8_t OnOkClick(
        std::uint32_t frame, std::uint32_t event,
        ISystemDialogActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnOkClick(
        std::uint32_t frame, std::uint32_t event,
        ISystemDialogVisibilityRuntime& visibility);
    [[nodiscard]] static std::uint32_t VirtualCall(
        std::uint32_t object, ISystemDialogActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleKeyInput(
        std::uint32_t frame, std::uint32_t event,
        ISystemDialogActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleKeyInput(
        std::uint32_t frame, std::uint32_t event,
        ISystemDialogActionsRuntime& runtime,
        ISystemDialogVisibilityRuntime& visibility);
};

}  // namespace pal4
