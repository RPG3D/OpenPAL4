#include "pal4/pal_movement_adapter.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalMovementAdapterRuntime {
    std::deque<std::uint32_t> next_allocations;
    std::vector<std::uint32_t> sizes;
    std::vector<std::string> calls;
    pal4::PalMovementAdapterState state{};
    pal4::PalMovementAdapterGlobalState* globals = nullptr;
    bool clear_first_after_pre_destroy = false;
    std::uint32_t registered = 0x5500u;
    std::array<float, 2> probe_scores{10.0f, 5.0f};
    std::array<std::uint32_t, 2> probe_valid{1u, 1u};
    bool allow_switch = true;
    std::deque<std::uint32_t> update_results;
    pal4::PalMovementProbeScratch* probe_scratch = nullptr;
    std::uint32_t active_probe_result = 0u;
    std::deque<std::uint32_t> active_probe_results;
    std::vector<std::uint32_t> active_probe_addresses;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        sizes.push_back(size);
        Check(!next_allocations.empty(), "allocation sequence provided");
        const auto result = next_allocations.front();
        next_allocations.pop_front();
        return result;
    }
    pal4::PalMovementAdapterState& StateAt(std::uint32_t address) override {
        calls.emplace_back("state");
        Check(address == 0x1000u, "factory constructs allocated object");
        return state;
    }
    std::uint32_t ConstructPrimary(std::uint32_t address) override {
        calls.emplace_back("primary");
        Check(address == 0x2000u, "primary constructor address");
        return 0x2100u;
    }
    std::uint32_t ConstructSecondary(std::uint32_t address) override {
        calls.emplace_back("secondary");
        Check(address == 0x3000u, "secondary constructor address");
        Check(state.words[0] == 0u || state.words[0] == 0x2100u,
              "primary result written before secondary construction");
        return 0x3100u;
    }
    std::uint32_t SingletonManager() override {
        calls.emplace_back("manager");
        Check(globals != nullptr && (globals->initialized & 1u) != 0u,
              "guard bit set before singleton lookup");
        return 0x4400u;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) override {
        calls.emplace_back("register");
        Check(manager == 0x4400u && name == "palMovmentAdapter" &&
                  id == 1017u && factory == 0x428ba0u &&
                  destroy == 0x428c00u,
              "singleton registration uses IDA name, id and callbacks");
        return registered;
    }
    void NoopDestroy(std::uint32_t address) override {
        calls.emplace_back("noop_destroy");
        Check(address == 0x1000u, "destroy forwards instance");
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free");
        Check(address == 0x1000u, "destroy frees instance after no-op");
        return 0x9001u;
    }
    std::uint32_t PreDestroy(
        std::uint32_t address,
        pal4::PalMovementAdapterState& object) override {
        calls.emplace_back(address == 0x2100u ? "pre_first" : "pre_second");
        Check(address == 0x2100u || address == 0x3100u,
              "pre-destroy receives current child");
        if (address == 0x2100u && clear_first_after_pre_destroy)
            object.words[0] = 0u;
        return address + 2u;
    }
    std::uint32_t DeleteChild(std::uint32_t address,
                              std::uint32_t flags) override {
        calls.emplace_back(address == 0x2100u ? "delete_first" : "delete_second");
        Check(flags == 1u &&
                  (address == 0x2100u || address == 0x3100u),
              "child deleting destructor receives flag one");
        return address + 1u;
    }
    std::uint32_t DispatchPrimary(
        std::uint32_t address,
        const std::array<std::uint32_t, 7>& arguments) override {
        calls.emplace_back("dispatch");
        Check(address == 0x2100u, "forwarding targets first child alias");
        received_arguments = arguments;
        return 0x87654321u;
    }
    std::array<std::uint32_t, 7> received_arguments{};
    std::uint32_t ProbeChild(
        std::uint32_t child, std::uint32_t fixed_first,
        std::uint32_t fixed_second, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        pal4::PalMovementProbeScratch& scratch) override {
        Check(fixed_first == 1u && fixed_second == 1u &&
                  first == 11u && second == 22u && third == 33u,
              "probe preserves fixed flags and caller arguments");
        if (probe_scratch == nullptr) probe_scratch = &scratch;
        Check(probe_scratch == &scratch,
              "both probes and re-probe share stack scratch");
        const auto index = child == 0x2100u ? 0u : 1u;
        Check(child == 0x2100u || child == 0x3100u,
              "probe targets one of the two child pointers");
        calls.emplace_back(index == 0u ? "probe_first" : "probe_second");
        scratch.score = probe_scores[index];
        return probe_valid[index];
    }
    std::uint32_t CanSwitch(
        std::uint32_t child, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t fourth) override {
        Check(child == 0x2100u && first == 11u && second == 22u &&
                  third == 33u && fourth == 44u,
              "switch gate receives selected child and four arguments");
        calls.emplace_back("can_switch");
        return allow_switch ? 1u : 0u;
    }
    std::uint32_t UpdateChild(
        std::uint32_t child, std::uint32_t first,
        std::uint32_t second, std::uint32_t third) override {
        Check(first == 11u && second == 22u && third == 33u,
              "child update forwards first three arguments");
        Check(child == 0x2100u || child == 0x3100u,
              "valid state has an active child for final update");
        calls.emplace_back(child == 0x2100u ?
                               "update_first" : "update_second");
        if (update_results.empty()) return 1u;
        const auto result = update_results.front();
        update_results.pop_front();
        return result;
    }
    void DeactivateChild(std::uint32_t child) override {
        Check(child == 0x3100u, "old active child receives slot 48");
        calls.emplace_back("deactivate_second");
    }
    void ActivateChild(std::uint32_t child) override {
        Check(child == 0x2100u, "selected child receives slot 44");
        calls.emplace_back("activate_first");
    }
    std::uint32_t ProbeActive(
        std::uint32_t child, std::uint32_t fixed_first,
        std::uint32_t fixed_second,
        const std::array<std::uint32_t, 4>& arguments) override {
        Check((child == 0x2100u || child == 0x3100u) &&
                  fixed_first == 0u &&
                  fixed_second == 0u &&
                  arguments == std::array<std::uint32_t, 4>{
                      11u, 22u, 33u, 44u},
              "active probe forwards two zeros and four caller arguments");
        calls.emplace_back("probe_active");
        active_probe_addresses.push_back(child);
        if (!active_probe_results.empty()) {
            const auto result = active_probe_results.front();
            active_probe_results.pop_front();
            return result;
        }
        return active_probe_result;
    }
};

