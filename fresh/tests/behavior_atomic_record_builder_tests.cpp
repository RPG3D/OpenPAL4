#include "pal4/behavior_atomic_record_builder.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBehaviorAtomicRecordRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::BehaviorAtomicAttributePosition> positions;
    std::vector<std::uint8_t> add_results{1u, 1u};
    std::vector<std::uint8_t> set_results{1u, 1u};
    std::size_t find_index = 0u;
    std::size_t add_index = 0u;
    std::size_t set_index = 0u;
    std::uint32_t next_attribute = 4u;

    std::uint32_t InitializeNode(const char* name) override {
        calls.emplace_back(std::string("node:") + name);
        return 1u;
    }
    std::uint32_t InitializeClassKey(
        const char* name, std::uint8_t incoming_byte) override {
        calls.emplace_back(std::string("class_key:") + name);
        Check(incoming_byte == 0x12u, "class search key seed");
        return 2u;
    }
    std::uint32_t InitializeProportionKey(
        const char* name, std::uint8_t incoming_byte) override {
        calls.emplace_back(std::string("proportion_key:") + name);
        Check(incoming_byte == 0x34u, "proportion search key seed");
        return 3u;
    }
    pal4::BehaviorAtomicAttributePosition FindAttribute(
        std::uint32_t node, std::uint32_t key) override {
        calls.emplace_back("find:" + std::to_string(key));
        Check(node == 1u && find_index < positions.size(),
              "search uses temporary node and expected query count");
        return positions[find_index++];
    }
    void CleanupKey(std::uint32_t key) override {
        calls.emplace_back("cleanup_key:" + std::to_string(key));
    }
    std::uint8_t SetAttributeInt(
        std::uint32_t attribute, std::int32_t value) override {
        calls.emplace_back("set:" + std::to_string(attribute) + ":" +
                           std::to_string(value));
        return set_results.at(set_index++);
    }
    std::uint32_t InitializeAttribute(
        const char* name, std::int32_t value) override {
        calls.emplace_back(std::string("attribute:") + name + ":" +
                           std::to_string(value));
        return next_attribute++;
    }
    std::uint8_t AddAttribute(
        std::uint32_t node, std::uint32_t attribute) override {
        calls.emplace_back("add:" + std::to_string(attribute));
        Check(node == 1u, "attribute is added to temporary node");
        return add_results.at(add_index++);
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        calls.emplace_back("destroy_attribute:" +
                           std::to_string(attribute));
    }
    void InsertNode(pal4::ScriptEventDataState& state,
                    std::uint32_t node) override {
        calls.emplace_back("insert");
        Check(node == 1u, "insert uses temporary node");
        if (state.words[6] == 0u) state.words[6] = 0x1000u;
        state.words[7] += 60u;
        if (state.words[7] < state.words[6]) state.words[7] = state.words[6] + 60u;
    }
    void DestroyNode(std::uint32_t node) override {
        calls.emplace_back("destroy_node");
        Check(node == 1u, "temporary node is destroyed");
    }
};

void TestTwoNewAttributes() {
    pal4::ScriptEventDataState state;
    Runtime runtime;
    runtime.positions = {{0x5000u, 0x5000u}, {0xffffffe4u, 0x6000u}};
    Check(pal4::BehaviorAtomicRecordBuilder::AddClassProportion(
              state, 7, 25, 0x12u, 0x34u, runtime) == 0x1000u &&
              state.words[6] == 0x1000u && state.words[7] == 0x103cu &&
              runtime.calls == std::vector<std::string>{
                  "node:behAIAtomic-attribute",
                  "class_key:behAIAtomic-classID", "find:2",
                  "cleanup_key:2", "attribute:behAIAtomic-classID:7",
                  "add:4", "destroy_attribute:4",
                  "proportion_key:behAIAtomic-proportion", "find:3",
                  "cleanup_key:3", "attribute:behAIAtomic-proportion:25",
                  "add:5", "destroy_attribute:5", "insert", "destroy_node"},
          "missing attributes are created in order and appended node is returned");
}

void TestExistingAttributes() {
    pal4::ScriptEventDataState state;
    state.words[6] = 0x1000u;
    state.words[7] = 0x103cu;
    Runtime runtime;
    runtime.positions = {{0x2000u, 0x3000u}, {0x4000u, 0x5000u}};
    Check(pal4::BehaviorAtomicRecordBuilder::AddClassProportion(
              state, 8, -2, 0x12u, 0x34u, runtime) == 0x103cu &&
              state.words[7] == 0x1078u &&
              runtime.calls == std::vector<std::string>{
                  "node:behAIAtomic-attribute",
                  "class_key:behAIAtomic-classID", "find:2",
                  "cleanup_key:2", "set:8220:8",
                  "proportion_key:behAIAtomic-proportion", "find:3",
                  "cleanup_key:3", "set:16412:-2", "insert", "destroy_node"},
          "existing attributes use iterator plus 28 and return last element");
}

void TestFirstAttributeFailure() {
    pal4::ScriptEventDataState state;
    Runtime runtime;
    runtime.positions = {{0u, 0u}};
    runtime.add_results = {0u};
    Check(pal4::BehaviorAtomicRecordBuilder::AddClassProportion(
              state, 1, 2, 0x12u, 0x34u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "node:behAIAtomic-attribute",
                  "class_key:behAIAtomic-classID", "find:2",
                  "cleanup_key:2", "attribute:behAIAtomic-classID:1",
                  "add:4", "destroy_attribute:4", "destroy_node"},
          "first attribute failure skips proportion and insertion");
}

void TestSecondAttributeFailure() {
    pal4::ScriptEventDataState state;
    Runtime runtime;
    runtime.positions = {{0x2000u, 0x3000u}, {0x4000u, 0x5000u}};
    runtime.set_results = {1u, 0u};
    Check(pal4::BehaviorAtomicRecordBuilder::AddClassProportion(
              state, 1, 2, 0x12u, 0x34u, runtime) == 0u &&
              runtime.find_index == 2u &&
              runtime.calls.back() == "destroy_node" &&
              state.words[6] == 0u,
          "second attribute failure destroys node without inserting it");
}

}  // namespace

int main() {
    TestTwoNewAttributes();
    TestExistingAttributes();
    TestFirstAttributeFailure();
    TestSecondAttributeFailure();
    pal4::ScriptEventDataState state;
    pal4::BehaviorAtomicRecordRuntimeStub stub;
    Check(pal4::BehaviorAtomicRecordBuilder::AddClassProportion(
              state, 0, 0, 0u, 0u, stub) == 0u,
          "default XML runtime links");
    std::cout << "4 behavior atomic record cases passed\n";
}
