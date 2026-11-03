#include "pal4/combat_message_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr auto kMessage = 0x4000u;
constexpr auto kHead = 0x8000u;
constexpr auto kEdit = 0x5000u;
constexpr auto kKey = 0x6000u;
constexpr auto kCallback = 0xd000u;

struct Runtime final : pal4::ICombatMessageWindowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> cleaned_strings;
    std::vector<std::uint32_t> released_slots;
    std::vector<std::uint32_t> copied_strings;
    std::uint32_t next = kHead;
    std::uint32_t layout_destructs = 0;
    std::uint32_t allocated_size = 0;
    std::uint32_t parent = 0;
    std::uint32_t group = 0xa000u;
    std::string looked_up_name;
    std::string text;
    std::u32string edit_text;
    std::uint32_t caret = 0u;
    std::uint32_t frame = 0u;
    std::uint32_t frame_manager_calls = 0u;
    std::uint32_t find_frame_calls = 0u;
    std::uint32_t register_frame_calls = 0u;
    std::uint32_t set_frame_calls = 0u;
    std::uint32_t frame_parent = 0u;
    bool frame_visible = false;
    bool create_frame_on_register = true;
    bool fail_allocation = false;
    bool visible = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        calls.emplace_back("allocate");
        allocated_size = size;
        if (fail_allocation) return 0u;
        const auto result = next;
        next += 0x100u;
        return result;
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void LoadLayout(const std::uint32_t message,
                    const std::string_view layout,
                    const std::string_view prefix,
                    const std::string_view override_name) override {
        calls.emplace_back("layout");
        Check(message == kMessage && layout == "CombatMessageWindow" &&
              prefix == "CombatMessageFrame" &&
              override_name == "CombatMessageFrame",
              "constructor uses IDA layout and type strings");
        words[message + 24u] = 0x2000u;
        strings[0x2000u] = "CombatMessageFrame/";
    }
    std::uint32_t DestructLayout(const std::uint32_t message) override {
        calls.emplace_back("destruct_layout");
        Check(message == kMessage, "base destructor receives message object");
        ++layout_destructs;
        return 0x77u;
    }
    void SetParent(const std::uint32_t message,
                   const bool enabled,
                   const std::uint32_t target) override {
        calls.emplace_back("parent");
        Check(message == kMessage, "parent setter receives message object");
        visible = enabled;
        parent = target;
    }
    std::string ReadCString(const std::uint32_t address) override {
        calls.emplace_back("read_string");
        return strings[address];
    }
    std::string ConvertToUtf8(const std::string& source) override {
        calls.emplace_back("utf8");
        return "utf8:" + source;
    }
    std::uint32_t GetWindowByName(
        const std::uint32_t message,
        const std::string& name) override {
        calls.emplace_back("lookup");
        Check(message == kMessage, "lookup uses message layout");
        looked_up_name = name;
        return kEdit;
    }
    void SetNormalTextColour(const std::uint32_t window,
                             const float red, const float green,
                             const float blue, const float alpha) override {
        calls.emplace_back("colour");
        Check(window == kEdit && red == 0.0f && green == 1.0f &&
              blue == 0.0f && alpha == 1.0f,
              "message text colour is opaque green");
    }
    void SetText(const std::uint32_t window,
                 const std::string_view value) override {
        calls.emplace_back("text");
        Check(window == kEdit, "text writes target the edit control");
        text = value;
    }
    std::uint32_t CleanupTemporaryString() override {
        calls.emplace_back("cleanup_temp");
        return 0x55u;
    }
    void CleanupMessageString(const std::uint32_t record) override {
        cleaned_strings.push_back(record);
    }
    void CopyMessageString(const std::uint32_t destination,
                           const std::uint32_t source) override {
        copied_strings.push_back(destination);
        for (auto offset : {0u, 4u, 8u})
            words[destination + offset] = words[source + offset];
    }
    std::u32string DecodeUtf8(const std::string& utf8) override {
        if (utf8 == "utf8:\xE6\xB1\x89") return U"\u6c49";
        Check(utf8.starts_with("utf8:"), "text passed through UTF-8 conversion");
        std::u32string decoded;
        for (std::size_t index = 5; index < utf8.size(); ++index)
            decoded.push_back(static_cast<unsigned char>(utf8[index]));
        return decoded;
    }
    std::u32string ReadEditText(const std::uint32_t window) override {
        Check(window == kEdit, "append reads the edit control's live text");
        return edit_text;
    }
    void SetUnicodeText(const std::uint32_t window,
                        const std::u32string& value) override {
        Check(window == kEdit, "append updates the edit control");
        edit_text = value;
    }
    void SetCaretIndex(const std::uint32_t window,
                       const std::uint32_t index) override {
        Check(window == kEdit, "caret belongs to the edit control");
        caret = index;
    }
    std::uint32_t FrameManager() override {
        ++frame_manager_calls;
        return 0x7000u;
    }
    std::uint32_t FindFrame(const std::uint32_t manager,
                            const std::uint32_t type) override {
        ++find_frame_calls;
        Check(manager == 0x7000u && type == 0x8b6924u,
              "show looks up the combat-message type");
        return frame;
    }
    void RegisterFrame(const std::uint32_t manager,
                       const std::uint32_t type) override {
        ++register_frame_calls;
        Check(manager == 0x7000u && type == 0x8b6924u,
              "missing visible frame is registered by type");
        if (create_frame_on_register) frame = 0x7500u;
    }
    void SetFrameVisible(const std::uint32_t target,
                         const bool enabled,
                         const std::uint32_t requested_parent) override {
        ++set_frame_calls;
        Check(target == frame, "visibility callback receives found frame");
        frame_visible = enabled;
        frame_parent = requested_parent;
    }
    std::uint32_t EventKey(const std::string_view) override {
        throw std::runtime_error("message key comes from its global pointer");
    }
    std::uint32_t MessageEventKey() override { return kKey; }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x9000u;
    }
    std::uint32_t Register(const std::uint32_t router,
                           const std::uint32_t mode,
                           const std::uint32_t key,
                           const std::uint32_t owner,
                           const std::uint32_t callback) override {
        calls.emplace_back("register");
        Check(router == 0x9000u && mode == 3u && key == kKey &&
              owner == kMessage &&
              callback == pal4::CombatMessageSubscriptions::kCallback,
              "visible message registers its mode-3 event handler");
        return kCallback;
    }
    std::uint32_t FindEventGroup(const std::uint32_t router,
                                 const std::uint32_t key) override {
        calls.emplace_back("find_group");
        Check(router == 0x9000u && key == kKey,
              "hidden message searches the saved key group");
        return group;
    }
    void ReleaseOwnedSlot(const std::uint32_t slot) override {
        released_slots.push_back(slot);
    }
    void FreeNode(const std::uint32_t node) override { Free(node); }
};

