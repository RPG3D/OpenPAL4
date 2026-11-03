#include "pal4/load_window_elements.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Subscription {
    std::uint32_t window{};
    pal4::LoadWindowEvent event{};
    std::uint32_t handler{};
    std::uint32_t callback{};
};

struct Runtime final : pal4::LoadWindowElementsRuntimeStub {
    std::vector<std::string> names;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> ids;
    std::vector<Subscription> subscriptions;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::vector<std::string> built;
    std::vector<std::uint32_t> destroyed_strings;
    std::vector<std::uint32_t> deleted_references;
    std::vector<std::uint32_t> item_ids;
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> callback_writes;
    std::string item_text;
    std::uint32_t next_callback = 0x6000u;
    std::uint32_t columns = 3u;
    std::uint32_t cleaned{};
    std::uint32_t removed_columns{};
    std::uint32_t disabled_mask{};
    std::uint32_t formatted_mask{};
    std::uint32_t sorted_list{};
    std::uint32_t sizing_list{};
    std::uint32_t dragging_list{};
    std::uint32_t sort_control_list{};
    std::uint32_t added_list{};
    std::uint32_t null_callback_index = 0xffffffffu;
    std::uint32_t fail_text_window{};
    bool fail_add_column{};
    bool append_nul{};

    void InitializeNameBuffer() override {
        calls.emplace_back("initialize-name");
    }
    std::string_view LayoutPrefix(
        const pal4::LoadWindowState& state) override {
        Check(state.Read(0x18u) == 0x7777u,
              "layout prefix is backed by state +0x18");
        calls.emplace_back("prefix");
        return "loadWindow/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        names.emplace_back(name);
        calls.emplace_back("find");
        if (name.starts_with("loadWindow/BtnLoad")) {
            const auto suffix = name.substr(18u);
            Check(suffix.size() == 1u && suffix[0] >= '0' &&
                      suffix[0] <= '9',
                  "button names use BtnLoad0 through BtnLoad9");
            return 0x1000u + static_cast<std::uint32_t>(suffix[0] - '0');
        }
        if (name == "loadWindow/BtnClose") return 0x2000u;
        if (name == "loadWindow/List") return 0x3000u;
        if (name == "loadWindow/ImgSelectMask") return 0x4000u;
        if (name == "loadWindow/StaticMask") return 0x5000u;
        Check(false, "lookup name belongs to load dialog");
        return 0u;
    }
    void SetWindowUserId(std::uint32_t window,
                         std::uint32_t id) override {
        ids.emplace_back(window, id);
        calls.emplace_back("id");
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "event callback allocation is 0x18 bytes");
        calls.emplace_back("allocate-callback");
        const auto index = static_cast<std::uint32_t>(subscriptions.size());
        if (index == null_callback_index) return 0u;
        const auto result = next_callback;
        next_callback += 0x20u;
        return result;
    }
    void WriteCallbackWord(std::uint32_t address,
                           std::uint32_t value) override {
        const auto base = address & ~0x1fu;
        callback_writes[base].push_back(address - base);
        const auto offset = address - base;
        if (offset == 0u) {
            Check(value == pal4::LoadWindowElements::kCallbackVtable,
                  "callback vtable is 0x8435bc");
        } else if (offset == 0x10u) {
            Check(value == 0x900u, "callback owner is load object");
        } else if (offset == 0xcu) {
            Check(value == 0u, "callback third field is zero");
        } else {
            Check(offset == 8u, "callback handler is at +8");
        }
        calls.emplace_back("write-callback");
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::LoadWindowEvent event,
                            std::uint32_t callback) override {
        const auto index = static_cast<std::uint32_t>(subscriptions.size());
        std::uint32_t handler{};
        if (index < 10u) {
            handler = pal4::LoadWindowElements::kPopulateHandler;
        } else if (index == 10u) {
            handler = pal4::LoadWindowElements::kCloseHandler;
        } else if (index == 11u) {
            handler = pal4::LoadWindowElements::kMouseClickHandler;
        } else {
            handler = pal4::LoadWindowElements::kHoverHandler;
        }
        subscriptions.push_back({window, event, handler, callback});
        calls.emplace_back("subscribe");
        return 0x7000u + index * 0x10u;
    }
    std::uint32_t ReferenceCount(std::uint32_t reference) override {
        Check(reference >= 0x7000u,
              "subscription returns a reference object");
        calls.emplace_back("reference-count");
        return reference == 0x7010u ? 2u : 1u;
    }
    void SetReferenceCount(std::uint32_t,
                           std::uint32_t count) override {
        Check(count <= 1u, "local subscription reference is decremented");
        calls.emplace_back("set-reference-count");
    }
    void DeleteCallback(std::uint32_t reference,
                        std::uint32_t flags) override {
        Check(flags == 1u, "zero reference deletes with flag one");
        deleted_references.push_back(reference);
        calls.emplace_back("delete-reference");
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x8000u;
    }
    std::uint32_t TriggerItem(std::uint32_t member,
                              std::uint32_t id) override {
        Check(member == 0x8034u && id >= 205u && id < 215u,
              "item record uses GameDB +0x34 and IDs 205 through 214");
        item_ids.push_back(id);
        calls.emplace_back("record");
        return 0xa000u + id;
    }
    std::string_view ItemText(std::uint32_t record) override {
        Check(record >= 0xa0cdu && record <= 0xa0d6u,
              "item text comes from record +4");
        item_text = "Item" + std::to_string(record - 0xa000u);
        calls.emplace_back("item-text");
        return item_text;
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        if (append_nul) return std::string(text) + std::string("\0tail", 5);
        return std::string(text);
    }
    std::uint32_t CreateCeguiString(std::string_view text) override {
        built.emplace_back(text);
        calls.emplace_back("create-text");
        return 0xb000u + static_cast<std::uint32_t>(built.size());
    }
    void SetText(std::uint32_t window,
                 std::uint32_t text) override {
        Check(text >= 0xb001u, "window receives temporary CEGUI text");
        texts.emplace_back(window, built.at(text - 0xb001u));
        calls.emplace_back("set-text");
        if (window == fail_text_window) throw std::runtime_error("set-text");
    }
    void DestroyCeguiString(std::uint32_t text) override {
        destroyed_strings.push_back(text);
        calls.emplace_back("destroy-text");
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check(!enabled, "selection mask is disabled");
        disabled_mask = window;
        calls.emplace_back("disable-mask");
    }
    void SetHorizontalFormatting(std::uint32_t window,
                                 std::uint32_t mode) override {
        Check(mode == 2u, "static mask uses formatting mode two");
        formatted_mask = window;
        calls.emplace_back("format-mask");
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x3000u, "column operations target List window");
        calls.emplace_back("column-count");
        return columns;
    }
    void RemoveColumn(std::uint32_t list,
                      std::uint32_t index) override {
        Check(list == 0x3000u && index == 0u && columns > 0u,
              "column zero is repeatedly removed");
        --columns;
        ++removed_columns;
        calls.emplace_back("remove-column");
    }
    void SetSortDirection(std::uint32_t list,
                          std::uint32_t direction) override {
        Check(direction == 0u, "sort direction is zero");
        sorted_list = list;
        calls.emplace_back("sort-direction");
    }
    void SetUserColumnSizingEnabled(std::uint32_t list,
                                    bool enabled) override {
        Check(!enabled, "user sizing is disabled");
        sizing_list = list;
        calls.emplace_back("sizing");
    }
    void SetUserColumnDraggingEnabled(std::uint32_t list,
                                      bool enabled) override {
        Check(!enabled, "user dragging is disabled");
        dragging_list = list;
        calls.emplace_back("dragging");
    }
    void SetUserSortControlEnabled(std::uint32_t list,
                                   bool enabled) override {
        Check(!enabled, "user sort control is disabled");
        sort_control_list = list;
        calls.emplace_back("sort-control");
    }
    void AddColumn(std::uint32_t list,
                   std::uint32_t text,
                   std::uint32_t id, float width) override {
        Check(list == 0x3000u && id == 0u && width == 1.0f &&
                  built.at(text - 0xb001u).empty(),
              "one full-width empty header column is added");
        added_list = list;
        calls.emplace_back("add-column");
        if (fail_add_column) throw std::runtime_error("add-column");
    }
    std::uint32_t CleanupNameBuffer() override {
        ++cleaned;
        calls.emplace_back("cleanup-name");
        return 0xcafeu;
    }
};

