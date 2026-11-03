#pragma once

#include <cstdint>

#include "pal4/save_window_lifecycle.h"

namespace pal4 {

class ISaveWindowNavigationRuntime {
public:
    virtual ~ISaveWindowNavigationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructBaseEvent() = 0;
    virtual void DestroyBaseEvent(std::uint32_t event) = 0;
    virtual void Close(SaveWindowState& state,
                       std::uint32_t object_address,
                       std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    virtual void UpHoverItem(std::uint32_t list) = 0;
    virtual void DownHoverItem(std::uint32_t list) = 0;
    virtual void PopulateSaveList(SaveWindowState& state,
                                  std::int32_t page,
                                  std::uint8_t reset) = 0;
    virtual void DisplayHoverItemInfo(SaveWindowState& state) = 0;
};

class SaveWindowNavigationRuntimeStub : public ISaveWindowNavigationRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructBaseEvent() override { return 0u; }
    void DestroyBaseEvent(std::uint32_t) override {}
    void Close(SaveWindowState&, std::uint32_t,
               std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RowCount(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t HoverItem(std::uint32_t) override { return 0u; }
    void UpHoverItem(std::uint32_t) override {}
    void DownHoverItem(std::uint32_t) override {}
    void PopulateSaveList(SaveWindowState&, std::int32_t,
                          std::uint8_t) override {}
    void DisplayHoverItemInfo(SaveWindowState&) override {}
};

class SaveWindowNavigation final {
public:
    [[nodiscard]] static std::uint8_t OnMemberEvent(
        SaveWindowState& state, std::uint32_t member_address,
        std::uint32_t event_address,
        ISaveWindowNavigationRuntime& runtime);
};

}  // namespace pal4
