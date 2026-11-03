#include "pal4/effect_system_task_factory_tree.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemTaskFactoryTreeRuntime {
    static constexpr std::uint32_t kTree = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNil = 0x93cdf4u;
    static constexpr std::uint32_t kInput = 0x2600u;

    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil}, {kHead + 8u, kHead},
        {kNil, kNil}, {kNil + 4u, 0u}, {kNil + 8u, kNil},
        {kNil + 32u, 1u}};
    std::unordered_map<std::uint32_t, std::string> names;
    std::uint32_t next_allocation = 0x4000u;
    std::uint32_t allocations = 0u;
    std::uint8_t duplicate_flag = 0u;

    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        Check(address == kTree + 8u, "task tree duplicate flag offset");
        return duplicate_flag;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 36u, "task factory node uses 36 bytes");
        const auto node = next_allocation;
        next_allocation += 0x40u;
        ++allocations;
        return node;
    }
    std::int32_t CompareNames(std::uint32_t first,
                              std::uint32_t second) override {
        const auto& a = names.at(first);
        const auto& b = names.at(second);
        return a < b ? -1 : (a > b ? 1 : 0);
    }
    void CopyFactoryRecord(std::uint32_t destination,
                           std::uint32_t source) override {
        names[destination] = names.at(source);
        words[destination + 16u] = words.at(source + 16u);
    }

    void SetInput(std::string name, std::uint32_t callback) {
        names[kInput] = std::move(name);
        words[kInput + 16u] = callback;
    }
};

int Verify(const Runtime& runtime, std::uint32_t node,
           std::uint32_t parent, std::vector<std::string>& ordered) {
    if (node == Runtime::kNil) return 1;
    Check(runtime.ReadDword(node + 4u) == parent,
          "task tree parent link");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 32u);
    if (color == 0u)
        Check(runtime.ReadDword(left + 32u) == 1u &&
                  runtime.ReadDword(right + 32u) == 1u,
              "task tree red node has black children");
    const auto left_height = Verify(runtime, left, node, ordered);
    ordered.push_back(runtime.names.at(node + 12u));
    const auto right_height = Verify(runtime, right, node, ordered);
    Check(left_height == right_height, "task tree black heights agree");
    return left_height + (color == 1u ? 1 : 0);
}

void TestInsertionAndLookup() {
    Runtime runtime;
    const std::vector<std::string> names{
        "M", "F", "T", "A", "H", "P", "Z", "B", "G"};
    runtime.SetInput("X", 0u);
    Check(pal4::EffectSystemTaskFactoryTree::FindExact(
              Runtime::kTree, Runtime::kInput, runtime) == Runtime::kHead,
          "empty task tree returns head");
    for (std::size_t i = 0u; i < names.size(); ++i) {
        runtime.SetInput(names[i], 0x6000u + static_cast<std::uint32_t>(i));
        const auto inserted =
            pal4::EffectSystemTaskFactoryTree::SearchAndInsert(
                Runtime::kTree, Runtime::kInput, runtime);
        Check(inserted.inserted &&
                  runtime.names.at(inserted.node + 12u) == names[i] &&
                  runtime.ReadDword(inserted.node + 28u) ==
                      0x6000u + static_cast<std::uint32_t>(i),
              "task tree copies factory name and callback");
        Check(pal4::EffectSystemTaskFactoryTree::FindExact(
                  Runtime::kTree, Runtime::kInput, runtime) == inserted.node,
              "exact lookup finds each inserted node");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        Check(runtime.ReadDword(root + 32u) == 1u,
              "task tree root remains black");
        std::vector<std::string> ordered;
        (void)Verify(runtime, root, Runtime::kHead, ordered);
        auto expected = std::vector<std::string>(
            names.begin(), names.begin() + runtime.allocations);
        std::sort(expected.begin(), expected.end());
        Check(ordered == expected, "task factory names stay ordered");
    }
    runtime.SetInput("M", 0x9999u);
    const auto duplicate =
        pal4::EffectSystemTaskFactoryTree::InsertTextNode(
            Runtime::kTree, Runtime::kInput, runtime);
    Check(!duplicate.inserted && runtime.allocations == names.size() &&
              runtime.ReadDword(duplicate.node + 28u) != 0x9999u,
          "unique insertion retains the existing callback");
    runtime.SetInput("Q", 0u);
    Check(pal4::EffectSystemTaskFactoryTree::FindExact(
              Runtime::kTree, Runtime::kInput, runtime) == Runtime::kHead,
          "between-key lookup returns head");
}

