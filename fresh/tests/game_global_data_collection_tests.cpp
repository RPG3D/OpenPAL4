#include "pal4/game_global_data_collection.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameGlobalDataCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint8_t empty = 0u;
    std::uint8_t previous_result = 7u;
    std::uint8_t insert_result = 9u;
    std::uint32_t previous_node = 0u;
    std::uint32_t inserted_child = 0u;
    std::uint32_t inserted_parent = 0u;
    std::int32_t inserted_key = 0;
    int insert_count = 0;
    int previous_count = 0;

    Runtime() {
        words[0x1088u] = 0x2000u;
        words[0x2004u] = 0xdeadu;
    }
    std::uint32_t GlobalDataSubsystem() override { return 0x1000u; }
    std::uint32_t NilNode() override { return 0xdeadu; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x108cu, "tree empty-byte address");
        return empty;
    }
    std::uint8_t PreviousNode(std::uint32_t& node) override {
        ++previous_count;
        node = previous_node;
        return previous_result;
    }
    std::uint8_t InsertNode(std::uint32_t tree, std::uint32_t child,
                            std::uint32_t parent, std::int32_t key) override {
        Check(tree == 0x1084u, "tree offset");
        ++insert_count;
        inserted_child = child;
        inserted_parent = parent;
        inserted_key = key;
        return insert_result;
    }
};

void EmptyTreeAndField() {
    Runtime runtime;
    runtime.empty = 1u;
    runtime.words[0x1090u] = 0xf2345678u;
    Check(pal4::GameGlobalDataCollection::InsertKey(-3, runtime) == 9u &&
              runtime.insert_count == 1 &&
              runtime.inserted_child == 0xdeadu &&
              runtime.inserted_parent == 0x2000u &&
              runtime.inserted_key == -3,
          "empty tree forwards header, nil and signed key");
    Check(pal4::GameGlobalDataCollection::WordAt90(runtime) == 0xf2345678u,
          "global field at +0x90");
}

void LeftRightAndDuplicate() {
    Runtime runtime;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x300cu] = 10u;
    runtime.words[0x3000u] = 0xdeadu;
    runtime.words[0x3008u] = 0xdeadu;
    Check(pal4::GameGlobalDataCollection::InsertKey(4, runtime) == 9u &&
              runtime.inserted_parent == 0x3000u &&
              runtime.inserted_child == 0xdeadu,
          "smaller signed key descends left and inserts at leftmost");
    runtime.insert_count = 0;
    Check(pal4::GameGlobalDataCollection::InsertKey(10, runtime) == 0u &&
              runtime.insert_count == 0,
          "equal key descends right and returns comparison byte");
    Check(pal4::GameGlobalDataCollection::InsertKey(12, runtime) == 9u &&
              runtime.inserted_parent == 0x3000u,
          "greater key descends right and inserts");
}

void PreviousNodeResultAndInsertion() {
    Runtime runtime;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2000u] = 0x4000u;
    runtime.words[0x300cu] = 10u;
    runtime.words[0x3000u] = 0x4000u;
    runtime.words[0x3008u] = 0x5000u;
    runtime.words[0x400cu] = 5u;
    runtime.words[0x4000u] = 0xdeadu;
    runtime.words[0x4008u] = 0xdeadu;
    runtime.words[0x500cu] = 15u;
    runtime.words[0x5000u] = 0xdeadu;
    runtime.words[0x5008u] = 0xdeadu;
    runtime.previous_node = 0x3000u;
    Check(pal4::GameGlobalDataCollection::InsertKey(12, runtime) == 9u &&
              runtime.previous_count == 1 &&
              runtime.inserted_parent == 0x5000u &&
              runtime.inserted_key == 12,
          "left insertion after predecessor keeps original insertion parent");
    runtime.previous_node = 0x6000u;
    runtime.words[0x600cu] = 20u;
    runtime.insert_count = 0;
    Check(pal4::GameGlobalDataCollection::InsertKey(12, runtime) == 7u &&
              runtime.insert_count == 0,
          "predecessor comparison returns native iterator low byte");
}

}  // namespace

int main() {
    try {
        EmptyTreeAndField();
        LeftRightAndDuplicate();
        PreviousNodeResultAndInsertion();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
