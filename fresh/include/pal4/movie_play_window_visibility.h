#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/movie_play_window_lifecycle.h"

namespace pal4 {

class IMoviePlayWindowVisibilityRuntime {
public:
    virtual ~IMoviePlayWindowVisibilityRuntime() = default;
    virtual void SetParent(MoviePlayWindowState& state, std::uint8_t showing,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertActive(std::uint32_t array,
                              std::uint32_t member) = 0;
    virtual void RemoveActive(std::uint32_t array,
                              std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t MusicPlayer() = 0;
    virtual void StopCurrentWithFade(std::uint32_t player,
                                     float seconds) = 0;
    virtual void FadeInBackground(std::uint32_t player,
                                  std::string_view file, float seconds,
                                  std::uint32_t flags) = 0;
};

class MoviePlayWindowVisibilityRuntimeStub :
    public IMoviePlayWindowVisibilityRuntime {
public:
    void SetParent(MoviePlayWindowState&, std::uint8_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertActive(std::uint32_t, std::uint32_t) override {}
    void RemoveActive(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t MusicPlayer() override { return 0u; }
    void StopCurrentWithFade(std::uint32_t, float) override {}
    void FadeInBackground(std::uint32_t, std::string_view, float,
                          std::uint32_t) override {}
};

class MoviePlayWindowVisibility final {
public:
    static void SetVisible(MoviePlayWindowState& state,
                           std::uint32_t object_address,
                           std::uint8_t showing, std::uint32_t parent,
                           IMoviePlayWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
