#pragma once

#include "pal4/introduction_window_visibility.h"

#include <cstdint>

namespace pal4 {

class IIntroductionWindowPanelEventsRuntime :
    public IIntroductionWindowVisibilityRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadEventCode(
        std::uint32_t event_member) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t ScrollbarOf(
        std::uint32_t information_window) = 0;
    [[nodiscard]] virtual float ScrollPosition(
        std::uint32_t scrollbar) = 0;
    virtual void SetScrollPosition(std::uint32_t scrollbar,
                                   float position) = 0;
};

class IntroductionWindowPanelEventsRuntimeStub :
    public IIntroductionWindowPanelEventsRuntime {
public:
    void SetParent(IntroductionWindowState&, std::uint32_t,
                   std::uint32_t) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertActive(std::uint32_t, std::uint32_t) override {}
    void RemoveActive(std::uint32_t, std::uint32_t) override {}
    void SetPanelVisibilityAndText(IntroductionWindowState&,
                                   std::uint32_t) override {}
    void SetPanelTextAndVisibility(IntroductionWindowState&,
                                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadEventCode(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] bool IsVisible(std::uint32_t) override { return false; }
    [[nodiscard]] bool IsDisabled(std::uint32_t) override { return false; }
    void HandleMouseEvent(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ScrollbarOf(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] float ScrollPosition(
        std::uint32_t) override { return 0.0f; }
    void SetScrollPosition(std::uint32_t, float) override {}
};

class IntroductionWindowPanelEvents final {
public:
    [[nodiscard]] static bool Handle(
        IntroductionWindowState& state, std::uint32_t member_address,
        std::uint32_t event_address,
        IIntroductionWindowPanelEventsRuntime& runtime);
};

}  // namespace pal4