void TestProcessEvent() {
    Runtime runtime;
    runtime.SetInput("SlowMotionTask", 0x6412e0u);
    Check(pal4::EffectSystemTaskFactoryTree::ProcessEvent(
              Runtime::kTree, Runtime::kInput, runtime) &&
              runtime.allocations == 1u &&
              runtime.ReadDword(0x4000u + 28u) == 0x6412e0u,
          "event registration inserts the task creator");
    runtime.SetInput("SlowMotionTask", 0x9999u);
    Check(!pal4::EffectSystemTaskFactoryTree::ProcessEvent(
              Runtime::kTree, Runtime::kInput, runtime) &&
              runtime.allocations == 1u &&
              runtime.ReadDword(0x4000u + 28u) == 0x6412e0u,
          "duplicate event leaves the old creator intact");
    runtime.SetInput("MessageTask", 0x6411a0u);
    Check(pal4::EffectSystemTaskFactoryTree::ProcessEvent(
              Runtime::kTree, Runtime::kInput, runtime) &&
              runtime.allocations == 2u,
          "another event class inserts into the same task tree");
}

void TestLowerBoundAndCursor() {
    Runtime runtime;
    for (const auto& name : {"M", "F", "T"}) {
        runtime.SetInput(name, 0x7000u);
        (void)pal4::EffectSystemTaskFactoryTree::InsertTextNode(
            Runtime::kTree, Runtime::kInput, runtime);
    }
    for (const auto& [name, expected] :
         {std::pair{"A", 0x4040u}, {"F", 0x4040u},
          {"H", 0x4000u}, {"M", 0x4000u},
          {"S", 0x4080u}, {"T", 0x4080u},
          {"Z", Runtime::kHead}}) {
        runtime.SetInput(name, 0u);
        Check(pal4::EffectSystemTaskFactoryTree::LowerBound(
                  Runtime::kTree, Runtime::kInput, runtime) == expected,
              "task tree lower bound respects name order");
    }
    const auto left = pal4::EffectSystemTaskFactoryTree::Previous(
        0x4040u, runtime);
    const auto root = pal4::EffectSystemTaskFactoryTree::Previous(
        0x4000u, runtime);
    const auto right = pal4::EffectSystemTaskFactoryTree::Previous(
        0x4080u, runtime);
    Check(left.next == Runtime::kHead && left.result == Runtime::kHead &&
              root.next == 0x4040u && root.result == Runtime::kNil &&
              right.next == 0x4000u && right.result == 0x4000u,
          "task tree predecessor preserves cursor and EAX outcomes");
}

void TestPositionedInsertAndRotations() {
    Runtime root;
    root.SetInput("Factory", 0x7777u);
    const auto node = pal4::EffectSystemTaskFactoryTree::InsertAt(
        Runtime::kTree, Runtime::kNil, Runtime::kHead,
        Runtime::kInput, root);
    Check(node == 0x4000u && root.allocations == 1u &&
              root.ReadDword(Runtime::kHead + 4u) == node &&
              root.ReadDword(node + 28u) == 0x7777u &&
              root.ReadDword(node + 32u) == 1u,
          "positioned task insertion creates black root with copied callback");

    Runtime left;
    left.words[Runtime::kHead + 4u] = 0x4000u;
    left.words[0x4000u] = Runtime::kNil;
    left.words[0x4004u] = Runtime::kHead;
    left.words[0x4008u] = 0x4040u;
    left.words[0x4040u] = 0x4080u;
    left.words[0x4044u] = 0x4000u;
    left.words[0x4048u] = Runtime::kNil;
    left.words[0x4084u] = 0x4040u;
    Check(pal4::EffectSystemTaskFactoryTree::RotateLeft(
              Runtime::kTree, 0x4000u, left) == 0x4040u &&
              left.ReadDword(Runtime::kHead + 4u) == 0x4040u &&
              left.ReadDword(0x4040u) == 0x4000u &&
              left.ReadDword(0x4008u) == 0x4080u &&
              left.ReadDword(0x4084u) == 0x4000u,
          "left rotation promotes right child and reattaches middle subtree");

    Runtime right;
    right.words[Runtime::kHead + 4u] = 0x4000u;
    right.words[0x4000u] = 0x4040u;
    right.words[0x4004u] = Runtime::kHead;
    right.words[0x4008u] = Runtime::kNil;
    right.words[0x4040u] = Runtime::kNil;
    right.words[0x4044u] = 0x4000u;
    right.words[0x4048u] = 0x4080u;
    right.words[0x4084u] = 0x4040u;
    Check(pal4::EffectSystemTaskFactoryTree::RotateRight(
              Runtime::kTree, 0x4000u, right) == 0x4040u &&
              right.ReadDword(Runtime::kHead + 4u) == 0x4040u &&
              right.ReadDword(0x4048u) == 0x4000u &&
              right.ReadDword(0x4000u) == 0x4080u &&
              right.ReadDword(0x4084u) == 0x4000u,
          "right rotation promotes left child and reattaches middle subtree");
}

