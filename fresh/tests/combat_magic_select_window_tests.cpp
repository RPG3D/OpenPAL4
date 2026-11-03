#include "pal4/combat_magic_select_window.h"
#include "pal4/combat_property_select_window.h"
#include "pal4/combat_stunt_select_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kWindow = 0x4000u;
constexpr std::uint32_t kList = 0x5000u;
constexpr std::uint32_t kDescription = 0x6000u;
constexpr std::uint32_t kRoot = 0x7000u;
constexpr std::uint32_t kCallback = 0x8000u;
constexpr std::uint32_t kEvent = 0xa000u;
constexpr std::uint32_t kPair = 0xe000u;

struct Runtime final : pal4::ICombatStuntSelectWindowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::uint32_t selection_count = 0u;
    bool fail_callback = false;
    bool fail_large_callback = false;
    std::uint32_t large_callback_count = 0u;
    std::uint32_t subscription_releases = 0u;
    std::vector<std::uint32_t> button_controls;
    std::vector<std::uint32_t> callback_functions;
    std::uint32_t expected_layout = 0xb000u;
    std::uint32_t expected_group = 0xc000u;
    bool property_mode = false;
    bool stunt_mode = false;
    bool stunt_usable = true;
    std::map<std::uint32_t, std::uint32_t> stunts_by_id;
    std::vector<std::uint32_t> checked_stunt_records;
    std::map<std::uint32_t, std::string> game_strings;
    std::map<std::uint32_t, std::uint32_t> magic_by_id;
    std::map<std::uint32_t, std::uint32_t> magic_keys;
    std::map<std::uint32_t, std::uint32_t> properties_by_id;
    std::vector<std::uint32_t> inventory_records;
    std::map<std::uint32_t, std::vector<std::uint32_t>> inventory_groups;
    std::uint32_t expected_inventory_group = 2u;
    std::map<std::uint32_t, bool> disabled_buttons;
    std::vector<std::string> button_calls;
    std::vector<std::uint32_t> iterated_elements;
    std::array<std::int32_t, 5> skill_counts{{0, 0, 0, 0, 0}};
    std::vector<std::uint32_t> skill_ids;
    std::vector<std::string> icons;
    std::vector<std::string> titles;
    std::vector<std::string> item_names;
    std::vector<std::string> item_costs;
    std::vector<std::array<std::uint32_t, 4>> placed_items;
    std::vector<std::uint32_t> allocations;
    std::uint32_t column_count = 0u;
    std::uint32_t row_count = 0u;
    std::uint32_t cursor = 0u;
    std::uint32_t released_iterators = 0u;
    std::uint32_t hover_item = 0u;
    std::uint32_t item_count = 0u;
    std::int32_t available_magic = 7;
    std::uint32_t expected_element = 2u;
    std::uint32_t last_iterator_element = 0u;
    bool allow_all_elements = false;
    std::uint32_t callback_magic = 0xffffffffu;
    std::uint32_t removed_count = 0u;
    std::string description_text;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocations.push_back(bytes);
        if (bytes == 12u) return fail_callback ? 0u : kCallback;
        if (bytes == 0x214u || bytes == 0x210u)
            return 0x11000u + 0x1000u * item_count++;
        Check(bytes == 24u, "list and button callbacks allocate 24 bytes");
        const auto address = 0x8100u + 0x100u * large_callback_count++;
        return fail_large_callback ? 0u : address;
    }
    void LoadLayout(const std::uint32_t window,
                    const std::uint32_t layout,
                    const std::uint32_t group,
                    const std::uint32_t flags) override {
        calls.emplace_back("layout");
        Check(window == kWindow && layout == expected_layout &&
              group == expected_group && flags == 0u,
              "constructor forwards layout and group with zero flags");
    }
    std::string_view ReadCString(const std::uint32_t address) override {
        return strings.at(address);
    }
    std::uint32_t GetWindowByName(const std::uint32_t window,
                                  const std::string_view name) override {
        calls.emplace_back("by_name");
        Check(window == kWindow, "named control uses this layout");
        names.emplace_back(name);
        if (name.ends_with("EditDescription")) return kDescription;
        Check(name.ends_with("List"), "first named control is the list");
        return kList;
    }
    std::uint32_t GetWindow(const std::uint32_t window,
                            const std::string_view name) override {
        Check(window == kWindow, "window lookup uses this layout");
        names.emplace_back(name);
        if (name.ends_with("Root")) {
            calls.emplace_back("root");
            return kRoot;
        }
        calls.emplace_back("button_window");
        if (property_mode) {
            constexpr std::array<std::string_view, 3> property_suffixes{{
                "BtnCure", "BtnAuxiliary", "BtnAttackable",
            }};
            for (std::uint32_t index = 0u;
                 index < property_suffixes.size(); ++index)
                if (name.ends_with(property_suffixes[index]))
                    return 0xb100u + 0x100u * index;
            throw std::runtime_error("unexpected property-select name");
        }
        constexpr std::array<std::string_view, 5> suffixes{{
            "BtnWater", "BtnFire", "BtnThunder", "BtnAir", "BtnEarth",
        }};
        for (std::uint32_t index = 0u; index < suffixes.size(); ++index)
            if (name.ends_with(suffixes[index]))
                return 0xa100u + 0x100u * index;
        throw std::runtime_error("unexpected magic-select window name");
    }
    std::uint32_t SubscribeMouseClick(
        const std::uint32_t control,
        const std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        if (control == kList) {
            CheckLargeCallback(callback,
                stunt_mode
                    ? pal4::CombatStuntSelectWindow::kListClickCallback
                    : property_mode
                    ? pal4::CombatPropertySelectWindow::kListClickCallback
                    : pal4::CombatMagicSelectWindow::kListClickCallback);
        } else if (stunt_mode) {
            Check(control == kRoot, "stunt root click uses Root window");
            CheckLargeCallback(callback,
                pal4::CombatStuntSelectWindow::kRootClickCallback);
        } else {
            Check(control == kRoot &&
                  (fail_callback ? callback == 0u :
                   callback == kCallback &&
                   words[kCallback] ==
                       pal4::CombatMagicSelectBase::kClickCallbackVtable &&
                   words[kCallback + 4u] ==
                       pal4::CombatMagicSelectBase::kClickCallback &&
                   words[kCallback + 8u] == kWindow),
                  "root mouse callback stores vtable, function and owner");
        }
        return 0x9000u;
    }
    void CheckLargeCallback(const std::uint32_t callback,
                            const std::uint32_t function) {
        if (fail_large_callback) {
            Check(callback == 0u,
                  "failed event allocation subscribes a null callback");
        } else {
            const auto expected_vtable = stunt_mode
                ? pal4::CombatStuntSelectWindow::kEventVtable
                : property_mode
                ? pal4::CombatPropertySelectWindow::kEventVtable
                : pal4::CombatMagicSelectWindow::kEventVtable;
            Check(callback != 0u &&
                  words[callback] == expected_vtable &&
                  words[callback + 8u] == function &&
                  words[callback + 12u] == 0u &&
                  words[callback + 16u] == kWindow,
                  "24-byte event object stores vtable, function and owner");
        }
        callback_functions.push_back(function);
    }
    std::uint32_t SubscribeListHover(
        const std::uint32_t list,
        const std::uint32_t callback) override {
        calls.emplace_back("hover");
        Check(list == kList, "hover subscription uses the list");
        CheckLargeCallback(callback,
            stunt_mode
                ? pal4::CombatStuntSelectWindow::kHoverCallback
                : property_mode
                ? pal4::CombatPropertySelectWindow::kHoverCallback
                : pal4::CombatMagicSelectWindow::kHoverCallback);
        return 0x9000u;
    }
    std::uint32_t SubscribeButtonClick(
        const std::uint32_t button,
        const std::uint32_t callback) override {
        calls.emplace_back("button_click");
        const auto first = property_mode ? 0xb100u : 0xa100u;
        const auto count = property_mode ? 3u : 5u;
        const auto index = static_cast<std::uint32_t>(button - first) / 0x100u;
        Check(index < count && button == first + 0x100u * index &&
              words[button + 280u] == index,
              "button click stores its zero-based element index");
        button_controls.push_back(button);
        CheckLargeCallback(callback,
            property_mode
                ? pal4::CombatPropertySelectWindow::kButtonClickCallback
                : pal4::CombatMagicSelectWindow::kButtonClickCallback);
        return 0x9000u;
    }
    void ReleaseSubscription(const std::uint32_t subscription) override {
        calls.emplace_back("release");
        ++subscription_releases;
        Check(subscription == 0x9000u,
              "temporary event subscription is released");
    }
    void SetSortColumn(const std::uint32_t list,
                       const std::uint32_t column) override {
        calls.emplace_back("sort_column");
        Check(list == kList && column == 0u,
              "list sorts on column zero");
    }
    void SetSortDirection(const std::uint32_t list,
                          const std::uint32_t direction) override {
        calls.emplace_back("sort_direction");
        Check(list == kList && direction == 0u,
              "list uses direction zero");
    }
    void SetUserColumnSizingEnabled(const std::uint32_t list,
                                    const bool enabled) override {
        calls.emplace_back("sizing");
        Check(list == kList && !enabled, "column sizing is disabled");
    }
    void SetUserColumnDraggingEnabled(const std::uint32_t list,
                                      const bool enabled) override {
        calls.emplace_back("dragging");
        Check(list == kList && !enabled, "column dragging is disabled");
    }
    void SetUserSortControlEnabled(const std::uint32_t list,
                                   const bool enabled) override {
        calls.emplace_back("sort_control");
        Check(list == kList && !enabled, "user sort control is disabled");
    }
    std::uint32_t CleanupTemporaryString() override {
        calls.emplace_back("cleanup");
        return 0x5au;
    }
    void InvokeVirtualSelection(const std::uint32_t window,
                                const std::uint32_t offset) override {
        Check(window == kWindow && offset == 24u,
              "root event invokes virtual slot +24");
        ++selection_count;
    }
    void InvokeVisibility(const std::uint32_t window,
                          const std::uint32_t visible,
                          const std::uint32_t parent) override {
        calls.emplace_back("visibility");
        if (visible == 0u) {
            Check(window == kWindow && parent == 0u &&
                  callback_magic != 0xffffffffu,
                  "list click hides window after callback");
            return;
        }
        Check(window == kWindow && visible == 1u && parent == 0x6600u &&
              (stunt_mode
                  ? words[window + 52u] == 0x2222u &&
                    words[window + 48u] == 0x1111u &&
                    words[window + 44u] == 0x9000u
                  : words[window + 64u] == 0x2222u &&
                    words[window + 60u] == 0x1111u &&
                    words[window + 52u] == 0x9000u),
              "setup writes pair and mode before virtual visibility call");
    }
    void SetText(const std::uint32_t control,
                 const std::string_view text) override {
        if (!text.empty()) {
            Check(control == kDescription,
                  "magic description uses description control");
            description_text = text;
            calls.emplace_back("description");
            return;
        }
        calls.emplace_back("clear_text");
        Check(control == kDescription && text.empty(),
              "setup clears the description field");
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("manager");
        return 0x7800u;
    }
    std::uint32_t InsertEventHandler(const std::uint32_t array,
                                     const std::uint32_t facet) override {
        calls.emplace_back("insert");
        Check(array == 0x7800u + 308u &&
              facet == kWindow + (stunt_mode ? 40u : 48u),
              "setup registers its embedded event handler facet");
        return 0x6du;
    }
    void ResetList(const std::uint32_t list) override {
        Check(list == kList, "reset uses magic list");
        calls.emplace_back("reset");
        row_count = 0u;
    }
    std::uint32_t GetColumnCount(const std::uint32_t list) override {
        Check(list == kList, "column count uses magic list");
        return column_count;
    }
    void RemoveColumn(const std::uint32_t list,
                      const std::uint32_t index) override {
        Check(list == kList && index == 0u && column_count > 0u,
              "columns are removed at index zero");
        calls.emplace_back("remove_column");
        --column_count;
    }
    std::string_view GetGameString(const std::uint32_t id) override {
        calls.emplace_back("game_string");
        return game_strings.at(id);
    }
    std::string ConvertToUtf8(const std::string_view text) override {
        calls.emplace_back("utf8");
        return std::string(text);
    }
    void AddColumn(const std::uint32_t list, const std::string_view title,
                   const std::uint32_t id, const float width) override {
        Check(list == kList && id == column_count &&
              width == (id == 0u ? 0.6f : 0.3f),
              "column IDs and widths match IDA constants");
        titles.emplace_back(title);
        ++column_count;
        calls.emplace_back("add_column");
    }
    std::uint32_t GetSkillIterator(const std::uint32_t collection,
                                   const std::uint32_t index) override {
        Check(collection == 0x9000u + 920u &&
              (allow_all_elements ? index < 5u : index == expected_element),
              "skill iterator uses actor+920 and selected element");
        calls.emplace_back("iterator");
        iterated_elements.push_back(index);
        last_iterator_element = index;
        cursor = 0u;
        return 0x9900u;
    }
    std::uint32_t GetStuntIterator(const std::uint32_t collection) override {
        Check(collection == 0x9000u + 920u,
              "stunt iterator uses actor+920 without element index");
        calls.emplace_back("stunt_iterator");
        cursor = 0u;
        return 0x9900u;
    }
    std::uint32_t FindStunt(const std::uint32_t id) override {
        calls.emplace_back("find_stunt");
        const auto found = stunts_by_id.find(id);
        return found == stunts_by_id.end() ? 0u : found->second;
    }
    bool IsStuntUsable(const std::uint32_t actor,
                       const std::uint32_t stunt_record) override {
        Check(actor == 0x9000u, "availability checks selected actor");
        bool owned_record = false;
        for (const auto& [id, address] : stunts_by_id)
            owned_record = owned_record || address == stunt_record;
        Check(owned_record, "availability receives a Stunt database record, not a UI cell");
        checked_stunt_records.push_back(stunt_record);
        calls.emplace_back("stunt_usable");
        return stunt_usable;
    }
    bool IteratorDone(const std::uint32_t iterator) override {
        Check(iterator == 0x9900u, "iterator pointer is preserved");
        return cursor == skill_ids.size();
    }
    std::uint32_t IteratorSkillId(const std::uint32_t iterator) override {
        Check(iterator == 0x9900u && cursor < skill_ids.size(),
              "iterator dereference uses current skill");
        return skill_ids[cursor];
    }
    void AdvanceIterator(const std::uint32_t iterator) override {
        Check(iterator == 0x9900u && cursor < skill_ids.size(),
              "iterator advances after every skill");
        ++cursor;
    }
    void ReleaseIterator(const std::uint32_t iterator) override {
        Check(iterator == 0x9900u, "iterator release uses original object");
        ++released_iterators;
    }
    std::int32_t IteratorSize(const std::uint32_t iterator) override {
        Check(iterator == 0x9900u, "skill count uses element iterator");
        return skill_counts[last_iterator_element];
    }
    void SetButtonSelected(const std::uint32_t button,
                           const bool selected) override {
        button_calls.push_back(std::string("selected:") +
            std::to_string(button) + (selected ? ":1" : ":0"));
    }
    void SetButtonEnabled(const std::uint32_t button,
                          const bool enabled) override {
        disabled_buttons[button] = !enabled;
        button_calls.push_back(std::string("enabled:") +
            std::to_string(button) + (enabled ? ":1" : ":0"));
    }
    void SetButtonVisible(const std::uint32_t button,
                          const bool visible) override {
        button_calls.push_back(std::string("visible:") +
            std::to_string(button) + (visible ? ":1" : ":0"));
    }
    bool IsButtonDisabled(const std::uint32_t button) override {
        calls.emplace_back("is_disabled");
        return disabled_buttons[button];
    }
    std::uint32_t RemoveEventHandler(const std::uint32_t array,
                                     const std::uint32_t facet) override {
        Check(array == 0x7800u + 308u &&
              facet == kWindow + (stunt_mode ? 40u : 48u),
              "magic facet removed from frame-manager event array");
        calls.emplace_back("remove_handler");
        ++removed_count;
        return 0x6eu;
    }
    void InvokeSelectionCallback(const std::uint32_t callback,
                                 const std::uint32_t owner,
                                 const std::uint32_t magic) override {
        Check(callback == 0x7700u && owner == 0x6600u,
              "stored selection callback receives owner and magic");
        if (property_mode && magic)
            Check(words[kWindow + 80u + 4u * words[kWindow + 56u]] ==
                      words[magic],
                  "property ID is saved before callback");
        calls.emplace_back("callback");
        callback_magic = magic;
    }
    std::int32_t GetInventoryCount(const std::uint32_t group) override {
        Check(!inventory_groups.empty() || group == expected_inventory_group,
              "inventory count uses selected property group");
        calls.emplace_back("inventory_count");
        return static_cast<std::int32_t>(inventory_groups.empty()
            ? inventory_records.size() : inventory_groups[group].size());
    }
    std::uint32_t GetInventoryElement(const std::uint32_t group,
                                      const std::uint32_t index) override {
        const auto& records = inventory_groups.empty()
            ? inventory_records : inventory_groups[group];
        Check((!inventory_groups.empty() ||
               group == expected_inventory_group) && index < records.size(),
              "inventory element uses selected group and ordered index");
        calls.emplace_back("inventory_element");
        return records[index];
    }
    std::uint32_t FindProperty(const std::uint32_t id) override {
        calls.emplace_back("find_property");
        const auto found = properties_by_id.find(id);
        return found == properties_by_id.end() ? 0u : found->second;
    }
    std::uint32_t FindMagic(const std::uint32_t id) override {
        calls.emplace_back("find_magic");
        const auto found = magic_by_id.find(id);
        return found == magic_by_id.end() ? 0u : found->second;
    }
    void AddRow(const std::uint32_t list,
                const std::uint32_t index) override {
        Check(list == kList && index == 0u, "magic row uses index zero");
        ++row_count;
        calls.emplace_back("add_row");
    }
    void ConstructNameItem(const std::uint32_t item,
                           const std::uint32_t list,
                           const std::string_view name,
                           const bool selected) override {
        Check(item && list == kList && selected,
              "name item receives list and selected flag");
        item_names.emplace_back(name);
    }
    void ConstructCostItem(const std::uint32_t item,
                           const std::uint32_t list,
                           const std::string_view cost) override {
        Check(item && list == kList, "cost item receives list");
        item_costs.emplace_back(cost);
    }
    std::uint32_t GetImage(const std::string_view imageset,
                           const std::string_view image) override {
        Check(imageset == "gy0", "all magic icons use gy0 imageset");
        icons.emplace_back(image);
        return 0x200u + static_cast<std::uint32_t>(icons.size());
    }
    std::uint32_t GetMagicKey(const std::uint32_t magic) override {
        return magic_keys.at(magic);
    }
    std::uint32_t GetRowCount(const std::uint32_t list) override {
        Check(list == kList, "row count uses magic list");
        return row_count;
    }
    std::uint32_t GetColumnWithId(const std::uint32_t list,
                                  const std::uint32_t id) override {
        Check(list == kList && id < 2u, "lookup uses both column IDs");
        return id + 10u;
    }
    void SetListItem(const std::uint32_t list, const std::uint32_t item,
                     const std::uint32_t row,
                     const std::uint32_t column) override {
        Check(list == kList && row < row_count &&
              (column == 10u || column == 11u),
              "item grid reference uses final row and resolved column");
        placed_items.push_back({item, row, column, words[item + 312u]});
    }
    std::int32_t GetActorStat(const std::uint32_t actor,
                              const std::uint32_t stat) override {
        Check(actor == 0x9000u && stat == 9u,
              "cost compares actor stat nine");
        return available_magic;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void SetHoverRowIndex(const std::uint32_t list,
                          const std::uint32_t index) override {
        Check(list == kList && index == 0u && row_count,
              "first populated magic row becomes hovered");
        calls.emplace_back("hover_first");
    }
    std::uint32_t GetHoverItem(const std::uint32_t list) override {
        Check(list == kList, "hover lookup uses magic list");
        return hover_item;
    }
};

