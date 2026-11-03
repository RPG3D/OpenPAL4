#include "pal4/message_box_dialog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Subscription {
    std::uint32_t window;
    std::uint32_t event;
    std::uint32_t handler;
};

struct Visibility {
    std::uint32_t window;
    bool visible;
    bool operator==(const Visibility&) const = default;
};

struct Event19 {
    std::uint32_t range;
    std::uint32_t end;
    std::uint32_t count;
    std::uint32_t field;
    bool operator==(const Event19&) const = default;
};

struct Runtime final : pal4::IMessageBoxDialogRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::unordered_map<std::uint32_t, float> heights;
    std::unordered_map<std::uint32_t, float> widths;
    std::unordered_map<std::uint32_t, float> xs;
    std::unordered_map<std::uint32_t, float> ys;
    std::vector<std::string> names;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> released;
    std::vector<std::string> cleanup_calls;
    std::vector<std::string> show_calls;
    std::vector<Visibility> visibility;
    std::vector<Event19> events19;
    std::vector<std::string> sounds;
    std::vector<std::uint32_t> allocations;
    std::string current_name;
    std::uint32_t next_memory = 0xa000u;
    std::uint32_t next_connection = 0xb000u;
    std::uint32_t initialized = 0u;
    std::uint32_t destroyed = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t layout = 0u;
    std::uint32_t ended = 0u;
    std::uint32_t allocation_count = 0u;
    std::uint32_t fail_allocation = 0u;
    std::uint32_t fail_find = 0u;
    bool zero_connection = false;
    std::string conversion_input;
    std::string converted_text;
    std::u32string shown_text;
    std::uint32_t show_count = 0u;
    std::uint32_t cloned_handler = 0u;
    std::uint32_t sheet_root = 0u;
    std::uint32_t positioned_root = 0u;
    std::uint32_t position_metrics = ~0u;
    float position_x = 0.0f;
    float position_y = 0.0f;
    std::uint32_t activated = 0u;
    std::uint32_t modal_root = 0u;
    std::uint32_t closed_dialog = 0u;
    std::uint32_t invoked_handler = 0u;
    std::uint32_t invoked_selection = 0u;
    std::uint32_t deleted_handler = 0u;

    void LoadLayout(std::uint32_t dialog,
                    std::uint32_t layout_name) override {
        Check(dialog == 0x1000u, "layout dialog address");
        layout = layout_name;
        words[dialog + 36u] = 0x8000u;
    }
    void InitializeSubobject(std::uint32_t address) override {
        initialized = address;
    }
    std::uint32_t DestroyBase(std::uint32_t dialog) override {
        cleanup_calls.emplace_back("base");
        destroyed = dialog;
        return 0xdeadu;
    }
    void DeleteHandler(std::uint32_t handler,
                       std::uint32_t flags) override {
        Check(flags == 1u, "handler delete flag");
        deleted_handler = handler;
        cleanup_calls.emplace_back("delete");
        show_calls.emplace_back("delete");
    }
    void Free(std::uint32_t address) override {
        cleanup_calls.emplace_back("free");
        show_calls.emplace_back("free");
        freed = address;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t BeginName(std::uint32_t source) override {
        Check(source == 0x1014u, "name source address");
        return 0x9000u;
    }
    void ResetName(std::uint32_t buffer,
                   std::uint32_t source) override {
        Check(buffer == 0x9000u && source == 0x1014u,
              "name reset arguments");
        current_name = "Dialog/";
    }
    void AppendName(std::uint32_t buffer,
                    std::string_view suffix) override {
        Check(buffer == 0x9000u, "name buffer address");
        current_name += suffix;
    }
    std::uint32_t FindWindow(std::uint32_t buffer) override {
        Check(buffer == 0x9000u, "window lookup buffer");
        names.push_back(current_name);
        if (names.size() == fail_find)
            throw std::runtime_error("find window");
        return 0x8100u + static_cast<std::uint32_t>(names.size()) * 0x100u;
    }
    void EndName(std::uint32_t buffer) override {
        Check(buffer == 0x9000u, "name buffer release");
        ++ended;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u || bytes == 4u, "callback allocation size");
        allocations.push_back(bytes);
        ++allocation_count;
        if (allocation_count == fail_allocation) return 0u;
        const auto memory = next_memory;
        next_memory += 0x100u;
        return memory;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::uint32_t event,
                            std::uint32_t handler) override {
        subscriptions.push_back({window, event, handler});
        if (zero_connection) return 0u;
        const auto connection = next_connection;
        next_connection += 0x100u;
        return connection;
    }
    void ReleaseConnection(std::uint32_t connection) override {
        released.push_back(connection);
    }
    std::string ConvertToUtf8(std::string_view text) override {
        conversion_input = text;
        return converted_text;
    }
    std::uint32_t CloneHandler(std::uint32_t handler) override {
        show_calls.emplace_back("clone");
        cloned_handler = handler;
        return handler + 0x10u;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        show_calls.emplace_back("visible");
        visibility.push_back({window, visible});
    }
    void ProcessEvent19(std::uint32_t range, std::uint32_t end,
                        std::uint32_t count,
                        std::uint32_t field) override {
        show_calls.emplace_back("event");
        events19.push_back({range, end, count, field});
    }
    void PlaySound(std::string_view file, std::uint32_t repeat) override {
        Check(repeat == 1u, "sound repeat argument");
        show_calls.emplace_back("sound");
        sounds.emplace_back(file);
    }
    void SetText(std::uint32_t window,
                 std::u32string_view text) override {
        Check(window == words.at(0x104cu), "message control");
        show_calls.emplace_back("text");
        ++show_count;
        shown_text = text;
    }
    void AddToSheet(std::uint32_t root) override {
        show_calls.emplace_back("sheet");
        sheet_root = root;
    }
    void SetPosition(std::uint32_t root, std::uint32_t metrics,
                     float x, float y) override {
        show_calls.emplace_back("position");
        positioned_root = root;
        position_metrics = metrics;
        position_x = x;
        position_y = y;
    }
    void SetAlwaysOnTop(std::uint32_t window, bool enabled) override {
        show_calls.emplace_back("top");
        Check(window == 0x8000u && enabled, "root always on top");
    }
    float GetTextHeight(std::uint32_t window) override {
        Check(window == 0x8400u, "text height control");
        return 24.0f;
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
    void SetHeight(std::uint32_t window, std::uint32_t metrics,
                   float height) override {
        Check(metrics == 1u, "height pixel metrics");
        show_calls.emplace_back("height");
        heights[window] = height;
    }
    float GetY(std::uint32_t window, std::uint32_t metrics) override {
        Check(metrics == 1u, "y pixel metrics");
        return ys.at(window);
    }
    void SetY(std::uint32_t window, std::uint32_t metrics,
              float y) override {
        Check(metrics == 1u, "set y pixel metrics");
        show_calls.emplace_back("y");
        ys[window] = y;
    }
    float GetX(std::uint32_t window, std::uint32_t metrics) override {
        Check(metrics == 1u, "x pixel metrics");
        return xs.at(window);
    }
    float GetWidth(std::uint32_t window, std::uint32_t metrics) override {
        Check(metrics == 1u, "width pixel metrics");
        return widths.at(window);
    }
    void SetX(std::uint32_t window, std::uint32_t metrics,
              float x) override {
        Check(metrics == 1u, "set x pixel metrics");
        show_calls.emplace_back("x");
        xs[window] = x;
    }
    float GetHeight(std::uint32_t window,
                    std::uint32_t metrics) override {
        Check(metrics == 1u, "get height pixel metrics");
        return heights.at(window);
    }
    void ActivateSubobject(std::uint32_t address) override {
        show_calls.emplace_back("activate");
        activated = address;
    }
    void SetModal(std::uint32_t window, bool enabled) override {
        show_calls.emplace_back("modal");
        Check(enabled, "modal enabled");
        modal_root = window;
    }
    void CloseWindow(std::uint32_t dialog) override {
        show_calls.emplace_back("close");
        closed_dialog = dialog;
    }
    void InvokeHandler(std::uint32_t handler,
                       std::uint32_t selection) override {
        show_calls.emplace_back("invoke");
        invoked_handler = handler;
        invoked_selection = selection;
    }
};

