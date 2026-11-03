#pragma once

#include "pal4/introduction_window_visibility.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IIntroductionWindowPanelRuntime :
    public IIntroductionWindowVisibilityRuntime {
public:
    [[nodiscard]] virtual std::int32_t WindowUserId(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SubWindowAt(
        const IntroductionWindowState& state,
        std::uint32_t window_slot_address) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual bool HasTrigger(
        std::uint32_t database_member,
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerRecord(
        std::uint32_t database_member,
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::string_view TriggerText(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string_view TriggerDescription(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CeguiNpos() = 0;
    [[nodiscard]] virtual std::uint32_t BuildCeguiString(
        std::string_view utf8, std::uint32_t length) = 0;
    virtual void SetText(std::uint32_t window,
                         std::uint32_t cegui_string) = 0;
    virtual void DestroyCeguiString(std::uint32_t cegui_string) = 0;
    virtual void SetWindowUserId(std::uint32_t window,
                                 std::uint32_t id) = 0;
    virtual void SelectFirstSub(IntroductionWindowState& state,
                                std::uint32_t window) = 0;
};

class IntroductionWindowPanelRuntimeStub :
    public IIntroductionWindowPanelRuntime {
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
    [[nodiscard]] std::int32_t WindowUserId(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t SubWindowAt(
        const IntroductionWindowState&, std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t GameDatabase() override { return 0u; }
    [[nodiscard]] bool HasTrigger(
        std::uint32_t, std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t TriggerRecord(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::string_view TriggerText(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::string_view TriggerDescription(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view text) override { return std::string(text); }
    [[nodiscard]] std::uint32_t CeguiNpos() override { return 0xffffffffu; }
    [[nodiscard]] std::uint32_t BuildCeguiString(
        std::string_view, std::uint32_t) override { return 0u; }
    void SetText(std::uint32_t, std::uint32_t) override {}
    void DestroyCeguiString(std::uint32_t) override {}
    void SetWindowUserId(std::uint32_t, std::uint32_t) override {}
    void SelectFirstSub(IntroductionWindowState&,
                        std::uint32_t) override {}
};

class IntroductionWindowPanelText final {
public:
    static void ShowMainGroup(
        IntroductionWindowState& state, std::uint32_t object_address,
        std::uint32_t clicked_window,
        IIntroductionWindowPanelRuntime& runtime);
    static void ShowSubSelection(
        IntroductionWindowState& state, std::uint32_t clicked_window,
        IIntroductionWindowPanelRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnMainButtonEvent(
        IntroductionWindowState& state, std::uint32_t object_address,
        std::uint32_t event_address,
        IIntroductionWindowPanelRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSubButtonEvent(
        IntroductionWindowState& state, std::uint32_t event_address,
        IIntroductionWindowPanelRuntime& runtime);
};

}  // namespace pal4
