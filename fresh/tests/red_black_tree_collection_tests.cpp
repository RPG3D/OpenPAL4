#include "pal4/red_black_tree_collection.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRedBlackTreeConstructorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed_roots;
    std::uint32_t nil = 0xdeadu;
    std::uint32_t users = 2u;
    std::uint32_t next = 0x2000u;
    std::uint32_t next_allocation = 0x8000u;
    Runtime() {
        words[0x1004u] = 0x2000u;
        words[0x100cu] = 0u;
        words[0x2000u] = 0x2000u;
        words[0x2004u] = nil;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read byte " + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write byte " + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override {
        calls.push_back("nil zero");
        nil = address;
    }
    std::uint32_t SharedNilUsers() override { return users; }
    void SetSharedNilUsers(std::uint32_t value) override {
        calls.push_back("users " + std::to_string(value));
        users = value;
    }
    void FreeNodes(std::uint32_t tree, std::uint32_t root) override {
        Check(tree == 0x1000u, "free-node owner");
        freed_roots.push_back(root);
        calls.push_back("bulk");
    }
    void NextNode(std::uint32_t& node) override {
        calls.push_back("next " + std::to_string(node));
        node = next;
    }
    void DeleteNode(std::uint32_t tree, std::uint32_t node) override {
        Check(tree == 0x1000u, "delete owner");
        calls.push_back("delete " + std::to_string(node));
    }
    void ConstructTree(std::uint32_t tree) override {
        Check(tree == 0x1000u, "native tree constructor address");
        calls.push_back("construct");
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return 0xcafeu;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 0x14u, "extended tree allocates 20-byte nodes");
        const auto address = next_allocation;
        next_allocation += 0x100u;
        calls.push_back("allocate " + std::to_string(address));
        return address;
    }
};

void EmptyAndSharedNil() {
    Runtime runtime;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    Check(collection.Destroy(runtime) == 1u &&
              runtime.words[0x1004u] == 0u &&
              runtime.words[0x100cu] == 0u &&
              runtime.nil == 0xdeadu &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "free 8192") < runtime.calls.end() - 1 &&
              runtime.calls.back() == "users 1",
          "empty collection frees header and retains shared nil");
}

void NonEmptyBulkAndFinalNil() {
    Runtime runtime;
    runtime.words[0x100cu] = 3u;
    runtime.words[0x2000u] = 0x4000u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.users = 1u;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    Check(collection.Destroy(runtime) == 0xcafeu &&
              runtime.words[0x2000u] == 0x2000u &&
              runtime.words[0x2004u] == 0xdeadu &&
              runtime.words[0x2008u] == 0x2000u &&
              runtime.users == 0u && runtime.nil == 0u &&
              runtime.freed_roots == std::vector<std::uint32_t>{0x3000u},
          "nonempty collection resets header and releases last shared nil");
    const auto bulk = std::find(runtime.calls.begin(), runtime.calls.end(), "bulk");
    const auto header_free = std::find(runtime.calls.begin(), runtime.calls.end(),
                                       "free 8192");
    const auto nil_free = std::find(runtime.calls.begin(), runtime.calls.end(),
                                    "free 57005");
    Check(bulk < header_free && header_free < nil_free,
          "bulk free, header free, nil free order");
}

void ConstructionAndLeftmostNode() {
    Runtime runtime;
    runtime.bytes[0x5000u] = 0x21u;
    runtime.bytes[0x6000u] = 0x43u;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    Check(collection.ConstructWithArgs(0x5000u, 0x65u, 0x6000u,
                                       runtime) == 0x1000u &&
              runtime.bytes[0x1000u] == 0x43u &&
              runtime.bytes[0x1001u] == 0x21u &&
              runtime.bytes[0x1008u] == 0x65u &&
              runtime.calls.back() == "construct",
          "constructor copies argument bytes before native tree setup");
    runtime.words[0x7000u] = 0xdeadu;
    Check(pal4::RedBlackTreeCollection::LeftmostNode(0x7000u,
                                                     runtime) == 0x7000u,
          "empty subtree returns input root slot");
    runtime.words[0x7000u] = 0x7100u;
    runtime.words[0x7100u] = 0x7200u;
    runtime.words[0x7200u] = 0xdeadu;
    Check(pal4::RedBlackTreeCollection::LeftmostNode(0x7000u,
                                                     runtime) == 0x7200u,
          "helper follows left links to final node");
}

