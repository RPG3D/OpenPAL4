#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/save_window_lifecycle.h"

namespace pal4 {

class ISaveWindowHoverDialogRuntime {
public:
    virtual ~ISaveWindowHoverDialogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::size_t bytes) = 0;
    virtual void WriteCallbackWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string TriggerItemText(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void CreateCommonDialog(std::uint32_t manager) = 0;
    virtual void ShowDialog(std::string_view text, float width,
                            float height, std::uint32_t mode,
                            std::uint32_t callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class SaveWindowHoverDialogRuntimeStub :
    public ISaveWindowHoverDialogRuntime {
public:
    [[nodiscard]] std::uint32_t HoverItem(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t AllocateCallback(std::size_t) override {
        return 0u;
    }
    void WriteCallbackWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string TriggerItemText(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void CreateCommonDialog(std::uint32_t) override {}
    void ShowDialog(std::string_view, float, float, std::uint32_t,
                    std::uint32_t) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class SaveWindowHoverDialog final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x8437b8u;
    static constexpr std::uint32_t kSaveHandler = 0x44d9b0u;

    [[nodiscard]] static std::uint8_t Display(
        SaveWindowState& state, std::uint32_t object_address,
        ISaveWindowHoverDialogRuntime& runtime);
};

}  // namespace pal4
