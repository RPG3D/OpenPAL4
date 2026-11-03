#include "pal4/quick_combat_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IQuickCombatWindowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    bool fail_window = false;
    bool fail_handler = false;

    Runtime() { words[pal4::QuickCombatFactory::kTypePointer] = 0x8b76c0u; }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        allocations.push_back(size);
        if (size == 0x28u) return fail_window ? 0u : 0x4000u;
        if (size == 8u) return fail_handler ? 0u : 0x5000u;
        throw std::runtime_error("unexpected allocation size");
    }
    void LoadLayout(const std::uint32_t window,
                    const std::uint32_t layout) override {
        Check(window == 0x4000u && layout == 0x6000u,
              "quick combat loads caller layout");
        calls.emplace_back("load_layout");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t RegisterFactory(const std::uint32_t type,
                                  const std::uint32_t creator) override {
        Check(type == 0x8b76c0u &&
              creator == pal4::QuickCombatFactory::kCreateCallback,
              "quick combat factory registers cached type and creator");
        calls.emplace_back("factory");
        return 0x33u;
    }
    void InsertShowEvent(const std::uint32_t tree,
                         const std::uint32_t key,
                         const std::uint32_t handler) override {
        Check(tree == 0x7000u + 260u &&
              key == pal4::QuickCombatFactory::kShowEventKey &&
              handler == (fail_handler ? 0u : 0x5000u),
              "show event uses frame manager event tree and key");
        if (handler)
            Check(words[handler] == 0x843494u &&
                  words[handler + 4u] == 0x8b76c0u,
                  "show event handler stores type pointer");
        calls.emplace_back("insert_event");
    }
    void DeleteHandler(const std::uint32_t handler,
                       const std::uint32_t flags) override {
        Check(handler == 0x5000u && flags == 1u,
              "temporary callback is deleted");
        calls.emplace_back("delete_handler");
    }
};

void TypeFactoryAndConstructor() {
    Runtime runtime;
    Check(pal4::QuickCombatFactory::GetTypeThunk(runtime) == 0x8b76c0u &&
          runtime.words[pal4::QuickCombatFactory::kTypeCache] == 0x8b76c0u &&
          pal4::QuickCombatFactory::RegisterFactoryThunk(runtime) == 0x33u &&
          runtime.calls == std::vector<std::string>{"manager", "factory"},
          "type thunk caches quickCombat and registers creator");
    Check(pal4::QuickCombatFactory::Create(0x6000u, runtime) == 0x4000u &&
          runtime.words[0x4000u] == pal4::QuickCombatWindow::kVtable &&
          runtime.allocations == std::vector<std::uint32_t>{0x28u} &&
          runtime.calls.back() == "load_layout",
          "factory allocates 40-byte quickCombat window");
    Runtime failed;
    failed.fail_window = true;
    Check(pal4::QuickCombatFactory::Create(0x6000u, failed) == 0u &&
          failed.calls.empty(),
          "failed window allocation skips layout constructor");
}

void ShowEventRegistration() {
    Runtime runtime;
    Check(pal4::QuickCombatFactory::RegisterShowEventThunk(runtime) ==
              pal4::QuickCombatFactory::kShowEventSentinel &&
          runtime.allocations == std::vector<std::uint32_t>{8u} &&
          runtime.calls == std::vector<std::string>{
              "manager", "insert_event", "delete_handler"},
          "show event installs and releases temporary type callback");
    Runtime failed;
    failed.fail_handler = true;
    Check(pal4::QuickCombatFactory::RegisterShowEvent(failed) ==
              pal4::QuickCombatFactory::kShowEventSentinel &&
          failed.calls == std::vector<std::string>{
              "manager", "insert_event"},
          "failed callback allocation still inserts null handler");
}

}  // namespace

int main() {
    try {
        TypeFactoryAndConstructor();
        ShowEventRegistration();
        std::cout << "quick_combat_window_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
