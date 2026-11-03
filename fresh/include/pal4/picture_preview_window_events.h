#pragma once

#include <cstdint>

#include "pal4/picture_preview_window_lifecycle.h"

namespace pal4 {

class IPicturePreviewWindowEventRuntime {
public:
    virtual ~IPicturePreviewWindowEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructBaseEvent() = 0;
    virtual void DestroyBaseEvent(std::uint32_t event) = 0;
    virtual void Close(PicturePreviewWindowState& state,
                       std::uint32_t object_address,
                       std::uint32_t event) = 0;
    virtual void HandleItemClick(PicturePreviewWindowState& state,
                                 std::uint32_t selected) = 0;
    virtual void UpdateStaticInfo(PicturePreviewWindowState& state,
                                  std::uint32_t selected) = 0;
};

class PicturePreviewWindowEventRuntimeStub :
    public IPicturePreviewWindowEventRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructBaseEvent() override { return 0u; }
    void DestroyBaseEvent(std::uint32_t) override {}
    void Close(PicturePreviewWindowState&, std::uint32_t,
               std::uint32_t) override {}
    void HandleItemClick(PicturePreviewWindowState&,
                         std::uint32_t) override {}
    void UpdateStaticInfo(PicturePreviewWindowState&,
                          std::uint32_t) override {}
};

class PicturePreviewWindowEvents final {
public:
    [[nodiscard]] static std::uint8_t OnMemberEvent(
        PicturePreviewWindowState& state, std::uint32_t member_address,
        std::uint32_t event_address,
        IPicturePreviewWindowEventRuntime& runtime);
};

}  // namespace pal4
