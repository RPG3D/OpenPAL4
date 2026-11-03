#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ISystemDialogVisibilityRuntime {
public:
    virtual ~ISystemDialogVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t TriggerVariable(
        std::uint32_t collection, std::string_view name) = 0;
    [[nodiscard]] virtual std::string_view TriggerText(
        std::uint32_t variable) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view source) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCeguiString(
        std::string_view utf8) = 0;
    virtual void SetText(std::uint32_t window,
                         std::uint32_t cegui_string) = 0;
    virtual void DestroyCeguiString(std::uint32_t cegui_string) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetParent(std::uint32_t frame, std::uint8_t visible,
                           std::uint32_t parent) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    virtual void SetModalState(std::uint32_t window,
                               std::uint8_t visible) = 0;
};

class SystemDialogVisibility final {
public:
    static void Update(std::uint32_t frame, std::uint8_t visible,
                       std::string_view variable_name,
                       std::uint32_t parent,
                       ISystemDialogVisibilityRuntime& runtime);
};

}  // namespace pal4
