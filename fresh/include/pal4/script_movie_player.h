#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptMoviePlayer {
public:
    virtual ~IScriptMoviePlayer() = default;
    [[nodiscard]] virtual std::uint32_t Play(std::string_view movie_name) = 0;
    [[nodiscard]] virtual std::uint32_t PlayFinal(
        std::string_view movie_name) = 0;
};

}  // namespace pal4