void SeedShow(Runtime& runtime) {
    runtime.words[0x1024u] = 0x8000u;
    runtime.words[0x1030u] = 0x9000u;
    runtime.words[0x1034u] = 0x9008u;
    runtime.words[0x1040u] = 0x8100u;
    runtime.words[0x1044u] = 0x8200u;
    runtime.words[0x1048u] = 0x8300u;
    runtime.words[0x104cu] = 0x8400u;
    runtime.words[0x1050u] = 0x8500u;
    runtime.words[0x1054u] = 0x8600u;
    runtime.words[0x105cu] = 0u;
    runtime.floats[0x8400u + 272u] = 20.0f;
    runtime.floats[0x8500u + 272u] = 100.0f;
    runtime.heights[0x8500u] = 100.0f;
    runtime.heights[0x8600u] = 20.0f;
    runtime.widths[0x8500u] = 200.0f;
    runtime.xs[0x8000u] = 300.0f;
    runtime.ys[0x8000u] = 200.0f;
    runtime.ys[0x8600u] = 10.0f;
    runtime.ys[0x8100u] = 20.0f;
    runtime.ys[0x8200u] = 30.0f;
    runtime.ys[0x8300u] = 40.0f;
}

void CheckConstruction() {
    Runtime runtime;
    using Dialog = pal4::MessageBoxDialog;
    Check(Dialog::Construct(0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.layout == 0x7000u &&
              runtime.initialized == 0x1028u &&
              runtime.destroyed == 0u && runtime.ended == 1u &&
              runtime.words.at(0x1000u) == Dialog::kFrameVtable &&
              runtime.words.at(0x1028u) == Dialog::kSubobjectVtable &&
              runtime.words.at(0x1058u) == 0u &&
              runtime.words.at(0x105cu) == 0u,
          "message dialog establishes layout, subobject and vtables");
    Check(runtime.names ==
              (std::vector<std::string>{"Dialog/BtOk", "Dialog/BtYes",
                                        "Dialog/BtNo", "Dialog/Message",
                                        "Dialog/FrameMessage",
                                        "Dialog/FrameButton"}),
          "six controls use the exact IDA suffixes in order");
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        Check(runtime.words.at(0x1040u + index * 4u) ==
                  0x8200u + index * 0x100u,
              "control stored at its verified dialog offset");
    }
    Check(runtime.words.at(0x8200u + 280u) == 4u &&
              runtime.words.at(0x8300u + 280u) == 1u &&
              runtime.words.at(0x8400u + 280u) == 2u,
          "button properties match OK, Yes and No identifiers");
    Check(runtime.subscriptions.size() == 4u,
          "three click callbacks and one key callback are installed");
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto& subscription = runtime.subscriptions[index];
        Check(subscription.window == 0x8200u + index * 0x100u &&
                  subscription.event == Dialog::kClickedEvent &&
                  subscription.handler == 0xa000u + index * 0x100u &&
                  runtime.words.at(subscription.handler) ==
                      Dialog::kKeyHandlerVtable &&
                  runtime.words.at(subscription.handler + 8u) ==
                      Dialog::kOnClick &&
                  runtime.words.at(subscription.handler + 12u) == 0u &&
                  runtime.words.at(subscription.handler + 16u) == 0x1000u,
              "button subscribes to clicked with its own handler");
    }
    Check(runtime.subscriptions[3].window == 0x8000u &&
              runtime.subscriptions[3].event == Dialog::kKeyDownEvent &&
              runtime.subscriptions[3].handler == 0xa300u &&
              runtime.words.at(0xa300u) == Dialog::kKeyHandlerVtable &&
              runtime.words.at(0xa308u) == Dialog::kOnKeyDown &&
              runtime.words.at(0xa30cu) == 0u &&
              runtime.words.at(0xa310u) == 0x1000u &&
              runtime.released ==
                  (std::vector<std::uint32_t>{0xb000u, 0xb100u,
                                              0xb200u, 0xb300u}),
          "root key handler and temporary connections are initialized");
}

