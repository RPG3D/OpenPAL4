#include "pal4/pal_utilities_field_schema.h"

#include <array>

namespace pal4 {
namespace {

struct Field {
    UtilitiesFieldKind kind;
    std::string_view name;
    std::uint32_t offset;
};

template <std::size_t N>
std::uint8_t Register(std::uint32_t object,
                      const std::array<Field, N>& fields,
                      IUtilitiesFieldSchemaRuntime& runtime) {
    for (const auto& field : fields) {
        runtime.BeginText(field.name);
        try {
            runtime.AddField(object, field.kind, field.name, field.offset);
        } catch (...) {
            runtime.ReleaseText();
            throw;
        }
        runtime.ReleaseText();
    }
    return 1;
}

constexpr std::array<Field, 2> kIdStringFields = {{
    {UtilitiesFieldKind::Integer, "ID", 0},
    {UtilitiesFieldKind::Text, "STRING", 4},
}};

constexpr std::array<Field, 6> kSceneFields = {{
    {UtilitiesFieldKind::Integer, "ID", 0},
    {UtilitiesFieldKind::Text, "SCENE", 4},
    {UtilitiesFieldKind::Text, "SECTION", 20},
    {UtilitiesFieldKind::Text, "NAME", 36},
    {UtilitiesFieldKind::Integer, "TYPE", 52},
    {UtilitiesFieldKind::Integer, "EARTHBALL", 56},
}};

constexpr std::array<Field, 4> kPreviewFields = {{
    {UtilitiesFieldKind::Integer, "ID", 0},
    {UtilitiesFieldKind::Text, "PREVIEW", 4},
    {UtilitiesFieldKind::Text, "DESC", 20},
    {UtilitiesFieldKind::Text, "FILE", 36},
}};

constexpr std::array<Field, 3> kIntroductionFields = {{
    {UtilitiesFieldKind::Integer, "ID", 0},
    {UtilitiesFieldKind::Text, "NAME", 4},
    {UtilitiesFieldKind::Text, "INTRODUCTION", 20},
}};

constexpr std::array<Field, 5> kWindowFields = {{
    {UtilitiesFieldKind::Integer, "ID", 0},
    {UtilitiesFieldKind::Integer, "WINDOW", 4},
    {UtilitiesFieldKind::Text, "TIPS", 24},
    {UtilitiesFieldKind::Text, "CLASS", 8},
    {UtilitiesFieldKind::Integer, "QUEST_ID", 40},
}};

constexpr std::array<Field, 4> kWorldFields = {{
    {UtilitiesFieldKind::Integer, "ID", 0},
    {UtilitiesFieldKind::Text, "WORLD", 4},
    {UtilitiesFieldKind::Text, "COMBAT_SCENE", 24},
    {UtilitiesFieldKind::Integer, "FLOOR", 20},
}};

constexpr std::array<Field, 2> kStringIdFields = {{
    {UtilitiesFieldKind::Integer, "ID", 0},
    {UtilitiesFieldKind::Integer, "STRING_ID", 4},
}};

}  // namespace

std::uint8_t PalUtilitiesFieldSchema::RegisterIdString(
    std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime) {
    return Register(object, kIdStringFields, runtime);
}

std::uint8_t PalUtilitiesFieldSchema::RegisterSceneFields(
    std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime) {
    return Register(object, kSceneFields, runtime);
}

std::uint8_t PalUtilitiesFieldSchema::RegisterPreviewFields(
    std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime) {
    return Register(object, kPreviewFields, runtime);
}

std::uint8_t PalUtilitiesFieldSchema::RegisterIntroductionFields(
    std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime) {
    return Register(object, kIntroductionFields, runtime);
}

std::uint8_t PalUtilitiesFieldSchema::RegisterWindowFields(
    std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime) {
    return Register(object, kWindowFields, runtime);
}

std::uint8_t PalUtilitiesFieldSchema::RegisterWorldFields(
    std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime) {
    return Register(object, kWorldFields, runtime);
}

std::uint8_t PalUtilitiesFieldSchema::RegisterStringIdFields(
    std::uint32_t object, IUtilitiesFieldSchemaRuntime& runtime) {
    return Register(object, kStringIdFields, runtime);
}

}  // namespace pal4
