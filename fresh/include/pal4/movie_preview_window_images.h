#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/movie_preview_window_lifecycle.h"

namespace pal4 {

class IMoviePreviewWindowImagesRuntime {
public:
    virtual ~IMoviePreviewWindowImagesRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UnlockMask() = 0;
    [[nodiscard]] virtual bool HasTriggerTask(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerTaskAddress(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerTaskId(
        std::uint32_t task) = 0;
    [[nodiscard]] virtual std::string TriggerTaskImageName(
        std::uint32_t task) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view raw) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    virtual void SetImagePointer(std::uint32_t window,
                                 std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void Hide(MoviePreviewWindowState& state,
                      std::uint32_t first,
                      std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint8_t Dispatch(
        std::string_view command, std::uint32_t first,
        std::uint32_t second) = 0;
};

class MoviePreviewWindowImagesRuntimeStub :
    public IMoviePreviewWindowImagesRuntime {
public:
    [[nodiscard]] std::uint32_t UnlockMask() override { return 0u; }
    [[nodiscard]] bool HasTriggerTask(std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t TriggerTaskAddress(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t TriggerTaskId(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::string TriggerTaskImageName(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view raw) override { return std::string(raw); }
    void SetEnabled(std::uint32_t, bool) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    void SetImagePointer(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void Hide(MoviePreviewWindowState&, std::uint32_t,
              std::uint32_t) override {}
    [[nodiscard]] std::uint8_t Dispatch(
        std::string_view, std::uint32_t,
        std::uint32_t) override { return 0u; }
};

class MoviePreviewWindowImages final {
public:
    static constexpr std::string_view kImageset = "biaoti30";
    static constexpr std::string_view kPlayCommand = "start_play_movie";

    static void SetImageAndDisable(MoviePreviewWindowState& state,
                                   std::uint32_t image,
                                   IMoviePreviewWindowImagesRuntime& runtime);
    static void Update(MoviePreviewWindowState& state,
                       IMoviePreviewWindowImagesRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleItemClick(
        MoviePreviewWindowState& state, std::uint32_t selected,
        IMoviePreviewWindowImagesRuntime& runtime);
};

}  // namespace pal4
