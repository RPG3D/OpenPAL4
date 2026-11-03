#include "pal4/sound_event_tree.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISoundEventTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<int, std::uint32_t> nodes_by_key;
    std::vector<std::uint32_t> freed;
    std::uint32_t next_address = 0x9000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t count) override {
        Check(count == pal4::SoundEventTree::kNodeBytes,
              "tree nodes are 184 bytes");
        const std::uint32_t result = next_address;
        next_address += 0x200u;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        return address;
    }
    bool Less(std::uint32_t left, std::uint32_t right) override {
        return static_cast<std::int32_t>(words[left]) <
               static_cast<std::int32_t>(words[right]);
    }
    void CopyPayload(std::uint32_t destination,
                     std::uint32_t source) override {
        words[destination] = words[source];
        nodes_by_key[static_cast<int>(words[source])] = destination - 12u;
    }
};

std::uint32_t Key(int value) {
    return 0x1000u + static_cast<std::uint32_t>(value) * 4u;
}

void VerifyTree(Runtime& runtime, const std::vector<int>& expected) {
    const std::uint32_t nil =
        runtime.ReadWord(pal4::SoundEventTree::kNilAddress);
    const std::uint32_t head = runtime.ReadWord(0x5004u);
    const std::uint32_t root = runtime.ReadWord(head + 4u);
    Check(runtime.ReadWord(0x500cu) == expected.size() &&
              runtime.ReadWord(nil + 180u) == 1u,
          "tree count and black shared nil");
    if (expected.empty()) {
        Check(root == nil && runtime.ReadWord(head) == head &&
                  runtime.ReadWord(head + 8u) == head,
              "empty tree head links");
        return;
    }
    Check(runtime.ReadWord(root + 4u) == head &&
              runtime.ReadWord(root + 180u) == 1u,
          "root parent and black colour");
    std::vector<int> inorder;
    std::set<std::uint32_t> visited;
    std::function<int(std::uint32_t, std::uint32_t)> walk =
        [&](std::uint32_t node, std::uint32_t parent) -> int {
        if (node == nil) return 1;
        Check(visited.insert(node).second &&
                  runtime.ReadWord(node + 4u) == parent,
              "acyclic tree and parent links");
        const std::uint32_t left = runtime.ReadWord(node);
        const std::uint32_t right = runtime.ReadWord(node + 8u);
        const std::uint32_t colour = runtime.ReadWord(node + 180u);
        Check(colour <= 1u, "node colour is red or black");
        if (colour == 0u)
            Check(runtime.ReadWord(left + 180u) == 1u &&
                      runtime.ReadWord(right + 180u) == 1u,
                  "red node has black children");
        const int left_height = walk(left, node);
        inorder.push_back(static_cast<int>(runtime.ReadWord(node + 12u)));
        const int right_height = walk(right, node);
        Check(left_height == right_height, "equal black heights");
        return left_height + static_cast<int>(colour);
    };
    (void)walk(root, head);
    Check(inorder == expected && visited.size() == expected.size(),
          "inorder keys match inserted values");
    Check(runtime.ReadWord(runtime.ReadWord(head) + 12u) ==
              static_cast<std::uint32_t>(expected.front()) &&
              runtime.ReadWord(runtime.ReadWord(head + 8u) + 12u) ==
                  static_cast<std::uint32_t>(expected.back()),
          "head tracks minimum and maximum");
}

void CheckConstructionAndAllocation() {
    Runtime runtime;
    Check(pal4::SoundEventTree::Construct(0x5000u, runtime) == 0x9200u &&
              runtime.ReadWord(0x8edfc8u) == 0x9000u &&
              runtime.ReadWord(0x8edfc4u) == 1u &&
              runtime.ReadWord(0x9004u) == 0u &&
              runtime.ReadWord(0x9008u) == 0u &&
              runtime.ReadWord(0x9000u) == 0u &&
              runtime.ReadWord(0x9204u) == 0x9000u &&
              runtime.ReadWord(0x92b4u) == 0u,
          "first tree installs shared black nil and red head");
    VerifyTree(runtime, {});
    Check(pal4::SoundEventTree::Construct(0x5100u, runtime) == 0x9600u &&
              runtime.ReadWord(0x8edfc8u) == 0x9000u &&
              runtime.ReadWord(0x8edfc4u) == 2u &&
              runtime.freed == std::vector<std::uint32_t>{0x9400u},
          "second tree reuses nil and frees candidate");
    Check(pal4::SoundEventTree::AllocateNode(
              0x1234u, 1u, runtime) == 0x9800u &&
              runtime.ReadWord(0x9804u) == 0x1234u &&
              runtime.ReadWord(0x98b4u) == 1u,
          "node allocation initializes parent and colour");
}