void TestSingletonRegistration() {
    pal4::PalMovementAdapterGlobalState globals{0x80u, 0x77u};
    Runtime runtime;
    runtime.globals = &globals;
    Check(pal4::PalMovementAdapter::Initialize(globals, runtime) == 0x5500u &&
              globals.initialized == 0x81u &&
              globals.instance == 0x5500u &&
              runtime.calls == std::vector<std::string>{"manager", "register"},
          "first registration sets guard and stores instance");
    runtime.calls.clear();
    runtime.registered = 0x6600u;
    Check(pal4::PalMovementAdapter::Initialize(globals, runtime) == 0x5500u &&
              runtime.calls.empty(),
          "later registration returns cached instance");
    globals = {0x83u, 0x9900u};
    Check(pal4::PalMovementAdapter::Initialize(globals, runtime) == 0x9900u &&
              globals.initialized == 0x83u && runtime.calls.empty(),
          "other guard bits and preinitialized cache are preserved");
}

void TestFactoryAndConstructor() {
    Runtime runtime;
    runtime.state.words.fill(0xdeadbeefu);
    runtime.next_allocations = {0x1000u, 0x2000u, 0x3000u};
    Check(pal4::PalMovementAdapter::Create(runtime) == 0x1000u &&
              runtime.sizes == std::vector<std::uint32_t>{0x14u, 0x74u, 0x88u} &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "state", "allocate", "primary",
                  "allocate", "secondary"},
          "factory allocates object and child records in IDA order");
    Check(runtime.state.words ==
              std::array<std::uint32_t, 5>{
                  0x2100u, 0x3100u, 0x2100u, 0u, 0u},
          "constructor stores children, first-child alias and zero tail");

    Runtime failed;
    failed.next_allocations = {0u};
    Check(pal4::PalMovementAdapter::Create(failed) == 0u &&
              failed.calls == std::vector<std::string>{"allocate"} &&
              failed.sizes == std::vector<std::uint32_t>{0x14u},
          "outer allocation failure skips constructor");

    for (const auto primary : {0u, 0x2000u}) {
        for (const auto secondary : {0u, 0x3000u}) {
            Runtime partial;
            partial.next_allocations = {primary, secondary};
            partial.state.words.fill(0xdeadbeefu);
            auto& returned = pal4::PalMovementAdapter::Construct(
                partial.state, partial);
            Check(&returned == &partial.state &&
                      partial.sizes ==
                          std::vector<std::uint32_t>{0x74u, 0x88u} &&
                      partial.state.words[0] ==
                          (primary ? 0x2100u : 0u) &&
                      partial.state.words[1] ==
                          (secondary ? 0x3100u : 0u) &&
                      partial.state.words[2] == partial.state.words[0] &&
                      partial.state.words[3] == 0u &&
                      partial.state.words[4] == 0u,
                  "each child-allocation outcome keeps the 20-byte layout");
        }
    }
}