void ConstructorAndControls() {
    Runtime runtime;
    runtime.words[kWindow + 24u] = 0xd000u;
    runtime.strings[0xd000u] = "CombatMagicSelect/";
    Check(pal4::CombatMagicSelectBase::Construct(
              kWindow, 0xb000u, 0xc000u, runtime) == kWindow &&
          runtime.words[kWindow] ==
              pal4::CombatMagicSelectBase::kVtable &&
          runtime.words[kWindow + 40u] == kList &&
          runtime.words[kWindow + 44u] == kDescription &&
          runtime.names == std::vector<std::string>{
              "CombatMagicSelect/List",
              "CombatMagicSelect/EditDescription",
              "CombatMagicSelect/Root"} &&
          runtime.calls == std::vector<std::string>{
              "layout", "by_name", "by_name", "root", "allocate",
              "subscribe", "release", "sort_column", "sort_direction",
              "sizing", "dragging", "sort_control", "cleanup"},
          "constructor initializes prefixed controls and list in IDA order");
}

void EmptyPrefixAndAllocationFailure() {
    Runtime runtime;
    runtime.words[kWindow] = pal4::CombatMagicSelectBase::kVtable;
    runtime.fail_callback = true;
    Check(pal4::CombatMagicSelectBase::Initialize(kWindow, runtime) ==
              0x5au &&
          runtime.names == std::vector<std::string>{
              "List", "EditDescription", "Root"} &&
          runtime.words[kWindow + 40u] == kList &&
          runtime.words[kWindow + 44u] == kDescription,
          "null prefix and callback allocation failure keep list setup");
}

