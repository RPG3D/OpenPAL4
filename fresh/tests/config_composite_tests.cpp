#include "pal4/config_composite.h"

#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Write = std::tuple<std::uint32_t, std::uint32_t>;

struct Runtime final : pal4::IConfigCompositeRuntime {
    std::vector<std::uint32_t> allocations{
        0x2000u, 0x3000u, 0x4000u, 0x5000u};
    std::vector<Write> writes;
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> events;
    std::uint32_t allocate_calls = 0u;
    std::uint32_t load_result = 0u;
    std::uint32_t atomic_result = 0u;
    std::uint32_t last_context = 0u;
    std::uint32_t last_callback = 0u;
    std::uint32_t bounds_calls = 0u;
    pal4::ConfigCompositeState* expected_state = nullptr;
    std::array<std::uint32_t, 5u> last_context_words{};

    [[nodiscard]] std::uint32_t AllocateNode(
        const std::uint32_t size) override {
        Check(size == 32u, "each list sentinel allocates 32 bytes");
        Check(allocate_calls < allocations.size(),
              "no fifth sentinel allocation");
        return allocations[allocate_calls++];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        writes.emplace_back(address, value);
        memory[address] = value;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        const auto it = memory.find(address);
        Check(it != memory.end(), "read from initialized list memory");
        return it->second;
    }
    [[nodiscard]] std::uint32_t FreeNode(
        const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    [[nodiscard]] std::uint32_t DestroyClump(
        const std::uint32_t address) override {
        events.push_back("clump:" + std::to_string(address));
        return address + 2u;
    }
    [[nodiscard]] std::uint32_t GameInstance() override {
        events.push_back("game");
        return 0x9000u;
    }
    [[nodiscard]] std::uint32_t LoadClump(
        const char* source, const std::uint32_t flags,
        const std::uint32_t texture_context) override {
        Check(flags == 0u, "clump load uses zero flags");
        last_context = texture_context;
        events.push_back(std::string("load:") + source);
        return load_result;
    }
    void AddClumpBounds(const std::uint32_t clump,
                        const std::uint32_t mode,
                        pal4::ConfigCompositeState& state,
                        const std::uint32_t bounds_word_index) override {
        Check(clump == load_result && mode == 0u &&
                  bounds_word_index == 14u && state.words[13u] == clump,
              "floor bounds use stored clump and word 14");
        ++bounds_calls;
        events.push_back("bounds");
    }
    [[nodiscard]] std::uint32_t ForEachAtomic(
        const std::uint32_t clump, const std::uint32_t callback,
        pal4::ConfigCompositeState& state) override {
        Check(clump == load_result && &state == expected_state,
              "atomic traversal uses loaded clump and object");
        last_callback = callback;
        events.push_back("atomics");
        return atomic_result;
    }
    [[nodiscard]] std::uint32_t ForEachAtomicWithContext(
        const std::uint32_t clump, const std::uint32_t callback,
        const std::array<std::uint32_t, 5u>& context) override {
        Check(clump == 0x6000u && callback == 0x5ffbc0u,
              "property traversal routes to fourth atomic callback");
        last_context_words = context;
        events.push_back("properties");
        return atomic_result;
    }
};

void LinkNodes(Runtime& runtime, pal4::ConfigCompositeState& state,
               const std::uint32_t list_index,
               const std::vector<std::uint32_t>& nodes) {
    const auto head = state.words[list_index * 3u + 2u];
    runtime.WriteWord(head, nodes.empty() ? head : nodes.front());
    runtime.WriteWord(head + 4u, nodes.empty() ? head : nodes.back());
    for (std::size_t index = 0u; index < nodes.size(); ++index) {
        runtime.WriteWord(nodes[index],
                          index + 1u == nodes.size() ? head : nodes[index + 1u]);
        runtime.WriteWord(nodes[index] + 4u,
                          index == 0u ? head : nodes[index - 1u]);
    }
    state.words[list_index * 3u + 3u] =
        static_cast<std::uint32_t>(nodes.size());
}

void TestFourSentinelsAndNativeFields() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    state.words.fill(0xaaaaaaaau);
    Check(&pal4::ConfigComposite::Initialize(
              state, 0x5a123456u, runtime) == &state,
          "constructor returns original state");
    Check(runtime.allocate_calls == 4u &&
              runtime.writes == std::vector<Write>{
                  {0x2000u, 0x2000u}, {0x2004u, 0x2000u},
                  {0x3000u, 0x3000u}, {0x3004u, 0x3000u},
                  {0x4000u, 0x4000u}, {0x4004u, 0x4000u},
                  {0x5000u, 0x5000u}, {0x5004u, 0x5000u}},
          "each 32-byte sentinel links its first two words to itself");
    Check(state.words[0u] == pal4::ConfigComposite::kVtable,
          "vtable installed after four allocations");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto base = index * 3u + 1u;
        Check(state.words[base] == 0xaaaaaa5au &&
                  state.words[base + 1u] == runtime.allocations[index] &&
                  state.words[base + 2u] == 0u,
              "four byte flags, sentinel pointers and empty sizes");
    }
    Check(state.words[13u] == 0u,
          "first clump/resource pointer cleared");
    for (std::uint32_t index = 14u; index < 20u; ++index) {
        Check(state.words[index] == 0xaaaaaaaau,
              "unwritten middle fields retain their previous contents");
    }
    for (std::uint32_t index = 20u; index <= 26u; ++index) {
        Check(state.words[index] == 0u,
              "last seven resource words cleared");
    }
}

