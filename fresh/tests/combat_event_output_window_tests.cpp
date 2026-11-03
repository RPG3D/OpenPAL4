#include "pal4/combat_event_output_window.h"

#include <algorithm>
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
constexpr std::uint32_t kEdit = 0x6000u;
constexpr std::uint32_t kCombo = 0x6100u;
constexpr std::uint32_t kRoot = 0x6200u;
constexpr std::uint32_t kEvent = 0x6300u;

struct Runtime final : pal4::ICombatEventOutputRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> controls;
    std::vector<std::string> labels;
    std::vector<std::uint32_t> combo_items;
    std::vector<std::uint32_t> event_handles;
    std::vector<std::uint32_t> removed_handles;
    std::u32string text;
    std::uint32_t selected = 0u;
    std::uint32_t node_count = 0u;
    std::uint32_t item_count = 0u;
    std::uint32_t register_count = 0u;
    bool fail_factory = false;
    bool fail_head = false;

    Runtime() {
        words[pal4::CombatEventOutputFactory::kTypePointer] = 0xa000u;
        words[pal4::CombatEventOutputWindow::kHandledKeyPointer] = 0xa100u;
        words[pal4::CombatEventOutputWindow::kUnhandledKeyPointer] = 0xa200u;
    }
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
        allocations.push_back(size);
        if (size == 0x4cu) return fail_factory ? 0u : kWindow;
        if (size == 20u) {
            if (fail_head && node_count == 0u) return 0u;
            return 0x5000u + 0x100u * node_count++;
        }
        if (size == 0x26cu) return 0x9000u + 0x300u * item_count++;
        if (size == 12u) return 0x7000u;
        if (size == 8u) return 0x7100u;
        throw std::runtime_error("unexpected allocation size");
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void LoadLayout(const std::uint32_t window,
                    const std::uint32_t layout) override {
        Check(window == kWindow && layout == 0xb000u,
              "event output loads caller-provided layout");
        calls.emplace_back("load_layout");
    }
    std::uint32_t DestructLayout(const std::uint32_t window) override {
        Check(window == kWindow, "destructor forwards window");
        calls.emplace_back("destruct_layout");
        return 0x7eu;
    }
    void SetParent(const std::uint32_t window, const bool visible,
                   const std::uint32_t parent) override {
        Check(window == kWindow && parent == 0x7700u,
              "visibility forwards parent");
        calls.emplace_back(visible ? "show" : "hide");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame_manager");
        return 0x8000u;
    }
    std::uint32_t RegisterFactory(const std::uint32_t type,
                                  const std::uint32_t creator) override {
        Check(type == 0xa000u &&
              creator == pal4::CombatEventOutputFactory::kCreateCallback,
              "factory uses cached CombatEventOutput type and creator");
        calls.emplace_back("register_factory");
        return 0x55u;
    }
    void InsertShowEvent(const std::uint32_t tree,
                         const std::uint32_t key,
                         const std::uint32_t handler) override {
        Check(tree == 0x8000u + 260u &&
              key == pal4::CombatEventOutputFactory::kShowEventKey &&
              handler == 0x7100u && words[handler] == 0x843494u &&
              words[handler + 4u] == 0xa000u,
              "show event stores type callback in UI event tree");
        calls.emplace_back("insert_show_event");
    }
    void DeleteHandler(const std::uint32_t handler,
                       const std::uint32_t flags) override {
        Check(handler == 0x7100u && flags == 1u,
              "temporary show handler uses deleting destructor");
        calls.emplace_back("delete_handler");
    }
    std::uint32_t FindControl(const std::uint32_t window,
                              const std::string_view name) override {
        Check(window == kWindow, "control lookup uses this window");
        controls.emplace_back(name);
        if (name.ends_with("EditEventOutput")) return kEdit;
        if (name.ends_with("ComboEventLv")) return kCombo;
        if (name.ends_with("WinCombatEventOutput")) return kRoot;
        throw std::runtime_error("unexpected control name");
    }
    void ConstructLevelItem(const std::uint32_t item,
                            const std::string_view label) override {
        Check(item != 0u, "level item is allocated");
        labels.emplace_back(label);
    }
    void AddComboItem(const std::uint32_t combo,
                      const std::uint32_t item) override {
        Check(combo == kCombo && item != 0u,
              "level item goes into combo");
        combo_items.push_back(item);
    }
    void SelectComboItem(const std::uint32_t combo,
                         const std::uint32_t index,
                         const bool is_selected) override {
        Check(combo == kCombo && index == 0u && is_selected,
              "default level is first item");
        selected = combo_items[index];
    }
    std::uint32_t SubscribeComboSelection(
        const std::uint32_t combo,
        const std::uint32_t callback) override {
        Check(combo == kCombo && callback == 0x7000u &&
              words[callback] ==
                  pal4::CombatEventOutputWindow::kComboCallbackVtable &&
              words[callback + 4u] ==
                  pal4::CombatEventOutputWindow::kComboCallback &&
              words[callback + 8u] == kWindow,
              "selection callback stores vtable, function and owner");
        calls.emplace_back("subscribe");
        return 0x7200u;
    }
    void ReleaseSubscription(const std::uint32_t subscription) override {
        Check(subscription == 0x7200u,
              "selection subscription releases temporary handle");
        calls.emplace_back("release_subscription");
    }
    std::uint32_t CleanupTemporaryString() override {
        calls.emplace_back("cleanup_string_buffer");
        return 0x5au;
    }
    std::uint32_t GetSelectedComboItem(
        const std::uint32_t combo) override {
        Check(combo == kCombo, "event level is read from combo");
        return selected;
    }
    std::string ReadCString(const std::uint32_t address) override {
        return strings.at(address);
    }
    void CopyString(const std::uint32_t destination,
                    const std::string_view value) override {
        const auto pointer = 0xc000u + destination;
        words[destination] = pointer;
        strings[pointer] = value;
        calls.emplace_back("copy_string");
    }
    void CleanupString(const std::uint32_t record) override {
        calls.emplace_back("cleanup_string");
        strings.erase(words[record]);
        words[record] = 0u;
    }
    void SetText(const std::uint32_t control,
                 const std::u32string& value) override {
        Check(control == kEdit, "output text uses edit control");
        text = value;
        calls.emplace_back("set_text");
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("event_router");
        return 0xd000u;
    }
    std::uint32_t RegisterRouterEvent(
        const std::uint32_t router, const std::uint32_t mode,
        const std::uint32_t key, const std::uint32_t owner,
        const std::uint32_t callback) override {
        Check(router == 0xd000u && mode == 2u && owner == kWindow &&
              key == (register_count ? 0xa200u : 0xa100u) &&
              callback == (register_count
                  ? pal4::CombatEventOutputWindow::kUnhandledCallback
                  : pal4::CombatEventOutputWindow::kHandledCallback),
              "event registrations use handled/unhandled keys and callbacks");
        const auto result = 0xe000u + 0x100u * register_count++;
        event_handles.push_back(result);
        calls.emplace_back("register_router_event");
        return result;
    }
    std::uint32_t RemoveRouterEvent(
        const std::uint32_t router, const std::uint32_t key,
        const std::uint32_t callback) override {
        Check(router == 0xd000u &&
              key == (removed_handles.empty() ? 0xa100u : 0xa200u),
              "cleanup uses handled then unhandled event group");
        removed_handles.push_back(callback);
        calls.emplace_back("remove_router_event");
        return 0x77u;
    }
};