void RootClickGate() {
    Runtime runtime;
    for (const auto value : {0u, 2u, 1u}) {
        runtime.words[kEvent + 28u] = value;
        Check(pal4::CombatMagicSelectBase::HandleRootClick(
                  kWindow, kEvent, runtime) == 1u,
              "root event always returns handled");
    }
    Check(runtime.selection_count == 1u,
          "only mouse action one invokes the virtual selection callback");
}

void DerivedConstructor() {
    Runtime runtime;
    runtime.expected_layout = pal4::CombatMagicSelectWindow::kLayoutName;
    runtime.expected_group = pal4::CombatMagicSelectWindow::kResourceGroup;
    Check(pal4::CombatMagicSelectWindow::Construct(kWindow, runtime) ==
              kWindow &&
          runtime.words[kWindow] ==
              pal4::CombatMagicSelectWindow::kVtable &&
          runtime.words[kWindow + 48u] ==
              pal4::CombatMagicSelectWindow::kFacetVtable &&
          runtime.words[kWindow + 52u] == 0u &&
          runtime.words[kWindow + 56u] == 5u &&
          runtime.words[kWindow + 60u] == 0u &&
          runtime.words[kWindow + 64u] == 0u &&
          runtime.calls.back() == "cleanup" &&
          runtime.names == std::vector<std::string>{
              "List", "EditDescription", "Root", "BtnWater", "BtnFire",
              "BtnThunder", "BtnAir", "BtnEarth"} &&
          runtime.large_callback_count == 7u &&
          runtime.subscription_releases == 8u &&
          runtime.callback_functions == std::vector<std::uint32_t>{
              pal4::CombatMagicSelectWindow::kHoverCallback,
              pal4::CombatMagicSelectWindow::kListClickCallback,
              pal4::CombatMagicSelectWindow::kButtonClickCallback,
              pal4::CombatMagicSelectWindow::kButtonClickCallback,
              pal4::CombatMagicSelectWindow::kButtonClickCallback,
              pal4::CombatMagicSelectWindow::kButtonClickCallback,
              pal4::CombatMagicSelectWindow::kButtonClickCallback},
          "concrete constructor composes base layout and derived facet");
    for (std::uint32_t index = 0u; index < 5u; ++index)
        Check(runtime.words[kWindow + 68u + 4u * index] ==
                  0xa100u + 0x100u * index &&
              runtime.words[0xa100u + 0x100u * index + 280u] == index &&
              runtime.button_controls[index] ==
                  0xa100u + 0x100u * index,
              "all five element buttons store pointers and indices in order");
}

void ButtonInitializationAllocationFailure() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 24u] = 0xd000u;
    runtime.strings[0xd000u] = "Magic/";
    runtime.fail_large_callback = true;
    Check(pal4::CombatMagicSelectWindow::InitializeButtons(
              kWindow, runtime) == 0x5au &&
          runtime.names == std::vector<std::string>{
              "Magic/BtnWater", "Magic/BtnFire", "Magic/BtnThunder",
              "Magic/BtnAir", "Magic/BtnEarth"} &&
          runtime.large_callback_count == 7u &&
          runtime.subscription_releases == 7u &&
          runtime.button_controls.size() == 5u,
          "null event objects still register all seven handlers and buttons");
}

