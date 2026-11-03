#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/picture_view_window_lifecycle.h"

namespace pal4 {

class IPictureViewWindowCommandsRuntime {
public:
    virtual ~IPictureViewWindowCommandsRuntime() = default;
    virtual void SetFrameVisible(std::string_view name, bool visible) = 0;
    virtual void Dispatch(std::string_view command, std::uint32_t first,
                          std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(std::string_view name) = 0;
    virtual void Display(std::uint32_t frame, std::uint32_t record) = 0;
    [[nodiscard]] virtual bool HasImageset(std::string_view name) = 0;
    virtual void DestroyImageset(std::string_view name) = 0;
    virtual void Hide(PictureViewWindowState& state, std::uint32_t first,
                      std::uint32_t second) = 0;
    virtual void UpdatePreview(std::uint32_t frame, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t HandleOtherMemberEvent(
        std::uint32_t member, std::uint32_t event) = 0;
};

class PictureViewWindowCommandsRuntimeStub :
    public IPictureViewWindowCommandsRuntime {
public:
    void SetFrameVisible(std::string_view, bool) override {}
    void Dispatch(std::string_view, std::uint32_t,
                  std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FindFrame(std::string_view) override { return 0u; }
    void Display(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] bool HasImageset(std::string_view) override { return false; }
    void DestroyImageset(std::string_view) override {}
    void Hide(PictureViewWindowState&, std::uint32_t,
              std::uint32_t) override {}
    void UpdatePreview(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t HandleOtherMemberEvent(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class PictureViewWindowCommands final {
public:
    static constexpr std::string_view kWindow = "PictureViewWindow";
    static constexpr std::string_view kPreviewCommand = "show_picture_preview";
    static constexpr std::string_view kPreviewWindow = "picturePreviewWindow";
    static constexpr std::string_view kImageset = "ui_picture_view_imageset";

    [[nodiscard]] static std::uint8_t HandleViewCommand(
        std::uint32_t visible, std::uint32_t record,
        IPictureViewWindowCommandsRuntime& runtime);
    [[nodiscard]] static std::uint8_t Close(
        PictureViewWindowState& state,
        IPictureViewWindowCommandsRuntime& runtime);
    [[nodiscard]] static std::uint8_t Previous(
        PictureViewWindowState& state, std::uint32_t frame,
        IPictureViewWindowCommandsRuntime& runtime);
    [[nodiscard]] static std::uint8_t Next(
        PictureViewWindowState& state, std::uint32_t frame,
        IPictureViewWindowCommandsRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnCloseMemberEvent(
        PictureViewWindowState& state, std::uint32_t member,
        std::uint32_t event_code, std::uint32_t event,
        IPictureViewWindowCommandsRuntime& runtime);
};

}  // namespace pal4
