#include "pal4/equipment_mouse_move_events.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
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

struct Runtime final : pal4::EquipmentMouseMoveRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::unordered_map<std::uint32_t, std::uint32_t> counts;
    std::uint32_t allocations{};
    std::uint32_t disabled{0x9802u};
    std::uint32_t source{0x9803u};
    bool fail_second_allocation{};

    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 12u, "mouse callback allocation size");
        calls.emplace_back("allocate");
        const auto index = allocations++;
        if (fail_second_allocation && index == 1u) return 0u;
        return 0xa000u + index * 0x10u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        writes.emplace_back(address, value);
    }
    std::uint32_t SubscribeMouseMove(std::uint32_t window,
                                     std::uint32_t callback) override {
        Check(window >= 0x9800u && window < 0x9806u,
              "subscribe equipment control");
        calls.emplace_back("subscribe:" + std::to_string(window) + ":" +
                           std::to_string(callback));
        const auto index = window - 0x9800u;
        if (index == 2u) return 0u;
        return 0xb000u + index * 0x10u;
    }
    std::uint32_t ReferenceCount(std::uint32_t reference) override {
        calls.emplace_back("count");
        return counts.at(reference);
    }
    void SetReferenceCount(std::uint32_t reference,
                           std::uint32_t count) override {
        calls.emplace_back("set-count");
        counts[reference] = count;
    }
    void DeleteReference(std::uint32_t reference,
                         std::uint32_t flags) override {
        Check(flags == 1u && counts.at(reference) == 0u,
              "zero-count reference deleting destructor");
        calls.emplace_back("delete-ref:" + std::to_string(reference));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5008u, "mouse event source at +8");
        calls.emplace_back("source");
        return source;
    }
    bool IsDisabled(std::uint32_t window) override {
        calls.emplace_back("disabled:" + std::to_string(window));
        return window == disabled;
    }
    void SetHoverFlag(std::uint32_t window, bool hovered) override {
        Check(!hovered, "mouse move only clears hover on other controls");
        calls.emplace_back("clear:" + std::to_string(window));
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.emplace_back("redraw:" + std::to_string(window));
    }
    int CallMemberVirtual(std::uint32_t member,
                          std::uint32_t slot) override {
        Check(member == 0x7000u && slot == 0x30u,
              "callback forwards to member virtual slot +0x30");
        calls.emplace_back("member-virtual");
        return 19;
    }
};

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    for (std::uint32_t index = 0u; index < 6u; ++index)
        state.Write(0x98u + 4u * index, 0x9800u + index);
    return state;
}

Runtime MakeRuntime() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < 6u; ++index)
        runtime.counts[0xb000u + index * 0x10u] =
            index == 0u ? 2u : 1u;
    return runtime;
}

void TestRegistration() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    Check(pal4::EquipmentMouseMoveEvents::Register(
              state, 0x7000u, runtime),
          "registration returns true");
    Check(runtime.allocations == 6u && runtime.writes.size() == 18u,
          "six 12-byte callbacks get three field writes each");
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto callback = 0xa000u + index * 0x10u;
        Check(runtime.writes[index * 3u] ==
                  std::pair{callback,
                            pal4::EquipmentMouseMoveEvents::kCallbackVtable} &&
                  runtime.writes[index * 3u + 1u] ==
                  std::pair{callback + 4u,
                            pal4::EquipmentMouseMoveEvents::kCallbackHandler} &&
                  runtime.writes[index * 3u + 2u] ==
                  std::pair{callback + 8u, 0x7000u},
              "callback stores vtable, handler and member address");
    }
    Check(runtime.counts.at(0xb000u) == 1u &&
              runtime.counts.at(0xb010u) == 0u &&
              runtime.counts.at(0xb020u) == 1u,
          "reference release decrements only non-null returns");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "delete-ref:45056") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "delete-ref:45072") != runtime.calls.end(),
          "only zero remaining reference is destroyed");

    runtime = MakeRuntime();
    runtime.fail_second_allocation = true;
    Check(pal4::EquipmentMouseMoveEvents::Register(
              state, 0x7000u, runtime),
          "allocation failure still completes registration loop");
    Check(runtime.writes.size() == 15u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "subscribe:38913:0") != runtime.calls.end(),
          "failed allocation subscribes null callback without field writes");
}

void TestMouseMove() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    Check(pal4::EquipmentMouseMoveEvents::Handle(
              state, 0x5000u, runtime),
          "mouse move returns true");
    Check(runtime.calls == std::vector<std::string>{
              "source", "disabled:38912", "clear:38912",
              "redraw:38912", "disabled:38913", "clear:38913",
              "redraw:38913", "disabled:38914", "disabled:38915",
              "disabled:38916", "clear:38916", "redraw:38916",
              "disabled:38917", "clear:38917", "redraw:38917"},
          "mouse move clears other enabled controls, not source or disabled");
}

void TestCallbackForwarder() {
    Runtime runtime;
    Check(pal4::EquipmentMouseMoveEvents::InvokeMemberVirtual(
              0x7000u, runtime) == 19 &&
              runtime.calls == std::vector<std::string>{"member-virtual"},
          "registered handler returns member virtual result");
}

}  // namespace

int main() {
    TestRegistration();
    TestMouseMove();
    TestCallbackForwarder();
}
