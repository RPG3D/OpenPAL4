#include "pal4/monster_manager_cleanup.h"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Event {
    std::string_view kind;
    std::uint32_t first{};
    std::uint32_t second{};
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::MonsterManagerCleanupRuntimeStub {
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        events.push_back({"write", address, value});
        words[address] = value;
    }
    void CallMonsterSlot2c(std::uint32_t monster) override {
        events.push_back({"slot2c", monster});
    }
    void DeleteMonster(std::uint32_t monster,
                       std::uint32_t flag) override {
        events.push_back({"delete-monster", monster, flag});
    }
    void CleanupTreeNodes(std::uint32_t tree,
                          std::uint32_t root) override {
        events.push_back({"cleanup-nodes", tree, root});
    }
    void ReleaseText(std::uint32_t text,
                     std::uint32_t argument) override {
        events.push_back({"release-text", text, argument});
    }
    void FreeNode(std::uint32_t node) override {
        events.push_back({"free-node", node});
    }
    std::uint32_t NextTreeNode(std::uint32_t node) override {
        events.push_back({"next", node});
        return next_nodes.at(node);
    }
    void DeleteTreeNode(std::uint32_t tree,
                        std::uint32_t node) override {
        events.push_back({"delete-node", tree, node});
    }
    void CleanupAuxiliaryTree(std::uint32_t manager) override {
        events.push_back({"cleanup-aux", manager});
    }
    void ClearGridElements(std::uint32_t grid) override {
        events.push_back({"clear-grid", grid});
    }
    void GetPreviousGridElement(std::uint32_t grid) override {
        events.push_back({"previous-grid", grid});
    }
    std::uint32_t ResetGrid(std::uint32_t grid) override {
        events.push_back({"reset-grid", grid});
        return 0x9911u;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> next_nodes;
    std::vector<Event> events;
};

constexpr std::uint32_t kManager = 0x5000u;
constexpr std::uint32_t kTree = 0x5028u;
constexpr std::uint32_t kHeader = 0x6000u;
constexpr std::uint32_t kNil = 0x9000u;

void CheckGridTail(const std::vector<Event>& events,
                   std::size_t start) {
    const std::vector<Event> tail(events.begin() + start, events.end());
    Check(tail == std::vector<Event>{
              {"cleanup-aux", kManager}, {"clear-grid", 0x5038u},
              {"previous-grid", 0x5038u}, {"reset-grid", 0x5038u}},
          "auxiliary cleanup and three grid calls stay ordered");
}

void TestEmptyTree() {
    Runtime runtime;
    runtime.words = {{0x502cu, kHeader}, {0x5034u, 0u},
                     {kHeader, kHeader}};
    const pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerCleanup::Run(
              kManager, globals, runtime) == 0x9911u,
          "cleanup propagates raw grid reset result");
    CheckGridTail(runtime.events, 0u);
    Check(runtime.words.at(0x502cu) == kHeader &&
              runtime.words.at(0x5034u) == 0u,
          "empty cleanup preserves header and count");
}

void TestSingleNode() {
    Runtime runtime;
    constexpr std::uint32_t node = 0x7100u;
    runtime.words = {
        {0x502cu, kHeader}, {0x5034u, 1u},
        {kHeader, node}, {kHeader + 4u, node},
        {kHeader + 8u, node},
        {node, kNil}, {node + 4u, kHeader},
        {node + 8u, kNil}, {node + 0x1cu, 0xa000u}};
    const pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerCleanup::Run(
              kManager, globals, runtime) == 0x9911u,
          "populated cleanup returns reset result");
    Check(runtime.events == std::vector<Event>{
              {"slot2c", 0xa000u}, {"delete-monster", 0xa000u, 1u},
              {"cleanup-nodes", kTree, kNil},
              {"release-text", node + 0x0cu}, {"free-node", node},
              {"write", kHeader + 4u, kNil}, {"write", 0x5034u, 0u},
              {"write", kHeader, kHeader},
              {"write", kHeader + 8u, kHeader},
              {"cleanup-aux", kManager}, {"clear-grid", 0x5038u},
              {"previous-grid", 0x5038u}, {"reset-grid", 0x5038u}},
          "single monster is destroyed before its tree node and grid");
}

void TestSuccessorWithRightSubtree() {
    Runtime runtime;
    constexpr std::uint32_t root = 0x7100u;
    constexpr std::uint32_t left = 0x7200u;
    constexpr std::uint32_t right = 0x7300u;
    constexpr std::uint32_t middle = 0x7400u;
    runtime.words = {
        {0x502cu, kHeader}, {0x5034u, 4u},
        {kHeader, left}, {kHeader + 4u, root},
        {kHeader + 8u, right},
        {root, left}, {root + 4u, kHeader},
        {root + 8u, right}, {root + 0x1cu, 0xa100u},
        {left, kNil}, {left + 4u, root},
        {left + 8u, kNil}, {left + 0x1cu, 0xa200u},
        {right, middle}, {right + 4u, root},
        {right + 8u, kNil}, {right + 0x1cu, 0xa300u},
        {middle, kNil}, {middle + 4u, right},
        {middle + 8u, kNil}, {middle + 0x1cu, 0xa400u}};
    const pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerCleanup::Run(
              kManager, globals, runtime) == 0x9911u,
          "multi-node cleanup returns reset result");
    Check(std::vector<Event>(runtime.events.begin(),
                             runtime.events.begin() + 8u) ==
              std::vector<Event>{
                  {"slot2c", 0xa200u},
                  {"delete-monster", 0xa200u, 1u},
                  {"slot2c", 0xa100u},
                  {"delete-monster", 0xa100u, 1u},
                  {"slot2c", 0xa400u},
                  {"delete-monster", 0xa400u, 1u},
                  {"slot2c", 0xa300u},
                  {"delete-monster", 0xa300u, 1u}},
          "objects are destroyed in in-order tree traversal");
    Check(std::vector<Event>(runtime.events.begin() + 8u,
                             runtime.events.begin() + 14u) ==
              std::vector<Event>{
                  {"cleanup-nodes", kTree, right},
                  {"release-text", root + 0x0cu},
                  {"free-node", root},
                  {"cleanup-nodes", kTree, kNil},
                  {"release-text", left + 0x0cu},
                  {"free-node", left}},
          "root and left spine release after recursive right cleanup");
    CheckGridTail(runtime.events, runtime.events.size() - 4u);
}

void TestZeroCountWithStaleCursor() {
    Runtime runtime;
    constexpr std::uint32_t node = 0x7100u;
    runtime.words = {
        {0x502cu, kHeader}, {0x5034u, 0u},
        {kHeader, node}, {kHeader + 4u, node},
        {kHeader + 8u, node},
        {node + 4u, kHeader}, {node + 8u, kNil},
        {node + 0x1cu, 0xa000u}};
    runtime.next_nodes[node] = kHeader;
    const pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerCleanup::Run(
              kManager, globals, runtime) == 0x9911u,
          "zero-count cursor branch returns reset result");
    Check(std::vector<Event>(runtime.events.begin(),
                             runtime.events.begin() + 4u) ==
              std::vector<Event>{
                  {"slot2c", 0xa000u},
                  {"delete-monster", 0xa000u, 1u},
                  {"next", node}, {"delete-node", kTree, node}},
          "stale cursor uses successor and tree deletion callbacks");
    CheckGridTail(runtime.events, 4u);
}

}  // namespace

int main() {
    TestEmptyTree();
    TestSingleNode();
    TestSuccessorWithRightSubtree();
    TestZeroCountWithStaleCursor();
}
