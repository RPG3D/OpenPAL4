#include "pal4/game_data_tree.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::uint32_t nil = 0x9000u;
    std::uint32_t users = 2u;
    std::uint32_t freed_result = 0xbeefu;
    std::uint32_t next_address = 0xa000u;
    bool fail_alloc = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t count) override {
        Check(count == 20u, "game-data tree nodes are 20 bytes");
        if (fail_alloc) return 0u;
        const auto result = next_address;
        next_address += 0x100u;
        return result;
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override {
        nil = address;
        calls.emplace_back("set nil");
    }
    std::uint32_t SharedNilUsers() override { return users; }
    void SetSharedNilUsers(std::uint32_t value) override {
        users = value;
        calls.emplace_back("set users");
    }
    void GetFirstElement(std::uint32_t tree) override {
        Check(tree == 0x1000u, "first element tree");
        calls.emplace_back("first");
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free memory");
        return freed_result;
    }
};

void PrepareTree(Runtime& runtime);

void CheckBulk() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x100cu] = 2u;
    runtime.words[0x2000u] = 0x2000u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2008u] = 0x2000u;
    runtime.words[0x3000u] = runtime.nil;
    runtime.words[0x3008u] = runtime.nil;
    Check(pal4::GameDataTree::Destroy(0x1000u, runtime) == 1u &&
              runtime.words[0x2000u] == 0x2000u &&
              runtime.words[0x2004u] == 0x9000u &&
              runtime.words[0x2008u] == 0x2000u &&
              runtime.words[0x1004u] == 0u &&
              runtime.words[0x100cu] == 0u &&
              runtime.users == 1u && runtime.nil == 0x9000u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x3000u, 0x2000u} &&
              runtime.calls == std::vector<std::string>{
                  "free memory", "first", "free memory", "set users"},
          "nonempty tree frees root and head but retains shared nil");
}

void CheckFreeNodesOrder() {
    Runtime runtime;
    runtime.words[0x3000u] = 0x3100u;
    runtime.words[0x3008u] = 0x3200u;
    runtime.words[0x3100u] = runtime.nil;
    runtime.words[0x3108u] = 0x3300u;
    runtime.words[0x3200u] = runtime.nil;
    runtime.words[0x3208u] = runtime.nil;
    runtime.words[0x3300u] = runtime.nil;
    runtime.words[0x3308u] = runtime.nil;
    Check(pal4::GameDataTree::FreeNodes(0x3000u, runtime) ==
              runtime.nil &&
              runtime.freed == std::vector<std::uint32_t>{
                  0x3200u, 0x3000u, 0x3300u, 0x3100u},
          "free right subtree before root, then follow left spine");
    runtime.freed.clear();
    Check(pal4::GameDataTree::FreeNodes(runtime.nil, runtime) ==
              runtime.nil && runtime.freed.empty(),
          "nil root does not free memory");
}

void CheckConstruct() {
    Runtime first;
    first.nil = 0u;
    first.users = 0u;
    const auto head = pal4::GameDataTree::Construct(0x1000u, first);
    Check(head == 0xa100u && first.nil == 0xa000u &&
              first.users == 1u && first.freed.empty() &&
              first.words[0xa000u] == 0u &&
              first.words[0xa004u] == 0u &&
              first.words[0xa008u] == 0u &&
              first.words[0xa010u] == 1u &&
              first.words[0x1004u] == head &&
              first.words[0x100cu] == 0u &&
              first.words[head] == head &&
              first.words[head + 4u] == first.nil &&
              first.words[head + 8u] == head &&
              first.words[head + 16u] == 0u &&
              first.calls == std::vector<std::string>{
                  "set nil", "set users"},
          "first tree retains candidate as shared nil and creates head");

    Runtime reused;
    Check(pal4::GameDataTree::Construct(0x1000u, reused) ==
              0xa100u &&
              reused.nil == 0x9000u && reused.users == 3u &&
              reused.freed == std::vector<std::uint32_t>{0xa000u} &&
              reused.words[0xa104u] == reused.nil &&
              reused.calls == std::vector<std::string>{
                  "set users", "free memory"},
          "later tree discards candidate and reuses shared nil");
}