void TestDestroyAndDestruct() {
    Runtime runtime;
    Check(pal4::PalMovementAdapter::Destroy(0u, 0x4321u, runtime) ==
              0x4321u && runtime.calls.empty(),
          "null singleton destroy preserves unspecified return register");
    Check(pal4::PalMovementAdapter::Destroy(0x1000u, 0u, runtime) ==
              0x9001u &&
              runtime.calls ==
                  std::vector<std::string>{"noop_destroy", "free"},
          "non-null singleton destroy calls no-op before free");

    runtime.calls.clear();
    runtime.state.words = {0x2100u, 0x3100u, 0x2100u, 0xfeedu, 0x1234u};
    Check(pal4::PalMovementAdapter::Destruct(
              runtime.state, 0x9999u, runtime) == 0x3101u &&
              runtime.calls == std::vector<std::string>{
                  "pre_first", "delete_first", "pre_second", "delete_second"},
          "destructor invokes both child virtuals in order");
    Check(runtime.state.words ==
              std::array<std::uint32_t, 5>{0u, 0u, 0x2100u, 0xfeedu, 0u},
          "destructor clears owned pointers and last field only");

    runtime.calls.clear();
    runtime.clear_first_after_pre_destroy = true;
    runtime.state.words = {0x2100u, 0x3100u, 0u, 0u, 1u};
    Check(pal4::PalMovementAdapter::Destruct(
              runtime.state, 0u, runtime) == 0x3101u &&
              runtime.calls == std::vector<std::string>{
                  "pre_first", "pre_second", "delete_second"},
          "post-callback pointer reread can suppress deleting destructor");

    runtime.calls.clear();
    runtime.state.words = {0u, 0u, 7u, 8u, 9u};
    Check(pal4::PalMovementAdapter::Destruct(
              runtime.state, 0xabab1234u, runtime) == 0xabab1234u &&
              runtime.calls.empty() && runtime.state.words[4] == 0u,
          "empty destructor leaves undefined return explicit and clears tail");
}

void TestPrimaryForwarding() {
    Runtime runtime;
    const std::array<std::uint32_t, 7> arguments{
        11u, 22u, 33u, 44u, 55u, 66u, 77u};
    runtime.state.words = {0u, 0u, 0u, 0u, 0u};
    Check(pal4::PalMovementAdapter::ForwardPrimary(
              runtime.state, arguments, runtime) == 0u &&
              runtime.calls.empty(),
          "missing alias returns zero without virtual call");
    runtime.state.words[2] = 0x2100u;
    Check(pal4::PalMovementAdapter::ForwardPrimary(
              runtime.state, arguments, runtime) == 0x87654321u &&
              runtime.received_arguments == arguments,
          "active first child receives all seven arguments unchanged");
    runtime.state.words[4] = 0xffffffffu;
    runtime.calls.clear();
    Check(pal4::PalMovementAdapter::ForwardPrimary(
              runtime.state, arguments, runtime) == 0x87654321u &&
              runtime.received_arguments ==
                  std::array<std::uint32_t, 7>{
                      11u, 0u, 33u, 44u, 55u, 66u, 77u} &&
              runtime.calls == std::vector<std::string>{"dispatch"},
          "nonzero tail masks only second argument");
}

pal4::PalMovementAdapterState SelectionState() {
    return {{{0x2100u, 0x3100u, 0x3100u, 99u, 0u}}};
}

std::uint32_t Select(pal4::PalMovementAdapterState& state,
                     Runtime& runtime) {
    return pal4::PalMovementAdapter::SelectAndUpdate(
        state, 11u, 22u, 33u, 44u, runtime);
}

void TestSelectionSwitchesInOrder() {
    auto state = SelectionState();
    Runtime runtime;
    Check(Select(state, runtime) == 1u &&
              state.words[2] == 0x2100u && state.words[3] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "probe_first", "probe_second", "can_switch",
                  "update_first", "deactivate_second", "activate_first",
                  "probe_first", "update_first"},
          "higher-scoring child switches after ordered virtual calls");
}