void CheckAllInsertOrders() {
    std::array<int, 5> order{1, 2, 3, 4, 5};
    std::size_t orders = 0u;
    do {
        Runtime runtime;
        (void)pal4::SoundEventTree::Construct(0x5000u, runtime);
        std::vector<int> inserted;
        for (const int value : order) {
            runtime.words[Key(value)] = static_cast<std::uint32_t>(value);
            Check(pal4::SoundEventTree::InsertOrGet(
                      0x5000u, 0x7000u, Key(value), runtime) ==
                      0x7000u && runtime.bytes.at(0x7004u) == 1u,
                  "new unique key is inserted");
            inserted.push_back(value);
            std::sort(inserted.begin(), inserted.end());
            VerifyTree(runtime, inserted);
        }
        ++orders;
    } while (std::next_permutation(order.begin(), order.end()));
    Check(orders == 120u, "all five-key insertion orders checked");
}

void CheckDuplicateAndLookup() {
    Runtime runtime;
    (void)pal4::SoundEventTree::Construct(0x5000u, runtime);
    for (const int value : {2, 1, 3}) {
        runtime.words[Key(value)] = static_cast<std::uint32_t>(value);
        (void)pal4::SoundEventTree::InsertOrGet(
            0x5000u, 0x7000u, Key(value), runtime);
    }
    VerifyTree(runtime, {1, 2, 3});
    const std::uint32_t existing = runtime.nodes_by_key.at(2);
    (void)pal4::SoundEventTree::InsertOrGet(
        0x5000u, 0x7000u, Key(2), runtime);
    Check(runtime.ReadWord(0x7000u) == existing &&
              runtime.ReadByte(0x7004u) == 0u &&
              runtime.ReadWord(0x500cu) == 3u,
          "unique duplicate returns existing node");
    runtime.bytes[0x5008u] = 1u;
    (void)pal4::SoundEventTree::InsertOrGet(
        0x5000u, 0x7000u, Key(2), runtime);
    Check(runtime.ReadByte(0x7004u) == 1u &&
              runtime.ReadWord(0x500cu) == 4u,
          "duplicate-enabled tree inserts another node");
    VerifyTree(runtime, {1, 2, 2, 3});

    runtime.words[Key(0)] = 0u;
    runtime.words[Key(4)] = 4u;
    Check(pal4::SoundEventTree::LowerBound(
              0x5000u, Key(0), runtime) ==
              runtime.nodes_by_key.at(1) &&
              pal4::SoundEventTree::LowerBound(
                  0x5000u, Key(4), runtime) ==
                  runtime.ReadWord(0x5004u),
          "lower bound below minimum and above maximum");
    Check(pal4::SoundEventTree::LowerBound(
              0x5000u, Key(2), runtime) !=
              runtime.ReadWord(0x5004u),
          "lower bound finds existing key");
}

void CheckDirectInsertAndPrevious() {
    Runtime runtime;
    (void)pal4::SoundEventTree::Construct(0x5000u, runtime);
    runtime.words[Key(2)] = 2u;
    runtime.words[Key(1)] = 1u;
    runtime.words[Key(3)] = 3u;
    const std::uint32_t head = runtime.ReadWord(0x5004u);
    const std::uint32_t nil = runtime.ReadWord(0x8edfc8u);
    Check(pal4::SoundEventTree::InsertNode(
              0x5000u, 0x7000u, nil, head, Key(2), runtime) ==
              0x7000u && runtime.ReadWord(0x7000u) ==
                  runtime.nodes_by_key.at(2),
          "direct insertion returns output pointer and node");
    (void)pal4::SoundEventTree::InsertOrGet(
        0x5000u, 0x7000u, Key(1), runtime);
    (void)pal4::SoundEventTree::InsertOrGet(
        0x5000u, 0x7000u, Key(3), runtime);
    VerifyTree(runtime, {1, 2, 3});

    runtime.words[0x7100u] = runtime.nodes_by_key.at(2);
    Check(pal4::SoundEventTree::Previous(0x7100u, runtime) == nil &&
              runtime.ReadWord(0x7100u) ==
                  runtime.nodes_by_key.at(1),
          "subtree predecessor writes rightmost left node");
    runtime.words[0x7100u] = runtime.nodes_by_key.at(1);
    Check(pal4::SoundEventTree::Previous(0x7100u, runtime) == head &&
              runtime.ReadWord(0x7100u) == head,
          "minimum predecessor climbs to head");
    runtime.words[0x7100u] = head;
    Check(pal4::SoundEventTree::Previous(0x7100u, runtime) ==
              runtime.nodes_by_key.at(3) &&
              runtime.ReadWord(0x7100u) ==
                  runtime.nodes_by_key.at(3),
          "head predecessor selects maximum");
}

}  // namespace

int main() {
    try {
        CheckConstructionAndAllocation();
        CheckAllInsertOrders();
        CheckDuplicateAndLookup();
        CheckDirectInsertAndPrevious();
        std::cout << "sound_event_tree_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