void CheckResidualNodes() {
    Runtime runtime;
    runtime.users = 1u;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x100cu] = 0u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2008u] = 0x3100u;
    runtime.words[0x3000u] = runtime.nil;
    runtime.words[0x3004u] = 0x2000u;
    runtime.words[0x3008u] = 0x3100u;
    runtime.words[0x3100u] = runtime.nil;
    runtime.words[0x3104u] = 0x3000u;
    runtime.words[0x3108u] = runtime.nil;
    Check(pal4::GameDataTree::Destroy(0x1000u, runtime) == 0xbeefu &&
              runtime.freed ==
                  std::vector<std::uint32_t>{
                      0x3000u, 0x3100u, 0x2000u, 0x9000u} &&
              runtime.users == 0u && runtime.nil == 0u &&
              runtime.calls == std::vector<std::string>{
                  "free memory", "free memory", "free memory",
                  "set users", "set nil", "free memory"},
          "residual-node path deletes individually and releases final nil");
}

void CheckEmpty() {
    Runtime runtime;
    runtime.users = 1u;
    runtime.nil = 0u;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2000u] = 0x2000u;
    Check(pal4::GameDataTree::Destroy(0x1000u, runtime) == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x2000u} &&
              runtime.users == 0u,
          "empty tree handles absent final shared nil");
}

void CheckNextCursor() {
    Runtime runtime;
    runtime.words[0x2008u] = 0x3100u;
    runtime.words[0x3008u] = 0x3100u;
    runtime.words[0x3004u] = 0x2000u;
    runtime.words[0x3100u] = 0x3200u;
    runtime.words[0x3104u] = 0x3000u;
    runtime.words[0x3108u] = runtime.nil;
    runtime.words[0x3200u] = runtime.nil;
    runtime.words[0x3204u] = 0x3100u;
    runtime.words[0x3208u] = runtime.nil;
    auto cursor = 0x3000u;
    Check(pal4::GameDataTree::NextCursor(cursor, runtime) == runtime.nil &&
              cursor == 0x3200u,
          "right-subtree path leaves nil in EAX but moves cursor leftmost");
    Check(pal4::GameDataTree::NextCursor(cursor, runtime) == 0x3100u &&
              cursor == 0x3100u,
          "left-child path climbs to its parent");
    cursor = 0x2000u;
    Check(pal4::GameDataTree::NextCursor(cursor, runtime) == runtime.nil &&
              cursor == 0x3200u,
          "head sentinel selects leftmost node below its right link");
    cursor = 0x3000u;
    Check(pal4::GameDataTree::PostIncrement(cursor, 0x4000u, runtime) ==
              0x4000u && cursor == 0x3200u &&
              runtime.words[0x4000u] == 0x3000u,
          "postincrement returns old node and advances into right subtree");
    Check(pal4::GameDataTree::PostIncrement(cursor, 0x4000u, runtime) ==
              0x4000u && cursor == 0x3100u &&
              runtime.words[0x4000u] == 0x3200u,
          "postincrement follows ancestor links");
}

void CheckPreviousCursor() {
    Runtime runtime;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2008u] = 0x3200u;
    runtime.words[0x2010u] = 0u;
    runtime.words[0x3004u] = 0x2000u;
    runtime.words[0x3000u] = 0x3100u;
    runtime.words[0x3010u] = 1u;
    runtime.words[0x3104u] = 0x3000u;
    runtime.words[0x3108u] = 0x3200u;
    runtime.words[0x3200u] = runtime.nil;
    runtime.words[0x3204u] = 0x3100u;
    runtime.words[0x3208u] = runtime.nil;
    auto cursor = 0x2000u;
    Check(pal4::GameDataTree::PreviousCursor(cursor, runtime) ==
              0x3200u && cursor == 0x3200u,
          "head predecessor selects recorded maximum");
    cursor = 0x3000u;
    Check(pal4::GameDataTree::PreviousCursor(cursor, runtime) ==
              runtime.nil && cursor == 0x3200u,
          "left-subtree predecessor moves cursor but returns nil");
    Check(pal4::GameDataTree::PreviousCursor(cursor, runtime) ==
              0x3100u && cursor == 0x3100u,
          "right-child predecessor climbs to parent");
}

