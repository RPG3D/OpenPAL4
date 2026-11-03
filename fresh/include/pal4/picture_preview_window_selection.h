#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/picture_preview_window_lifecycle.h"

namespace pal4 {

class IPicturePreviewWindowSelectionRuntime {
public:
    virtual ~IPicturePreviewWindowSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window,
                                          bool inherited) = 0;
    virtual void RemoveChild(std::uint32_t parent, std::uint32_t child) = 0;
    virtual void AddChild(std::uint32_t parent, std::uint32_t child) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetWidth(std::uint32_t window, std::uint32_t mode,
                          float value) = 0;
    virtual void SetHeight(std::uint32_t window, std::uint32_t mode,
                           float value) = 0;
    virtual void SetX(std::uint32_t window, std::uint32_t mode,
                      float value) = 0;
    virtual void SetY(std::uint32_t window, std::uint32_t mode,
                      float value) = 0;
    [[nodiscard]] virtual std::string TriggerTaskText(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view raw) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    virtual void UpdateStaticInfo(PicturePreviewWindowState& state,
                                  std::uint32_t selected) = 0;
    virtual void HandleItemClick(PicturePreviewWindowState& state,
                                 std::uint32_t selected) = 0;
    [[nodiscard]] virtual bool HasTriggerTask(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerTask(std::uint32_t index) = 0;
    virtual void Hide(PicturePreviewWindowState& state,
                      std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint8_t DispatchPictureView(
        std::uint32_t first, std::uint32_t task) = 0;
};

class PicturePreviewWindowSelectionRuntimeStub :
    public IPicturePreviewWindowSelectionRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] bool IsDisabled(std::uint32_t, bool) override {
        return false;
    }
    void RemoveChild(std::uint32_t, std::uint32_t) override {}
    void AddChild(std::uint32_t, std::uint32_t) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetWidth(std::uint32_t, std::uint32_t, float) override {}
    void SetHeight(std::uint32_t, std::uint32_t, float) override {}
    void SetX(std::uint32_t, std::uint32_t, float) override {}
    void SetY(std::uint32_t, std::uint32_t, float) override {}
    [[nodiscard]] std::string TriggerTaskText(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view raw) override { return std::string(raw); }
    void SetText(std::uint32_t, std::u32string_view) override {}
    void UpdateStaticInfo(PicturePreviewWindowState&, std::uint32_t) override {}
    void HandleItemClick(PicturePreviewWindowState&, std::uint32_t) override {}
    [[nodiscard]] bool HasTriggerTask(std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t TriggerTask(std::uint32_t) override {
        return 0u;
    }
    void Hide(PicturePreviewWindowState&, std::uint32_t,
              std::uint32_t) override {}
    [[nodiscard]] std::uint8_t DispatchPictureView(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class PicturePreviewWindowSelection final {
public:
    static void UpdateStaticInfo(PicturePreviewWindowState& state,
                                 std::uint32_t selected,
                                 IPicturePreviewWindowSelectionRuntime& runtime);
    static void UpdateStaticInfoByIndex(
        PicturePreviewWindowState& state, std::int32_t index,
        IPicturePreviewWindowSelectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnMouseMove(
        PicturePreviewWindowState& state, std::uint32_t event,
        IPicturePreviewWindowSelectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnMouseClick(
        PicturePreviewWindowState& state, std::uint32_t event,
        IPicturePreviewWindowSelectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleItemClick(
        PicturePreviewWindowState& state, std::uint32_t selected,
        IPicturePreviewWindowSelectionRuntime& runtime);
};

}  // namespace pal4
