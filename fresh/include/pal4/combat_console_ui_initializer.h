#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatConsoleUiRuntime {
public:
    virtual ~ICombatConsoleUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructFontElement(
        std::uint32_t memory) = 0;
    virtual void SetDimensions(std::uint32_t element,
                               float width, float height) = 0;
    virtual void SetImageset(std::uint32_t element,
                             std::string_view imageset,
                             std::string_view image, float scale) = 0;
    virtual void SetFont(std::uint32_t element,
                         std::string_view text,
                         std::string_view font, float scale) = 0;
    virtual void SetFontProperties(std::uint32_t element,
                                   float first, float second) = 0;
    [[nodiscard]] virtual std::uint32_t ElementList() = 0;
    virtual void AddElement(std::uint32_t list,
                            bool constructed,
                            std::uint32_t element) = 0;
};

class CombatConsoleUiInitializer final {
public:
    // IDA ui_initializeCombatConsole at 0x5483f0.
    [[nodiscard]] static std::uint8_t Initialize(
        ICombatConsoleUiRuntime& runtime);
};

}  // namespace pal4