void DeleteFullAndPartialRanges() {
    Runtime full;
    full.words[0x100cu] = 2u;
    full.words[0x2000u] = 0x4000u;
    full.words[0x2004u] = 0x3000u;
    full.words[0x3000u] = 0x4000u;
    full.words[0x3008u] = 0x5000u;
    full.words[0x4000u] = 0xdeadu;
    full.words[0x4008u] = 0xdeadu;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    std::uint32_t output = 0u;
    Check(&collection.DeleteRange(output, 0x4000u, 0x2000u, full) ==
              &output && output == 0x2000u &&
              full.words[0x100cu] == 0u &&
              full.words[0x2004u] == 0xdeadu &&
              full.freed_roots ==
                  std::vector<std::uint32_t>{0x5000u, 0xdeadu} &&
              std::find(full.calls.begin(), full.calls.end(),
                        "free 12288") <
                  std::find(full.calls.begin(), full.calls.end(),
                            "free 16384"),
          "full range frees right subtrees and left chain, then resets header");

    Runtime partial;
    partial.words[0x3008u] = 0x4000u;
    partial.words[0x4000u] = 0xdeadu;
    partial.words[0x4004u] = 0x3000u;
    partial.words[0x4008u] = 0xdeadu;
    partial.words[0x3004u] = 0x2000u;
    partial.words[0x2008u] = 0x2000u;
    Check(&collection.DeleteRange(output, 0x3000u, 0x4000u, partial) ==
              &output && output == 0x4000u &&
              std::find(partial.calls.begin(), partial.calls.end(),
                        "delete 12288") != partial.calls.end(),
          "partial range chooses leftmost node in right subtree");
    partial.calls.clear();
    Check(collection.DeleteRange(output, 0x4000u, 0x4000u, partial) ==
              output &&
              std::find(partial.calls.begin(), partial.calls.end(),
                        "delete 16384") == partial.calls.end(),
          "empty range does not delete nodes");
}

void DeleteRangeAscendsWhenNoRightChild() {
    Runtime runtime;
    runtime.words[0x3000u] = 0x4000u;
    runtime.words[0x3004u] = 0x2000u;
    runtime.words[0x3008u] = 0x5000u;
    runtime.words[0x4004u] = 0x3000u;
    runtime.words[0x4008u] = 0xdeadu;
    runtime.words[0x5004u] = 0x3000u;
    runtime.words[0x5008u] = 0xdeadu;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2008u] = 0x5000u;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    std::uint32_t output = 0u;
    (void)collection.DeleteRange(output, 0x4000u, 0x3000u, runtime);
    Check(output == 0x3000u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "delete 16384") != runtime.calls.end(),
          "left child with no right subtree advances to parent");
    runtime.calls.clear();
    (void)collection.DeleteRange(output, 0x5000u, 0x2000u, runtime);
    Check(output == 0x2000u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "delete 20480") != runtime.calls.end(),
          "rightmost child ascends through parent to header");
}

void DialogTreeClearRange() {
    Runtime runtime;
    runtime.nil = 0x90abfcu;
    runtime.words[0x100cu] = 1u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x3000u] = runtime.nil;
    runtime.words[0x3008u] = runtime.nil;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    std::uint32_t output = 0;
    Check(&collection.ClearRange(output, 0x3000u, 0x2000u, runtime) ==
              &output && output == 0x2000u &&
              runtime.words[0x100cu] == 0 &&
              runtime.words[0x2004u] == runtime.nil &&
              runtime.freed_roots == std::vector<std::uint32_t>{runtime.nil},
          "dialog tree full range uses its own shared nil");
    runtime.calls.clear();
    Check(collection.ClearRange(output, 0x2000u, 0x2000u, runtime) ==
              output &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "delete 8192") == runtime.calls.end(),
          "dialog tree empty range keeps iterator");
}

void DialogTreeIteratorTraversal() {
    Runtime runtime;
    runtime.nil = 0x90abfcu;
    runtime.words[0x3008u] = 0x4000u;
    runtime.words[0x4000u] = 0x3500u;
    runtime.words[0x3500u] = runtime.nil;
    std::uint32_t node = 0x3000u;
    Check(pal4::RedBlackTreeCollection::AdvanceIterator(node, runtime) ==
              runtime.nil && node == 0x3500u,
          "right subtree advances to minimum and returns nil");

    runtime.words[0x3508u] = runtime.nil;
    runtime.words[0x3504u] = 0x4000u;
    runtime.words[0x4008u] = 0x4000u;
    node = 0x3500u;
    Check(pal4::RedBlackTreeCollection::AdvanceIterator(node, runtime) ==
              0x4000u && node == 0x4000u,
          "left child ascends to parent");
}

