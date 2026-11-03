#include "pal4/game_data_equipment_text_formatter.h"

#include <array>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

std::uint32_t AssignFormatted(std::uint32_t destination, std::uint8_t seed,
                              std::string_view text,
                              IGameDataEquipmentTextRuntime& runtime) {
    const auto allocated = runtime.PrepareTemporary(text);
    runtime.InitializeDestination(destination, seed);
    runtime.Assign(destination, allocated ? text : std::string_view{});
    if (allocated) runtime.ReleaseTemporary();
    return destination;
}

}  // namespace

std::uint32_t GameDataEquipmentTextFormatter::FromInteger(
    std::uint32_t destination, std::int32_t value, std::uint8_t seed,
    IGameDataEquipmentTextRuntime& runtime) {
    return AssignFormatted(destination, seed, std::to_string(value), runtime);
}

std::uint32_t GameDataEquipmentTextFormatter::Format(
    std::uint32_t destination, std::uint8_t seed,
    IGameDataEquipmentTextRuntime& runtime, const char* format, ...) {
    std::array<char, 1024> buffer{};
    va_list arguments;
    va_start(arguments, format);
    const auto length = std::vsnprintf(buffer.data(), buffer.size(), format,
                                       arguments);
    va_end(arguments);
    if (length < 0 || static_cast<std::size_t>(length) >= buffer.size())
        throw std::length_error("equipment formatted text exceeds 1023 bytes");
    return AssignFormatted(destination, seed,
                           std::string_view(buffer.data(), length), runtime);
}

}  // namespace pal4