void SetupAndFirstEnabledButton() {
    Runtime runtime;
    runtime.words[kPair] = 0x1111u;
    runtime.words[kPair + 4u] = 0x2222u;
    runtime.words[kWindow + 44u] = kDescription;
    runtime.words[kWindow + 40u] = kList;
    runtime.game_strings = {{0u, "Magic"}, {0x134u, "Cost"}};
    runtime.allow_all_elements = true;
    runtime.skill_counts[2] = 1;
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.words[kWindow + 68u + 4u * index] =
            0xa100u + 0x100u * index;
    Check(pal4::CombatMagicSelectWindow::Setup(
              kWindow, kPair, 0x9000u, 0x6600u, runtime) == 0x6du &&
          runtime.words[kWindow + 56u] == 2u &&
          runtime.words[kWindow + 60u] == 0x1111u &&
          runtime.words[kWindow + 64u] == 0x2222u &&
          runtime.button_calls.size() == 32u &&
          runtime.button_calls.back() == "selected:41728:1" &&
          runtime.iterated_elements ==
              std::vector<std::uint32_t>{
                  0u, 1u, 2u, 3u, 4u, 0u,
                  0u, 1u, 2u, 3u, 4u, 2u} &&
          runtime.released_iterators == 12u &&
          runtime.calls.front() == "visibility" &&
          runtime.calls[1] == "clear_text" &&
          runtime.calls[runtime.calls.size() - 2u] == "manager" &&
          runtime.calls.back() == "insert",
          "setup selects the first enabled button before registration");

    Runtime none;
    none.words[kPair] = 0x1111u;
    none.words[kPair + 4u] = 0x2222u;
    none.words[kWindow + 44u] = kDescription;
    none.words[kWindow + 40u] = kList;
    none.game_strings = {{0u, "Magic"}, {0x134u, "Cost"}};
    none.allow_all_elements = true;
    for (std::uint32_t index = 0u; index < 5u; ++index)
        none.words[kWindow + 68u + 4u * index] =
            0xa100u + 0x100u * index;
    Check(pal4::CombatMagicSelectWindow::Setup(
              kWindow, kPair, 0x9000u, 0x6600u, none) == 0x6du &&
          none.words[kWindow + 56u] == 0u &&
          none.button_calls.size() == 16u &&
          none.button_calls.back() == "selected:41216:1" &&
          none.released_iterators == 6u &&
          none.calls.back() == "insert",
          "setup keeps initial button when none has skills");
}

void MagicListGatesAndEmptyCollection() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 56u] = 2u;
    pal4::CombatMagicSelectWindow::UpdateMagicList(kWindow, runtime);
    runtime.words[kWindow + 52u] = 0x9000u;
    runtime.words[kWindow + 56u] = 5u;
    pal4::CombatMagicSelectWindow::UpdateMagicList(kWindow, runtime);
    Check(runtime.calls.empty(), "null actor and element five do no work");

    runtime.words[kWindow + 56u] = 2u;
    runtime.game_strings = {{0u, "Name"}, {0x134u, "Cost"}};
    pal4::CombatMagicSelectWindow::UpdateMagicList(kWindow, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "reset", "reset", "game_string", "utf8", "add_column",
              "game_string", "utf8", "add_column", "iterator"} &&
          runtime.titles == std::vector<std::string>{"Name", "Cost"} &&
          runtime.row_count == 0u && runtime.released_iterators == 1u,
          "empty skill collection still builds columns and releases iterator");
}

void MagicListRowsAndAffordability() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 52u] = 0x9000u;
    runtime.words[kWindow + 56u] = 2u;
    runtime.column_count = 3u;
    runtime.game_strings = {{0u, "Name"}, {0x134u, "Cost"}};
    runtime.skill_ids = {1u, 2u, 3u, 4u, 5u, 6u};
    runtime.magic_by_id = {{1u, 0x20000u}, {2u, 0x21000u},
                           {3u, 0x22000u}, {4u, 0x23000u},
                           {5u, 0x24000u}};
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto magic = 0x20000u + 0x1000u * index;
        runtime.magic_keys[magic] = index + 1u;
        runtime.words[magic + 16u] = 0x30000u + index;
        runtime.strings[0x30000u + index] = "Magic" +
            std::to_string(index);
    }
    runtime.words[0x20000u + 8u] = 0u;
    runtime.words[0x21000u + 8u] = 2u;
    runtime.words[0x22000u + 8u] = 1u;
    runtime.words[0x23000u + 8u] = 3u;
    runtime.words[0x24000u + 8u] = 6u;
    runtime.words[0x20000u + 76u] = 4u;
    runtime.words[0x21000u + 76u] = 9u;
    runtime.words[0x22000u + 76u] = 7u;

    pal4::CombatMagicSelectWindow::UpdateMagicList(kWindow, runtime);
    Check(runtime.calls[0] == "reset" && runtime.calls[1] == "reset" &&
          runtime.calls[2] == "remove_column" &&
          runtime.calls[3] == "remove_column" &&
          runtime.calls[4] == "remove_column" &&
          runtime.calls.back() == "hover_first" &&
          runtime.column_count == 2u && runtime.row_count == 3u &&
          runtime.cursor == runtime.skill_ids.size() &&
          runtime.released_iterators == 1u,
          "refresh removes all old columns and visits every skill");
    Check(runtime.item_names ==
              std::vector<std::string>{"Magic0", "Magic1", "Magic2"} &&
          runtime.item_costs ==
              std::vector<std::string>{"4", "9", "7"} &&
          runtime.icons == std::vector<std::string>{
              "skillattack", "skillcure", "skillbuff"} &&
          runtime.allocations == std::vector<std::uint32_t>{
              0x214u, 0x210u, 0x214u, 0x210u, 0x214u, 0x210u},
          "eligible magic gets exact item sizes, names, costs and icons");
    Check(runtime.placed_items ==
              std::vector<std::array<std::uint32_t, 4>>{
                  {0x11000u, 0u, 10u, 1u},
                  {0x12000u, 0u, 11u, 1u},
                  {0x13000u, 1u, 10u, 2u},
                  {0x14000u, 1u, 11u, 2u},
                  {0x15000u, 2u, 10u, 3u},
                  {0x16000u, 2u, 11u, 3u}} &&
          runtime.words[0x11000u + 528u] == 0x201u &&
          runtime.words[0x13000u + 528u] == 0x202u &&
          runtime.words[0x15000u + 528u] == 0x203u,
          "both items carry magic key and name items receive icon pointer");
    Check(runtime.bytes.size() == 2u &&
          runtime.bytes[0x13000u + 421u] == 1u &&
          runtime.bytes[0x14000u + 421u] == 1u,
          "only a cost above actor stat nine disables both row items");

    runtime.hover_item = 0x11000u;
    Check(pal4::CombatMagicSelectWindow::GetHoveredMagic(
              kWindow, runtime) == 0x20000u,
          "hover lookup resolves enabled item's magic key");
    runtime.hover_item = 0x13000u;
    Check(pal4::CombatMagicSelectWindow::GetHoveredMagic(
              kWindow, runtime) == 0u,
          "disabled item does not resolve a magic");
    runtime.hover_item = 0u;
    Check(pal4::CombatMagicSelectWindow::GetHoveredMagic(
              kWindow, runtime) == 0u,
          "empty hover item returns null");
}

void ButtonSelectionAndClickBridge() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 52u] = 0x9000u;
    runtime.game_strings = {{0u, "Name"}, {0x134u, "Cost"}};
    runtime.allow_all_elements = true;
    runtime.skill_counts = {{0, 2, 0, 1, 3}};
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.words[kWindow + 68u + 4u * index] =
            0xa100u + 0x100u * index;
    pal4::CombatMagicSelectWindow::SelectButton(kWindow, 2u, runtime);
    Check(runtime.words[kWindow + 56u] == 2u &&
          runtime.iterated_elements ==
              std::vector<std::uint32_t>{0u, 1u, 2u, 3u, 4u, 2u} &&
          runtime.released_iterators == 6u &&
          runtime.button_calls.size() == 16u &&
          runtime.button_calls[0] == "selected:41216:0" &&
          runtime.button_calls[1] == "enabled:41216:0" &&
          runtime.button_calls[2] == "visible:41216:1" &&
          runtime.button_calls[4] == "enabled:41472:1" &&
          runtime.button_calls[13] == "enabled:42240:1" &&
          runtime.button_calls.back() == "selected:41728:1" &&
          runtime.calls.back() == "iterator",
          "selection refreshes five buttons then selected element list");

    runtime.words[kEvent + 8u] = 0xa400u;
    runtime.words[0xa400u + 280u] = 3u;
    Check(pal4::CombatMagicSelectWindow::HandleButtonClick(
              kWindow, kEvent, runtime) == 1u &&
          runtime.words[kWindow + 56u] == 3u &&
          runtime.button_calls.back() == "selected:41984:1" &&
          runtime.released_iterators == 12u,
          "button event reads sender user data and selects its element");
}

