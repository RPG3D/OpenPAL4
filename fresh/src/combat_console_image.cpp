#include "pal4/combat_console.h"

#include <string>
#include <string_view>

namespace pal4 {
namespace {

std::u32string WidenBytes(const std::string_view source) {
    std::u32string result;
    const auto end = source.find('\0');
    const auto bytes = source.substr(0u, end);
    result.reserve(bytes.size());
    for (const unsigned char byte : bytes)
        result.push_back(static_cast<char32_t>(byte));
    return result;
}

std::uint32_t ShowImage(
    const std::uint32_t console, const std::int32_t index,
    const std::uint32_t slot_offset,
    const std::string_view image_name,
    ICombatConsoleImageRuntime& runtime) {
    const auto slot = console + slot_offset +
                      4u * static_cast<std::uint32_t>(index);
    const auto image = runtime.ReadWord(slot);
    runtime.SetChildVisible(image, true);
    runtime.SetImage(image, WidenBytes(runtime.ImageSetName()),
                     WidenBytes(image_name));
    const auto result = runtime.SetPosition(image, 1u, console + 44u);
    runtime.WriteWord(image + 280u, 1u);
    return result;
}

std::uint32_t HideImage(const std::uint32_t console,
                        const std::int32_t index,
                        const std::int32_t upper_bound,
                        const std::uint32_t slot_offset,
                        ICombatConsoleImageRuntime& runtime) {
    if (index >= upper_bound) return 0u;
    const auto slot = console + slot_offset +
                      4u * static_cast<std::uint32_t>(index);
    const auto image = runtime.ReadWord(slot);
    runtime.SetChildVisible(image, false);
    runtime.WriteWord(image + 280u, 0u);
    return image;
}

}  // namespace

std::uint32_t CombatConsoleWindow::SetImageAndPosition(
    const std::uint32_t console, const std::int32_t index,
    const std::string_view image_name,
    ICombatConsoleImageRuntime& runtime) {
    const auto state = runtime.GameState();
    if (state == 1u) return state;
    if (index >= 6)
        return static_cast<std::uint32_t>(index);

    return ShowImage(console, index, 224u, image_name, runtime);
}

std::uint32_t CombatConsoleWindow::HideElement(
    const std::uint32_t console, const std::int32_t index,
    ICombatConsoleImageRuntime& runtime) {
    return HideImage(console, index, 3, 224u, runtime);
}

std::uint32_t CombatConsoleWindow::SetImageAndPositionSecond(
    const std::uint32_t console, const std::int32_t index,
    const std::string_view image_name,
    ICombatConsoleImageRuntime& runtime) {
    const auto state = runtime.GameState();
    if (state == 0u) return state;
    if (index >= 6)
        return static_cast<std::uint32_t>(index);
    return ShowImage(console, index, 236u, image_name, runtime);
}

std::uint32_t CombatConsoleWindow::HideElementSecond(
    const std::uint32_t console, const std::int32_t index,
    ICombatConsoleImageRuntime& runtime) {
    return HideImage(console, index, 6, 236u, runtime);
}

}  // namespace pal4
