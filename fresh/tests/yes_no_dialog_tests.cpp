#include "pal4/yes_no_dialog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Subscription {
    std::uint32_t window;
    std::uint32_t event;
    std::uint32_t handler;
    bool operator==(const Subscription&) const = default;
};

struct Event19 {
    std::uint32_t range;
    std::uint32_t end;
    std::uint32_t count;
    std::uint32_t field;
    bool operator==(const Event19&) const = default;
};

struct Runtime final : pal4::IYesNoDialogRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> names;
    std::vector<std::string> calls;
    std::vector<Subscription> subscriptions;
    std::vector<Event19> events;
    std::vector<std::uint32_t> released;
    std::vector<std::uint32_t> visible;
    std::uint32_t initialized = 0u;
    std::uint32_t destroyed = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t ended = 0u;
    std::uint32_t allocation_count = 0u;
    std::uint32_t fail_allocation = 0u;
    std::uint32_t fail_find = 0u;
    std::string layout;
    std::string name;
    std::string conversion_input;
    std::string converted_text;
    std::u32string shown_text;
    std::uint32_t shown_window = 0u;
    std::uint32_t shown_count = 0u;
    std::uint32_t sheet_root = 0u;
    std::uint32_t position_root = 0u;
    std::uint32_t position_metrics = ~0u;
    float position_x = 0.0f;
    float position_y = 0.0f;
    std::uint32_t activated = 0u;
    std::uint32_t modal_root = 0u;
    bool modal_enabled = false;
    std::uint32_t removed_parent = 0u;
    std::uint32_t removed_child = 0u;

    void LoadLayout(std::uint32_t dialog,
                    std::string_view layout_name) override {
        Check(dialog == 0x1000u, "layout owner");
        layout = layout_name;
        words[0x1024u] = 0x8000u;
        words[0x1030u] = 0x9000u;
        words[0x1034u] = 0x9010u;
    }
    void InitializeSubobject(std::uint32_t address) override {
        initialized = address;
    }
    std::uint32_t DestroyBase(std::uint32_t dialog) override {
        destroyed = dialog;
        calls.emplace_back("destroy");
        return 0xdeadu;
    }
    void Free(std::uint32_t address) override {
        freed = address;
        calls.emplace_back("free");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t BeginName(std::uint32_t source) override {
        Check(source == 0x1014u, "name source");
        return 0x7000u;
    }
    void ResetName(std::uint32_t buffer,
                   std::uint32_t source) override {
        Check(buffer == 0x7000u && source == 0x1014u,
              "name reset arguments");
        name = "Common/";
    }
    void AppendName(std::uint32_t buffer,
                    std::string_view suffix) override {
        Check(buffer == 0x7000u, "name buffer");
        name += suffix;
    }
    std::uint32_t FindWindow(std::uint32_t buffer) override {
        Check(buffer == 0x7000u, "find window buffer");
        names.push_back(name);
        if (names.size() == fail_find)
            throw std::runtime_error("find failed");
        return 0x8100u + static_cast<std::uint32_t>(names.size()) * 0x100u;
    }
    void EndName(std::uint32_t buffer) override {
        Check(buffer == 0x7000u, "end name buffer");
        ++ended;
        calls.emplace_back("end_name");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "handler allocation size");
        ++allocation_count;
        if (allocation_count == fail_allocation) return 0u;
        return 0xa000u + (allocation_count - 1u) * 0x100u;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::uint32_t event,
                            std::uint32_t handler) override {
        subscriptions.push_back({window, event, handler});
        return 0xb000u + static_cast<std::uint32_t>(subscriptions.size()) *
                             0x100u;
    }
    void ReleaseConnection(std::uint32_t connection) override {
        released.push_back(connection);
    }
    void SetVisible(std::uint32_t window, bool is_visible) override {
        Check(is_visible, "constructor only shows buttons");
        calls.emplace_back("visible");
        visible.push_back(window);
    }
    void ProcessEvent19(std::uint32_t range, std::uint32_t end,
                        std::uint32_t count,
                        std::uint32_t field) override {
        calls.emplace_back("event");
        events.push_back({range, end, count, field});
    }
    std::string ConvertToUtf8(std::string_view text) override {
        conversion_input = text;
        return converted_text;
    }
    void PlaySound(std::string_view file,
                   std::uint32_t repeat) override {
        Check(file == "wa03.wav" && repeat == 1u, "show sound");
        calls.emplace_back("sound");
    }
    void SetText(std::uint32_t window,
                 std::u32string_view text) override {
        calls.emplace_back("text");
        shown_window = window;
        shown_text = text;
        ++shown_count;
    }
    void AddToSheet(std::uint32_t root) override {
        calls.emplace_back("sheet");
        sheet_root = root;
    }
    void SetPosition(std::uint32_t root, std::uint32_t metrics,
                     float x, float y) override {
        calls.emplace_back("position");
        position_root = root;
        position_metrics = metrics;
        position_x = x;
        position_y = y;
    }
    void SetAlwaysOnTop(std::uint32_t root,
                         bool enabled) override {
        Check(root == 0x8000u && enabled, "root always on top");
        calls.emplace_back("top");
    }
    void ActivateSubobject(std::uint32_t address) override {
        calls.emplace_back("activate");
        activated = address;
    }
    void SetModal(std::uint32_t root, bool enabled) override {
        calls.emplace_back(enabled ? "modal_on" : "modal_off");
        modal_root = root;
        modal_enabled = enabled;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        calls.emplace_back("remove");
        removed_parent = parent;
        removed_child = child;
    }
};

