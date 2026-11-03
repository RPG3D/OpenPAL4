#include "pal4/game_manager_collection_walker.h"

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameManagerCollectionRuntime {
public:
    std::uint32_t ReadDword(const std::uint32_t address) override {
        const auto found = memory.find(address);
        Check(found != memory.end(), "tree traversal reads mapped node field");
        return found->second;
    }
    void Invoke(const std::uint32_t object,
                const std::uint32_t slot_offset) override {
        Check(slot_offset == 0x28u || slot_offset == 0x2cu,
              "only observed callback slots are used");
        calls.emplace_back(object, slot_offset);
    }
    void Node(const std::uint32_t address, const std::uint32_t left,
              const std::uint32_t parent, const std::uint32_t right,
              const std::uint32_t object) {
        memory[address] = left;
        memory[address + 4u] = parent;
        memory[address + 8u] = right;
        memory[address + 16u] = object;
    }
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
};

pal4::ScreenEffectDispatcherState State() {
    pal4::ScreenEffectDispatcherState state{};
    state.tree_sentinel = 0x9000u;
    return state;
}

void EmptyTree() {
    const auto state = State();
    Runtime runtime;
    runtime.memory[0x9000u] = 0x9000u;
    Check(pal4::GameManagerCollectionWalker::NotifySlot40(
              state, runtime) == 0x9000u &&
              pal4::GameManagerCollectionWalker::NotifySlot44(
                  state, runtime) == 0x9000u && runtime.calls.empty(),
          "empty collection returns sentinel without callbacks");
    pal4::GameManagerCollectionRuntimeStub stub;
    Check(pal4::GameManagerCollectionWalker::NotifySlot40(
              state, stub) == 0x9000u,
          "empty linkable runtime exits at sentinel");
}

void InOrderTraversalAndSlots() {
    constexpr auto nil = pal4::GameManagerCollectionWalker::kNilNode;
    const auto state = State();
    Runtime runtime;
    runtime.Node(0x9000u, 0x1100u, 0x1000u, 0x1200u, 0u);
    runtime.Node(0x1000u, 0x1100u, 0x9000u, 0x1200u, 0x5000u);
    runtime.Node(0x1100u, nil, 0x1000u, nil, 0x5100u);
    runtime.Node(0x1200u, 0x1300u, 0x1000u, nil, 0x5200u);
    runtime.Node(0x1300u, nil, 0x1200u, nil, 0x5300u);
    Check(pal4::GameManagerCollectionWalker::NotifySlot40(
              state, runtime) == 0x9000u,
          "slot-40 traversal finishes with header sentinel");
    Check(runtime.calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>({
              {0x5100u, 0x28u}, {0x5000u, 0x28u},
              {0x5300u, 0x28u}, {0x5200u, 0x28u}}),
          "right-subtree left descent and parent ascent keep key order");
    runtime.calls.clear();
    Check(pal4::GameManagerCollectionWalker::NotifySlot44(
              state, runtime) == 0x9000u &&
              runtime.calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                  {0x5100u, 0x2cu}, {0x5000u, 0x2cu},
                  {0x5300u, 0x2cu}, {0x5200u, 0x2cu}}),
          "second function differs only in callback slot +44");
}

}  // namespace

int main() {
    EmptyTree();
    InOrderTraversalAndSlots();
    std::cout << "2 game manager collection walker test cases passed\n";
}
