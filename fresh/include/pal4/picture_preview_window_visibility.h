#pragma once

#include <cstdint>

#include "pal4/picture_preview_window_lifecycle.h"

namespace pal4 {

class IPicturePreviewWindowVisibilityRuntime {
public:
    virtual ~IPicturePreviewWindowVisibilityRuntime() = default;
    virtual void SetParent(PicturePreviewWindowState& state,
                           std::uint8_t showing, std::uint32_t parent) = 0;
    virtual void UpdateItemList(PicturePreviewWindowState& state) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window,
                                          bool inherited) = 0;
    virtual void UpdateStaticInfo(PicturePreviewWindowState& state,
                                  std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t array, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActive(
        std::uint32_t array, std::uint32_t member) = 0;
};

class PicturePreviewWindowVisibilityRuntimeStub :
    public IPicturePreviewWindowVisibilityRuntime {
public:
    void SetParent(PicturePreviewWindowState&, std::uint8_t,
                   std::uint32_t) override {}
    void UpdateItemList(PicturePreviewWindowState&) override {}
    [[nodiscard]] bool IsDisabled(std::uint32_t, bool) override {
        return false;
    }
    void UpdateStaticInfo(PicturePreviewWindowState&,
                          std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class PicturePreviewWindowVisibility final {
public:
    [[nodiscard]] static std::uint32_t SetVisible(
        PicturePreviewWindowState& state, std::uint32_t object_address,
        std::uint8_t showing, std::uint32_t parent,
        IPicturePreviewWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