pal4::LoadWindowState MakeState() {
    pal4::LoadWindowState state{};
    state.Write(0x18u, 0x7777u);
    return state;
}

void CheckNormal() {
    auto state = MakeState();
    Runtime runtime;
    runtime.append_nul = true;
    Check(pal4::LoadWindowElements::Initialize(
              state, 0x900u, runtime) == 0xcafeu &&
              runtime.names.size() == 14u &&
              runtime.ids.size() == 10u &&
              runtime.subscriptions.size() == 13u &&
              runtime.item_ids.size() == 10u &&
              runtime.texts.size() == 10u &&
              runtime.built.size() == 11u &&
              runtime.destroyed_strings.size() == 11u &&
              runtime.deleted_references.size() == 12u &&
              runtime.cleaned == 1u,
          "load controls, subscriptions, item texts and cleanup all run");
    for (std::uint32_t index = 0u; index < 10u; ++index) {
        Check(runtime.names[index] ==
                  "loadWindow/BtnLoad" + std::to_string(index) &&
                  state.Read(0x2cu + index * 4u) == 0x1000u + index &&
                  runtime.ids[index] ==
                      std::pair<std::uint32_t, std::uint32_t>{
                          0x1000u + index, index} &&
                  runtime.item_ids[index] == 205u + index &&
                  runtime.texts[index] ==
                      std::pair<std::uint32_t, std::string>{
                          0x1000u + index,
                          "Item" + std::to_string(205u + index)} &&
                  runtime.subscriptions[index].event ==
                      pal4::LoadWindowEvent::Clicked &&
                  runtime.subscriptions[index].handler ==
                      pal4::LoadWindowElements::kPopulateHandler,
              "each load button has its indexed state, callback and item text");
    }
    Check(runtime.names[10] == "loadWindow/BtnClose" &&
              runtime.names[11] == "loadWindow/List" &&
              runtime.names[12] == "loadWindow/ImgSelectMask" &&
              runtime.names[13] == "loadWindow/StaticMask" &&
              state.Read(0x58u) == 0x2000u &&
              state.Read(0x54u) == 0x3000u &&
              state.Read(0x5cu) == 0x4000u &&
              state.Read(0x60u) == 0x5000u &&
              runtime.subscriptions[10].handler ==
                  pal4::LoadWindowElements::kCloseHandler &&
              runtime.subscriptions[11].event ==
                  pal4::LoadWindowEvent::MouseClick &&
              runtime.subscriptions[12].event ==
                  pal4::LoadWindowEvent::HoverRowChanged &&
              runtime.disabled_mask == 0x4000u &&
              runtime.formatted_mask == 0x5000u,
          "close, list and masks use observed names, slots and event types");
    Check(runtime.removed_columns == 3u && runtime.columns == 0u &&
              runtime.sorted_list == 0x3000u &&
              runtime.sizing_list == 0x3000u &&
              runtime.dragging_list == 0x3000u &&
              runtime.sort_control_list == 0x3000u &&
              runtime.added_list == 0x3000u &&
              runtime.built.back().empty(),
          "list columns are cleared, configured and replaced by one column");
    for (const auto& [callback, offsets] : runtime.callback_writes) {
        (void)callback;
        Check(offsets == std::vector<std::uint32_t>{0x10u, 8u, 0xcu, 0u},
              "callback owner, handler, zero and vtable write order");
    }
}

void CheckFailures() {
    auto state = MakeState();
    Runtime runtime;
    runtime.null_callback_index = 0u;
    Check(pal4::LoadWindowElements::Initialize(
              state, 0x900u, runtime) == 0xcafeu &&
              runtime.subscriptions[0].callback == 0u,
          "null callback allocation still subscribes the event");

    runtime = Runtime{};
    runtime.fail_text_window = 0x1003u;
    try {
        (void)pal4::LoadWindowElements::Initialize(
            state, 0x900u, runtime);
        Check(false, "setText exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.names.size() == 4u &&
                  runtime.destroyed_strings.size() == 4u &&
                  runtime.cleaned == 1u,
              "text failure destroys temporary CEGUI text and name buffer");
    }

    runtime = Runtime{};
    runtime.fail_add_column = true;
    try {
        (void)pal4::LoadWindowElements::Initialize(
            state, 0x900u, runtime);
        Check(false, "addColumn exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.destroyed_strings.size() == 11u &&
                  runtime.cleaned == 1u &&
                  runtime.added_list == 0x3000u,
              "column failure destroys header text and name buffer");
    }
}

}  // namespace

int main() {
    CheckNormal();
    CheckFailures();
    std::cout << "Load window elements tests passed\n";
}
