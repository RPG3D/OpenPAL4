#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IPropertyWindowDetailsRuntime {
public:
    virtual ~IPropertyWindowDetailsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryElement(
        std::uint32_t id, std::uint32_t category) = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateModel(
        PropertyWindowState& state, std::uint32_t record) = 0;
};

class PropertyWindowDetails final {
public:
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;

    [[nodiscard]] static std::uint32_t HoveredRecord(
        const PropertyWindowState& state,
        IPropertyWindowDetailsRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        PropertyWindowState& state, std::uint32_t record,
        IPropertyWindowDetailsRuntime& runtime);
};

}  // namespace pal4
