#include "pal4/cegui_map_operations.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiMapRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> predecessors;
    std::uint32_t insertion_parent = 0u;
    std::uint32_t insertion_child = 0u;
    pal4::CeguiKeyMapping inserted_mapping{};
    std::uint32_t new_node = 0x4000u;
    int insertion_count = 0;
    int predecessor_count = 0;

    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t Predecessor(std::uint32_t node) override {
        ++predecessor_count;
        return predecessors.at(node);
    }
    std::uint32_t InsertNode(
        std::uint32_t map, std::uint32_t child,
        std::uint32_t parent, pal4::CeguiKeyMapping mapping) override {
        Check(map == 0x1000u, "node insertion targets requested map");
        ++insertion_count;
        insertion_parent = parent;
        insertion_child = child;
        inserted_mapping = mapping;
        return new_node;
    }
};

void EmptyTreeAndDuplicateMode() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, 0x9000u};
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2000u] = 0x2000u;
    runtime.words[0x2004u] = 0x9000u;
    runtime.bytes[0x1008u] = 0u;
    Check(pal4::CeguiMapOperations::ValueSlot(
              0x1000u, 42u, globals, runtime) == 0x4010u &&
              runtime.insertion_parent == 0x2000u &&
              runtime.insertion_child == 0x9000u &&
              runtime.inserted_mapping.windows_code == 42u &&
              runtime.inserted_mapping.cegui_code == 0u,
          "operator-style lookup inserts default value into empty tree");
    runtime.insertion_count = 0;
    const auto result = pal4::CeguiMapOperations::Insert(
        0x1000u, {42u, 77u}, globals, runtime);
    Check(result.node == 0x4000u && result.inserted &&
              runtime.inserted_mapping.cegui_code == 77u &&
              runtime.insertion_count == 1,
          "pair insertion returns inserted node and true");
    runtime.bytes[0x1008u] = 1u;
    Check(pal4::CeguiMapOperations::ValueSlot(
              0x1000u, 42u, globals, runtime) == 0x4010u &&
              runtime.insertion_count == 2,
          "duplicate mode inserts without predecessor check");
}

void ExistingKeyAndRightInsertion() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, 0x9000u};
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x300cu] = 42u;
    runtime.words[0x3008u] = 0x9000u;
    runtime.words[0x3000u] = 0x9000u;
    runtime.bytes[0x1008u] = 0u;
    Check(pal4::CeguiMapOperations::ValueSlot(
              0x1000u, 42u, globals, runtime) == 0x3010u &&
              runtime.insertion_count == 0,
          "equal key reuses existing node value slot");
    const auto existing = pal4::CeguiMapOperations::Insert(
        0x1000u, {42u, 99u}, globals, runtime);
    Check(existing.node == 0x3000u && !existing.inserted &&
              runtime.insertion_count == 0,
          "pair insert reports existing node without replacing its value");
    Check(pal4::CeguiMapOperations::ValueSlot(
              0x1000u, 43u, globals, runtime) == 0x4010u &&
              runtime.insertion_parent == 0x3000u &&
              runtime.insertion_child == 0x9000u,
          "larger key inserts as the right child of searched parent");
    const auto left = pal4::CeguiMapOperations::Insert(
        0x1000u, {41u, 10u}, globals, runtime);
    Check(left.inserted && runtime.insertion_parent == 0x3000u &&
              runtime.predecessor_count == 0,
          "new minimum inserts directly when parent is first element");
}

void LeftSearchUsesPredecessorButOriginalParentForInsert() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, 0x9000u};
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2000u] = 0x2500u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x300cu] = 42u;
    runtime.words[0x3000u] = 0x9000u;
    runtime.words[0x250cu] = 40u;
    runtime.bytes[0x1008u] = 0u;
    runtime.predecessors[0x3000u] = 0x2500u;
    Check(pal4::CeguiMapOperations::ValueSlot(
              0x1000u, 41u, globals, runtime) == 0x4010u &&
              runtime.predecessor_count == 1 &&
              runtime.insertion_parent == 0x3000u,
          "predecessor only decides uniqueness; insertion retains search parent");
    runtime.words[0x250cu] = 41u;
    const auto existing = pal4::CeguiMapOperations::Insert(
        0x1000u, {41u, 7u}, globals, runtime);
    Check(existing.node == 0x2500u && !existing.inserted &&
              runtime.insertion_count == 1 &&
              runtime.predecessor_count == 2,
          "equal predecessor key prevents insertion");
    runtime.words[0x250cu] = 40u;
    const auto inserted = pal4::CeguiMapOperations::Insert(
        0x1000u, {41u, 7u}, globals, runtime);
    Check(inserted.node == 0x4000u && inserted.inserted &&
              runtime.insertion_parent == 0x3000u &&
              runtime.inserted_mapping.cegui_code == 7u,
          "pair insertion preserves original parent after predecessor check");
}

void LowerBoundSearchesLeftmostMatchingNode() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, 0x9000u};
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x300cu] = 20u;
    runtime.words[0x3000u] = 0x3100u;
    runtime.words[0x3008u] = 0x3200u;
    runtime.words[0x310cu] = 10u;
    runtime.words[0x3100u] = 0x9000u;
    runtime.words[0x3108u] = 0x9000u;
    runtime.words[0x320cu] = 30u;
    runtime.words[0x3200u] = 0x9000u;
    runtime.words[0x3208u] = 0x9000u;
    Check(pal4::CeguiMapOperations::LowerBound(
              0x1000u, 5u, globals, runtime) == 0x3100u &&
              pal4::CeguiMapOperations::LowerBound(
                  0x1000u, 10u, globals, runtime) == 0x3100u &&
              pal4::CeguiMapOperations::LowerBound(
                  0x1000u, 11u, globals, runtime) == 0x3000u &&
              pal4::CeguiMapOperations::LowerBound(
                  0x1000u, 20u, globals, runtime) == 0x3000u &&
              pal4::CeguiMapOperations::LowerBound(
                  0x1000u, 21u, globals, runtime) == 0x3200u &&
              pal4::CeguiMapOperations::LowerBound(
                  0x1000u, 31u, globals, runtime) == 0x2000u,
          "lower-bound search returns first key at or above query");
    runtime.words[0x2004u] = 0x9000u;
    Check(pal4::CeguiMapOperations::LowerBound(
              0x1000u, 20u, globals, runtime) == 0x2000u,
          "empty tree lower bound is header");
}

}  // namespace

int main() {
    EmptyTreeAndDuplicateMode();
    ExistingKeyAndRightInsertion();
    LeftSearchUsesPredecessorButOriginalParentForInsert();
    LowerBoundSearchesLeftmostMatchingNode();
    std::cout << "4 CEGUI map operation test cases passed\n";
}
