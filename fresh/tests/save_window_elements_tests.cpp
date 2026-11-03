#include "pal4/save_window_elements.h"

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

struct Subscription {
    std::uint32_t window{};
    std::string event;
    std::uint32_t callback{};
};

struct Runtime final : pal4::SaveWindowElementsRuntimeStub {
    std::vector<std::string> names;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> task_indices;
    std::vector<std::string> list_calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next_callback{0xa000u};
    std::uint32_t fail_lookup_index{0xffffffffu};
    std::uint32_t columns{2u};
    std::uint32_t text_count{};
    bool null_callbacks{};
    bool mask_disabled{};
    bool static_mask_formatted{};

    std::string ReadPrefix(std::uint32_t address) override {
        Check(address == 0x9000u, "layout prefix at +0x18");
        return "SaveWindow/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        if (names.size() == fail_lookup_index)
            throw std::runtime_error("lookup");
        names.emplace_back(name);
        return 0x2000u + static_cast<std::uint32_t>(names.size()) * 0x100u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "event callback is 0x18 bytes");
        if (null_callbacks) return 0u;
        const auto value = next_callback;
        next_callback += 0x20u;
        return value;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::string_view event,
                            std::uint32_t callback) override {
        subscriptions.push_back({window, std::string(event), callback});
        return 0xc000u + static_cast<std::uint32_t>(subscriptions.size()) * 4u;
    }
    std::uint32_t ReferenceCount(std::uint32_t) override { return 1u; }
    void SetReferenceCount(std::uint32_t,
                           std::uint32_t count) override {
        Check(count == 0u, "subscription local owner decremented");
    }
    void DeleteReference(std::uint32_t reference,
                         std::uint32_t flags) override {
        Check(flags == 1u, "zero-ref subscription owner deleted");
        deleted.push_back(reference);
    }
    std::string TriggerItemText(std::uint32_t index) override {
        task_indices.push_back(index);
        return "raw";
    }
    std::string ConvertToUtf8(std::string_view raw) override {
        Check(raw == "raw", "GameDB text converted to UTF-8");
        return "A\xE4\xB8\xAD";
    }
    void SetText(std::uint32_t window,
                 std::u32string_view text) override {
        Check(window == 0x2100u + text_count * 0x100u &&
                  text == U"A\u4e2d", "button receives converted task name");
        ++text_count;
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check(window == 0x2d00u && !enabled,
              "ImgSelectMask initially disabled");
        mask_disabled = true;
    }
    void SetHorizontalFormatting(std::uint32_t window,
                                 std::uint32_t mode) override {
        Check(window == 0x2e00u && mode == 2u,
              "StaticMask horizontal format is two");
        static_mask_formatted = true;
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x2c00u, "column count uses List control");
        list_calls.emplace_back("count");
        return columns;
    }
    void RemoveColumn(std::uint32_t list,
                      std::uint32_t index) override {
        Check(list == 0x2c00u && index == 0u && columns != 0u,
              "existing columns removed at zero");
        --columns;
        list_calls.emplace_back("remove");
    }
    void SetSortDirection(std::uint32_t list,
                          std::uint32_t direction) override {
        Check(list == 0x2c00u && direction == 0u, "sort direction reset");
        list_calls.emplace_back("sort");
    }
    void SetUserColumnSizingEnabled(std::uint32_t list,
                                     bool enabled) override {
        Check(list == 0x2c00u && !enabled, "column sizing disabled");
        list_calls.emplace_back("size");
    }
    void SetUserColumnDraggingEnabled(std::uint32_t list,
                                       bool enabled) override {
        Check(list == 0x2c00u && !enabled, "column dragging disabled");
        list_calls.emplace_back("drag");
    }
    void SetUserSortControlEnabled(std::uint32_t list,
                                    bool enabled) override {
        Check(list == 0x2c00u && !enabled, "user sort disabled");
        list_calls.emplace_back("user-sort");
    }
    void AddColumn(std::uint32_t list, std::u32string_view title,
                   std::uint32_t id, float width) override {
        Check(list == 0x2c00u && title.empty() && id == 0u &&
                  width == 1.0f, "single blank full-width column added");
        list_calls.emplace_back("add");
    }
};