void ButtonNavigation() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto button = 0xa100u + 0x100u * index;
        runtime.words[kWindow + 68u + 4u * index] = button;
        runtime.disabled_buttons[button] = index != 1u && index != 3u;
    }
    Check(pal4::CombatMagicSelectWindow::FindNextEnabledButton(
              kWindow, -1, runtime) == 1 &&
          pal4::CombatMagicSelectWindow::FindNextEnabledButton(
              kWindow, 1, runtime) == 3 &&
          pal4::CombatMagicSelectWindow::FindNextEnabledButton(
              kWindow, 3, runtime) == 3 &&
          pal4::CombatMagicSelectWindow::FindNextEnabledButton(
              kWindow, 4, runtime) == 4 &&
          pal4::CombatMagicSelectWindow::FindPreviousEnabledButton(
              kWindow, 5, runtime) == 3 &&
          pal4::CombatMagicSelectWindow::FindPreviousEnabledButton(
              kWindow, 3, runtime) == 1 &&
          pal4::CombatMagicSelectWindow::FindPreviousEnabledButton(
              kWindow, 1, runtime) == 1 &&
          pal4::CombatMagicSelectWindow::FindPreviousEnabledButton(
              kWindow, 0, runtime) == 0,
          "navigation skips disabled buttons and keeps current at edge");
}

void KeyboardRouting() {
    struct Navigation final : pal4::ICombatSelectListNavigationRuntime {
        std::vector<std::uint32_t> up, down;
        void UpHoverItem(std::uint32_t list) override { up.push_back(list); }
        void DownHoverItem(std::uint32_t list) override { down.push_back(list); }
    } navigation;
    Runtime runtime;
    runtime.words[kWindow + 52u] = 0x9000u;
    runtime.game_strings = {{0u, "Name"}, {0x134u, "Cost"}};
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 56u] = 1u;
    runtime.allow_all_elements = true;
    runtime.skill_counts = {0, 1, 0, 1, 0};
    for (std::uint32_t index = 0; index < 5; ++index) {
        const auto button = 0xa100u + 0x100u * index;
        runtime.words[kWindow + 68u + 4u * index] = button;
        runtime.disabled_buttons[button] = index != 1u && index != 3u;
    }
    const auto key = [&](std::uint32_t scan_code) {
        runtime.words[kEvent + 16u] = scan_code;
        return pal4::CombatMagicSelectWindow::HandleKeyDown(
            kWindow, kEvent, runtime, navigation);
    };
    Check(key(0x01u) == 1u && runtime.selection_count == 1u,
          "Escape invokes original cancellation virtual slot");
    Check(key(0xcdu) == 1u && runtime.words[kWindow + 56u] == 3u,
          "Right skips unavailable elements");
    const auto refreshed = runtime.released_iterators;
    Check(key(0xcdu) == 1u && runtime.words[kWindow + 56u] == 3u &&
          runtime.released_iterators == refreshed,
          "Right at last available element does not refresh or wrap");
    Check(key(0xcbu) == 1u && runtime.words[kWindow + 56u] == 1u,
          "Left selects previous available element");
    Check(key(0xd0u) == 1u && key(0xc8u) == 1u &&
          navigation.down == std::vector<std::uint32_t>{kList} &&
          navigation.up == std::vector<std::uint32_t>{kList},
          "vertical keys delegate to DLL hover navigation");
    Check(key(0x1cu) == 1u && runtime.removed_count == 0u,
          "Enter without hovered item consumes key without dispatch");
    runtime.hover_item = 0x11000u;
    runtime.words[0x11000u + 312u] = 7u;
    runtime.magic_by_id[7u] = 0x20000u;
    runtime.bytes[0x11000u + 421u] = 1u;
    Check(key(0x1cu) == 1u && runtime.removed_count == 0u,
          "Enter refuses disabled unaffordable spell");
    runtime.bytes[0x11000u + 421u] = 0u;
    runtime.words[kWindow + 60u] = 0x6600u;
    runtime.words[kWindow + 64u] = 0x7700u;
    Check(key(0x1cu) == 1u && runtime.callback_magic == 0x20000u &&
          runtime.removed_count == 1u && runtime.calls.back() == "visibility",
          "Enter reuses selection callback, input removal and hide path");
    const auto call_count = runtime.calls.size();
    Check(key(0x39u) == 0u && runtime.calls.size() == call_count,
          "unhandled scan code returns false without side effects");
}

void ListClickDescriptionAndCleanup() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 44u] = kDescription;
    runtime.words[kWindow + 60u] = 0x6600u;
    runtime.words[kWindow + 64u] = 0x7700u;
    Check(pal4::CombatMagicSelectWindow::HandleListClick(
              kWindow, runtime) == 1u &&
          runtime.removed_count == 0u &&
          runtime.callback_magic == 0xffffffffu &&
          pal4::CombatMagicSelectWindow::UpdateMagicDescription(
              kWindow, runtime) == 1u &&
          runtime.description_text.empty(),
          "missing hovered item neither selects nor updates description");

    runtime.hover_item = 0x11000u;
    runtime.words[0x11000u + 312u] = 7u;
    Check(pal4::CombatMagicSelectWindow::UpdateMagicDescription(
              kWindow, runtime) == 1u &&
          runtime.description_text.empty(),
          "missing magic leaves prior description intact");
    runtime.magic_by_id[7u] = 0x20000u;
    runtime.words[0x20000u + 48u] = 0x30000u;
    runtime.strings[0x30000u] = "Fire \xE7\x81\xAB";
    Check(pal4::CombatMagicSelectWindow::UpdateMagicDescription(
              kWindow, runtime) == 1u &&
          runtime.description_text == "Fire \xE7\x81\xAB",
          "description uses magic+48 and preserves converted UTF-8");
    Check(pal4::CombatMagicSelectWindow::HandleListClick(
              kWindow, runtime) == 1u &&
          runtime.callback_magic == 0x20000u &&
          runtime.removed_count == 1u &&
          runtime.calls[runtime.calls.size() - 4u] == "manager" &&
          runtime.calls[runtime.calls.size() - 3u] == "remove_handler" &&
          runtime.calls[runtime.calls.size() - 2u] == "callback" &&
          runtime.calls.back() == "visibility",
          "list click removes handler, invokes callback and hides window");
    Check(pal4::CombatMagicSelectWindow::Cleanup(kWindow, runtime) == 0x6eu &&
          runtime.callback_magic == 0u &&
          runtime.removed_count == 2u &&
          runtime.calls[runtime.calls.size() - 3u] == "callback" &&
          runtime.calls[runtime.calls.size() - 2u] == "manager" &&
          runtime.calls.back() == "remove_handler",
          "cleanup invokes callback with null magic before unregistering");
}

void PropertyConstructorAndButtons() {
    Runtime runtime;
    runtime.property_mode = true;
    runtime.expected_layout =
        pal4::CombatPropertySelectWindow::kLayoutName;
    runtime.expected_group =
        pal4::CombatPropertySelectWindow::kResourceGroup;
    runtime.words[kWindow + 24u] = 0xd000u;
    runtime.strings[0xd000u] = "Property/";
    Check(pal4::CombatPropertySelectWindow::Construct(
              kWindow, runtime) == kWindow &&
          runtime.words[kWindow] ==
              pal4::CombatPropertySelectWindow::kVtable &&
          runtime.words[kWindow + 48u] ==
              pal4::CombatPropertySelectWindow::kFacetVtable &&
          runtime.words[kWindow + 52u] == 0u &&
          runtime.words[kWindow + 56u] == 0u &&
          runtime.words[kWindow + 60u] == 0u &&
          runtime.words[kWindow + 64u] == 0u &&
          runtime.words[kWindow + 80u] == 0u &&
          runtime.words[kWindow + 84u] == 0u &&
          runtime.words[kWindow + 88u] == 0u &&
          runtime.words[kWindow + 40u] == kList &&
          runtime.words[kWindow + 44u] == kDescription,
          "property constructor composes base, facet and trailing fields");
    Check(runtime.names == std::vector<std::string>{
              "Property/List", "Property/EditDescription",
              "Property/Root", "Property/BtnCure",
              "Property/BtnAuxiliary", "Property/BtnAttackable"} &&
          runtime.large_callback_count == 5u &&
          runtime.subscription_releases == 6u &&
          runtime.callback_functions == std::vector<std::uint32_t>{
              pal4::CombatPropertySelectWindow::kHoverCallback,
              pal4::CombatPropertySelectWindow::kListClickCallback,
              pal4::CombatPropertySelectWindow::kButtonClickCallback,
              pal4::CombatPropertySelectWindow::kButtonClickCallback,
              pal4::CombatPropertySelectWindow::kButtonClickCallback} &&
          runtime.button_calls == std::vector<std::string>{
              "enabled:45312:0", "enabled:45568:0",
              "enabled:45824:0"},
          "property controls and five event callbacks use IDA order");
    for (std::uint32_t index = 0u; index < 3u; ++index)
        Check(runtime.words[kWindow + 68u + 4u * index] ==
                  0xb100u + 0x100u * index &&
              runtime.words[0xb100u + 0x100u * index + 280u] == index &&
              runtime.button_controls[index] ==
                  0xb100u + 0x100u * index,
              "property buttons store pointer and zero-based index");
}