void CheckRotations() {
    Runtime runtime;
    PrepareTree(runtime);
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x3000u] = runtime.nil;
    runtime.words[0x3004u] = 0x2000u;
    runtime.words[0x3008u] = 0x3100u;
    runtime.words[0x3100u] = 0x3200u;
    runtime.words[0x3104u] = 0x3000u;
    runtime.words[0x3108u] = runtime.nil;
    runtime.words[0x3200u] = runtime.nil;
    runtime.words[0x3204u] = 0x3100u;
    runtime.words[0x3208u] = runtime.nil;
    Check(pal4::GameDataTree::RotateLeft(0x1000u, 0x3000u, runtime) ==
              0x3100u &&
              runtime.words[0x2004u] == 0x3100u &&
              runtime.words[0x3104u] == 0x2000u &&
              runtime.words[0x3100u] == 0x3000u &&
              runtime.words[0x3004u] == 0x3100u &&
              runtime.words[0x3008u] == 0x3200u &&
              runtime.words[0x3204u] == 0x3000u,
          "left root rotation reparents middle subtree");
    Check(pal4::GameDataTree::RotateRight(0x1000u, 0x3100u, runtime) ==
              0x3000u &&
              runtime.words[0x2004u] == 0x3000u &&
              runtime.words[0x3004u] == 0x2000u &&
              runtime.words[0x3008u] == 0x3100u &&
              runtime.words[0x3204u] == 0x3100u,
          "right root rotation reverses the left rotation");

    runtime.words[0x2004u] = 0x3500u;
    runtime.words[0x3500u] = 0x3000u;
    runtime.words[0x3004u] = 0x3500u;
    runtime.words[0x3100u] = runtime.nil;
    Check(pal4::GameDataTree::RotateLeft(0x1000u, 0x3000u, runtime) ==
              0x3100u &&
              runtime.words[0x3500u] == 0x3100u &&
              runtime.words[0x3104u] == 0x3500u &&
              runtime.words[0x3100u] == 0x3000u,
          "interior left rotation updates parent child link");
}

std::uint32_t Key(int value) {
    return 0x4000u + static_cast<std::uint32_t>(value + 3) * 4u;
}

void PrepareTree(Runtime& runtime) {
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x100cu] = 0u;
    runtime.words[0x2000u] = 0x2000u;
    runtime.words[0x2004u] = runtime.nil;
    runtime.words[0x2008u] = 0x2000u;
    runtime.words[runtime.nil + 16u] = 1u;
}

void VerifyTree(Runtime& runtime, const std::vector<int>& expected) {
    const auto head = runtime.words[0x1004u];
    const auto root = runtime.words[head + 4u];
    Check(runtime.words[0x100cu] == expected.size() &&
              runtime.words[runtime.nil + 16u] == 1u,
          "tree count and black nil");
    if (expected.empty()) {
        Check(root == runtime.nil && runtime.words[head] == head &&
                  runtime.words[head + 8u] == head,
              "empty tree head links");
        return;
    }
    Check(runtime.words[root + 4u] == head &&
              runtime.words[root + 16u] == 1u,
          "black root parent is head");
    std::vector<int> inorder;
    std::set<std::uint32_t> visited;
    std::function<int(std::uint32_t, std::uint32_t)> walk =
        [&](std::uint32_t node, std::uint32_t parent) -> int {
        if (node == runtime.nil) return 1;
        Check(visited.insert(node).second &&
                  runtime.words[node + 4u] == parent,
              "acyclic parent links");
        const auto left = runtime.words[node];
        const auto right = runtime.words[node + 8u];
        const auto colour = runtime.words[node + 16u];
        Check(colour <= 1u, "red or black node");
        if (colour == 0u)
            Check(runtime.words[left + 16u] == 1u &&
                      runtime.words[right + 16u] == 1u,
                  "red node has black children");
        const auto left_height = walk(left, node);
        inorder.push_back(static_cast<std::int32_t>(
            runtime.words[node + 12u]));
        const auto right_height = walk(right, node);
        Check(left_height == right_height, "equal black heights");
        return left_height + static_cast<int>(colour);
    };
    (void)walk(root, head);
    Check(inorder == expected && visited.size() == expected.size(),
          "inorder signed keys match inserted values");
    Check(static_cast<std::int32_t>(
              runtime.words[runtime.words[head] + 12u]) ==
              expected.front() &&
              static_cast<std::int32_t>(
                  runtime.words[runtime.words[head + 8u] + 12u]) ==
                  expected.back(),
          "head tracks extrema");
}

void CheckAllInsertionOrders() {
    std::array<int, 5> order{-2, -1, 0, 1, 2};
    std::size_t orders = 0u;
    do {
        Runtime runtime;
        PrepareTree(runtime);
        std::vector<int> inserted;
        for (const auto value : order) {
            runtime.words[Key(value)] =
                static_cast<std::uint32_t>(value);
            Check(pal4::GameDataTree::InsertOrGet(
                      0x1000u, 0x3000u, Key(value), runtime) ==
                      0x3000u && runtime.bytes[0x3004u] == 1u,
                  "new signed key is inserted");
            inserted.push_back(value);
            std::sort(inserted.begin(), inserted.end());
            VerifyTree(runtime, inserted);
        }
        ++orders;
    } while (std::next_permutation(order.begin(), order.end()));
    Check(orders == 120u, "all five-key insertion orders");
}

