#pragma once

#include "pal4/introduction_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IIntroductionWindowVisibilityRuntime {
public:
    virtual ~IIntroductionWindowVisibilityRuntime() = default;
    virtual void SetParent(IntroductionWindowState& state,
                           std::uint32_t showing,
                           std::uint32_t parent) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertActive(std::uint32_t collection,
                              std::uint32_t member) = 0;
    virtual void RemoveActive(std::uint32_t collection,
                              std::uint32_t member) = 0;
    virtual void SetPanelVisibilityAndText(
        IntroductionWindowState& state, std::uint32_t window) = 0;
    virtual void SetPanelTextAndVisibility(
        IntroductionWindowState& state, std::uint32_t window) = 0;
};

class IntroductionWindowVisibilityRuntimeStub :
    public IIntroductionWindowVisibilityRuntime {
public:
    void SetParent(IntroductionWindowState&, std::uint32_t,
                   std::uint32_t) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertActive(std::uint32_t, std::uint32_t) override {}
    void RemoveActive(std::uint32_t, std::uint32_t) override {}
    void SetPanelVisibilityAndText(
        IntroductionWindowState&, std::uint32_t) override {}
    void SetPanelTextAndVisibility(
        IntroductionWindowState&, std::uint32_t) override {}
};

class IntroductionWindowVisibility final {
public:
    static void SetVisibility(IntroductionWindowState& state,
                              std::uint32_t object_address,
                              std::uint32_t showing,
                              std::uint32_t parent,
                              IIntroductionWindowVisibilityRuntime& runtime);
    static void SetPanelVisibility(
        IntroductionWindowState& state, bool visible,
        IIntroductionWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
