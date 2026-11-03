#include "pal4/load_window_save_list.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::LoadWindowSaveListRuntimeStub {
    std::int32_t save_count = 34;
    std::int32_t failed_construct_slot = -1;
    std::int32_t null_allocation_slot = -1;
    std::int32_t current_slot{};
    std::uint32_t parent = 0x9000u;
    std::uint32_t global_calls{};
    std::uint32_t rows{};
    std::uint32_t copied_text_source{};
    std::uint32_t mask_user_id{};
    std::uint32_t hover_row = 0xffffffffu;
    std::uint32_t redraws{};
    std::uint32_t scrollbar_updates{};
    float scroll_position{};
    std::vector<std::int32_t> loaded_slots;
    std::vector<std::int32_t> described_slots;
    std::vector<std::int32_t> constructed_slots;
    std::vector<std::uint32_t> constructed_descriptions;
    std::vector<std::uint32_t> row_items;
    std::vector<std::uint32_t> freed_allocations;
    std::vector<std::uint32_t> freed_text;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> decremented_text;
    std::vector<std::string> calls;

    std::uint32_t ScrollbarOf(std::uint32_t list) override {
        Check(list == 0x5400u, "scrollbar belongs to list +0x54");
        calls.emplace_back("scrollbar");
        return 0x7000u;
    }
    float ScrollbarPosition(std::uint32_t scrollbar) override {
        Check(scrollbar == 0x7000u,
              "item height source is scrollbar +0x4b4");
        calls.emplace_back("size-source");
        return 80.0f;
    }
    std::uint32_t GlobalData() override {
        ++global_calls;
        calls.emplace_back("global");
        return 0x8000u;
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        Check(address == 0x802cu,
              "global save count is read at +0x2c");
        calls.emplace_back("save-count");
        return save_count;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x8030u,
              "saved scrollbar position is read at +0x30");
        calls.emplace_back("saved-scroll");
        return 42.5f;
    }
    std::uint32_t ParentOf(std::uint32_t mask) override {
        Check(mask == 0x5c00u,
              "selection mask parent is inspected at +0x44");
        calls.emplace_back("parent");
        return parent;
    }
    void RemoveChild(std::uint32_t owner,
                     std::uint32_t mask) override {
        Check(owner == 0x9000u && mask == 0x5c00u,
              "selection mask is detached from previous owner");
        calls.emplace_back("remove-child");
    }
    void AddChild(std::uint32_t owner,
                  std::uint32_t mask) override {
        Check(owner == 0x2c03u && mask == 0x5c00u,
              "selection mask moves to page-three button");
        calls.emplace_back("add-child");
    }
    void SetClippedByParent(std::uint32_t mask,
                            bool clipped) override {
        Check(mask == 0x5c00u && !clipped,
              "selection mask is not clipped by its parent");
        calls.emplace_back("clip");
    }
    void SetXPosition(std::uint32_t mask,
                      std::uint32_t mode, float value) override {
        Check(mask == 0x5c00u && mode == 1u && value == -30.0f,
              "selection mask X position is absolute minus thirty");
        calls.emplace_back("x");
    }
    void SetYPosition(std::uint32_t mask,
                      std::uint32_t mode, float value) override {
        Check(mask == 0x5c00u && mode == 1u && value == -25.0f,
              "selection mask Y position is absolute minus twenty-five");
        calls.emplace_back("y");
    }
    void CopyWindowText(std::uint32_t destination,
                        std::uint32_t source,
                        std::uint32_t offset) override {
        Check(destination == 0x6000u && offset == 0x4cu,
              "static mask copies source window CEGUI text member");
        copied_text_source = source;
        calls.emplace_back("copy-text");
    }
    std::uint32_t WindowUserId(std::uint32_t window) override {
        Check(window == 0x2c03u,
              "page number is read from selected button +0x118");
        calls.emplace_back("button-id");
        return 3u;
    }
    void SetWindowUserId(std::uint32_t window,
                         std::uint32_t id) override {
        Check(window == 0x5c00u,
              "selection mask receives selected button ID");
        mask_user_id = id;
        calls.emplace_back("mask-id");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x5400u, "list resets before slot scan");
        calls.emplace_back("reset-list");
    }
    bool LoadSaveFile(std::int32_t slot, std::uint32_t mode,
                      pal4::SaveSlotData& data,
                      pal4::SaveSlotExtras& extras) override {
        Check(mode == 1u && slot >= 30 && slot < 40 &&
                  (extras.flags == 0u || extras.flags == 1u) &&
                  extras.text == 0u && extras.reserved1 == 0u &&
                  extras.reserved2 == 0u,
              "save reader receives slot, mode and fresh extras");
        current_slot = slot;
        loaded_slots.push_back(slot);
        data.words[12] = 0xabcu;
        extras.text = 0xa001u +
                      static_cast<std::uint32_t>(slot - 30) * 0x10u;
        calls.emplace_back("load-save");
        return (slot & 1) == 0;
    }
    std::uint32_t Allocate(std::size_t bytes) override {
        Check(bytes == 0x208u,
              "numbered item allocation is 0x208 bytes");
        calls.emplace_back("allocate");
        if (current_slot == null_allocation_slot) return 0u;
        return 0xb000u + static_cast<std::uint32_t>(current_slot);
    }
    std::uint32_t DescribeSlot(std::int32_t slot) override {
        Check(slot == current_slot && (slot & 1) == 0,
              "description is queried only for present saves");
        described_slots.push_back(slot);
        calls.emplace_back("describe");
        return 0xc000u + static_cast<std::uint32_t>(slot);
    }
    std::uint32_t ConstructItem(std::uint32_t storage,
                                std::int32_t slot,
                                pal4::SaveSlotData& data,
                                std::uint32_t description,
                                std::uint32_t mode,
                                pal4::SaveSlotExtras& extras) override {
        Check(storage == 0xb000u + static_cast<std::uint32_t>(slot) &&
                  mode == 1u && extras.text != 0u,
              "item constructor receives allocated storage and extras");
        if ((slot & 1) == 0) {
            Check(description == 0xc000u +
                      static_cast<std::uint32_t>(slot),
                  "present save receives description");
        } else {
            Check(description == 0u && data.words[12] == 0u,
                  "absent save clears word twelve and description");
        }
        constructed_slots.push_back(slot);
        constructed_descriptions.push_back(description);
        calls.emplace_back("construct-item");
        if (slot == failed_construct_slot) {
            throw std::runtime_error("construct-item");
        }
        return storage + 0x1000u;
    }
    void FreeAllocation(std::uint32_t storage) override {
        freed_allocations.push_back(storage);
        calls.emplace_back("free-allocation");
    }
    void SetItemSize(std::uint32_t item, float size) override {
        Check(size == 20.0f,
              "numbered item size is one quarter of scrollbar metric");
        if (current_slot == null_allocation_slot) {
            Check(item == 0u, "zero allocation reaches SetItemSize");
        }
        calls.emplace_back("size-item");
    }
    void AddRow(std::uint32_t list, std::uint32_t id) override {
        Check(list == 0x5400u && id == 0u,
              "row is added to first column with ID zero");
        ++rows;
        calls.emplace_back("add-row");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5400u, "row count belongs to save list");
        calls.emplace_back("row-count");
        return rows;
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column,
                 std::uint32_t row) override {
        Check(list == 0x5400u && column == 0u &&
                  row == rows - 1u,
              "item is placed in the newly added row, column zero");
        row_items.push_back(item);
        calls.emplace_back("set-item");
    }
    std::uint8_t ReadTextPrefixByte(
        std::uint32_t address) override {
        Check(address >= 0xa000u,
              "temporary text refcount precedes text pointer");
        const auto slot = static_cast<std::int32_t>(
            30u + (address - 0xa000u) / 0x10u);
        calls.emplace_back("read-text-count");
        if (slot % 3 == 0) return 0u;
        if (slot % 3 == 1) return 0xffu;
        return 2u;
    }
    void WriteTextPrefixByte(std::uint32_t address,
                             std::uint8_t value) override {
        Check(value == 1u,
              "shared temporary text reference is decremented");
        decremented_text.emplace_back(address, value);
        calls.emplace_back("decrement-text");
    }
    void FreeText(std::uint32_t address) override {
        freed_text.push_back(address);
        calls.emplace_back("free-text");
    }
    void SetHoverRowIndex(std::uint32_t list,
                          std::int32_t row) override {
        Check(list == 0x5400u,
              "hover selection targets save list");
        hover_row = static_cast<std::uint32_t>(row);
        calls.emplace_back("hover");
    }
    void SetScrollPosition(std::uint32_t scrollbar,
                           float position) override {
        Check(scrollbar == 0x7000u,
              "saved position is restored to list scrollbar");
        ++scrollbar_updates;
        scroll_position = position;
        calls.emplace_back("set-scroll");
    }
    void RequestRedraw(std::uint32_t list) override {
        Check(list == 0x5400u, "redraw targets save list");
        ++redraws;
        calls.emplace_back("redraw");
    }
};

