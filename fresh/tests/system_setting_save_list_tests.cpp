#include "pal4/system_setting_save_list.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingSaveListRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::int32_t> loaded_slots;
    std::vector<std::tuple<std::int32_t, std::uint32_t, bool,
                           std::uint32_t>> constructed;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> items;
    std::vector<std::uint32_t> sizes;
    std::vector<std::uint32_t> hover_rows;
    std::vector<float> scroll_positions;
    std::uint32_t columns = 2u;
    std::uint32_t rows = 0u;
    std::uint32_t global_count = 0u;
    std::uint32_t released = 0u;
    std::uint32_t redraws = 0u;
    std::uint32_t removed_parent = 0u;
    std::uint32_t added_parent = 0u;
    std::uint32_t x_bits = 0u;
    std::uint32_t y_bits = 0u;
    std::uint32_t text_source = 0u;
    bool clipped = true;
    bool fail_construct = false;
    bool allocate_null = false;

    Runtime() {
        words[0x4000u + 104u] = 0x5000u;
        words[0x5000u + 1204u] = 0x6000u;
        words[0x6000u + 1204u] = std::bit_cast<std::uint32_t>(80.0f);
        words[0x7000u + 44u] = 23u;
        words[0x7000u + 48u] = std::bit_cast<std::uint32_t>(0.625f);
        words[0x4000u + 504u] = 2u;
        words[0x4000u + 176u] = 0x8000u;
        words[0x8000u + 68u] = 0x9000u;
        words[0x4000u + 136u + 8u] = 0xa000u;
        words[0x4000u + 108u] = 0xb000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t GlobalData() override { ++global_count; return 0x7000u; }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x5000u, "reset list");
        rows = 0u;
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x5000u, "column count list");
        return columns;
    }
    void RemoveColumn(std::uint32_t list, std::uint32_t index) override {
        Check(list == 0x5000u && index == 0u && columns > 0u,
              "remove first column");
        --columns;
    }
    void AddColumn(std::uint32_t list, std::uint32_t id,
                   float width) override {
        Check(list == 0x5000u && id == 0u && width == 1.0f &&
              columns == 0u, "add single full-width column");
        columns = 1u;
    }
    bool LoadSaveFile(std::int32_t slot, std::uint32_t mode,
                      pal4::SaveDataWords& data,
                      pal4::SaveExtraWords& extra) override {
        Check(mode == 1u, "load mode");
        loaded_slots.push_back(slot);
        data[12] = 99u;
        extra[0] = static_cast<std::uint32_t>(slot);
        return slot % 2 == 0;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x208u, "item allocation size");
        return allocate_null ? 0u : 0xc000u +
            static_cast<std::uint32_t>(loaded_slots.size()) * 0x300u;
    }
    std::uint32_t SlotMetadata(std::int32_t slot) override {
        Check(slot % 2 == 0, "metadata only for loaded slot");
        return static_cast<std::uint32_t>(slot + 100);
    }
    std::uint32_t ConstructItem(
        std::uint32_t allocation, std::int32_t slot,
        const pal4::SaveDataWords& data, std::uint32_t metadata,
        bool option_two, const pal4::SaveExtraWords& extra) override {
        Check(allocation != 0u &&
              extra[0] == static_cast<std::uint32_t>(slot),
              "constructor allocation and extra data");
        if (fail_construct) throw std::runtime_error("constructor");
        constructed.emplace_back(slot, metadata, option_two, data[12]);
        return allocation;
    }
    void SetItemSize(std::uint32_t item, float size) override {
        Check(size == 20.0f, "item size is quarter of scrollbar metric");
        sizes.push_back(item);
    }
    void AddRow(std::uint32_t list, std::uint32_t id) override {
        Check(list == 0x5000u && id == 0u, "add row arguments");
        ++rows;
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5000u, "row count list");
        return rows;
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        items.emplace_back(item, column, row);
        Check(list == 0x5000u && column == 0u && row + 1u == rows,
              "set item at newly added row");
    }
    void ReleaseExtra(pal4::SaveExtraWords& extra) override {
        ++released;
        extra.fill(0u);
    }
    void SetHoverRow(std::uint32_t list,
                     std::uint32_t row) override {
        Check(list == 0x5000u, "hover list");
        hover_rows.push_back(row);
    }
    void SetScrollPosition(std::uint32_t scrollbar,
                           float position) override {
        Check(scrollbar == 0x6000u, "scrollbar address");
        scroll_positions.push_back(position);
    }
    void RequestRedraw(std::uint32_t list) override {
        Check(list == 0x5000u, "redraw list");
        ++redraws;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        Check(child == 0x8000u, "remove mask");
        removed_parent = parent;
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        Check(child == 0x8000u, "add mask");
        added_parent = parent;
    }
    void SetClippedByParent(std::uint32_t child,
                            bool value) override {
        Check(child == 0x8000u, "clip mask");
        clipped = value;
    }
    void SetXPosition(std::uint32_t window,
                      std::uint32_t scale,
                      std::uint32_t offset_bits) override {
        Check(window == 0x8000u && scale == 1u, "mask x position");
        x_bits = offset_bits;
    }
    void SetYPosition(std::uint32_t window,
                      std::uint32_t scale,
                      std::uint32_t offset_bits) override {
        Check(window == 0x8000u && scale == 1u, "mask y position");
        y_bits = offset_bits;
    }
    void CopyText(std::uint32_t target,
                  std::uint32_t source) override {
        Check(target == 0xb000u, "mask label");
        text_source = source;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::SystemSettingSaveList::Populate(0x4000u, 2, 1u, runtime);
    Check(runtime.global_count == 2u && runtime.columns == 1u &&
          runtime.rows == 10u && runtime.released == 10u &&
          runtime.redraws == 1u, "ten rows and two global lookups");
    for (std::size_t i = 0; i < 10u; ++i) {
        const auto slot = static_cast<std::int32_t>(20 + i);
        Check(runtime.loaded_slots[i] == slot &&
              std::get<0>(runtime.constructed[i]) == slot &&
              std::get<1>(runtime.constructed[i]) ==
                  static_cast<std::uint32_t>(slot % 2 == 0 ? slot + 100 : 0) &&
              std::get<2>(runtime.constructed[i]) &&
              std::get<3>(runtime.constructed[i]) ==
                  static_cast<std::uint32_t>(slot % 2 == 0 ? 99 : 0) &&
              std::get<2>(runtime.items[i]) == i,
              "slot load, metadata, empty marker and row index");
    }
    Check(runtime.hover_rows == std::vector<std::uint32_t>{3u} &&
          runtime.scroll_positions == std::vector<float>{0.625f},
          "current slot hover and scroll restoration");
    Check(runtime.removed_parent == 0x9000u &&
          runtime.added_parent == 0xa000u &&
          runtime.words.at(0x8000u + 280u) == 2u &&
          !runtime.clipped &&
          runtime.x_bits == static_cast<std::uint32_t>(-1041235968) &&
          runtime.y_bits == static_cast<std::uint32_t>(-1043857408) &&
          runtime.text_source == 0xa000u, "mask reparenting and geometry");

    Runtime no_selection;
    no_selection.words[0x4000u + 504u] = 1u;
    no_selection.words[0x8000u + 68u] = 0u;
    no_selection.allocate_null = true;
    pal4::SystemSettingSaveList::Populate(0x4000u, 2, 0u, no_selection);
    Check(no_selection.hover_rows == std::vector<std::uint32_t>{0u} &&
          no_selection.scroll_positions.empty() &&
          no_selection.removed_parent == 0u &&
          no_selection.constructed.empty() &&
          no_selection.sizes == std::vector<std::uint32_t>(10u, 0u),
          "option one, absent parent and null item allocation");

    Runtime failure;
    failure.fail_construct = true;
    bool caught = false;
    try {
        pal4::SystemSettingSaveList::Populate(0x4000u, 2, 1u, failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.released == 1u && failure.rows == 0u,
          "row exception releases transient save parameters");
}
