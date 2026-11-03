#pragma once

#include <cstdint>

#include "pal4/movie_preview_window_lifecycle.h"

namespace pal4 {

class IMoviePreviewWindowVisibilityRuntime {
public:
    virtual ~IMoviePreviewWindowVisibilityRuntime() = default;
    virtual void SetParent(MoviePreviewWindowState& state,
                           std::uint8_t showing, std::uint32_t parent) = 0;
    virtual void UpdateImages(MoviePreviewWindowState& state) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window,
                                          bool inherited) = 0;
    virtual void UpdateStaticInfo(MoviePreviewWindowState& state,
                                  std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t array, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActive(
        std::uint32_t array, std::uint32_t member) = 0;
};

class MoviePreviewWindowVisibilityRuntimeStub :
    public IMoviePreviewWindowVisibilityRuntime {
public:
    void SetParent(MoviePreviewWindowState&, std::uint8_t,
                   std::uint32_t) override {}
    void UpdateImages(MoviePreviewWindowState&) override {}
    [[nodiscard]] bool IsDisabled(std::uint32_t, bool) override {
        return false;
    }
    void UpdateStaticInfo(MoviePreviewWindowState&,
                          std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class MoviePreviewWindowVisibility final {
public:
    [[nodiscard]] static std::uint32_t SetVisible(
        MoviePreviewWindowState& state, std::uint32_t object_address,
        std::uint8_t showing, std::uint32_t parent,
        IMoviePreviewWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