void TestAllocationFailureRetainsConstructedPrefix() {
    Runtime runtime;
    runtime.allocations[2u] = 0u;
    pal4::ConfigCompositeState state;
    state.words.fill(0x77777777u);
    bool threw = false;
    try {
        (void)pal4::ConfigComposite::Initialize(
            state, 0x20123456u, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.allocate_calls == 3u &&
              runtime.writes.size() == 4u,
          "zero allocator result raises defined C++ failure after two lists");
    Check(state.words[0u] == 0x77777777u &&
              state.words[2u] == 0x2000u &&
              state.words[5u] == 0x3000u &&
              state.words[7u] == 0x77777720u &&
              state.words[8u] == 0x77777777u,
          "partial native state retained and vtable not installed");
}

void TestResourceCleanupOnPopulatedLists() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    (void)pal4::ConfigComposite::Initialize(state, 0u, runtime);
    LinkNodes(runtime, state, 0u, {0x2100u, 0x2200u});
    LinkNodes(runtime, state, 1u, {0x3100u});
    LinkNodes(runtime, state, 2u, {});
    LinkNodes(runtime, state, 3u, {0x5100u, 0x5200u});
    state.words[13u] = 0x6000u;
    state.words[20u] = 0x7000u;
    state.words[21u] = 0x8000u;
    state.words[22u] = 0x9999u;
    for (std::uint32_t index = 23u; index <= 26u; ++index) {
        state.words[index] = 0x9999u;
    }

    Check(pal4::ConfigComposite::CleanupResources(state, runtime) == 0x8002u,
          "resource cleanup returns last clump destroy result");
    Check(runtime.events == std::vector<std::string>{
              "free:8448", "free:8704", "free:12544", "free:20736",
              "free:20992", "clump:24576", "clump:28672", "clump:32768"},
          "nodes and clumps released in native list order");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto head = state.words[index * 3u + 2u];
        Check(state.words[index * 3u + 3u] == 0u &&
                  runtime.memory[head] == head &&
                  runtime.memory[head + 4u] == head,
              "list count and sentinel links restored");
    }
    Check(state.words[13u] == 0u && state.words[20u] == 0u &&
              state.words[21u] == 0u && state.words[22u] == 0x9999u,
          "only owned clump slots reset");
    for (std::uint32_t index = 23u; index <= 26u; ++index) {
        Check(state.words[index] == 0u, "resource tail cleared");
    }
    runtime.events.clear();
    Check(pal4::ConfigComposite::CleanupResources(state, runtime) == 0u &&
              runtime.events.empty(),
          "empty resource cleanup is repeatable");
}

void TestFullCleanupOrderAndReturn() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    (void)pal4::ConfigComposite::Initialize(state, 0u, runtime);
    LinkNodes(runtime, state, 0u, {0x2100u});
    LinkNodes(runtime, state, 1u, {});
    LinkNodes(runtime, state, 2u, {0x4100u});
    LinkNodes(runtime, state, 3u, {0x5100u});
    state.words[0u] = 0u;
    Check(pal4::ConfigComposite::CleanupFull(state, runtime) == 0x2001u,
          "full cleanup returns first sentinel free result");
    Check(runtime.events == std::vector<std::string>{
              "free:8448", "free:16640", "free:20736", "free:20480",
              "free:16384", "free:12288", "free:8192"},
          "resource nodes precede reverse-order sentinel release");
    Check(state.words[0u] == pal4::ConfigComposite::kVtable,
          "full cleanup reinstalls class vtable");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        Check(state.words[index * 3u + 2u] == 0u &&
                  state.words[index * 3u + 3u] == 0u,
              "full cleanup clears sentinel and count");
    }
}

