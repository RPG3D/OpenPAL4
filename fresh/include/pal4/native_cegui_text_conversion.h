#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

// Converts original database text using the game's ACP-to-UTF-8 path.
[[nodiscard]] std::optional<std::string> ConvertGameTextToUtf8(
    std::string_view source);

}  // namespace pal4
