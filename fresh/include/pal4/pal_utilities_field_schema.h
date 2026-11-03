#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class UtilitiesFieldKind { Integer, Text };

class IUtilitiesFieldSchemaRuntime {
public:
    virtual ~IUtilitiesFieldSchemaRuntime() = default;
    virtual void BeginText(std::string_view name) = 0;
    virtual void AddField(std::uint32_t object,
                          UtilitiesFieldKind kind,
                          std::string_view name,
                          std::uint32_t offset) = 0;
    virtual void ReleaseText() = 0;
};

class PalUtilitiesFieldSchema final {
public:
    // IDA 0x4ffbb0: register ID and STRING at offsets 0 and 4.
    [[nodiscard]] static std::uint8_t RegisterIdString(
        std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime);
    // IDA 0x4ffd10: register six scene/section/name/type fields.
    [[nodiscard]] static std::uint8_t RegisterSceneFields(
        std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime);
    [[nodiscard]] static std::uint8_t RegisterPreviewFields(
        std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime);
    [[nodiscard]] static std::uint8_t RegisterIntroductionFields(
        std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime);
    [[nodiscard]] static std::uint8_t RegisterWindowFields(
        std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime);
    [[nodiscard]] static std::uint8_t RegisterWorldFields(
        std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime);
    [[nodiscard]] static std::uint8_t RegisterStringIdFields(
        std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime);
};

}  // namespace pal4