void TestThreeClumpLoads() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    runtime.expected_state = &state;
    runtime.load_result = 0x6000u;
    runtime.atomic_result = 0x1234u;
    Check(pal4::ConfigComposite::LoadFloorClump(state, "floor.dff", runtime)
              == 0x6000u && state.words[13u] == 0x6000u,
          "floor load stores and returns clump instead of callback result");
    Check(runtime.events ==
              std::vector<std::string>{"game", "load:floor.dff", "bounds", "atomics"} &&
              runtime.last_context == 0x91acu &&
              runtime.last_callback == 0x5ff9e0u && runtime.bounds_calls == 1u,
          "floor load uses game texture context, bounds and first callback");

    runtime.events.clear();
    runtime.load_result = 0x7000u;
    Check(pal4::ConfigComposite::LoadIntriClump(state, "intri.dff", runtime)
              == 0x7000u && state.words[20u] == 0x7000u &&
              runtime.last_callback == 0x5ffa80u &&
              runtime.events == std::vector<std::string>{
                  "game", "load:intri.dff", "atomics"},
          "intri load stores clump and uses second callback");

    runtime.events.clear();
    runtime.load_result = 0x8000u;
    Check(pal4::ConfigComposite::LoadWallClump(state, "wall.dff", runtime)
              == 0x8000u && state.words[21u] == 0x8000u &&
              runtime.last_callback == 0x5ffb20u &&
              runtime.events == std::vector<std::string>{
                  "game", "load:wall.dff", "atomics"},
          "wall load stores clump and uses third callback");

    runtime.events.clear();
    runtime.load_result = 0u;
    Check(pal4::ConfigComposite::LoadFloorClump(state, "missing", runtime)
              == 0u && state.words[13u] == 0u &&
              runtime.events == std::vector<std::string>{"game", "load:missing"} &&
              runtime.bounds_calls == 1u,
          "failed floor load clears slot and skips bounds and atomic traversal");
    runtime.events.clear();
    Check(pal4::ConfigComposite::LoadIntriClump(state, "missing", runtime)
              == 0u && state.words[20u] == 0u &&
              runtime.events == std::vector<std::string>{"game", "load:missing"},
          "failed intri load skips atomic traversal");
    runtime.events.clear();
    Check(pal4::ConfigComposite::LoadWallClump(state, "missing", runtime)
              == 0u && state.words[21u] == 0u &&
              runtime.events == std::vector<std::string>{"game", "load:missing"},
          "failed wall load skips atomic traversal");
}

void TestClumpPropertiesContext() {
    Runtime runtime;
    runtime.atomic_result = 0xabcdu;
    Check(pal4::ConfigComposite::SetClumpProperties(
              0x12340000u, 0u, 1u, 2u, 3u, 4u, runtime) == 0u &&
              runtime.events.empty(),
          "null clump bypasses atomic traversal");
    Check(pal4::ConfigComposite::SetClumpProperties(
              0x12340000u, 0x6000u, 11u, 22u, 33u, 44u, runtime)
              == 0xabcdu &&
              runtime.last_context_words ==
                  std::array<std::uint32_t, 5u>{
                      0x12340000u, 11u, 22u, 33u, 44u} &&
              runtime.events == std::vector<std::string>{"properties"},
          "five-word native context and traversal result preserved");
}

void TestUnregisterClumpAtomics() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    runtime.expected_state = &state;
    runtime.load_result = 0x6000u;
    runtime.atomic_result = 0x9000u;
    state.words[22u] = 17u;
    Check(pal4::ConfigComposite::UnregisterClumpAtomics(
              state, 0u, runtime) == 0u && state.words[22u] == 17u &&
              runtime.events.empty(),
          "null clump leaves unregister flag untouched");
    Check(pal4::ConfigComposite::UnregisterClumpAtomics(
              state, 0x6000u, runtime) == 0x9000u &&
              state.words[22u] == 1u &&
              runtime.last_callback == 0x5ffc60u &&
              runtime.events == std::vector<std::string>{"atomics"},
          "atomic unregister result and flag preserved");
}

