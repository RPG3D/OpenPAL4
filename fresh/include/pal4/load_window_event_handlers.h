#pragma once

#include "pal4/load_window_save_list.h"

#include <cstdint>

namespace pal4 {

class ILoadWindowEventRuntime {
public:
    virtual ~ILoadWindowEventRuntime() = default;
    [[nodiscard]] virtual ILoadWindowSaveListRuntime& SaveListRuntime() = 0;
    [[nodiscard]] virtual std::uint32_t EventWindow(
        std::uint32_t event_member) = 0;
    [[nodiscard]] virtual std::uint32_t EventCode(
        std::uint32_t event_member) = 0;
    virtual void DispatchBaseEvent(LoadWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ItemWord(
        std::uint32_t item, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    virtual void InitializeSaveLoadCallback(
        std::uint32_t callback, LoadWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t CreateCommonDialog() = 0;
    virtual void ShowDialog(std::uint32_t dialog, std::uint32_t text,
                            float width, float height, std::uint32_t mode,
                            std::uint32_t& callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
    [[nodiscard]] virtual std::int32_t HoverRowIndex(
        std::uint32_t list) = 0;
    virtual void UpHoverItem(std::uint32_t list) = 0;
    virtual void DownHoverItem(std::uint32_t list) = 0;
};

class LoadWindowEventRuntimeStub :
    public LoadWindowSaveListRuntimeStub,
    public ILoadWindowEventRuntime {
public:
    [[nodiscard]] ILoadWindowSaveListRuntime& SaveListRuntime() override {
        return *this;
    }
    [[nodiscard]] std::uint32_t EventWindow(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EventCode(
        std::uint32_t) override { return 0u; }
    void DispatchBaseEvent(LoadWindowState&) override {}
    [[nodiscard]] std::uint32_t HoverItem(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ItemWord(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::uint32_t) override { return 0u; }
    void InitializeSaveLoadCallback(
        std::uint32_t, LoadWindowState&) override {}
    [[nodiscard]] std::uint32_t TriggerText(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EmptyText() override { return 0u; }
    [[nodiscard]] std::uint32_t CreateCommonDialog() override { return 0u; }
    void ShowDialog(std::uint32_t, std::uint32_t, float, float,
                    std::uint32_t, std::uint32_t&) override {}
    void ReleaseCallback(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::int32_t HoverRowIndex(
        std::uint32_t) override { return 0; }
    void UpHoverItem(std::uint32_t) override {}
    void DownHoverItem(std::uint32_t) override {}
};

class LoadWindowEventHandlers final {
public:
    [[nodiscard]] static std::uint8_t OnPageButton(
        LoadWindowState& state, std::uint32_t event_address,
        ILoadWindowEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnMouseClick(
        LoadWindowState& state, ILoadWindowEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnItemClick(
        LoadWindowState& state, ILoadWindowEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnHoverRowChanged(
        LoadWindowState& state, ILoadWindowEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleListEvent(
        LoadWindowState& state, std::uint32_t event_address,
        ILoadWindowEventRuntime& runtime);
};

}  // namespace pal4