void CheckElements() {
    pal4::SaveWindowState state{};
    state.Write(0x18u, 0x9000u);
    Runtime runtime;
    pal4::SaveWindowElements::Initialize(state, 0x6000u, runtime);
    Check(runtime.names.size() == 14u &&
              runtime.subscriptions.size() == 13u &&
              runtime.deleted.size() == 13u &&
              runtime.task_indices.size() == 10u &&
              runtime.text_count == 10u && runtime.mask_disabled &&
              runtime.static_mask_formatted,
          "ten save buttons and four auxiliary controls initialized");
    for (std::uint32_t i = 0u; i < 10u; ++i) {
        const auto button = state.Read(0x2cu + i * 4u);
        Check(runtime.names[i] ==
                  "SaveWindow/BtnLoad" + std::to_string(i) &&
                  button == 0x2100u + i * 0x100u &&
                  runtime.words.at(button + 0x118u) == i &&
                  runtime.task_indices[i] == 205u + i &&
                  runtime.subscriptions[i].window == button &&
                  runtime.subscriptions[i].event == "Clicked" &&
                  runtime.words.at(runtime.subscriptions[i].callback + 8u) ==
                      0x44d650u,
              "button names, slots, indices, GameDB IDs and handlers");
    }
    Check(runtime.names[10] == "SaveWindow/BtnClose" &&
              runtime.names[11] == "SaveWindow/List" &&
              runtime.names[12] == "SaveWindow/ImgSelectMask" &&
              runtime.names[13] == "SaveWindow/StaticMask" &&
              state.Read(0x54u) == 0x2c00u &&
              state.Read(0x58u) == 0x2b00u &&
              state.Read(0x5cu) == 0x2d00u &&
              state.Read(0x60u) == 0x2e00u,
          "auxiliary names and slots");
    Check(runtime.subscriptions[10].event == "Clicked" &&
              runtime.words.at(runtime.subscriptions[10].callback + 8u) ==
                  0x4b6890u &&
              runtime.subscriptions[11].event == "MouseClick" &&
              runtime.words.at(runtime.subscriptions[11].callback + 8u) ==
                  0x44d670u &&
              runtime.subscriptions[12].event == "HoverRowChanged" &&
              runtime.words.at(runtime.subscriptions[12].callback + 8u) ==
                  0x447d60u,
          "close and list callbacks use observed handlers");
    for (const auto& sub : runtime.subscriptions) {
        const auto callback = sub.callback;
        Check(runtime.words.at(callback) == 0x8437b0u &&
                  runtime.words.at(callback + 0xcu) == 0u &&
                  runtime.words.at(callback + 0x10u) == 0x6000u,
              "callback vtable, zero field and owner");
    }
    Check(runtime.list_calls == std::vector<std::string>{
              "count", "remove", "count", "remove", "count", "sort",
              "size", "drag", "user-sort", "add"},
          "list columns cleared and settings applied in order");
}

void CheckFailures() {
    pal4::SaveWindowState state{};
    state.Write(0x18u, 0x9000u);
    Runtime runtime;
    runtime.null_callbacks = true;
    pal4::SaveWindowElements::Initialize(state, 0x6000u, runtime);
    Check(runtime.subscriptions.size() == 13u &&
              runtime.subscriptions.front().callback == 0u &&
              runtime.words.size() == 10u,
          "null callback allocation still subscribes and tags buttons");
    state = {};
    state.Write(0x18u, 0x9000u);
    runtime = Runtime{};
    runtime.fail_lookup_index = 1u;
    try {
        pal4::SaveWindowElements::Initialize(state, 0x6000u, runtime);
        Check(false, "second lookup failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.names.size() == 1u &&
                  runtime.subscriptions.size() == 1u &&
                  runtime.text_count == 1u &&
                  state.Read(0x2cu) == 0x2100u &&
                  state.Read(0x30u) == 0u,
              "failure leaves first button initialized");
    }
}

}  // namespace

int main() {
    CheckElements();
    CheckFailures();
    std::cout << "Save window element tests passed\n";
}
