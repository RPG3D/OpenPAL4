#include "pal4/save_window_page.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::SaveWindowPageRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::int32_t> loaded_slots;
    std::vector<std::int32_t> auxiliary_slots;
    std::vector<std::int32_t> constructed_slots;
    std::vector<std::uint32_t> row_items;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::int32_t save_index{23};
    std::int32_t fail_slot{-1};
    std::int32_t null_item_slot{-1};
    std::uint32_t rows{};
    std::uint32_t released{};
    std::uint32_t hover_row{0xffffffffu};
    float scroll_position{-1.0f};
    bool redrawn{};
    bool reset_flag{};

    float ScrollbarExtent(std::uint32_t list) override {
        Check(list == 0x5000u, "scrollbar belongs to List");
        calls.emplace_back("extent");
        return 80.0f;
    }
    std::int32_t GlobalSaveIndex() override {
        calls.emplace_back("global-index");
        return save_index;
    }
    float GlobalScrollPosition() override {
        calls.emplace_back("global-scroll");
        return 0.75f;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x6118u, "mask receives page index");
        words[address] = value;
        calls.emplace_back("mask-index");
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        Check(parent == 0x7000u && child == 0x6000u,
              "mask removed from previous parent");
        calls.emplace_back("remove-parent");
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        Check(parent == 0x2300u && child == 0x6000u,
              "mask attached to selected page button");
        calls.emplace_back("add-parent");
    }
    void SetClippedByParent(std::uint32_t window,
                            bool clipped) override {
        Check(window == 0x6000u && !clipped, "mask clipping disabled");
        calls.emplace_back("clip");
    }
    void SetX(std::uint32_t window, std::uint32_t mode,
              float value) override {
        Check(window == 0x6000u && mode == 1u && value == -30.0f,
              "mask X offset is -30 pixels");
        calls.emplace_back("x");
    }
    void SetY(std::uint32_t window, std::uint32_t mode,
              float value) override {
        Check(window == 0x6000u && mode == 1u && value == -25.0f,
              "mask Y offset is -25 pixels");
        calls.emplace_back("y");
    }
    void CopyButtonText(std::uint32_t target,
                        std::uint32_t source) override {
        Check(target == 0x6200u && source == 0x2300u,
              "StaticMask copies selected page button text");
        calls.emplace_back("text");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x5000u, "list reset target");
        rows = 0u;
        calls.emplace_back("reset-list");
    }
    bool LoadSave(std::int32_t slot, std::uint32_t mode,
                  pal4::SaveSlotData& data,
                  pal4::SaveListExtra& extra) override {
        Check(mode == 1u && slot >= 20 && slot < 30 &&
                  (extra.words[0] == 0u || extra.words[0] == 1u),
              "save read slot, mode and reset flag");
        reset_flag = extra.words[0] != 0u;
        loaded_slots.push_back(slot);
        calls.emplace_back("load");
        if (slot == fail_slot) throw std::runtime_error("load");
        data.words[12] = 0xabcu;
        extra.words[1] = 0x9900u;
        return (slot & 1) == 0;
    }
    std::uint32_t AllocateItem(std::size_t bytes) override {
        Check(bytes == 0x208u, "numbered item is 0x208 bytes");
        calls.emplace_back("allocate");
        const auto slot = loaded_slots.back();
        if (slot == null_item_slot) return 0u;
        return 0xa000u + static_cast<std::uint32_t>(slot) * 0x20u;
    }
    std::uint32_t NumberedItemAux(std::int32_t slot) override {
        Check((slot & 1) == 0, "auxiliary lookup only for loaded saves");
        auxiliary_slots.push_back(slot);
        calls.emplace_back("aux");
        return 0xabcdu;
    }
    std::uint32_t ConstructItem(
        std::uint32_t storage, std::int32_t slot,
        const pal4::SaveSlotData& data, std::uint32_t auxiliary,
        std::uint32_t zero, const pal4::SaveListExtra& extra) override {
        Check(storage == 0xa000u + static_cast<std::uint32_t>(slot) * 0x20u &&
                  zero == 0u && extra.words[1] == 0x9900u,
              "numbered item receives storage, slot and extra data");
        Check((slot & 1) == 0 ?
                  (auxiliary == 0xabcdu && data.words[12] == 0xabcu) :
                  (auxiliary == 0u && data.words[12] == 0u),
              "loaded and missing save constructor arguments differ");
        constructed_slots.push_back(slot);
        calls.emplace_back("construct");
        return storage + 4u;
    }
    void SetItemSize(std::uint32_t item, float size) override {
        Check(size == 20.0f, "item height is one quarter scrollbar extent");
        if (loaded_slots.back() == null_item_slot)
            Check(item == 0u, "null allocation reaches item size call");
        calls.emplace_back("size");
    }
    void AddRow(std::uint32_t list, std::uint32_t value) override {
        Check(list == 0x5000u && value == 0u, "row added in first column");
        ++rows;
        calls.emplace_back("add-row");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5000u, "row count target");
        calls.emplace_back("row-count");
        return rows;
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        Check(list == 0x5000u && column == 0u && row == rows - 1u,
              "numbered item placed in newly added row");
        row_items.push_back(item);
        calls.emplace_back("set-item");
    }
    void ReleaseExtra(pal4::SaveListExtra& extra) override {
        Check(extra.words[1] == 0x9900u ||
                  loaded_slots.back() == fail_slot,
              "extra reference released after each save read");
        ++released;
        calls.emplace_back("release");
    }
    void SetHoverRow(std::uint32_t list,
                     std::uint32_t row) override {
        Check(list == 0x5000u, "hover target is List");
        hover_row = row;
        calls.emplace_back("hover");
    }
    void SetScrollPosition(std::uint32_t list, float value) override {
        Check(list == 0x5000u, "scroll target is List");
        scroll_position = value;
        calls.emplace_back("scroll");
    }
    void RequestRedraw(std::uint32_t list) override {
        Check(list == 0x5000u, "redraw target is List");
        redrawn = true;
        calls.emplace_back("redraw");
    }
};

