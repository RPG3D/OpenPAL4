#pragma once

#include "pal4/load_window_lifecycle.h"
#include "pal4/save_slot_data.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct SaveSlotExtras {
    std::uint32_t flags{};
    std::uint32_t text{};
    std::uint32_t reserved1{};
    std::uint32_t reserved2{};
};

class ILoadWindowSaveListRuntime {
public:
    virtual ~ILoadWindowSaveListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ScrollbarOf(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual float ScrollbarPosition(
        std::uint32_t scrollbar) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ParentOf(
        std::uint32_t window) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    virtual void SetClippedByParent(std::uint32_t window,
                                    bool clipped) = 0;
    virtual void SetXPosition(std::uint32_t window,
                              std::uint32_t mode, float value) = 0;
    virtual void SetYPosition(std::uint32_t window,
                              std::uint32_t mode, float value) = 0;
    virtual void CopyWindowText(std::uint32_t destination,
                                std::uint32_t source,
                                std::uint32_t source_member_offset) = 0;
    [[nodiscard]] virtual std::uint32_t WindowUserId(
        std::uint32_t window) = 0;
    virtual void SetWindowUserId(std::uint32_t window,
                                 std::uint32_t id) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual bool LoadSaveFile(
        std::int32_t slot, std::uint32_t mode,
        SaveSlotData& data, SaveSlotExtras& extras) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::size_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t DescribeSlot(
        std::int32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructItem(
        std::uint32_t storage, std::int32_t slot,
        SaveSlotData& data, std::uint32_t description,
        std::uint32_t mode, SaveSlotExtras& extras) = 0;
    virtual void FreeAllocation(std::uint32_t storage) = 0;
    virtual void SetItemSize(std::uint32_t item, float size) = 0;
    virtual void AddRow(std::uint32_t list, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(
        std::uint32_t list) = 0;
    virtual void SetItem(std::uint32_t list,
                         std::uint32_t item,
                         std::uint32_t column,
                         std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint8_t ReadTextPrefixByte(
        std::uint32_t address) = 0;
    virtual void WriteTextPrefixByte(std::uint32_t address,
                                     std::uint8_t value) = 0;
    virtual void FreeText(std::uint32_t address) = 0;
    virtual void SetHoverRowIndex(std::uint32_t list,
                                  std::int32_t row) = 0;
    virtual void SetScrollPosition(std::uint32_t scrollbar,
                                   float position) = 0;
    virtual void RequestRedraw(std::uint32_t list) = 0;
};

class LoadWindowSaveListRuntimeStub : public ILoadWindowSaveListRuntime {
public:
    [[nodiscard]] std::uint32_t ScrollbarOf(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] float ScrollbarPosition(
        std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t GlobalData() override { return 0u; }
    [[nodiscard]] std::int32_t ReadSignedWord(
        std::uint32_t) override { return 0; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t ParentOf(
        std::uint32_t) override { return 0u; }
    void RemoveChild(std::uint32_t, std::uint32_t) override {}
    void AddChild(std::uint32_t, std::uint32_t) override {}
    void SetClippedByParent(std::uint32_t, bool) override {}
    void SetXPosition(std::uint32_t, std::uint32_t, float) override {}
    void SetYPosition(std::uint32_t, std::uint32_t, float) override {}
    void CopyWindowText(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    [[nodiscard]] std::uint32_t WindowUserId(
        std::uint32_t) override { return 0u; }
    void SetWindowUserId(std::uint32_t, std::uint32_t) override {}
    void ResetList(std::uint32_t) override {}
    [[nodiscard]] bool LoadSaveFile(std::int32_t,
                                    std::uint32_t, SaveSlotData&,
                                    SaveSlotExtras&) override { return false; }
    [[nodiscard]] std::uint32_t Allocate(
        std::size_t) override { return 0u; }
    [[nodiscard]] std::uint32_t DescribeSlot(
        std::int32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructItem(
        std::uint32_t storage, std::int32_t,
        SaveSlotData&, std::uint32_t,
        std::uint32_t, SaveSlotExtras&) override { return storage; }
    void FreeAllocation(std::uint32_t) override {}
    void SetItemSize(std::uint32_t, float) override {}
    void AddRow(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RowCount(
        std::uint32_t) override { return 0u; }
    void SetItem(std::uint32_t, std::uint32_t,
                 std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t ReadTextPrefixByte(
        std::uint32_t) override { return 0u; }
    void WriteTextPrefixByte(std::uint32_t,
                             std::uint8_t) override {}
    void FreeText(std::uint32_t) override {}
    void SetHoverRowIndex(std::uint32_t,
                          std::int32_t) override {}
    void SetScrollPosition(std::uint32_t, float) override {}
    void RequestRedraw(std::uint32_t) override {}
};

class LoadWindowSaveList final {
public:
    static void Populate(LoadWindowState& state, std::int32_t page,
                         std::uint32_t refresh,
                         ILoadWindowSaveListRuntime& runtime);
};

}  // namespace pal4
