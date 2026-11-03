#include "pal4/role_selector_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRoleSelectorVisibilityRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::uint32_t destroyed = 0u;
    std::uint32_t destroy_flags = 0u;
    std::uint32_t allocated_bytes = 0u;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t cloned_source = 0u;
    std::uint32_t dispatched_name = 0u;
    std::uint32_t dispatched_visible = 99u;
    std::uint32_t dispatched_option = 99u;
    std::uint32_t found_name = 0u;
    std::uint32_t modal_root = 0u;
    bool modal_enabled = false;
    std::uint32_t parent_object = 0u;
    std::uint32_t parent_visible = 0u;
    std::uint32_t parent_window = 0u;
    bool throw_on_clone = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void DestroyCallback(std::uint32_t callback,
                         std::uint32_t flags) override {
        calls.emplace_back("destroy-callback");
        destroyed = callback;
        destroy_flags = flags;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        calls.emplace_back("free");
        freed.push_back(pointer);
        return 0xfeedu;
    }
    std::uint32_t EmptyCallbackResult() override {
        calls.emplace_back("empty-result");
        return 0xbbbbu;
    }
    std::uint32_t DestructBase(std::uint32_t object) override {
        calls.emplace_back("destruct-base");
        Check(object == 0x1000u, "base destructor object");
        return 0xabcdu;
    }
    std::uint32_t Dispatch(std::uint32_t name,
                           std::uint32_t visible,
                           std::uint32_t option) override {
        calls.emplace_back("dispatch");
        dispatched_name = name;
        dispatched_visible = visible;
        dispatched_option = option;
        return 0x1234u;
    }
    std::uint32_t FindFrame(std::uint32_t name) override {
        calls.emplace_back("find-frame");
        found_name = name;
        return 0x1000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocated_bytes = bytes;
        return allocation;
    }
    std::uint32_t CloneCallback(std::uint32_t callback) override {
        calls.emplace_back("clone");
        cloned_source = callback;
        if (throw_on_clone) throw std::runtime_error("clone failed");
        return 0x9000u;
    }
    std::uint32_t LayoutButtons(std::uint32_t object) override {
        calls.emplace_back("layout");
        Check(object == 0x1000u, "layout object");
        return 0x4567u;
    }
    void SetModal(std::uint32_t root, bool enabled) override {
        calls.emplace_back("modal");
        modal_root = root;
        modal_enabled = enabled;
    }
    void SetParent(std::uint32_t object,
                   std::uint32_t visible,
                   std::uint32_t parent) override {
        calls.emplace_back("parent");
        parent_object = object;
        parent_visible = visible;
        parent_window = parent;
    }
};

void Seed(Runtime& runtime, std::uint32_t slot,
          std::uint32_t callback) {
    runtime.words[0x1040u] = slot;
    runtime.words[0x1044u] = 77u;
    runtime.words[0x1024u] = 0x5000u;
    runtime.words[0x1030u] = 0x6000u;
    if (slot != 0u) runtime.words[slot] = callback;
}

void CheckCleanup(std::uint32_t slot, std::uint32_t callback) {
    Runtime runtime;
    Seed(runtime, slot, callback);
    const auto result = pal4::RoleSelectorWindow::CleanupCallback(
        0x1000u, runtime);
    Check(result == (slot == 0u ? 0xbbbbu : 0xfeedu) &&
          runtime.words.at(0x1040u) == 0u &&
          runtime.words.at(0x1044u) == 0u &&
          runtime.freed ==
              (slot == 0u ? std::vector<std::uint32_t>{}
                          : std::vector<std::uint32_t>{slot}) &&
          runtime.destroyed == callback &&
          runtime.destroy_flags == (callback == 0u ? 0u : 1u),
          "callback slot cleanup and result");
    Check(runtime.calls ==
              (slot == 0u
                   ? std::vector<std::string>{
                         "read", "empty-result", "write", "write"}
                   : callback == 0u
                       ? std::vector<std::string>{
                             "read", "read", "free", "write", "write"}
                       : std::vector<std::string>{
                             "read", "read", "destroy-callback",
                             "free", "write", "write"}),
          "callback destruction precedes slot free and zeroing");
}