pal4::SaveWindowState State() {
    pal4::SaveWindowState state{};
    state.Write(0x54u, 0x5000u);
    state.Write(0x5cu, 0x6000u);
    state.Write(0x60u, 0x6200u);
    state.Write(0x34u, 0x2300u);
    return state;
}

Runtime ReadyRuntime() {
    Runtime runtime;
    runtime.words[0x6044u] = 0x7000u;
    runtime.words[0x2418u] = 2u;
    return runtime;
}

void CheckPopulateAndRestore() {
    auto state = State();
    auto runtime = ReadyRuntime();
    pal4::SaveWindowPage::Populate(state, 2, 1u, runtime);
    Check(runtime.words.at(0x6118u) == 2u && runtime.rows == 10u &&
              runtime.released == 10u && runtime.loaded_slots.size() == 10u &&
              runtime.loaded_slots.front() == 20 &&
              runtime.loaded_slots.back() == 29 &&
              runtime.auxiliary_slots ==
                  std::vector<std::int32_t>{20, 22, 24, 26, 28} &&
              runtime.constructed_slots.size() == 10u,
          "page two builds ten rows and only loaded saves request auxiliary");
    Check(runtime.hover_row == 3u && runtime.scroll_position == 0.75f &&
              runtime.redrawn && runtime.reset_flag &&
              runtime.calls[0] == "extent" &&
              runtime.calls[1] == "global-index" &&
              runtime.calls[2] == "global-scroll" &&
              runtime.calls[3] == "read:24644" &&
              runtime.calls[4] == "remove-parent" &&
              runtime.calls[5] == "add-parent" &&
              runtime.calls[6] == "clip" &&
              runtime.calls[7] == "x" &&
              runtime.calls[8] == "y" &&
              runtime.calls[9] == "text" &&
              runtime.calls.back() == "redraw",
          "mask and global state precede row construction and scroll restore");
}

void CheckNoRestoreAndNullItem() {
    auto state = State();
    auto runtime = ReadyRuntime();
    runtime.null_item_slot = 23;
    pal4::SaveWindowPage::Populate(state, 2, 0u, runtime);
    Check(runtime.rows == 10u && runtime.row_items[3] == 0u &&
              runtime.constructed_slots.size() == 9u &&
              runtime.hover_row == 0u && runtime.scroll_position == -1.0f &&
              !runtime.redrawn && !runtime.reset_flag,
          "non-reset page selects first row and retains null item behavior");
    runtime = ReadyRuntime();
    runtime.save_index = -1;
    pal4::SaveWindowPage::Populate(state, 2, 1u, runtime);
    Check(runtime.hover_row == 0u && !runtime.redrawn,
          "negative global slot remainder falls back to first row");
}

void CheckLoadFailure() {
    auto state = State();
    auto runtime = ReadyRuntime();
    runtime.fail_slot = 22;
    try {
        pal4::SaveWindowPage::Populate(state, 2, 1u, runtime);
        Check(false, "save read failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.loaded_slots ==
                  std::vector<std::int32_t>{20, 21, 22} &&
                  runtime.rows == 2u && runtime.released == 3u &&
                  runtime.calls.back() == "release",
              "load failure releases current extra and stops row creation");
    }
}

}  // namespace

int main() {
    CheckPopulateAndRestore();
    CheckNoRestoreAndNullItem();
    CheckLoadFailure();
    std::cout << "Save window page tests passed\n";
}