void ConstructionAndInitialization() {
    Runtime runtime;
    Check(pal4::CombatMessageWindow::Construct(kMessage, 0x1234u, runtime) ==
              kMessage &&
          runtime.allocated_size == 20u &&
          runtime.words[kHead] == kHead &&
          runtime.words[kHead + 4u] == kHead &&
          runtime.words[kMessage] == pal4::CombatMessageWindow::kVtable &&
          runtime.words[kMessage + 40u] == 0u &&
          runtime.words[kMessage + 44u] == 0u &&
          runtime.bytes[kMessage + 48u] == 0u &&
          runtime.words[kMessage + 52u] == kHead &&
          runtime.words[kMessage + 56u] == 0u &&
          runtime.words[kMessage + 60u] == 100u &&
          runtime.words[kMessage + 64u] == kEdit,
          "constructor initializes callback, list, limit, vtable and edit slots");
    Check(runtime.looked_up_name ==
              "utf8:CombatMessageFrame/EditRoleProperty" &&
          runtime.text.empty() &&
          runtime.calls == std::vector<std::string>{
              "layout", "allocate", "read_string", "utf8", "lookup",
              "colour", "text", "cleanup_temp"},
          "initialization formats the prefixed edit name and clears text");

    runtime.words[kMessage + 24u] = 0u;
    runtime.calls.clear();
    Check(pal4::CombatMessageWindow::Initialize(kMessage, runtime) == 0x55u &&
          runtime.looked_up_name == "utf8:EditRoleProperty" &&
          runtime.calls == std::vector<std::string>{
              "utf8", "lookup", "colour", "text", "cleanup_temp"},
          "null layout prefix falls back to the empty string");
}

void AllocationFailureAndDestruction() {
    Runtime failed;
    failed.fail_allocation = true;
    Check(pal4::CombatMessageWindow::Construct(kMessage, 0u, failed) == 0u &&
          failed.layout_destructs == 1u &&
          failed.calls == std::vector<std::string>{
              "layout", "allocate", "destruct_layout"},
          "failed list-head allocation releases the loaded base layout");

    Runtime runtime;
    (void)pal4::CombatMessageWindow::Construct(kMessage, 0u, runtime);
    constexpr auto first = 0xa100u;
    constexpr auto second = 0xa200u;
    runtime.words[kHead] = first;
    runtime.words[kHead + 4u] = second;
    runtime.words[first] = second;
    runtime.words[first + 4u] = kHead;
    runtime.words[second] = kHead;
    runtime.words[second + 4u] = first;
    runtime.words[kMessage + 56u] = 2u;
    Check(pal4::CombatMessageWindow::Destruct(kMessage, runtime) == 0x77u &&
          runtime.cleaned_strings == std::vector<std::uint32_t>{
              first + 8u, second + 8u} &&
          runtime.freed == std::vector<std::uint32_t>{
              first, second, kHead} &&
          runtime.words[kMessage + 52u] == 0u &&
          runtime.words[kMessage + 56u] == 0u &&
          runtime.words[kHead] == kHead &&
          runtime.words[kHead + 4u] == kHead &&
          runtime.layout_destructs == 1u,
          "destructor unlinks and releases every message before base layout");
}