void TestSelectionKeepsCurrentOnEqualOrInvalidScores() {
    auto state = SelectionState();
    state.words[2] = 0x2100u;
    Runtime runtime;
    runtime.probe_scores = {10.0f, 10.0f};
    Check(Select(state, runtime) == 1u &&
              state.words[2] == 0x2100u && state.words[3] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "probe_first", "probe_second", "update_first"},
          "strict score comparison leaves equal second candidate unselected");

    state = SelectionState();
    runtime = Runtime{};
    runtime.probe_valid = {0u, 0u};
    Check(Select(state, runtime) == 0u &&
              state.words[2] == 0x3100u && state.words[3] == 99u &&
              runtime.calls == std::vector<std::string>{
                  "probe_first", "probe_second", "update_second"},
          "failed probes keep current child and previous selected index");

    state = SelectionState();
    runtime = Runtime{};
    runtime.probe_scores = {
        std::numeric_limits<float>::quiet_NaN(), 5.0f};
    Check(Select(state, runtime) == 1u &&
              state.words[2] == 0x3100u && state.words[3] == 1u,
          "unordered score is skipped and second valid score wins");

    state = SelectionState();
    runtime = Runtime{};
    runtime.probe_scores = {
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::infinity()};
    Check(Select(state, runtime) == 0u && state.words[3] == 99u,
          "sentinel equality and negative infinity do not select a child");
}

void TestSelectionRejectedOrDeferredSwitch() {
    auto state = SelectionState();
    Runtime runtime;
    runtime.allow_switch = false;
    Check(Select(state, runtime) == 0u &&
              state.words[2] == 0x3100u && state.words[3] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "probe_first", "probe_second", "can_switch",
                  "update_second"},
          "failed switch gate clears result but retains selected index");

    state = SelectionState();
    runtime = Runtime{};
    runtime.update_results = {0u};
    Check(Select(state, runtime) == 1u &&
              state.words[2] == 0x3100u && state.words[3] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "probe_first", "probe_second", "can_switch",
                  "update_first", "update_second"},
          "failed selected-child update skips switch but preserves score result");
}

void TestActiveProbe() {
    auto state = SelectionState();
    Runtime runtime;
    const std::array<std::uint32_t, 4> arguments{
        11u, 22u, 33u, 44u};
    state.words[2] = 0u;
    Check(!pal4::PalMovementAdapter::CheckActive(
              state, arguments, runtime) && runtime.calls.empty(),
          "null active child suppresses virtual probe");
    state.words[2] = 0x3100u;
    for (const auto result : {0u, 2u, 0xffffffffu}) {
        runtime.calls.clear();
        runtime.active_probe_result = result;
        Check(pal4::PalMovementAdapter::CheckActive(
                  state, arguments, runtime) == (result != 0u) &&
                  runtime.calls ==
                      std::vector<std::string>{"probe_active"},
              "active virtual result is normalized to boolean");
    }
}

void TestAnyChildMatches() {
    const auto state = SelectionState();
    const std::array<std::uint32_t, 4> arguments{
        11u, 22u, 33u, 44u};
    Runtime runtime;
    runtime.active_probe_results = {2u};
    Check(pal4::PalMovementAdapter::AnyChildMatches(
              state, arguments, runtime) &&
              runtime.active_probe_addresses ==
                  std::vector<std::uint32_t>{0x2100u},
          "first matching child short-circuits second probe");
    runtime.calls.clear();
    runtime.active_probe_addresses.clear();
    runtime.active_probe_results = {0u, 0xffffffffu};
    Check(pal4::PalMovementAdapter::AnyChildMatches(
              state, arguments, runtime) &&
              runtime.active_probe_addresses ==
                  std::vector<std::uint32_t>{0x2100u, 0x3100u},
          "second matching child returns normalized one");
    runtime.active_probe_addresses.clear();
    runtime.active_probe_results = {0u, 0u};
    Check(!pal4::PalMovementAdapter::AnyChildMatches(
              state, arguments, runtime) &&
              runtime.active_probe_addresses ==
                  std::vector<std::uint32_t>{0x2100u, 0x3100u},
          "two failed probes return zero");
}

}  // namespace

int main() {
    TestSingletonRegistration();
    TestFactoryAndConstructor();
    TestDestroyAndDestruct();
    TestPrimaryForwarding();
    TestSelectionSwitchesInOrder();
    TestSelectionKeepsCurrentOnEqualOrInvalidScores();
    TestSelectionRejectedOrDeferredSwitch();
    TestActiveProbe();
    TestAnyChildMatches();
    pal4::PalMovementAdapterRuntimeStub runtime;
    pal4::PalMovementAdapterGlobalState globals;
    Check(pal4::PalMovementAdapter::Initialize(globals, runtime) == 0u &&
              pal4::PalMovementAdapter::Create(runtime) == 0u,
          "linkable runtime stub covers singleton and factory");
}