void PropertySetupAndAllocationFailure() {
    Runtime runtime;
    runtime.property_mode = true;
    runtime.words[kPair] = 0x1111u;
    runtime.words[kPair + 4u] = 0x2222u;
    runtime.words[kWindow + 44u] = kDescription;
    runtime.words[kWindow + 40u] = kList;
    runtime.game_strings = {{0u, "Property"}, {1u, "Count"}};
    runtime.inventory_groups = {
        {0u, {}}, {1u, {}}, {2u, {0x50200u}},
    };
    runtime.words[0x50200u] = 0x43000u;
    runtime.words[0x50200u + 8u] = 4u;
    runtime.words[0x43000u] = 101u;
    runtime.words[0x43000u + 8u] = 5u;
    runtime.words[0x43000u + 20u] = 0x60000u;
    runtime.strings[0x60000u] = "Aux";
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.words[kWindow + 68u + 4u * index] =
            0xb100u + 0x100u * index;
    Check(pal4::CombatPropertySelectWindow::Setup(
              kWindow, kPair, 0x9000u, 0x6600u, runtime) == 0x6du &&
          runtime.words[kWindow + 60u] == 0x1111u &&
          runtime.words[kWindow + 64u] == 0x2222u &&
          runtime.words[kWindow + 52u] == 0x9000u &&
          runtime.words[kWindow + 56u] == 2u &&
          runtime.button_calls.size() == 20u &&
          runtime.button_calls.back() == "selected:45568:1" &&
          runtime.row_count == 1u &&
          runtime.calls.front() == "visibility" &&
          runtime.calls[1] == "clear_text" &&
          runtime.calls[runtime.calls.size() - 2u] == "manager" &&
          runtime.calls.back() == "insert",
          "property setup stores callback pair and selects first enabled tab");

    Runtime failed;
    failed.property_mode = true;
    failed.fail_large_callback = true;
    failed.words[kWindow + 40u] = kList;
    Check(pal4::CombatPropertySelectWindow::InitializeButtons(
              kWindow, failed) == 0x5au &&
          failed.names == std::vector<std::string>{
              "BtnCure", "BtnAuxiliary", "BtnAttackable"} &&
          failed.large_callback_count == 5u &&
          failed.subscription_releases == 5u &&
          failed.button_controls.size() == 3u &&
          failed.button_calls.size() == 3u,
          "property button setup preserves null allocation behavior");
}

void PropertyListGatesAndEmptyInventory() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 56u] = 2u;
    pal4::CombatPropertySelectWindow::UpdatePropertyList(kWindow, runtime);
    Check(runtime.calls.empty(), "null actor leaves property list untouched");

    runtime.words[kWindow + 52u] = 0x9000u;
    runtime.game_strings = {{0u, "Property"}, {1u, "Count"}};
    pal4::CombatPropertySelectWindow::UpdatePropertyList(kWindow, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "reset", "game_string", "utf8", "add_column",
              "game_string", "utf8", "add_column", "inventory_count"} &&
          runtime.titles ==
              std::vector<std::string>{"Property", "Count"} &&
          runtime.row_count == 0u,
          "empty inventory builds two columns without hover selection");
}

void PropertyListPartitionAndHover() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 52u] = 0x9000u;
    runtime.words[kWindow + 56u] = 2u;
    runtime.words[kWindow + 88u] = 100u;
    runtime.column_count = 2u;
    runtime.game_strings = {{0u, "Property"}, {1u, "Count"}};
    runtime.inventory_records = {0x50000u, 0x50100u,
                                 0x50200u, 0x50300u};
    constexpr std::array<std::uint32_t, 4> properties{{
        0x41000u, 0x42000u, 0x43000u, 0x44000u,
    }};
    constexpr std::array<std::uint32_t, 4> ids{{100u, 200u, 101u, 100u}};
    constexpr std::array<std::uint32_t, 4> types{{0u, 6u, 5u, 1u}};
    constexpr std::array<std::uint32_t, 4> quantities{{2u, 3u, 4u, 9u}};
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto record = runtime.inventory_records[index];
        const auto property = properties[index];
        runtime.words[record] = property;
        runtime.words[record + 8u] = quantities[index];
        runtime.words[property] = ids[index];
        runtime.words[property + 8u] = types[index];
        runtime.words[property + 20u] = 0x60000u + index;
        runtime.strings[0x60000u + index] = std::string(1, 'A' + index);
    }
    runtime.properties_by_id = {{100u, 0x41000u}, {101u, 0x43000u}};
    pal4::CombatPropertySelectWindow::UpdatePropertyList(kWindow, runtime);
    Check(runtime.calls[0] == "reset" &&
          runtime.calls[1] == "remove_column" &&
          runtime.calls[2] == "remove_column" &&
          runtime.calls.back() == "hover_first" &&
          runtime.row_count == 3u && runtime.column_count == 2u &&
          runtime.item_costs == std::vector<std::string>{
              "D", "9", "A", "2", "C", "4"} &&
          runtime.allocations == std::vector<std::uint32_t>(6u, 0x210u),
          "preferred IDs insert at front, invalid category is skipped");
    Check(runtime.placed_items ==
              std::vector<std::array<std::uint32_t, 4>>{
                  {0x11000u, 0u, 10u, 100u},
                  {0x12000u, 0u, 11u, 100u},
                  {0x13000u, 1u, 10u, 100u},
                  {0x14000u, 1u, 11u, 100u},
                  {0x15000u, 2u, 10u, 101u},
                  {0x16000u, 2u, 11u, 101u}},
          "property name and quantity cells share ID and correct grid row");

    runtime.hover_item = 0x11000u;
    Check(pal4::CombatPropertySelectWindow::GetHoveredProperty(
              kWindow, runtime) == 0x41000u,
          "enabled hover resolves property by item ID");
    runtime.bytes[0x11000u + 421u] = 1u;
    Check(pal4::CombatPropertySelectWindow::GetHoveredProperty(
              kWindow, runtime) == 0u,
          "disabled property item does not resolve");
    runtime.hover_item = 0u;
    Check(pal4::CombatPropertySelectWindow::GetHoveredProperty(
              kWindow, runtime) == 0u,
          "absent hovered property returns null");
}

void PropertyEnabledSelectionAndNavigation() {
    Runtime runtime;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 52u] = 0x9000u;
    runtime.game_strings = {{0u, "Property"}, {1u, "Count"}};
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.words[kWindow + 68u + 4u * index] =
            0xb100u + 0x100u * index;
    runtime.inventory_groups = {
        {0u, {0x50000u}}, {1u, {}}, {2u, {0x50200u}},
    };
    runtime.words[0x50000u] = 0x41000u;
    runtime.words[0x41000u + 8u] = 6u;
    runtime.words[0x50200u] = 0x43000u;
    runtime.words[0x50200u + 8u] = 4u;
    runtime.words[0x43000u] = 101u;
    runtime.words[0x43000u + 8u] = 5u;
    runtime.words[0x43000u + 20u] = 0x60000u;
    runtime.strings[0x60000u] = "Aux";

    Check(!pal4::CombatPropertySelectWindow::CheckEnabled(0u, runtime) &&
          pal4::CombatPropertySelectWindow::CheckEnabled(1u, runtime) &&
          !pal4::CombatPropertySelectWindow::CheckEnabled(2u, runtime) &&
          !pal4::CombatPropertySelectWindow::CheckEnabled(3u, runtime),
          "button availability maps 0/1/2 to inventory groups 0/2/1");
    pal4::CombatPropertySelectWindow::SelectButton(kWindow, 1u, runtime);
    Check(runtime.words[kWindow + 56u] == 2u &&
          runtime.words[0xb100u + 280u] == 0u &&
          runtime.words[0xb200u + 280u] == 1u &&
          runtime.words[0xb300u + 280u] == 2u &&
          runtime.button_calls == std::vector<std::string>{
              "selected:45312:0", "enabled:45312:0",
              "visible:45312:1", "selected:45568:0",
              "enabled:45568:1", "visible:45568:1",
              "selected:45824:0", "enabled:45824:0",
              "visible:45824:1", "selected:45568:1"} &&
          runtime.row_count == 1u &&
          runtime.item_costs ==
              std::vector<std::string>{"Aux", "4"},
          "selected auxiliary tab refreshes all three buttons and group two");
    Check(pal4::CombatPropertySelectWindow::FindNextEnabledButton(
              kWindow, -1, runtime) == 1 &&
          pal4::CombatPropertySelectWindow::FindNextEnabledButton(
              kWindow, 1, runtime) == 1 &&
          pal4::CombatPropertySelectWindow::FindNextEnabledButton(
              kWindow, 2, runtime) == 2,
          "next navigation skips disabled property buttons");

    runtime.words[kEvent + 8u] = 0xb300u;
    Check(pal4::CombatPropertySelectWindow::HandleButtonClick(
              kWindow, kEvent, runtime) == 1u &&
          runtime.words[kWindow + 56u] == 1u &&
          runtime.row_count == 0u &&
          runtime.button_calls.back() == "selected:45824:1",
          "button click reads sender index and maps attackable to group one");
}