void VisibilitySubscriptions() {
    Runtime runtime;
    runtime.words[kMessage + 40u] = 0u;
    Check(pal4::CombatMessageWindow::SetVisible(
              kMessage, true, 0x3210u, runtime) == kCallback &&
          runtime.visible && runtime.parent == 0x3210u &&
          runtime.words[kMessage + 40u] == kCallback &&
          runtime.words[kMessage + 44u] == kKey &&
          runtime.calls == std::vector<std::string>{
              "parent", "router", "register"},
          "showing message sets parent before subscribing");

    constexpr auto list_head = 0xb000u;
    constexpr auto node = 0xc000u;
    runtime.words[runtime.group + 4u] = list_head;
    runtime.words[runtime.group + 8u] = 1u;
    runtime.words[list_head] = node;
    runtime.words[list_head + 4u] = node;
    runtime.words[node] = list_head;
    runtime.words[node + 4u] = list_head;
    runtime.words[node + 12u] = kCallback;
    runtime.calls.clear();
    Check(pal4::CombatMessageWindow::SetVisible(
              kMessage, false, 0x1230u, runtime) == 0u &&
          !runtime.visible && runtime.parent == 0x1230u &&
          runtime.words[kMessage + 40u] == 0u &&
          runtime.words[kMessage + 44u] == 0u &&
          runtime.words[runtime.group + 8u] == 0u &&
          runtime.words[list_head] == list_head &&
          runtime.released_slots == std::vector<std::uint32_t>{node + 8u} &&
          runtime.freed == std::vector<std::uint32_t>{node} &&
          runtime.calls == std::vector<std::string>{
              "parent", "router", "find_group"},
          "hiding message detaches parent and removes owned subscription");
}

void MessageInsertionAndLimit() {
    Runtime runtime;
    (void)pal4::CombatMessageWindow::Construct(kMessage, 0u, runtime);
    runtime.words[kMessage + 60u] = 1u;
    constexpr auto first_source = 0x1000u;
    constexpr auto second_source = 0x1010u;
    constexpr auto third_source = 0x1020u;
    runtime.words[first_source] = 0x3000u;
    runtime.words[second_source] = 0x3010u;
    runtime.words[third_source] = 0x3020u;
    runtime.strings[0x3000u] = "first";
    runtime.strings[0x3010u] = "\xE6\xB1\x89";
    runtime.strings[0x3020u] = "third";
    runtime.edit_text = U"seed\n";
    pal4::CombatMessageWindow::AddMessage(kMessage, first_source, runtime);
    Check(runtime.words[kMessage + 56u] == 1u &&
          runtime.edit_text == U"seed\nfirst\n" &&
          runtime.caret == runtime.edit_text.size() - 1u &&
          runtime.words[kHead] == 0x8100u &&
          runtime.words[0x8100u + 8u] == 0x3000u,
          "first message appends to live text and inserts a 20-byte node");
    pal4::CombatMessageWindow::AddMessage(kMessage, second_source, runtime);
    Check(runtime.words[kMessage + 56u] == 2u &&
          runtime.edit_text == U"seed\nfirst\n\u6c49\n" &&
          runtime.caret == runtime.edit_text.size() - 1u &&
          runtime.freed.empty(),
          "count equal to limit still accepts one more Unicode message");
    pal4::CombatMessageWindow::AddMessage(kMessage, third_source, runtime);
    Check(runtime.words[kMessage + 56u] == 2u &&
          runtime.edit_text == U"\u6c49\nthird\n" &&
          runtime.caret == runtime.edit_text.size() - 1u &&
          runtime.words[kHead] == 0x8200u &&
          runtime.words[kHead + 4u] == 0x8300u &&
          runtime.freed == std::vector<std::uint32_t>{0x8100u} &&
          runtime.cleaned_strings ==
              std::vector<std::uint32_t>{0x8108u} &&
          runtime.copied_strings == std::vector<std::uint32_t>{
              0x8108u, 0x8208u, 0x8308u},
          "overflow removes oldest and rebuilds text from remaining nodes");
    Check(pal4::CombatMessageWindow::ClearMessages(kMessage, runtime) == 0u &&
          runtime.words[kMessage + 52u] == 0u &&
          runtime.words[kMessage + 56u] == 0u &&
          runtime.cleaned_strings == std::vector<std::uint32_t>{
              0x8108u, 0x8208u, 0x8308u} &&
          runtime.freed == std::vector<std::uint32_t>{
              0x8100u, 0x8200u, 0x8300u, kHead},
          "clear helper releases all records and sentinel");
}

