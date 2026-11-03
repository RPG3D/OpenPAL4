#pragma once

#include <string>
#include <string_view>

namespace pal4 {

class GameWorldResourceName final {
public:
    [[nodiscard]] static std::string Build(std::string_view path);
};

}  // namespace pal4