void DialogTreeFreeSubtree() {
    Runtime runtime;
    runtime.nil = 0x90abfcu;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    Check(collection.FreeSubtree(runtime.nil, runtime) == runtime.nil &&
              runtime.calls.empty(),
          "empty subtree returns shared nil without freeing it");
    runtime.words[0x3000u] = 0x2000u;
    runtime.words[0x3008u] = 0x4000u;
    runtime.words[0x2000u] = runtime.nil;
    runtime.words[0x2008u] = runtime.nil;
    runtime.words[0x4000u] = runtime.nil;
    runtime.words[0x4008u] = 0x5000u;
    runtime.words[0x5000u] = runtime.nil;
    runtime.words[0x5008u] = runtime.nil;
    runtime.calls.clear();
    Check(collection.FreeSubtree(0x3000u, runtime) == runtime.nil,
          "nonempty subtree returns shared nil");
    std::vector<std::string> freed;
    for (const auto& call : runtime.calls)
        if (call.starts_with("free ")) freed.push_back(call);
    Check(freed == std::vector<std::string>{
                       "free 20480", "free 16384", "free 12288",
                       "free 8192"},
          "right subtrees release before left spine and nil is retained");
}

void DialogTreeConstructExtended() {
    const pal4::RedBlackTreeCollection collection(0x1000u);
    Runtime first;
    first.nil = 0;
    first.users = 0;
    Check(collection.ConstructExtended(first) == 0x8100u &&
              first.nil == 0x8000u && first.users == 1 &&
              first.words[0x8000u] == 0 &&
              first.words[0x8004u] == 0 &&
              first.words[0x8008u] == 0 &&
              first.words[0x8010u] == 1 &&
              first.words[0x1004u] == 0x8100u &&
              first.words[0x100cu] == 0 &&
              first.words[0x8100u] == 0x8100u &&
              first.words[0x8104u] == 0x8000u &&
              first.words[0x8108u] == 0x8100u &&
              first.words[0x8110u] == 0,
          "first extended tree initializes shared nil and head");
    Check(std::find(first.calls.begin(), first.calls.end(), "free 32768") ==
              first.calls.end(),
          "first shared nil allocation is retained");

    Runtime later;
    later.nil = 0x7000u;
    later.users = 3;
    Check(collection.ConstructExtended(later) == 0x8100u &&
              later.nil == 0x7000u && later.users == 4 &&
              later.words[0x8104u] == 0x7000u &&
              std::find(later.calls.begin(), later.calls.end(),
                        "free 32768") != later.calls.end(),
          "existing shared nil releases temporary allocation");
}

void DialogTreeMinimumNode() {
    Runtime runtime;
    runtime.nil = 0x90abfcu;
    runtime.words[0x6000u] = runtime.nil;
    Check(pal4::RedBlackTreeCollection::LeftmostNode(0x6000u,
                                                     runtime) == 0x6000u,
          "empty dialog tree returns root slot");
    runtime.words[0x6000u] = 0x6100u;
    runtime.words[0x6100u] = 0x6200u;
    runtime.words[0x6200u] = 0x6300u;
    runtime.words[0x6300u] = runtime.nil;
    Check(pal4::RedBlackTreeCollection::LeftmostNode(0x6000u,
                                                     runtime) == 0x6300u,
          "dialog tree minimum follows left links to nil");
}

void ZeroCountStillDeletesLinkedNodes() {
    Runtime runtime;
    runtime.words[0x2000u] = 0x3000u;
    runtime.next = 0x2000u;
    runtime.users = 1u;
    runtime.nil = 0u;
    const pal4::RedBlackTreeCollection collection(0x1000u);
    Check(collection.Destroy(runtime) == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "next 12288") <
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "delete 12288") &&
              std::find(runtime.calls.begin(), runtime.calls.end(), "bulk") ==
                  runtime.calls.end(),
          "zero count walks and deletes nodes before header cleanup");
}

}  // namespace

int main() {
    try {
        EmptyAndSharedNil();
        NonEmptyBulkAndFinalNil();
        ZeroCountStillDeletesLinkedNodes();
        ConstructionAndLeftmostNode();
        DeleteFullAndPartialRanges();
        DeleteRangeAscendsWhenNoRightChild();
        DialogTreeClearRange();
        DialogTreeIteratorTraversal();
        DialogTreeFreeSubtree();
        DialogTreeConstructExtended();
        DialogTreeMinimumNode();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
