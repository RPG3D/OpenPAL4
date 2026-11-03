#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/ui_object_lifecycle.h"

namespace pal4 {

class IUiObjectTextRuntime {
public:
    virtual ~IUiObjectTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EventWindow(
        std::uint32_t event_args_member) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t WindowUserId(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t TriggerRecord(
        std::uint32_t database_member, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string_view TriggerText(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CeguiNpos() = 0;
    [[nodiscard]] virtual std::uint32_t BuildCeguiString(
        std::string_view utf8, std::uint32_t length) = 0;
    virtual void SetText(std::uint32_t window,
                         std::uint32_t cegui_string) = 0;
    virtual void DestroyCeguiString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual int InsertActive(
        std::uint32_t collection_address,
        std::uint32_t member_address) = 0;
};

class UiObjectTextRuntimeStub : public IUiObjectTextRuntime {
public:
    [[nodiscard]] std::uint32_t EventWindow(
        std::uint32_t) override { return 0u; }
    void SetEnabled(std::uint32_t, bool) override {}
    void SetVisible(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t WindowUserId(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GameDatabase() override { return 0u; }
    [[nodiscard]] std::uint32_t TriggerRecord(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::string_view TriggerText(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view text) override { return std::string(text); }
    [[nodiscard]] std::uint32_t CeguiNpos() override {
        return 0xffffffffu;
    }
    [[nodiscard]] std::uint32_t BuildCeguiString(
        std::string_view, std::uint32_t) override { return 0u; }
    void SetText(std::uint32_t, std::uint32_t) override {}
    void DestroyCeguiString(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] int InsertActive(std::uint32_t,
                                    std::uint32_t) override { return 0; }
};

class UiObjectTextDisplay final {
public:
    [[nodiscard]] static bool HandleClick(
        UiObjectState& state, std::uint32_t object_address,
        std::uint32_t event_args_address, IUiObjectTextRuntime& runtime);
    [[nodiscard]] static int SetTextAndVisibility(
        UiObjectState& state, std::uint32_t object_address,
        std::uint32_t clicked_window, IUiObjectTextRuntime& runtime);
};

}  // namespace pal4