void FactoryAndConstruction() {
    Runtime runtime;
    Check(pal4::CombatEventOutputFactory::GetTypeThunk(runtime) == 0xa000u &&
          runtime.words[pal4::CombatEventOutputFactory::kTypeCache] ==
              0xa000u &&
          pal4::CombatEventOutputFactory::RegisterFactoryThunk(runtime) ==
              0x55u,
          "type thunk caches name and factory thunk registers creator");
    Check(pal4::CombatEventOutputFactory::RegisterShowEventThunk(runtime) ==
              pal4::CombatEventOutputFactory::kShowEventSentinel &&
          runtime.calls[runtime.calls.size() - 2u] ==
              "insert_show_event" &&
          runtime.calls.back() == "delete_handler",
          "show event installs and releases temporary callback");

    Check(pal4::CombatEventOutputFactory::Create(0xb000u, runtime) ==
              kWindow &&
          runtime.words[kWindow] ==
              pal4::CombatEventOutputWindow::kVtable &&
          runtime.bytes[kWindow + 48u] == 0u &&
          runtime.words[kWindow + 52u] == 0x5000u &&
          runtime.words[0x5000u] == 0x5000u &&
          runtime.words[0x5000u + 4u] == 0x5000u &&
          runtime.words[kWindow + 56u] == 0u &&
          runtime.words[kWindow + 60u] == 20u &&
          runtime.words[kWindow + 64u] == kEdit &&
          runtime.words[kWindow + 68u] == kCombo &&
          runtime.words[kWindow + 72u] == kRoot &&
          runtime.controls == std::vector<std::string>{
              "CombatEventOutput/EditEventOutput",
              "CombatEventOutput/ComboEventLv",
              "CombatEventOutput/WinCombatEventOutput"} &&
          runtime.labels == std::vector<std::string>{
              "CEL_PRIOR", "CEL_STANDARD", "CEL_INFORMATIVE",
              "CEL_INSANE"} &&
          runtime.combo_items.size() == 4u &&
          runtime.selected == runtime.combo_items[0] &&
          runtime.calls.back() == "cleanup_string_buffer",
          "constructor builds level combo, controls and circular list head");
    for (std::uint32_t index = 0u; index < 4u; ++index)
        Check(runtime.words[runtime.combo_items[index] + 312u] == index,
              "level item carries its ordinal at +312");

    Runtime failed;
    failed.fail_factory = true;
    Check(pal4::CombatEventOutputFactory::Create(0xb000u, failed) == 0u &&
          failed.calls.empty(),
          "factory allocation failure does not construct window");
}