void DefaultHundredMessageLimit() {
    Runtime runtime;
    (void)pal4::CombatMessageWindow::Construct(kMessage, 0u, runtime);
    constexpr auto source = 0x1000u;
    runtime.words[source] = 0x3000u;
    runtime.strings[0x3000u] = "x";
    for (std::uint32_t index = 0u; index < 101u; ++index)
        pal4::CombatMessageWindow::AddMessage(kMessage, source, runtime);
    Check(runtime.words[kMessage + 60u] == 100u &&
          runtime.words[kMessage + 56u] == 101u &&
          runtime.edit_text.size() == 202u &&
          runtime.caret == 201u && runtime.freed.empty(),
          "default limit accepts 101 messages before dropping an old entry");
    pal4::CombatMessageWindow::AddMessage(kMessage, source, runtime);
    Check(runtime.words[kMessage + 56u] == 101u &&
          runtime.edit_text.size() == 202u &&
          runtime.freed == std::vector<std::uint32_t>{0x8100u} &&
          runtime.cleaned_strings ==
              std::vector<std::uint32_t>{0x8108u},
          "102nd message drops the oldest and retains 101 list entries");
}

void EventAndStringHelpers() {
    Runtime runtime;
    (void)pal4::CombatMessageWindow::Construct(kMessage, 0u, runtime);
    constexpr auto source = 0x1000u;
    constexpr auto event = 0x1100u;
    runtime.words[event] = source;
    runtime.words[source] = 0x3000u;
    runtime.strings[0x3000u] = "event";
    pal4::CombatMessageWindow::HandleEvent(kMessage, event, runtime);
    Check(runtime.words[kMessage + 56u] == 1u &&
          runtime.edit_text == U"event\n",
          "event handler dereferences payload and forwards its text record");
    runtime.fail_allocation = true;
    pal4::CombatMessageWindow::AddMessage(kMessage, source, runtime);
    Check(runtime.words[kMessage + 56u] == 1u &&
          runtime.edit_text == U"event\n",
          "node allocation failure preserves list and visible text");
    runtime.fail_allocation = false;
    Check(pal4::CombatMessageStringRecord::CopyConstruct(
              0u, source, runtime) == 0u &&
          runtime.copied_strings.size() == 1u,
          "null string-copy destination is skipped");
    Check(pal4::CombatMessageStringRecord::Cleanup(
              0x400u, 0u, runtime) == 0x400u &&
          runtime.freed.empty(),
          "record cleanup without delete flag keeps its storage");
    Check(pal4::CombatMessageStringRecord::Cleanup(
              0x500u, 1u, runtime) == 0x500u &&
          runtime.freed == std::vector<std::uint32_t>{0x500u} &&
          runtime.cleaned_strings == std::vector<std::uint32_t>{
              0x400u, 0x500u},
          "record cleanup with flag one frees the record");
}

void ShowFrame() {
    Runtime runtime;
    constexpr auto event = 0x1000u;
    runtime.words[0x8b6920u] = 0x8b6924u;
    runtime.words[event] = 1u;
    runtime.words[event + 4u] = 0x1234u;
    Check(pal4::CombatMessageWindow::Show(event, runtime) &&
          runtime.frame == 0x7500u && runtime.frame_visible &&
          runtime.frame_parent == 0x1234u &&
          runtime.frame_manager_calls == 3u &&
          runtime.find_frame_calls == 2u &&
          runtime.register_frame_calls == 1u &&
          runtime.set_frame_calls == 1u,
          "show registers a missing frame then retries lookup");
    runtime.words[event] = 0u;
    Check(pal4::CombatMessageWindow::Show(event, runtime) &&
          !runtime.frame_visible &&
          runtime.register_frame_calls == 1u &&
          runtime.set_frame_calls == 2u,
          "hide uses the existing frame without registration");
    runtime.frame = 0u;
    Check(!pal4::CombatMessageWindow::Show(event, runtime) &&
          runtime.set_frame_calls == 2u,
          "hide of a missing frame returns false");
    runtime.words[event] = 1u;
    runtime.create_frame_on_register = false;
    Check(!pal4::CombatMessageWindow::Show(event, runtime) &&
          runtime.register_frame_calls == 2u &&
          runtime.set_frame_calls == 2u,
          "show returns false if registration still yields no frame");
}

}  // namespace

int main() {
    try {
        ConstructionAndInitialization();
        AllocationFailureAndDestruction();
        VisibilitySubscriptions();
        MessageInsertionAndLimit();
        DefaultHundredMessageLimit();
        EventAndStringHelpers();
        ShowFrame();
        std::cout << "combat_message_window_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
