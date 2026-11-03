#include "pal4/game_arena_string_matcher.h"

#include <cctype>
#include <string_view>

namespace pal4 {
namespace {

bool SameIgnoreCase(std::string_view left, std::string_view right) {
    left = left.substr(0, left.find('\0'));
    right = right.substr(0, right.find('\0'));
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) return false;
    }
    return true;
}

}  // namespace

bool GameArenaStringMatcher::Matches(
    GameArenaStringArgument arena, GameArenaStringArgument area,
    IGameArenaStringRuntime& runtime) {
    const std::string_view requested_arena = arena.value ? arena.value : "";
    if (!SameIgnoreCase(runtime.ReadText(runtime.ArenaManager() + 240u),
                        requested_arena)) return false;
    if (area.compare_area == 0u) return true;
    const std::string_view requested_area = area.value ? area.value : "";
    return SameIgnoreCase(runtime.ReadText(runtime.ArenaManager() + 252u),
                          requested_area);
}

}  // namespace pal4