void SubscriptionsAndVisibility() {
    Runtime runtime;
    Check(pal4::CombatEventOutputWindow::RegisterEvents(
              kWindow, runtime) == 0xe100u &&
          runtime.event_handles == std::vector<std::uint32_t>{
              0xe000u, 0xe100u} &&
          runtime.words[kWindow + 40u] == 0xe100u &&
          runtime.words[kWindow + 44u] == 0u,
          "second event registration overwrites first handle at +40");
    Check(pal4::CombatEventOutputWindow::ClearEvents(
              kWindow, runtime) == 0x77u &&
          runtime.removed_handles == std::vector<std::uint32_t>{
              0xe100u, 0u} &&
          runtime.words[kWindow + 40u] == 0u &&
          runtime.words[kWindow + 44u] == 0u,
          "cleanup reads overwritten +40 and unchanged +44 slots");

    Runtime visible;
    Check(pal4::CombatEventOutputWindow::SetVisible(
              kWindow, true, 0x7700u, visible) == 0xe100u &&
          visible.calls.front() == "show" &&
          pal4::CombatEventOutputWindow::SetVisible(
              kWindow, false, 0x7700u, visible) == 0x77u &&
          std::find(visible.calls.begin(), visible.calls.end(), "hide") !=
              visible.calls.end(),
          "visibility dispatches registration and removal after parent set");
}

void EventFilteringRolloverAndLifetime() {
    Runtime runtime;
    Check(pal4::CombatEventOutputWindow::Construct(
              kWindow, 0xb000u, runtime) == kWindow,
          "test window constructs");
    runtime.words[kEvent] = 0xb100u;
    runtime.strings[0xb100u] = "first";
    runtime.words[kEvent + 4u] = 1u;
    pal4::CombatEventOutputWindow::HandleEventHandled(
        kWindow, kEvent, runtime);
    Check(runtime.words[kWindow + 56u] == 0u && runtime.text.empty(),
          "priority level rejects standard event");
    runtime.words[kEvent + 4u] = 0u;
    pal4::CombatEventOutputWindow::HandleEventHandled(
        kWindow, kEvent, runtime);
    Check(runtime.words[kWindow + 56u] == 1u &&
          runtime.text == U"first\n",
          "handled event appends text and newline");

    runtime.selected = runtime.combo_items[3];
    runtime.words[kWindow + 60u] = 1u;
    runtime.words[kEvent] = 0xb200u;
    runtime.strings[0xb200u] = "second";
    runtime.words[kEvent + 4u] = 3u;
    pal4::CombatEventOutputWindow::HandleEventUnhandled(
        kWindow, kEvent, runtime);
    Check(runtime.words[kWindow + 56u] == 1u &&
          runtime.text == U"second\n" &&
          runtime.freed == std::vector<std::uint32_t>{0x5100u},
          "rollover removes oldest node after count exceeds limit");
    Check(pal4::CombatEventOutputWindow::ClearEntries(
              kWindow, runtime) == 1u &&
          runtime.words[kWindow + 56u] == 0u && runtime.text.empty() &&
          runtime.words[0x5000u] == 0x5000u,
          "combo selection clears entries and rebuilds empty text");
    Check(pal4::CombatEventOutputWindow::DeletingDestructor(
              kWindow, 1u, runtime) == kWindow &&
          runtime.words[kWindow + 52u] == 0u &&
          std::find(runtime.freed.begin(), runtime.freed.end(), 0x5000u) !=
              runtime.freed.end() &&
          runtime.freed.back() == kWindow,
          "deleting destructor releases list head then object");
}

}  // namespace

int main() {
    try {
        FactoryAndConstruction();
        SubscriptionsAndVisibility();
        EventFilteringRolloverAndLifetime();
        std::cout << "combat_event_output_window_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
