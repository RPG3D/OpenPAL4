#pragma once

#include <algorithm>
#include <string_view>

namespace pal4 {

[[nodiscard]] inline bool ValidNativeActorModelName(
    const std::string_view model) {
    return !model.empty() &&
           std::all_of(model.begin(), model.end(), [](const char value) {
               return (value >= '0' && value <= '9') ||
                      (value >= 'A' && value <= 'Z') ||
                      (value >= 'a' && value <= 'z') || value == '_';
           });
}

}  // namespace pal4
