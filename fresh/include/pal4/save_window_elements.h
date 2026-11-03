#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/save_window_lifecycle.h"

namespace pal4 {

class ISaveWindowElementsRuntime {
public:
    virtual ~ISaveWindowElementsRuntime() = default;
    [[nodiscard]] virtual std::string ReadPrefix(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(std::string_view name) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(std::size_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::string_view event,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteReference(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::string TriggerItemText(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view raw) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetHorizontalFormatting(std::uint32_t window,
                                         std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(
        std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list,
                              std::uint32_t index) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void SetUserColumnSizingEnabled(std::uint32_t list,
                                             bool enabled) = 0;
    virtual void SetUserColumnDraggingEnabled(std::uint32_t list,
                                               bool enabled) = 0;
    virtual void SetUserSortControlEnabled(std::uint32_t list,
                                            bool enabled) = 0;
    virtual void AddColumn(std::uint32_t list, std::u32string_view title,
                           std::uint32_t id, float width) = 0;
};

class SaveWindowElementsRuntimeStub : public ISaveWindowElementsRuntime {
public:
    [[nodiscard]] std::string ReadPrefix(std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t FindWindow(std::string_view) override {
        return 0u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateCallback(std::size_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t Subscribe(
        std::uint32_t, std::string_view, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(std::uint32_t) override {
        return 0u;
    }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string TriggerItemText(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view raw) override { return std::string(raw); }
    void SetText(std::uint32_t, std::u32string_view) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetHorizontalFormatting(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ColumnCount(std::uint32_t) override {
        return 0u;
    }
    void RemoveColumn(std::uint32_t, std::uint32_t) override {}
    void SetSortDirection(std::uint32_t, std::uint32_t) override {}
    void SetUserColumnSizingEnabled(std::uint32_t, bool) override {}
    void SetUserColumnDraggingEnabled(std::uint32_t, bool) override {}
    void SetUserSortControlEnabled(std::uint32_t, bool) override {}
    void AddColumn(std::uint32_t, std::u32string_view,
                   std::uint32_t, float) override {}
};

class SaveWindowElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x8437b0u;
    static constexpr std::uint32_t kSaveClickHandler = 0x44d650u;
    static constexpr std::uint32_t kCloseHandler = 0x4b6890u;
    static constexpr std::uint32_t kListClickHandler = 0x44d670u;
    static constexpr std::uint32_t kHoverHandler = 0x447d60u;

    static void Initialize(SaveWindowState& state,
                           std::uint32_t object_address,
                           ISaveWindowElementsRuntime& runtime);
};

}  // namespace pal4