void CheckConstruction() {
    Runtime runtime;
    using Dialog = pal4::YesNoDialog;
    Check(Dialog::Construct(0x1000u, "commondialog2", runtime) == 0x1000u &&
              runtime.layout == "commondialog2" &&
              runtime.initialized == 0x1028u &&
              runtime.words.at(0x1000u) == Dialog::kFrameVtable &&
              runtime.words.at(0x1028u) == Dialog::kSubobjectVtable &&
              runtime.names == (std::vector<std::string>{
                  "Common/BtYes", "Common/BtNo", "Common/Message"}) &&
              runtime.ended == 1u && runtime.destroyed == 0u,
          "constructor loads layout, initializes subobject and finds controls");
    Check(runtime.words.at(0x1040u) == 0x8200u &&
              runtime.words.at(0x1044u) == 0x8300u &&
              runtime.words.at(0x1048u) == 0x8400u &&
              runtime.words.at(0x8200u + 280u) == 1u &&
              runtime.words.at(0x8300u + 280u) == 0u &&
              runtime.words.at(0x1034u) == 0x9000u,
          "control offsets, IDs and cleared range match IDA");
    Check(runtime.subscriptions ==
              (std::vector<Subscription>{
                  {0x8200u, Dialog::kClickedEvent, 0xa000u},
                  {0x8300u, Dialog::kClickedEvent, 0xa100u},
                  {0x8000u, Dialog::kKeyDownEvent, 0xa200u}}) &&
              runtime.words.at(0xa000u) == Dialog::kKeyHandlerVtable &&
              runtime.words.at(0xa008u) == Dialog::kOnClick &&
              runtime.words.at(0xa00cu) == 0u &&
              runtime.words.at(0xa010u) == 0x1000u &&
              runtime.words.at(0xa100u) == Dialog::kKeyHandlerVtable &&
              runtime.words.at(0xa108u) == Dialog::kOnClick &&
              runtime.words.at(0xa200u) == Dialog::kKeyHandlerVtable &&
              runtime.words.at(0xa208u) == Dialog::kOnKeyDown &&
              runtime.words.at(0xa20cu) == 0u &&
              runtime.words.at(0xa210u) == 0x1000u &&
              runtime.released ==
                  (std::vector<std::uint32_t>{0xb100u, 0xb200u, 0xb300u}),
          "two click subscriptions and the root key handler are installed");
    Check(runtime.visible ==
              (std::vector<std::uint32_t>{0x8200u, 0x8300u}) &&
              runtime.events ==
              (std::vector<Event19>{{0x102cu, 0x9000u, 1u, 0x1040u},
                                    {0x102cu, 0x9000u, 1u, 0x1044u}}) &&
              runtime.calls ==
              (std::vector<std::string>{"visible", "event", "visible",
                                        "event", "end_name"}),
          "both buttons become visible and are processed in order");
}

void CheckNullAllocations() {
    Runtime runtime;
    runtime.fail_allocation = 1u;
    (void)pal4::YesNoDialog::Construct(0x1000u, "layout", runtime);
    Check(runtime.subscriptions.size() == 3u &&
              runtime.subscriptions[0].handler == 0u &&
              runtime.words.at(0xa108u) == pal4::YesNoDialog::kOnClick &&
              runtime.words.at(0x8200u + 280u) == 1u &&
              runtime.visible.size() == 2u,
          "null click handler still subscribes and initializes both buttons");

    Runtime key_failure;
    key_failure.fail_allocation = 3u;
    (void)pal4::YesNoDialog::Construct(0x1000u, "layout", key_failure);
    Check(key_failure.subscriptions[2].handler == 0u &&
              key_failure.visible.size() == 2u,
          "null key handler still subscribes the root");
}