void CheckNullAllocationsAndConnections() {
    Runtime runtime;
    runtime.fail_allocation = 1u;
    runtime.zero_connection = true;
    (void)pal4::MessageBoxDialog::Construct(0x1000u, 0x7000u, runtime);
    Check(runtime.subscriptions.size() == 4u &&
              runtime.subscriptions[0].handler == 0u &&
              runtime.words.at(0xa100u + 8u) ==
                  pal4::MessageBoxDialog::kOnClick &&
              runtime.released.empty() &&
              runtime.words.at(0x8200u + 280u) == 4u,
          "null callback allocation still subscribes and sets button ID");
}

void CheckExceptionCleanup() {
    Runtime runtime;
    runtime.fail_find = 4u;
    bool caught = false;
    try {
        (void)pal4::MessageBoxDialog::Construct(
            0x1000u, 0x7000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.ended == 1u &&
              runtime.destroyed == 0x1000u &&
              runtime.names.size() == 4u,
          "failed control lookup releases name and base dialog");
}

void CheckDestruction() {
    Runtime runtime;
    runtime.words[0x1030u] = 0x9000u;
    runtime.words[0x1034u] = 0x9100u;
    runtime.words[0x1038u] = 0x9200u;
    runtime.words[0x105cu] = 0u;
    Check(pal4::MessageBoxDialog::Destruct(0x1000u, runtime) ==
              0xdeadu &&
              runtime.words.at(0x1000u) ==
                  pal4::MessageBoxDialog::kFrameVtable &&
              runtime.words.at(0x1028u) ==
                  pal4::MessageBoxDialog::kSubobjectVtable &&
              runtime.freed == 0x9000u &&
              runtime.words.at(0x1030u) == 0u &&
              runtime.words.at(0x1034u) == 0u &&
              runtime.words.at(0x1038u) == 0u &&
              runtime.destroyed == 0x1000u &&
              runtime.cleanup_calls ==
                  (std::vector<std::string>{"free", "base"}),
          "destructor releases resources and buffer before base cleanup");
}

void CheckConversionAndShow() {
    Runtime runtime;
    SeedShow(runtime);
    runtime.words[0x9000u] = 0x9100u;
    runtime.words[0x105cu] = 0x7000u;
    runtime.words[0x7000u] = 0x7100u;
    runtime.converted_text = "A\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80";
    pal4::MessageBoxDialog::ConvertAndShow(
        0x1000u, "source", 0x1234u, 5u, 0x9000u, runtime);
    Check(runtime.conversion_input == "source" &&
              runtime.shown_text == U"A\u00e9\u4e2d\U0001f600" &&
              runtime.cloned_handler == 0x9100u &&
              runtime.words.at(0xa000u) == 0x9110u &&
              runtime.words.at(0x105cu) == 0xa000u &&
              runtime.show_count == 1u,
          "converted UTF-8 becomes CEGUI code points before display");

    runtime.converted_text = std::string("Hi\0ignored", 10u);
    pal4::MessageBoxDialog::ConvertAndShow(
        0x1000u, "another", 0u, 0u, 0u, runtime);
    Check(runtime.shown_text == U"Hi" && runtime.show_count == 2u,
          "converted text stops at the first null byte");

    runtime.converted_text = "\xE4\xB8";
    bool caught = false;
    try {
        pal4::MessageBoxDialog::ConvertAndShow(
            0x1000u, "truncated", 0u, 0u, 0u, runtime);
    } catch (const std::length_error&) {
        caught = true;
    }
    Check(caught && runtime.show_count == 2u,
          "truncated UTF-8 does not display incomplete text");
}

void CheckShowWithFlags() {
    Runtime runtime;
    SeedShow(runtime);
    runtime.words[0x9000u] = 0x9100u;
    runtime.words[0x105cu] = 0x7000u;
    runtime.words[0x7000u] = 0x7100u;
    pal4::MessageBoxDialog::ShowWithFlags(
        0x1000u, U"confirm", 0x1234u, 7u, 0x9000u, runtime);
    Check(runtime.words.at(0x1034u) == 0x9000u &&
              runtime.words.at(0x103cu) == 0u &&
              runtime.words.at(0x1058u) == 2u &&
              runtime.words.at(0x105cu) == 0xa000u &&
              runtime.words.at(0xa000u) == 0x9110u &&
              runtime.allocations == (std::vector<std::uint32_t>{4u}) &&
              runtime.cleanup_calls ==
                  (std::vector<std::string>{"delete", "free"}) &&
              runtime.deleted_handler == 0x7100u &&
              runtime.freed == 0x7000u,
          "show resets range and clones a supplied handler");
    Check(runtime.visibility ==
              (std::vector<Visibility>{{0x8100u, true},
                                       {0x8200u, true},
                                       {0x8300u, true}}) &&
              runtime.events19 ==
              (std::vector<Event19>{{0x102cu, 0x9000u, 1u, 0x1040u},
                                    {0x102cu, 0x9000u, 1u, 0x1044u},
                                    {0x102cu, 0x9000u, 1u, 0x1048u}}) &&
              runtime.sounds == (std::vector<std::string>{"wa04.wav"}),
          "all three enabled buttons are visible and processed in order");
    Check(runtime.show_calls ==
              (std::vector<std::string>{
                  "delete", "free", "clone", "visible", "event", "sound",
                  "visible", "event", "visible", "event", "text",
                  "sheet", "position", "top", "height", "height",
                  "y", "y", "y", "y", "x", "y", "activate",
                  "modal"}) &&
              runtime.shown_text == U"confirm" &&
              runtime.sheet_root == 0x8000u &&
              runtime.positioned_root == 0x8000u &&
              runtime.position_metrics == 0u &&
              runtime.position_x == 0.5f && runtime.position_y == 0.5f &&
              runtime.activated == 0x1028u &&
              runtime.modal_root == 0x8000u &&
              runtime.heights.at(0x8400u) == 32.0f &&
              runtime.heights.at(0x8500u) == 112.0f &&
              runtime.ys.at(0x8600u) == 22.0f &&
              runtime.ys.at(0x8100u) == 32.0f &&
              runtime.ys.at(0x8200u) == 42.0f &&
              runtime.ys.at(0x8300u) == 52.0f &&
              runtime.xs.at(0x8000u) == 200.0f &&
              runtime.ys.at(0x8000u) == 78.0f,
          "CEGUI operations finish with layout, activation and modal state");
}

void CheckLayout() {
    Runtime runtime;
    SeedShow(runtime);
    pal4::MessageBoxDialog::CreateYesNoBox(0x1000u, runtime);
    Check(runtime.heights.at(0x8400u) == 32.0f &&
              runtime.heights.at(0x8500u) == 112.0f &&
              runtime.ys.at(0x8600u) == 22.0f &&
              runtime.ys.at(0x8100u) == 32.0f &&
              runtime.ys.at(0x8200u) == 42.0f &&
              runtime.ys.at(0x8300u) == 52.0f &&
              runtime.xs.at(0x8000u) == 200.0f &&
              runtime.ys.at(0x8000u) == 78.0f,
          "layout expands text area, shifts controls and centers root");
}

void CheckShowFlagsAndAllocationFailure() {
    for (std::uint8_t flags = 0u; flags < 8u; ++flags) {
        Runtime runtime;
        SeedShow(runtime);
        runtime.words[0x105cu] = 0x7777u;
        pal4::MessageBoxDialog::ShowWithFlags(
            0x1000u, U"message", 0u, flags, 0u, runtime);
        Check(runtime.words.at(0x105cu) == 0x7777u &&
                  runtime.cleanup_calls.empty() &&
                  runtime.allocations.empty() &&
                  runtime.visibility ==
                      (std::vector<Visibility>{
                          {0x8100u, (flags & 4u) != 0u},
                          {0x8200u, (flags & 1u) != 0u},
                          {0x8300u, (flags & 2u) != 0u}}) &&
                  runtime.events19.size() ==
                      static_cast<std::size_t>(((flags & 1u) != 0u) +
                                               ((flags & 2u) != 0u) +
                                               ((flags & 4u) != 0u)) &&
                  runtime.sounds ==
                      (std::vector<std::string>{
                          (flags & 4u) != 0u ? "wa04.wav" : "wa03.wav"}) &&
                  runtime.modal_root == 0x8000u,
              "each button flag combination follows its visibility and sound branch");
    }

    Runtime failed;
    SeedShow(failed);
    failed.words[0x9000u] = 0x9100u;
    failed.fail_allocation = 1u;
    pal4::MessageBoxDialog::ShowWithFlags(
        0x1000u, U"failed allocation", 0u, 0u, 0x9000u, failed);
    Check(failed.words.at(0x105cu) == 0u &&
              failed.cloned_handler == 0u &&
              failed.show_count == 1u && failed.modal_root == 0x8000u &&
              failed.show_calls.front() == "sound",
          "failed callback allocation still shows the dialog without cloning");
}

void CheckReleaseResource() {
    Runtime runtime;
    runtime.words[0x105cu] = 0u;
    pal4::MessageBoxDialog::ReleaseResource(0x1000u, runtime);
    Check(runtime.cleanup_calls.empty(), "empty handler record is untouched");

    runtime.words[0x105cu] = 0x7000u;
    runtime.words[0x7000u] = 0u;
    pal4::MessageBoxDialog::ReleaseResource(0x1000u, runtime);
    Check(runtime.cleanup_calls == (std::vector<std::string>{"free"}) &&
              runtime.freed == 0x7000u &&
              runtime.words.at(0x105cu) == 0u,
          "record without handler is freed and cleared");

    runtime.cleanup_calls.clear();
    runtime.words[0x105cu] = 0x7100u;
    runtime.words[0x7100u] = 0x7200u;
    pal4::MessageBoxDialog::ReleaseResource(0x1000u, runtime);
    Check(runtime.cleanup_calls ==
              (std::vector<std::string>{"delete", "free"}) &&
              runtime.deleted_handler == 0x7200u &&
              runtime.freed == 0x7100u &&
              runtime.words.at(0x105cu) == 0u,
          "record with handler deletes, frees, then clears");
}

void CheckSelectionAndClick() {
    Runtime runtime;
    runtime.words[0x1058u] = 2u;
    Check(pal4::MessageBoxDialog::GetLastSelection(0x1000u, runtime) == 2u,
          "last selection reads dialog offset 88");
    runtime.words[0x2008u] = 0x3000u;
    runtime.words[0x3000u + 280u] = 4u;
    runtime.words[0x105cu] = 0x4000u;
    runtime.words[0x4000u] = 0x5000u;
    Check(pal4::MessageBoxDialog::OnClick(0x1000u, 0x2000u, runtime) &&
              runtime.words.at(0x1058u) == 4u &&
              runtime.closed_dialog == 0x1000u &&
              runtime.invoked_handler == 0x5000u &&
              runtime.invoked_selection == 4u &&
              runtime.show_calls ==
                  (std::vector<std::string>{"close", "invoke"}),
          "click stores selected button, closes, then invokes handler");
    runtime.words[0x3000u + 280u] = 1u;
    runtime.words[0x105cu] = 0u;
    runtime.show_calls.clear();
    Check(pal4::MessageBoxDialog::OnClick(0x1000u, 0x2000u, runtime) &&
              runtime.words.at(0x1058u) == 1u &&
              runtime.show_calls == (std::vector<std::string>{"close"}),
          "click without callback still stores selection and closes");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckNullAllocationsAndConnections();
        CheckExceptionCleanup();
        CheckDestruction();
        CheckConversionAndShow();
        CheckShowWithFlags();
        CheckShowFlagsAndAllocationFailure();
        CheckSelectionAndClick();
        CheckReleaseResource();
        CheckLayout();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
