#include "pal4/game_world_resource_name.h"

#include <stdexcept>

namespace pal4 {

std::string GameWorldResourceName::Build(const std::string_view path) {
    const auto first_separator = path.find('\\');
    const auto last_separator = path.find_last_of('\\');
    const auto extension = path.find_last_of('.');
    if (first_separator == std::string_view::npos ||
        last_separator == std::string_view::npos ||
        extension == std::string_view::npos ||
        extension < last_separator)
        throw std::out_of_range("resource path has no directory or extension");
    std::string result = "Accelerate";
    result.append(path.substr(first_separator,
                              last_separator - first_separator));
    result.append(path.substr(last_separator,
                              extension - last_separator));
    result += "_d3d9.txd";
    return result;
}

}  // namespace pal4
