#pragma once

#include <cstdint>

#include "pal4/save_window_lifecycle.h"

namespace pal4 {

class ISaveWindowSaveActionRuntime {
public:
    virtual ~ISaveWindowSaveActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t ReadSlot(std::uint32_t item) = 0;
    [[nodiscard]] virtual float ScrollPosition(std::uint32_t list) = 0;
    virtual void SetGlobalSaveIndex(std::int32_t slot) = 0;
    virtual void SetGlobalScrollPosition(float position) = 0;
    virtual void SaveSettings() = 0;
    virtual void SaveGameSlot(std::int32_t slot) = 0;
    virtual void PopulateSaveList(SaveWindowState& state,
                                  std::int32_t page,
                                  std::uint8_t reset) = 0;
};

class SaveWindowSaveActionRuntimeStub :
    public ISaveWindowSaveActionRuntime {
public:
    [[nodiscard]] std::uint32_t HoverItem(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::int32_t ReadSlot(std::uint32_t) override { return 0; }
    [[nodiscard]] float ScrollPosition(std::uint32_t) override { return 0.0f; }
    void SetGlobalSaveIndex(std::int32_t) override {}
    void SetGlobalScrollPosition(float) override {}
    void SaveSettings() override {}
    void SaveGameSlot(std::int32_t) override {}
    void PopulateSaveList(SaveWindowState&, std::int32_t,
                          std::uint8_t) override {}
};

class SaveWindowSaveAction final {
public:
    [[nodiscard]] static std::uint8_t Confirm(
        SaveWindowState& state, std::int32_t response,
        ISaveWindowSaveActionRuntime& runtime);
};

}  // namespace pal4