pal4::LoadWindowState MakeState() {
    pal4::LoadWindowState state{};
    state.Write(0x54u, 0x5400u);
    state.Write(0x5cu, 0x5c00u);
    state.Write(0x60u, 0x6000u);
    state.Write(0x2cu + 3u * 4u, 0x2c03u);
    return state;
}

void CheckRefresh() {
    auto state = MakeState();
    Runtime runtime;
    pal4::LoadWindowSaveList::Populate(state, 3, 1u, runtime);
    Check(runtime.global_calls == 2u &&
              runtime.copied_text_source == 0x2c03u &&
              runtime.mask_user_id == 3u &&
              runtime.loaded_slots.size() == 10u &&
              runtime.described_slots.size() == 5u &&
              runtime.constructed_slots.size() == 10u &&
              runtime.row_items.size() == 10u &&
              runtime.rows == 10u &&
              runtime.freed_text.size() == 7u &&
              runtime.decremented_text.size() == 3u &&
              runtime.hover_row == 4u &&
              runtime.scrollbar_updates == 1u &&
              runtime.scroll_position == 42.5f &&
              runtime.redraws == 1u,
          "refresh scans ten slots and restores hover and scrollbar");
    for (std::int32_t index = 0; index < 10; ++index) {
        Check(runtime.loaded_slots[index] == 30 + index &&
                  runtime.constructed_slots[index] == 30 + index &&
                  runtime.row_items[index] != 0u,
              "save slots and rows advance in address order");
    }
    Check(runtime.calls[0] == "scrollbar" &&
              runtime.calls[1] == "size-source" &&
              runtime.calls[2] == "global" &&
              runtime.calls[3] == "save-count" &&
              runtime.calls[4] == "global" &&
              runtime.calls[5] == "saved-scroll" &&
              runtime.calls[6] == "parent" &&
              runtime.calls[7] == "remove-child" &&
              runtime.calls[8] == "add-child",
          "save count and scrollbar are read before moving mask");
}

