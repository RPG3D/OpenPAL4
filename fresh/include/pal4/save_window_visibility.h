#pragma once

#include <cstdint>

#include "pal4/save_window_lifecycle.h"

namespace pal4 {

class ISaveWindowVisibilityRuntime {
public:
    virtual ~ISaveWindowVisibilityRuntime() = default;
    virtual void SetParent(SaveWindowState& state, std::uint8_t showing,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::int32_t GlobalSaveIndex() = 0;
    virtual void PopulateSaveList(SaveWindowState& state,
                                  std::int32_t page, bool reset) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentState() = 0;
    [[nodiscard]] virtual std::uint32_t StateId(std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void CaptureActiveCameraRaster(
        std::uint32_t manager) = 0;
    virtual void NotifyPropertyReturn(std::uint32_t event_tree,
                                    std::uint32_t first,
                                    std::uint32_t second) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t list, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActive(
        std::uint32_t list, std::uint32_t member) = 0;
};

class SaveWindowVisibilityRuntimeStub : public ISaveWindowVisibilityRuntime {
public:
    void SetParent(SaveWindowState&, std::uint8_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::int32_t GlobalSaveIndex() override { return 0; }
    void PopulateSaveList(SaveWindowState&, std::int32_t,
                          bool) override {}
    [[nodiscard]] std::uint32_t CurrentState() override { return 0u; }
    [[nodiscard]] std::uint32_t StateId(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void CaptureActiveCameraRaster(std::uint32_t) override {}
    void NotifyPropertyReturn(std::uint32_t, std::uint32_t,
                            std::uint32_t) override {}
    void SetAlwaysOnTop(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class SaveWindowVisibility final {
public:
    [[nodiscard]] static std::uint32_t SetVisible(
        SaveWindowState& state, std::uint32_t object_address,
        std::uint8_t showing, std::uint32_t parent,
        ISaveWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
