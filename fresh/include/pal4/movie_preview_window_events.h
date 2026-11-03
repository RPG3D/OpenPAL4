#pragma once

#include <cstdint>

#include "pal4/movie_preview_window_lifecycle.h"

namespace pal4 {

class IMoviePreviewWindowEventRuntime {
public:
    virtual ~IMoviePreviewWindowEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructBaseEvent() = 0;
    virtual void DestroyBaseEvent(std::uint32_t event) = 0;
    virtual void Close(MoviePreviewWindowState& state,
                       std::uint32_t object_address,
                       std::uint32_t event) = 0;
    virtual void HandleItemClick(MoviePreviewWindowState& state,
                                 std::uint32_t selected) = 0;
    virtual void UpdateStaticInfo(MoviePreviewWindowState& state,
                                  std::uint32_t selected) = 0;
};

class MoviePreviewWindowEventRuntimeStub :
    public IMoviePreviewWindowEventRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructBaseEvent() override { return 0u; }
    void DestroyBaseEvent(std::uint32_t) override {}
    void Close(MoviePreviewWindowState&, std::uint32_t,
               std::uint32_t) override {}
    void HandleItemClick(MoviePreviewWindowState&,
                         std::uint32_t) override {}
    void UpdateStaticInfo(MoviePreviewWindowState&,
                          std::uint32_t) override {}
};

class MoviePreviewWindowEvents final {
public:
    [[nodiscard]] static std::uint8_t OnMemberEvent(
        MoviePreviewWindowState& state, std::uint32_t member_address,
        std::uint32_t event_address,
        IMoviePreviewWindowEventRuntime& runtime);
};

}  // namespace pal4
