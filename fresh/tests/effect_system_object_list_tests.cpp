#include "pal4/effect_system_object_list.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IEffectSystemObjectListRuntime {
    static constexpr std::uint32_t kList = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kList + 8u, 0u}, {kHead, kHead}, {kHead + 4u, kHead}};
    std::vector<std::uint32_t> frees;
    std::uint32_t next_node = 0x4000u;
    std::uint32_t allocations = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "list nodes occupy twelve bytes");
        const auto node = next_node;
        next_node += 0x20u;
        ++allocations;
        return node;
    }
    void Free(std::uint32_t pointer) override { frees.push_back(pointer); }
};

void TestNodeConstruction() {
    Runtime runtime;
    const auto empty = pal4::EffectSystemObjectList::CreateNode(
        0u, 0u, runtime);
    Check(runtime.words.at(empty) == empty &&
          runtime.words.at(empty + 4u) == empty &&
          !runtime.words.contains(empty + 8u),
          "null neighbors make a self-linked head without a payload");

    const auto linked = pal4::EffectSystemObjectList::CreateNode(
        0x1100u, 0x1200u, runtime);
    Check(runtime.words.at(linked) == 0x1100u &&
          runtime.words.at(linked + 4u) == 0x1200u &&
          !runtime.words.contains(linked + 8u),
          "explicit neighbors initialize both links only");
}

void TestInsertAndErase() {
    Runtime runtime;
    const auto first = pal4::EffectSystemObjectList::InsertBefore(
        Runtime::kList, Runtime::kHead, 0xa1u, runtime);
    const auto last = pal4::EffectSystemObjectList::InsertBefore(
        Runtime::kList, Runtime::kHead, 0xa2u, runtime);
    const auto middle = pal4::EffectSystemObjectList::InsertBefore(
        Runtime::kList, last, 0xa3u, runtime);
    Check(runtime.words.at(Runtime::kHead) == first &&
          runtime.words.at(first) == middle &&
          runtime.words.at(middle) == last &&
          runtime.words.at(last) == Runtime::kHead &&
          runtime.words.at(first + 4u) == Runtime::kHead &&
          runtime.words.at(middle + 4u) == first &&
          runtime.words.at(last + 4u) == middle &&
          runtime.words.at(Runtime::kHead + 4u) == last &&
          runtime.words.at(first + 8u) == 0xa1u &&
          runtime.words.at(middle + 8u) == 0xa3u &&
          runtime.words.at(last + 8u) == 0xa2u &&
          runtime.words.at(Runtime::kList + 8u) == 3u,
          "insert-before preserves circular links, payloads and count");

    Check(pal4::EffectSystemObjectList::EraseRange(
              Runtime::kList, first, last, runtime) == last &&
          runtime.frees == (std::vector<std::uint32_t>{first, middle}) &&
          runtime.words.at(Runtime::kHead) == last &&
          runtime.words.at(last + 4u) == Runtime::kHead &&
          runtime.words.at(Runtime::kList + 8u) == 1u,
          "half-open erase unlinks two nodes before its end");

    Check(pal4::EffectSystemObjectList::EraseRange(
              Runtime::kList, last, Runtime::kHead, runtime) ==
              Runtime::kHead &&
          runtime.words.at(Runtime::kHead) == Runtime::kHead &&
          runtime.words.at(Runtime::kHead + 4u) == Runtime::kHead &&
          runtime.words.at(Runtime::kList + 8u) == 0u &&
          runtime.frees.back() == last,
          "final erase restores the empty circular list");
    const auto frees = runtime.frees.size();
    Check(pal4::EffectSystemObjectList::EraseRange(
              Runtime::kList, Runtime::kHead,
              Runtime::kHead, runtime) == Runtime::kHead &&
          runtime.frees.size() == frees,
          "empty range performs no deallocation");
}

void TestNullPredecessor() {
    Runtime runtime;
    runtime.words[Runtime::kHead + 4u] = 0u;
    const auto node = pal4::EffectSystemObjectList::InsertBefore(
        Runtime::kList, Runtime::kHead, 0xb0u, runtime);
    Check(runtime.words.at(node) == node &&
          runtime.words.at(node + 4u) == node &&
          runtime.words.at(Runtime::kHead + 4u) == node &&
          runtime.words.at(node + 8u) == 0xb0u,
          "native null-predecessor branch self-links the new node");
}

}  // namespace

int main() {
    TestNodeConstruction();
    TestInsertAndErase();
    TestNullPredecessor();
}
