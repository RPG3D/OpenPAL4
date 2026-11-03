#pragma once

#include <cstdint>

#include "pal4/save_window_lifecycle.h"

namespace pal4 {

class ISaveWindowListEventRuntime {
public:
    virtual ~ISaveWindowListEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void PopulateSaveList(SaveWindowState& state,
                                  std::int32_t page,
                                  std::uint8_t reset) = 0;
    virtual void DisplayHoverItemInfo(SaveWindowState& state) = 0;
};

class SaveWindowListEventRuntimeStub : public ISaveWindowListEventRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void PopulateSaveList(SaveWindowState&, std::int32_t,
                          std::uint8_t) override {}
    void DisplayHoverItemInfo(SaveWindowState&) override {}
};

class SaveWindowListEvents final {
public:
    [[nodiscard]] static std::uint8_t OnPageButtonClick(
        SaveWindowState& state, std::uint32_t event,
        ISaveWindowListEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnListClick(
        SaveWindowState& state, std::uint32_t event,
        ISaveWindowListEventRuntime& runtime);
};

}  // namespace pal4