void TestEmbeddedListCleanup() {
    for (std::uint32_t selected = 0u; selected < 4u; ++selected) {
        Runtime runtime;
        pal4::ConfigCompositeState state;
        (void)pal4::ConfigComposite::Initialize(state, 0u, runtime);
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            LinkNodes(runtime, state, index,
                      index == selected
                          ? std::vector<std::uint32_t>{
                                0x6000u + index * 0x100u,
                                0x7000u + index * 0x100u}
                          : std::vector<std::uint32_t>{});
        }
        const auto head = state.words[selected * 3u + 2u];
        Check(pal4::ConfigComposite::CleanupList(
                  state, selected, runtime) == 0u,
              "embedded list destructor always returns zero");
        Check(runtime.events == std::vector<std::string>{
                  "free:" + std::to_string(0x6000u + selected * 0x100u),
                  "free:" + std::to_string(0x7000u + selected * 0x100u),
                  "free:" + std::to_string(head)},
              "embedded list releases nodes then sentinel");
        Check(state.words[selected * 3u + 2u] == 0u &&
                  state.words[selected * 3u + 3u] == 0u &&
                  runtime.memory[head] == head &&
                  runtime.memory[head + 4u] == head,
              "embedded list clears head and count after unlinking");
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            if (index == selected) continue;
            Check(state.words[index * 3u + 2u] == runtime.allocations[index],
                  "other list objects remain allocated");
        }
    }
}

void TestRemoveListNodeHelper() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    (void)pal4::ConfigComposite::Initialize(state, 0u, runtime);
    LinkNodes(runtime, state, 2u, {0x6000u, 0x7000u});
    std::uint32_t cursor = 0x9999u;
    Check(&pal4::ConfigComposite::RemoveListNode(
              state, 2u, 0x6000u, cursor, runtime) == &cursor &&
              cursor == 0x7000u && state.words[9u] == 1u &&
              runtime.memory[0x4000u] == 0x7000u &&
              runtime.memory[0x7004u] == 0x4000u &&
              runtime.events == std::vector<std::string>{"free:24576"},
          "remove helper returns supplied cursor after unlink and decrement");
    Check(&pal4::ConfigComposite::RemoveListNode(
              state, 2u, 0x7000u, cursor, runtime) == &cursor &&
              cursor == 0x4000u && state.words[9u] == 0u &&
              runtime.memory[0x4000u] == 0x4000u &&
              runtime.memory[0x4004u] == 0x4000u,
          "last removal points cursor at self-linked sentinel");
}

std::array<std::uint32_t, 6u> PayloadAt(
    Runtime& runtime, const std::uint32_t node) {
    std::array<std::uint32_t, 6u> payload{};
    for (std::uint32_t index = 0u; index < payload.size(); ++index) {
        payload[index] = runtime.memory.at(node + 8u + index * 4u);
    }
    return payload;
}

void TestDefaultAtomicRegistrations() {
    Runtime runtime;
    runtime.allocations.insert(runtime.allocations.end(),
                               {0x6000u, 0x7000u, 0x8000u, 0x9000u});
    pal4::ConfigCompositeState state;
    (void)pal4::ConfigComposite::Initialize(state, 0u, runtime);
    Check(pal4::ConfigComposite::RegisterFloorAtomic(
              state, 0xa000u, runtime) == 0xa000u &&
              pal4::ConfigComposite::RegisterIntriAtomic(
                  state, 0xb000u, runtime) == 0xb000u &&
              pal4::ConfigComposite::RegisterWallAtomic(
                  state, 0xc000u, runtime) == 0xc000u,
          "three registration callbacks return supplied atomic");
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto head = state.words[index * 3u + 2u];
        const auto node = runtime.allocations[4u + index];
        const auto atomic = 0xa000u + index * 0x1000u;
        Check(runtime.memory[head] == node &&
                  runtime.memory[head + 4u] == node &&
                  runtime.memory[node] == head &&
                  runtime.memory[node + 4u] == head &&
                  state.words[index * 3u + 3u] == 1u &&
                  PayloadAt(runtime, node) ==
                      std::array<std::uint32_t, 6u>{
                          atomic, 1u, 0u, 1u, 0u, 0u},
              "three callbacks use their own list and default 24-byte payload");
    }
    Check(runtime.allocate_calls == 7u &&
              pal4::ConfigComposite::RegisterFloorAtomic(
                  state, 0xa000u, runtime) == 0xa000u &&
              pal4::ConfigComposite::RegisterIntriAtomic(
                  state, 0xb000u, runtime) == 0xb000u &&
              pal4::ConfigComposite::RegisterWallAtomic(
                  state, 0xc000u, runtime) == 0xc000u &&
              runtime.allocate_calls == 7u,
          "duplicate atomics do not allocate or increase counts");
    Check(pal4::ConfigComposite::RegisterFloorAtomic(
              state, 0xd000u, runtime) == 0xd000u &&
              state.words[3u] == 2u &&
              runtime.memory[0x2000u] == 0x6000u &&
              runtime.memory[0x2004u] == 0x9000u &&
              runtime.memory[0x6000u] == 0x9000u &&
              runtime.memory[0x9004u] == 0x6000u,
          "new atomic appends before sentinel after prior node");
}

