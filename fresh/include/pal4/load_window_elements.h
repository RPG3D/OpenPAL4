#pragma once

#include "pal4/load_window_lifecycle.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

enum class LoadWindowEvent : std::uint8_t {
    Clicked,
    MouseClick,
    HoverRowChanged,
};

class ILoadWindowElementsRuntime {
public:
    virtual ~ILoadWindowElementsRuntime() = default;
    virtual void InitializeNameBuffer() = 0;
    [[nodiscard]] virtual std::string_view LayoutPrefix(
        const LoadWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    virtual void SetWindowUserId(std::uint32_t window,
                                 std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::size_t bytes) = 0;
    virtual void WriteCallbackWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, LoadWindowEvent event,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteCallback(std::uint32_t reference,
                                std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t TriggerItem(
        std::uint32_t database_member, std::uint32_t item_id) = 0;
    [[nodiscard]] virtual std::string_view ItemText(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCeguiString(
        std::string_view text) = 0;
    virtual void SetText(std::uint32_t window,
                         std::uint32_t cegui_string) = 0;
    virtual void DestroyCeguiString(std::uint32_t cegui_string) = 0;
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
    virtual void AddColumn(std::uint32_t list,
                           std::uint32_t cegui_string,
                           std::uint32_t id, float width) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer() = 0;
};

class LoadWindowElementsRuntimeStub : public ILoadWindowElementsRuntime {
public:
    void InitializeNameBuffer() override {}
    [[nodiscard]] std::string_view LayoutPrefix(
        const LoadWindowState&) override { return {}; }
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view) override { return 0u; }
    void SetWindowUserId(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::size_t) override { return 0u; }
    void WriteCallbackWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Subscribe(
        std::uint32_t, LoadWindowEvent, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GameDatabase() override { return 0u; }
    [[nodiscard]] std::uint32_t TriggerItem(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::string_view ItemText(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view text) override { return std::string(text); }
    [[nodiscard]] std::uint32_t CreateCeguiString(
        std::string_view) override { return 0u; }
    void SetText(std::uint32_t, std::uint32_t) override {}
    void DestroyCeguiString(std::uint32_t) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetHorizontalFormatting(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ColumnCount(
        std::uint32_t) override { return 0u; }
    void RemoveColumn(std::uint32_t, std::uint32_t) override {}
    void SetSortDirection(std::uint32_t, std::uint32_t) override {}
    void SetUserColumnSizingEnabled(std::uint32_t, bool) override {}
    void SetUserColumnDraggingEnabled(std::uint32_t, bool) override {}
    void SetUserSortControlEnabled(std::uint32_t, bool) override {}
    void AddColumn(std::uint32_t, std::uint32_t,
                   std::uint32_t, float) override {}
    [[nodiscard]] std::uint32_t CleanupNameBuffer() override { return 0u; }
};

class LoadWindowElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x8435bcu;
    static constexpr std::uint32_t kPopulateHandler = 0x447d30u;
    static constexpr std::uint32_t kCloseHandler = 0x44a150u;
    static constexpr std::uint32_t kMouseClickHandler = 0x447d50u;
    static constexpr std::uint32_t kHoverHandler = 0x447d60u;

    [[nodiscard]] static std::uint32_t Initialize(
        LoadWindowState& state, std::uint32_t object_address,
        ILoadWindowElementsRuntime& runtime);
};

}  // namespace pal4
