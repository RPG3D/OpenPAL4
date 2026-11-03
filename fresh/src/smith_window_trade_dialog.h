#pragma once

#include "pal4/smith_window.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

void ShowSmithTradeDialog(
    ISmithWindowDialogRuntime& runtime, std::uint32_t object,
    std::uint32_t template_id, std::uint32_t style,
    std::optional<std::string_view> name = std::nullopt,
    std::optional<SmithWindowTradeCallback> callback_kind = std::nullopt);

}  // namespace pal4