void TestConfiguredRegistrationAndUnregister() {
    Runtime runtime;
    runtime.allocations.insert(runtime.allocations.end(),
                               {0x6000u, 0x7000u});
    pal4::ConfigCompositeState state;
    (void)pal4::ConfigComposite::Initialize(state, 0u, runtime);
    const std::array<std::uint32_t, 5u> context{
        0x12345678u, 2u, 3u, 4u, 0x9000u};
    Check(pal4::ConfigComposite::RegisterConfiguredAtomic(
              state, 0xa000u, context, runtime) == 0xa000u &&
              PayloadAt(runtime, 0x6000u) ==
                  std::array<std::uint32_t, 6u>{
                      0xa000u, 2u, 0u, 3u, 4u, 0x9000u} &&
              state.words[12u] == 1u,
          "configured callback writes mode, entered, two flags and callback");
    Check(pal4::ConfigComposite::RegisterConfiguredAtomic(
              state, 0xb000u, context, runtime) == 0xb000u &&
              state.words[12u] == 2u,
          "second configured atomic appends to fourth list");
    Check(pal4::ConfigComposite::UnregisterAtomic(
              state, 0xc000u, runtime) == 0xc000u &&
              state.words[12u] == 2u && runtime.events.empty(),
          "missing atomic returns input without list mutation");
    Check(pal4::ConfigComposite::UnregisterAtomic(
              state, 0xa000u, runtime) == 0xa000u &&
              state.words[12u] == 1u &&
              runtime.memory[0x5000u] == 0x7000u &&
              runtime.memory[0x7004u] == 0x5000u &&
              runtime.events == std::vector<std::string>{"free:24576"},
          "unregister removes first matching atomic and decrements count");
    Check(pal4::ConfigComposite::UnregisterAtomic(
              state, 0xb000u, runtime) == 0xb000u &&
              state.words[12u] == 0u &&
              runtime.memory[0x5000u] == 0x5000u &&
              runtime.memory[0x5004u] == 0x5000u,
          "unregister restores empty sentinel");
}

void TestAtomicRegistrationAllocationFailure() {
    Runtime runtime;
    runtime.allocations.push_back(0u);
    pal4::ConfigCompositeState state;
    (void)pal4::ConfigComposite::Initialize(state, 0u, runtime);
    bool threw = false;
    try {
        (void)pal4::ConfigComposite::RegisterFloorAtomic(
            state, 0xa000u, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && state.words[3u] == 0u &&
              runtime.memory[0x2000u] == 0x2000u &&
              runtime.memory[0x2004u] == 0x2000u,
          "zero allocation leaves list unchanged and raises defined failure");
}

}  // namespace

int main() {
    TestFourSentinelsAndNativeFields();
    TestAllocationFailureRetainsConstructedPrefix();
    TestResourceCleanupOnPopulatedLists();
    TestFullCleanupOrderAndReturn();
    TestThreeClumpLoads();
    TestClumpPropertiesContext();
    TestUnregisterClumpAtomics();
    TestEmbeddedListCleanup();
    TestRemoveListNodeHelper();
    TestDefaultAtomicRegistrations();
    TestConfiguredRegistrationAndUnregister();
    TestAtomicRegistrationAllocationFailure();
}