struct Lifecycle final : pal4::IEffectSystemTaskFactoryTreeLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {Runtime::kTree + 4u, Runtime::kHead},
        {Runtime::kTree + 12u, 0u},
        {Runtime::kHead, Runtime::kHead},
        {Runtime::kHead + 4u, Runtime::kNil},
        {Runtime::kHead + 8u, Runtime::kHead}};
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void DeleteSubtree(std::uint32_t root) override {
        calls.push_back("subtree:" + std::to_string(root));
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back("release:" + std::to_string(record));
    }
    void FreeNode(std::uint32_t node) override {
        calls.push_back("free:" + std::to_string(node));
    }
    std::uint32_t Advance(std::uint32_t node) override {
        calls.push_back("advance:" + std::to_string(node));
        return words.at(node + 16u);
    }
    void EraseNode(std::uint32_t tree, std::uint32_t node) override {
        Check(tree == Runtime::kTree, "task erase receives tree address");
        calls.push_back("erase:" + std::to_string(node));
    }
};

void TestClear() {
    Lifecycle populated;
    populated.words[Runtime::kTree + 12u] = 3u;
    populated.words[Runtime::kHead] = 0x4100u;
    populated.words[Runtime::kHead + 4u] = 0x4000u;
    populated.words[Runtime::kHead + 8u] = 0x4200u;
    populated.words[0x4000u] = 0x4100u;
    populated.words[0x4008u] = 0x4200u;
    populated.words[0x4100u] = Runtime::kNil;
    populated.words[0x4108u] = Runtime::kNil;
    Check(pal4::EffectSystemTaskFactoryTree::Clear(
              Runtime::kTree, populated) == Runtime::kHead &&
              populated.calls == std::vector<std::string>{
                  "subtree:16896", "release:16396", "free:16384",
                  "subtree:9686516", "release:16652", "free:16640"} &&
              populated.words.at(Runtime::kHead) == Runtime::kHead &&
              populated.words.at(Runtime::kHead + 4u) == Runtime::kNil &&
              populated.words.at(Runtime::kHead + 8u) == Runtime::kHead &&
              populated.words.at(Runtime::kTree + 12u) == 0u,
          "populated task clear frees right subtrees before left chain");

    Lifecycle fallback;
    fallback.words[Runtime::kHead] = 0x4000u;
    fallback.words[0x4010u] = 0x4100u;
    fallback.words[0x4110u] = Runtime::kHead;
    Check(pal4::EffectSystemTaskFactoryTree::Clear(
              Runtime::kTree, fallback) == Runtime::kHead &&
              fallback.calls == std::vector<std::string>{
                  "advance:16384", "erase:16384",
                  "advance:16640", "erase:16640"},
          "zero-count fallback advances before erasing");
    Lifecycle empty;
    Check(pal4::EffectSystemTaskFactoryTree::Clear(
              Runtime::kTree, empty) == Runtime::kHead &&
              empty.calls.empty(),
          "empty task tree clear is a no-op");
}

}  // namespace

int main() {
    TestInsertionAndLookup();
    TestProcessEvent();
    TestLowerBoundAndCursor();
    TestPositionedInsertAndRotations();
    TestClear();
}