void CheckExceptionalCleanup() {
    Runtime runtime;
    runtime.fail_find = 2u;
    bool caught = false;
    try {
        (void)pal4::YesNoDialog::Construct(0x1000u, "layout", runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.names.size() == 2u &&
              runtime.ended == 1u && runtime.destroyed == 0x1000u &&
              runtime.calls ==
                  (std::vector<std::string>{"end_name", "destroy"}),
          "failed lookup releases temporary name and the base dialog");
}

void CheckDestruction() {
    Runtime runtime;
    runtime.words[0x1030u] = 0x9000u;
    runtime.words[0x1034u] = 0x9010u;
    runtime.words[0x1038u] = 0x9020u;
    Check(pal4::YesNoDialog::Destruct(0x1000u, runtime) == 0xdeadu &&
              runtime.words.at(0x1000u) ==
                  pal4::YesNoDialog::kFrameVtable &&
              runtime.words.at(0x1028u) ==
                  pal4::YesNoDialog::kSubobjectVtable &&
              runtime.freed == 0x9000u &&
              runtime.words.at(0x1030u) == 0u &&
              runtime.words.at(0x1034u) == 0u &&
              runtime.words.at(0x1038u) == 0u &&
              runtime.destroyed == 0x1000u &&
              runtime.calls ==
                  (std::vector<std::string>{"free", "destroy"}),
          "destructor restores vtables, releases range and forwards base return");
}

void CheckShowAndConversion() {
    Runtime runtime;
    runtime.words[0x1024u] = 0x8000u;
    runtime.words[0x1048u] = 0x8400u;
    runtime.words[0x103cu] = 0x55u;
    runtime.words[0x104cu] = 2u;
    runtime.converted_text = "A\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80";
    pal4::YesNoDialog::ConvertAndShow(0x1000u, "source", runtime);
    Check(runtime.conversion_input == "source" &&
              runtime.shown_text == U"A\u00e9\u4e2d\U0001f600" &&
              runtime.shown_window == 0x8400u &&
              runtime.shown_count == 1u &&
              runtime.words.at(0x103cu) == 0u &&
              runtime.words.at(0x104cu) == 0u &&
              runtime.sheet_root == 0x8000u &&
              runtime.position_root == 0x8000u &&
              runtime.position_metrics == 0u &&
              runtime.position_x == 0.2875f &&
              runtime.position_y == 0.32f &&
              runtime.activated == 0x1028u &&
              runtime.modal_root == 0x8000u &&
              runtime.modal_enabled &&
              runtime.calls == (std::vector<std::string>{
                  "sound", "text", "sheet", "position", "top",
                  "activate", "modal_on"}),
          "converted text follows audio, CEGUI placement and modal order");

    runtime.converted_text = std::string("Hi\0ignored", 10u);
    pal4::YesNoDialog::ConvertAndShow(0x1000u, "second", runtime);
    Check(runtime.shown_text == U"Hi" && runtime.shown_count == 2u,
          "converted text stops at embedded NUL");

    runtime.converted_text = "\xE4\xB8";
    bool caught = false;
    try {
        pal4::YesNoDialog::ConvertAndShow(0x1000u, "truncated", runtime);
    } catch (const std::length_error&) {
        caught = true;
    }
    Check(caught && runtime.shown_count == 2u,
          "truncated UTF-8 does not enter display path");
}

void CheckCloseAndClick() {
    Runtime no_parent;
    no_parent.words[0x1024u] = 0x8000u;
    no_parent.words[0x8044u] = 0u;
    pal4::YesNoDialog::Close(0x1000u, no_parent);
    Check(no_parent.calls == (std::vector<std::string>{"modal_off"}) &&
              no_parent.modal_root == 0x8000u &&
              !no_parent.modal_enabled,
          "close clears modal state when root has no parent");

    Runtime runtime;
    runtime.words[0x1024u] = 0x8000u;
    runtime.words[0x8044u] = 0x9000u;
    runtime.words[0x2008u] = 0x3000u;
    runtime.words[0x3000u + 280u] = 1u;
    Check(pal4::YesNoDialog::OnClick(0x1000u, 0x2000u, runtime) &&
              runtime.words.at(0x104cu) == 1u &&
              runtime.removed_parent == 0x9000u &&
              runtime.removed_child == 0x8000u &&
              runtime.calls ==
                  (std::vector<std::string>{"modal_off", "remove"}),
          "Yes click stores button property then closes the window");
    runtime.words[0x3000u + 280u] = 0u;
    runtime.calls.clear();
    Check(pal4::YesNoDialog::OnClick(0x1000u, 0x2000u, runtime) &&
              runtime.words.at(0x104cu) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"modal_off", "remove"}),
          "No click records zero and follows the same close path");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckNullAllocations();
        CheckExceptionalCleanup();
        CheckDestruction();
        CheckShowAndConversion();
        CheckCloseAndClick();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