void CheckDestruct() {
    Runtime runtime;
    Seed(runtime, 0x3000u, 0x9000u);
    Check(pal4::RoleSelectorWindow::Destruct(
              0x1000u, runtime) == 0xabcdu &&
          runtime.words.at(0x1000u) ==
              pal4::RoleSelectorWindow::kVtable &&
          runtime.words.at(0x1028u) ==
              pal4::RoleSelectorWindow::kNavigationVtable &&
          runtime.words.at(0x1030u) == 0u &&
          runtime.words.at(0x1034u) == 0u &&
          runtime.words.at(0x1038u) == 0u &&
          runtime.freed ==
              std::vector<std::uint32_t>{0x3000u, 0x6000u} &&
          runtime.calls.back() == "destruct-base",
          "destructor vtables, callback, navigation storage and base");
}

void CheckHide() {
    Runtime runtime;
    Check(pal4::RoleSelectorWindow::ShowOrHide(
              0x100u, 0x2000u, 7u, runtime) == 0x1234u &&
          runtime.dispatched_name == 0x8b1a60u &&
          runtime.dispatched_visible == 0u &&
          runtime.dispatched_option == 0u &&
          runtime.calls == std::vector<std::string>{"dispatch"},
          "hidden path only dispatches zero event");
}

void CheckShow(bool callback_present, bool allocate) {
    Runtime runtime;
    Seed(runtime, 0x4000u, 0x9000u);
    runtime.words[0x2000u] = 0x8000u;
    if (!allocate) runtime.allocation = 0u;
    Check(pal4::RoleSelectorWindow::ShowOrHide(
              0x101u, callback_present ? 0x2000u : 0u,
              17u, runtime) == 0x4567u &&
          runtime.dispatched_name == 0x8b1a60u &&
          runtime.dispatched_visible == 1u &&
          runtime.dispatched_option == 0u &&
          runtime.found_name == 0x8b1a60u &&
          runtime.freed == std::vector<std::uint32_t>{0x4000u} &&
          runtime.destroyed == 0x9000u &&
          runtime.calls.back() == "layout",
          "show dispatches, finds, clears and relayouts frame");
    if (!callback_present) {
        Check(runtime.words.at(0x1040u) == 0u &&
              runtime.words.at(0x1044u) == 0u &&
              runtime.allocated_bytes == 0u,
              "no callback leaves cleared slot and context");
        return;
    }
    Check(runtime.allocated_bytes == 4u &&
          runtime.words.at(0x1040u) ==
              (allocate ? 0x3000u : 0u) &&
          runtime.words.at(0x1044u) == 17u &&
          runtime.cloned_source == (allocate ? 0x8000u : 0u),
          "optional callback clone and context");
    if (allocate)
        Check(runtime.words.at(0x3000u) == 0x9000u,
              "clone stored in allocated slot");
}

void CheckCloneException() {
    Runtime runtime;
    Seed(runtime, 0u, 0u);
    runtime.words[0x2000u] = 0x8000u;
    runtime.throw_on_clone = true;
    bool caught = false;
    try {
        (void)pal4::RoleSelectorWindow::ShowOrHide(
            1u, 0x2000u, 17u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
          runtime.freed == std::vector<std::uint32_t>{0x3000u} &&
          runtime.words.at(0x1040u) == 0u &&
          runtime.words.at(0x1044u) == 0u &&
          runtime.calls.back() == "free",
          "failed callback clone frees transient slot");
}

void CheckModal(bool visible) {
    Runtime runtime;
    Seed(runtime, 0x3000u, 0x9000u);
    pal4::RoleSelectorWindow::SetModalState(
        0x1000u, visible ? 0x101u : 0x100u,
        0x7000u, runtime);
    Check(runtime.modal_root == 0x5000u &&
          runtime.modal_enabled == visible &&
          runtime.parent_object == 0x1000u &&
          runtime.parent_visible ==
              (visible ? 0x101u : 0x100u) &&
          runtime.parent_window == 0x7000u &&
          runtime.calls.back() == "parent",
          "modal and parent arguments");
    Check(runtime.freed ==
              (visible ? std::vector<std::uint32_t>{}
                       : std::vector<std::uint32_t>{0x3000u}) &&
          runtime.words.at(0x1040u) ==
              (visible ? 0x3000u : 0u),
          "modal exit clears callback first");
}

}  // namespace

int main() {
    try {
        CheckCleanup(0u, 0u);
        CheckCleanup(0x3000u, 0u);
        CheckCleanup(0x3000u, 0x9000u);
        CheckDestruct();
        CheckHide();
        CheckShow(false, true);
        CheckShow(true, false);
        CheckShow(true, true);
        CheckCloneException();
        CheckModal(false);
        CheckModal(true);
        std::cout << "role_selector_lifecycle_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