void CheckDuplicatesAndFailure() {
    Runtime runtime;
    PrepareTree(runtime);
    runtime.words[Key(2)] = 2u;
    runtime.words[Key(-1)] = 0xffffffffu;
    (void)pal4::GameDataTree::InsertOrGet(
        0x1000u, 0x3000u, Key(2), runtime);
    const auto original = runtime.words[0x3000u];
    (void)pal4::GameDataTree::InsertOrGet(
        0x1000u, 0x3000u, Key(-1), runtime);
    (void)pal4::GameDataTree::InsertOrGet(
        0x1000u, 0x3000u, Key(2), runtime);
    Check(runtime.bytes[0x3004u] == 0u &&
              runtime.words[0x3000u] == original &&
              runtime.words[0x100cu] == 2u,
          "unique mode returns existing duplicate node");
    runtime.bytes[0x1008u] = 1u;
    (void)pal4::GameDataTree::InsertOrGet(
        0x1000u, 0x3000u, Key(2), runtime);
    Check(runtime.bytes[0x3004u] == 1u &&
              runtime.words[0x100cu] == 3u,
          "duplicate-enabled mode inserts another node");
    VerifyTree(runtime, {-1, 2, 2});

    Runtime failing;
    PrepareTree(failing);
    failing.words[Key(1)] = 1u;
    failing.fail_alloc = true;
    bool threw = false;
    try {
        (void)pal4::GameDataTree::InsertOrGet(
            0x1000u, 0x3000u, Key(1), failing);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failing.words[0x100cu] == 0u,
          "allocation failure leaves tree count unchanged");
}

void CheckDirectInsert() {
    Runtime runtime;
    PrepareTree(runtime);
    runtime.words[Key(-2)] = 0xfffffffeu;
    Check(pal4::GameDataTree::InsertNode(
              0x1000u, 0x3000u, runtime.nil, 0x2000u,
              Key(-2), runtime) == 0x3000u &&
              runtime.words[0x3000u] == 0xa000u,
          "direct insertion stores allocated node in output");
    VerifyTree(runtime, {-2});
    runtime.words[Key(1)] = 1u;
    Check(pal4::GameDataTree::InsertNode(
              0x1000u, 0x3000u, runtime.nil, 0xa000u,
              Key(1), runtime) == 0x3000u &&
              runtime.words[0x3000u] == 0xa100u,
          "direct insertion links under supplied parent");
    VerifyTree(runtime, {-2, 1});
}

void CheckAllDeletionOrders() {
    std::array<int, 5> insertion{-2, -1, 0, 1, 2};
    std::size_t combinations = 0u;
    do {
        std::array<int, 5> deletion{-2, -1, 0, 1, 2};
        do {
            Runtime runtime;
            PrepareTree(runtime);
            std::map<int, std::uint32_t> nodes;
            for (const auto value : insertion) {
                runtime.words[Key(value)] =
                    static_cast<std::uint32_t>(value);
                (void)pal4::GameDataTree::InsertOrGet(
                    0x1000u, 0x3000u, Key(value), runtime);
                nodes[value] = runtime.words[0x3000u];
            }
            VerifyTree(runtime, {-2, -1, 0, 1, 2});
            std::vector<int> remaining{-2, -1, 0, 1, 2};
            for (const auto value : deletion) {
                const auto it = std::find(remaining.begin(),
                                          remaining.end(), value);
                const auto successor = std::next(it) == remaining.end()
                    ? 0x2000u : nodes[*std::next(it)];
                const auto freed_before = runtime.freed.size();
                Check(pal4::GameDataTree::DeleteNode(
                          0x1000u, 0x3000u, nodes[value], runtime) ==
                          0x3000u &&
                          runtime.words[0x3000u] == successor &&
                          runtime.freed.size() == freed_before + 1u &&
                          runtime.freed.back() == nodes[value],
                      "delete writes successor and frees selected node");
                remaining.erase(it);
                VerifyTree(runtime, remaining);
            }
            ++combinations;
        } while (std::next_permutation(deletion.begin(), deletion.end()));
    } while (std::next_permutation(insertion.begin(), insertion.end()));
    Check(combinations == 14400u,
          "all five-key insertion and deletion order pairs");
}

}  // namespace

int main() {
    try {
        CheckBulk();
        CheckFreeNodesOrder();
        CheckConstruct();
        CheckResidualNodes();
        CheckEmpty();
        CheckNextCursor();
        CheckPreviousCursor();
        CheckRotations();
        CheckAllInsertionOrders();
        CheckDuplicatesAndFailure();
        CheckDirectInsert();
        CheckAllDeletionOrders();
        std::cout << "game_data_tree_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