void PropertyClickAndDescription() {
    Runtime runtime;
    runtime.property_mode = true;
    runtime.words[kWindow + 40u] = kList;
    runtime.words[kWindow + 44u] = kDescription;
    runtime.words[kWindow + 56u] = 2u;
    runtime.words[kWindow + 60u] = 0x6600u;
    runtime.words[kWindow + 64u] = 0x7700u;
    Check(pal4::CombatPropertySelectWindow::HandleListClick(
              kWindow, runtime) == 1u &&
          pal4::CombatPropertySelectWindow::UpdatePropertyDescription(
              kWindow, runtime) == 1u &&
          runtime.removed_count == 0u &&
          runtime.callback_magic == 0xffffffffu,
          "empty property hover leaves callback and text untouched");

    runtime.hover_item = 0x11000u;
    runtime.words[0x11000u + 312u] = 100u;
    runtime.properties_by_id[100u] = 0x41000u;
    runtime.words[0x41000u] = 100u;
    runtime.words[0x41000u + 52u] = 0x60000u;
    runtime.strings[0x60000u] = "Cure \xE6\xB2\xBB";
    runtime.bytes[0x11000u + 421u] = 1u;
    Check(pal4::CombatPropertySelectWindow::UpdatePropertyDescription(
              kWindow, runtime) == 1u &&
          pal4::CombatPropertySelectWindow::HandleListClick(
              kWindow, runtime) == 1u &&
          runtime.description_text.empty() &&
          runtime.removed_count == 0u,
          "disabled property cannot update text or trigger selection");
    runtime.bytes[0x11000u + 421u] = 0u;
    Check(pal4::CombatPropertySelectWindow::UpdatePropertyDescription(
              kWindow, runtime) == 1u &&
          runtime.description_text == "Cure \xE6\xB2\xBB",
          "property description reads property+52 and converts UTF-8");
    Check(pal4::CombatPropertySelectWindow::HandleListClick(
              kWindow, runtime) == 1u &&
          runtime.words[kWindow + 88u] == 100u &&
          runtime.callback_magic == 0x41000u &&
          runtime.removed_count == 1u &&
          runtime.calls[runtime.calls.size() - 4u] == "manager" &&
          runtime.calls[runtime.calls.size() - 3u] == "remove_handler" &&
          runtime.calls[runtime.calls.size() - 2u] == "callback" &&
          runtime.calls.back() == "visibility",
          "property click saves ID, removes handler, invokes callback and hides");
}

void StuntConstructorAndSubscriptions() {
    Runtime runtime;
    runtime.stunt_mode = true;
    runtime.expected_layout = pal4::CombatStuntSelectWindow::kLayoutName;
    runtime.expected_group = pal4::CombatStuntSelectWindow::kResourceGroup;
    runtime.words[kWindow + 24u] = 0xd000u;
    runtime.strings[0xd000u] = "Fight/";
    Check(pal4::CombatStuntSelectWindow::Construct(kWindow, runtime) ==
              kWindow &&
          runtime.words[kWindow] == pal4::CombatStuntSelectWindow::kVtable &&
          runtime.words[kWindow + 40u] ==
              pal4::CombatStuntSelectWindow::kFacetVtable &&
          runtime.words[kWindow + 44u] == 0u &&
          runtime.words[kWindow + 48u] == 0u &&
          runtime.words[kWindow + 52u] == 0u &&
          runtime.words[kWindow + 56u] == kList &&
          runtime.words[kWindow + 60u] == kDescription &&
          runtime.names == std::vector<std::string>{
              "Fight/List", "Fight/EditDescription", "Fight/Root"} &&
          runtime.callback_functions == std::vector<std::uint32_t>{
              pal4::CombatStuntSelectWindow::kHoverCallback,
              pal4::CombatStuntSelectWindow::kListClickCallback,
              pal4::CombatStuntSelectWindow::kRootClickCallback} &&
          runtime.subscription_releases == 3u,
          "stunt constructor binds its own facet, list and three callbacks");

    Runtime failed;
    failed.stunt_mode = true;
    failed.fail_large_callback = true;
    failed.expected_layout = pal4::CombatStuntSelectWindow::kLayoutName;
    failed.expected_group = pal4::CombatStuntSelectWindow::kResourceGroup;
    Check(pal4::CombatStuntSelectWindow::Construct(kWindow, failed) ==
              kWindow && failed.callback_functions.size() == 3u &&
          failed.subscription_releases == 3u,
          "failed 24-byte event allocations still follow subscription path");
}

void StuntSetupAndRows() {
    Runtime runtime;
    runtime.stunt_mode = true;
    runtime.words[kWindow + 56u] = kList;
    runtime.words[kWindow + 60u] = kDescription;
    runtime.words[kPair] = 0x1111u;
    runtime.words[kPair + 4u] = 0x2222u;
    runtime.game_strings = {{0u, "Name"}, {0x2au, "Cost"}};
    runtime.column_count = 1u;
    runtime.skill_ids = {10u, 20u, 30u, 40u, 50u};
    runtime.stunts_by_id = {{10u, 0x41000u}, {20u, 0x42000u},
                            {30u, 0x43000u}, {40u, 0x44000u}};
    runtime.words[0x41000u + 4u] = 3u;
    runtime.words[0x42000u + 4u] = 6u;
    runtime.words[0x43000u + 4u] = 2u;
    runtime.words[0x43000u + 12u] = 0x60000u;
    runtime.words[0x43000u + 116u] = 12u;
    runtime.words[0x44000u + 4u] = 0u;
    runtime.words[0x44000u + 12u] = 0x60100u;
    runtime.words[0x44000u + 116u] = 5u;
    runtime.strings[0x60000u] = "Heal";
    runtime.strings[0x60100u] = "Strike";
    runtime.stunt_usable = false;
    Check(pal4::CombatStuntSelectWindow::Setup(
              kWindow, kPair, 0x9000u, 0x6600u, runtime) == 0x6du &&
          runtime.words[kWindow + 44u] == 0x9000u &&
          runtime.words[kWindow + 48u] == 0x1111u &&
          runtime.words[kWindow + 52u] == 0x2222u &&
          runtime.row_count == 2u && runtime.column_count == 2u &&
          runtime.titles == std::vector<std::string>{"Name", "Cost"} &&
          runtime.item_names == std::vector<std::string>{"Heal", "Strike"} &&
          runtime.item_costs == std::vector<std::string>{"12", "5"} &&
          runtime.icons == std::vector<std::string>{
              "skillcure", "skillattack"} &&
          runtime.placed_items.size() == 4u &&
          runtime.placed_items[0][3] == 30u &&
          runtime.placed_items[1][3] == 30u &&
          runtime.placed_items[2][3] == 40u &&
          runtime.placed_items[3][3] == 40u &&
          runtime.checked_stunt_records ==
              std::vector<std::uint32_t>{0x43000u, 0x44000u} &&
          runtime.bytes[runtime.placed_items[0][0] + 421u] == 1u &&
          runtime.bytes[runtime.placed_items[1][0] + 421u] == 1u &&
          runtime.released_iterators == 1u &&
          runtime.calls.back() == "insert",
          "setup filters stunts, builds cells and checks the corresponding Stunt records");
    runtime.hover_item = runtime.placed_items[0][0];
    Check(pal4::CombatStuntSelectWindow::GetHoveredStunt(
              kWindow, runtime) == 0u,
          "unusable stunt cannot be selected");
    runtime.bytes[runtime.hover_item + 421u] = 0u;
    Check(pal4::CombatStuntSelectWindow::GetHoveredStunt(
              kWindow, runtime) == 0x43000u,
          "available hovered stunt resolves its database entry");
}

