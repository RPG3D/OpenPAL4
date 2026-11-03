#include "pal4/npc_manager_object_processor.h"

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::NpcManagerProcessRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::vector<std::uint32_t>> dispatches;
    std::vector<std::vector<std::uint32_t>> grid_updates;

    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void Dispatch(std::uint32_t npc, std::uint32_t slot,
                  std::uint32_t first,
                  std::uint32_t second) override {
        dispatches.push_back({npc, slot, first, second});
    }
    void UpdateGrid(std::uint32_t grid,
                    std::uint32_t first) override {
        grid_updates.push_back({grid, first});
    }
};

struct ForEachRuntime final : pal4::NpcManagerForEachRuntimeStub {
    std::vector<std::uint32_t> arguments;
    std::uint32_t ForEachNpc(std::uint32_t manager,
        std::uint32_t callback, std::uint32_t context) override {
        arguments = {manager, callback, context};
        return 0x4567u;
    }
};

struct CallbackRuntime final : pal4::NpcManagerCallbackRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> visited;
    std::uint32_t stop_after{};
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t Invoke(std::uint32_t callback,
                         std::uint32_t npc,
                         std::uint32_t context) override {
        Check(callback == 0x1100u && context == 0x2200u,
              "callback address and context forwarded");
        visited.push_back(npc);
        return visited.size() == stop_after ? 0u : 1u;
    }
};

pal4::NpcManagerTreeGlobals Globals() {
    return {1u, 0x6000u};
}

void EmptyTree() {
    Runtime runtime;
    runtime.words = {{0x502cu, 0x7000u}, {0x7000u, 0x7000u}};
    Check(pal4::NpcManagerObjectProcessor::Process(
              0x5000u, 0x10u, Globals(), runtime) == 0x7000u &&
          runtime.dispatches.empty() &&
          runtime.reads ==
              std::vector<std::uint32_t>{0x502cu, 0x7000u},
          "empty tree returns header without touching NPCs");
    runtime.reads.clear();
    Check(pal4::NpcManagerObjectProcessor::ProcessExtended(
              0x5000u, 0x11u, 0x22u, Globals(), runtime) == 0x7000u &&
          runtime.grid_updates ==
              std::vector<std::vector<std::uint32_t>>{
                  {0x5038u, 0x11u}} &&
          runtime.reads ==
              std::vector<std::uint32_t>{0x502cu, 0x7000u},
          "extended entry updates grid even when the tree is empty");
}

void SingleNodeAndActivityGate() {
    Runtime runtime;
    runtime.words = {
        {0x502cu, 0x7000u},
        {0x7000u, 0x7100u}, {0x7004u, 0x7100u},
        {0x7008u, 0x7100u},
        {0x7104u, 0x7000u}, {0x7108u, 0x6000u},
        {0x711cu, 0x9000u}, {0x91f0u, 0u}};
    Check(pal4::NpcManagerObjectProcessor::Process(
              0x5000u, 0x11u, Globals(), runtime) == 0x7100u &&
          runtime.dispatches.empty(),
          "inactive NPC skips virtual slot +0x34");

    runtime.words[0x91f0u] = 1u;
    runtime.reads.clear();
    Check(pal4::NpcManagerObjectProcessor::Process(
              0x5000u, 0x11u, Globals(), runtime) == 0x7100u &&
          runtime.dispatches ==
              std::vector<std::vector<std::uint32_t>>{
                  {0x9000u, 0x34u, 0x11u, 0u}},
          "active NPC receives virtual slot +0x34 and one argument");
    runtime.dispatches.clear();
    Check(pal4::NpcManagerObjectProcessor::ProcessAlternative(
              0x5000u, 0x22u, Globals(), runtime) == 0x7100u &&
          runtime.dispatches ==
              std::vector<std::vector<std::uint32_t>>{
                  {0x9000u, 0x24u, 0x22u, 0u}},
          "alternative entry selects virtual slot +0x24");
}

void RightSubtreeAndExtendedDispatch() {
    Runtime runtime;
    runtime.words = {
        {0x502cu, 0x7000u},
        {0x7000u, 0x7100u}, {0x7004u, 0x7100u},
        {0x7008u, 0x7200u},
        {0x7104u, 0x7000u}, {0x7108u, 0x7200u},
        {0x711cu, 0x9000u}, {0x91f0u, 1u},
        {0x7200u, 0x6000u}, {0x7204u, 0x7100u},
        {0x7208u, 0x6000u}, {0x721cu, 0x9100u},
        {0x92f0u, 1u}};
    Check(pal4::NpcManagerObjectProcessor::ProcessExtended(
              0x5000u, 0x11u, 0x22u, Globals(), runtime) == 0x7000u &&
          runtime.grid_updates ==
              std::vector<std::vector<std::uint32_t>>{
                  {0x5038u, 0x11u}} &&
          runtime.dispatches ==
              std::vector<std::vector<std::uint32_t>>{
                  {0x9000u, 0x38u, 0x11u, 0x22u},
                  {0x9100u, 0x38u, 0x11u, 0x22u}},
          "right subtree and parent ascent visit both active NPCs in order");
}

void ForEachForwarding() {
    ForEachRuntime runtime;
    Check(pal4::NpcManagerForEach::Apply(
              0x5000u, 0x1100u, 0x2200u, runtime) == 0x4567u &&
          runtime.arguments ==
              std::vector<std::uint32_t>{0x5000u, 0x1100u, 0x2200u},
          "for-all entry forwards manager, callback, context and raw result");
}

void CallbackTraversal() {
    CallbackRuntime empty;
    empty.words = {{0x502cu, 0x7000u}, {0x7000u, 0x7000u}};
    pal4::NpcManagerCallbackWalker::Walk(
        0x5000u, 0x1100u, 0x2200u, Globals(), empty);
    Check(empty.visited.empty(), "empty callback tree is untouched");

    CallbackRuntime runtime;
    runtime.words = {
        {0x502cu, 0x7000u},
        {0x7000u, 0x7100u}, {0x7008u, 0x7300u},
        {0x7100u, 0x6000u}, {0x7104u, 0x7000u},
        {0x7108u, 0x7200u}, {0x711cu, 0x9000u},
        {0x7200u, 0x6000u}, {0x7204u, 0x7100u},
        {0x7208u, 0x7300u}, {0x721cu, 0x9100u},
        {0x7300u, 0x6000u}, {0x7304u, 0x7200u},
        {0x7308u, 0x6000u}, {0x731cu, 0x9200u}};
    pal4::NpcManagerCallbackWalker::Walk(
        0x5000u, 0x1100u, 0x2200u, Globals(), runtime);
    Check(runtime.visited ==
              std::vector<std::uint32_t>{0x9000u, 0x9100u, 0x9200u},
          "callback visits all nodes in tree order");

    runtime.visited.clear();
    runtime.stop_after = 2u;
    pal4::NpcManagerCallbackWalker::Walk(
        0x5000u, 0x1100u, 0x2200u, Globals(), runtime);
    Check(runtime.visited ==
              std::vector<std::uint32_t>{0x9000u, 0x9100u},
          "zero callback result stops before successor traversal");
}

}  // namespace

int main() {
    EmptyTree();
    SingleNodeAndActivityGate();
    RightSubtreeAndExtendedDispatch();
    ForEachForwarding();
    CallbackTraversal();
    std::cout << "NPC manager object processing cases passed\n";
}