void CheckFallbacks() {
    auto state = MakeState();
    Runtime runtime;
    runtime.parent = 0u;
    runtime.null_allocation_slot = 32;
    pal4::LoadWindowSaveList::Populate(state, 3, 0u, runtime);
    Check(runtime.rows == 10u && runtime.row_items[2] == 0u &&
              runtime.described_slots.size() == 4u &&
              runtime.hover_row == 0u &&
              runtime.scrollbar_updates == 0u &&
              runtime.redraws == 0u,
          "null item and non-refresh path still fill ten rows");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "remove-child") == runtime.calls.end(),
          "mask with no previous parent is not detached");

    runtime = Runtime{};
    runtime.save_count = -1;
    pal4::LoadWindowSaveList::Populate(state, 3, 1u, runtime);
    Check(runtime.hover_row == 0u && runtime.redraws == 0u,
          "negative signed remainder fails unsigned row-count check");
}

void CheckConstructorUnwind() {
    auto state = MakeState();
    Runtime runtime;
    runtime.failed_construct_slot = 31;
    try {
        pal4::LoadWindowSaveList::Populate(state, 3, 1u, runtime);
        Check(false, "numbered item construction must propagate failure");
    } catch (const std::runtime_error&) {
        Check(runtime.loaded_slots.size() == 2u &&
                  runtime.freed_allocations ==
                      std::vector<std::uint32_t>{0xb01fu} &&
                  runtime.freed_text.size() == 2u &&
                  runtime.rows == 1u,
              "failed item construction frees storage and temporary text");
    }
}

}  // namespace

int main() {
    CheckRefresh();
    CheckFallbacks();
    CheckConstructorUnwind();
    std::cout << "Load window save list tests passed\n";
}