void StuntListBoundaries() {
    Runtime runtime;
    runtime.stunt_mode = true;
    runtime.words[kWindow + 56u] = kList;
    pal4::CombatStuntSelectWindow::UpdateStuntList(kWindow, runtime);
    Check(runtime.calls.empty() && runtime.row_count == 0u,
          "null actor does not touch stunt list");

    runtime.words[kWindow + 44u] = 0x9000u;
    runtime.game_strings = {{0u, "Name"}, {0x2au, "Cost"}};
    pal4::CombatStuntSelectWindow::UpdateStuntList(kWindow, runtime);
    Check(runtime.row_count == 0u && runtime.column_count == 2u &&
          runtime.released_iterators == 1u &&
          runtime.checked_stunt_records.empty(),
          "empty stunt collection still refreshes columns and releases cursor");

    runtime.skill_ids = {9u};
    runtime.stunts_by_id[9u] = 0x41000u;
    runtime.words[0x41000u + 4u] = 1u;
    runtime.words[0x41000u + 116u] = 0u;
    pal4::CombatStuntSelectWindow::UpdateStuntList(kWindow, runtime);
    Check(runtime.row_count == 1u &&
          runtime.icons == std::vector<std::string>{"skillbuff"} &&
          runtime.item_costs == std::vector<std::string>{"0"} &&
          runtime.checked_stunt_records == std::vector<std::uint32_t>{0x41000u} &&
          runtime.bytes[runtime.placed_items[0][0] + 421u] == 0u &&
          runtime.bytes[runtime.placed_items[1][0] + 421u] == 0u &&
          runtime.released_iterators == 2u,
          "usable buff stunt retains enabled cells and hover row");
}

void StuntDescriptionClickAndCancel() {
    Runtime runtime;
    runtime.stunt_mode = true;
    runtime.words[kWindow + 56u] = kList;
    runtime.words[kWindow + 60u] = kDescription;
    runtime.words[kWindow + 48u] = 0x6600u;
    runtime.words[kWindow + 52u] = 0x7700u;
    Check(pal4::CombatStuntSelectWindow::UpdateStuntDescription(
              kWindow, runtime) == 0u &&
          pal4::CombatStuntSelectWindow::HandleListClick(
              kWindow, runtime) == 1u && runtime.removed_count == 0u,
          "empty hover leaves description and callback untouched");
    runtime.hover_item = 0x11000u;
    runtime.words[0x11000u + 312u] = 30u;
    runtime.stunts_by_id[30u] = 0x43000u;
    runtime.words[0x43000u + 44u] = 0x60000u;
    runtime.strings[0x60000u] = "Stunt description";
    runtime.bytes[0x11000u + 421u] = 1u;
    Check(pal4::CombatStuntSelectWindow::UpdateStuntDescription(
              kWindow, runtime) == 1u &&
          runtime.description_text == "Stunt description" &&
          pal4::CombatStuntSelectWindow::HandleListClick(
              kWindow, runtime) == 1u && runtime.removed_count == 0u,
          "description follows hover even when item is disabled");
    runtime.bytes[0x11000u + 421u] = 0u;
    Check(pal4::CombatStuntSelectWindow::HandleListClick(
              kWindow, runtime) == 1u &&
          runtime.callback_magic == 0x43000u &&
          runtime.removed_count == 1u &&
          runtime.calls[runtime.calls.size() - 4u] == "manager" &&
          runtime.calls[runtime.calls.size() - 3u] == "remove_handler" &&
          runtime.calls[runtime.calls.size() - 2u] == "callback" &&
          runtime.calls.back() == "visibility",
          "stunt click removes facet, invokes callback and hides window");
    runtime.words[kEvent + 28u] = 0u;
    Check(pal4::CombatStuntSelectWindow::HandleRootClick(
              kWindow, kEvent, runtime) == 1u &&
          runtime.removed_count == 1u,
          "root click ignores non-primary button");
    runtime.words[kEvent + 28u] = 1u;
    Check(pal4::CombatStuntSelectWindow::HandleRootClick(
              kWindow, kEvent, runtime) == 1u &&
          runtime.callback_magic == 0u &&
          runtime.removed_count == 2u &&
          runtime.calls[runtime.calls.size() - 3u] == "callback" &&
          runtime.calls[runtime.calls.size() - 2u] == "manager" &&
          runtime.calls.back() == "remove_handler",
          "primary root click cancels through callback then facet removal");
}

void SelectEventRecordConstructors() {
    Runtime runtime;
    constexpr std::uint32_t record = 0xf000u;
    runtime.words[record + 4u] = 0xdeadbeefu;
    Check(pal4::CombatSelectEventRecord::ConstructMagic(
              record, 0x11u, 0x22u, 0x33u, runtime) == record &&
          runtime.words[record] == 0x845610u &&
          runtime.words[record + 4u] == 0xdeadbeefu &&
          runtime.words[record + 8u] == 0x11u &&
          runtime.words[record + 12u] == 0x22u &&
          runtime.words[record + 16u] == 0x33u,
          "magic event record writes three fields and vtable");
    Check(pal4::CombatSelectEventRecord::ConstructProperty(
              record, 0x44u, 0x55u, 0x66u, runtime) == record &&
          runtime.words[record] == 0x84564cu &&
          runtime.words[record + 4u] == 0xdeadbeefu &&
          runtime.words[record + 8u] == 0x44u &&
          runtime.words[record + 12u] == 0x55u &&
          runtime.words[record + 16u] == 0x66u,
          "property event record leaves word +4 untouched");
}

void StuntKeyboardRouting() {
    struct Navigation final : pal4::ICombatSelectListNavigationRuntime {
        unsigned up{}, down{};
        void UpHoverItem(std::uint32_t list) override { Check(list==kList,"Stunt Up uses its own list offset");++up; }
        void DownHoverItem(std::uint32_t list) override { Check(list==kList,"Stunt Down uses its own list offset");++down; }
    } navigation;
    Runtime runtime;runtime.stunt_mode=true;
    runtime.words[kWindow+56u]=kList;
    runtime.words[kWindow+48u]=0x6600u;runtime.words[kWindow+52u]=0x7700u;
    const auto key=[&](std::uint32_t code){runtime.words[kEvent+16u]=code;return pal4::CombatStuntSelectWindow::HandleKeyDown(kWindow,kEvent,runtime,navigation);};
    Check(key(0xd0u)==1u && key(0xc8u)==1u && navigation.up==1u && navigation.down==1u,"Stunt keys route DLL hover navigation");
    Check(key(0xcbu)==0u && runtime.calls.empty(),"Stunt does not consume Magic element navigation keys");
    Check(key(0x1cu)==1u && runtime.removed_count==0u,"empty Stunt Enter leaves selection active");
    runtime.hover_item=0x11000u;runtime.words[runtime.hover_item+312u]=30u;runtime.stunts_by_id[30u]=0x43000u;
    runtime.bytes[runtime.hover_item+421u]=1u;
    Check(key(0x1cu)==1u && runtime.removed_count==0u,"unusable Stunt Enter does not publish selection");
    runtime.bytes[runtime.hover_item+421u]=0u;
    Check(key(0x1cu)==1u && runtime.callback_magic==0x43000u && runtime.removed_count==1u,"affordable Stunt Enter selects original record");
    Check(key(0x01u)==1u && runtime.callback_magic==0u && runtime.removed_count==2u,"Stunt Escape cancels through original callback and facet removal");
}

}  // namespace

int main() {
    try {
        ConstructorAndControls();
        EmptyPrefixAndAllocationFailure();
        RootClickGate();
        DerivedConstructor();
        ButtonInitializationAllocationFailure();
        SetupAndFirstEnabledButton();
        MagicListGatesAndEmptyCollection();
        MagicListRowsAndAffordability();
        ButtonSelectionAndClickBridge();
        ButtonNavigation();
        KeyboardRouting();
        ListClickDescriptionAndCleanup();
        PropertyConstructorAndButtons();
        PropertySetupAndAllocationFailure();
        PropertyListGatesAndEmptyInventory();
        PropertyListPartitionAndHover();
        PropertyEnabledSelectionAndNavigation();
        PropertyClickAndDescription();
        StuntConstructorAndSubscriptions();
        StuntSetupAndRows();
        StuntListBoundaries();
        StuntDescriptionClickAndCancel();
        StuntKeyboardRouting();
        SelectEventRecordConstructors();
        std::cout << "combat_magic_select_window_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
