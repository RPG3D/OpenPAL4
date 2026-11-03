#include "pal4/ui_frame_manager_singleton.h"

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

struct Runtime final : pal4::IUiFrameManagerFullDestructRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> next;
    std::size_t next_index = 0u;
    std::uint32_t first_override = 0u;
    std::uint32_t nil = 0xc000u;
    std::uint32_t users = 1u;

    Runtime() {
        words[0x51a4u] = 0x8000u;
        words[0x518cu] = 0x9000u;
        words[0x5138u] = 0xa000u;
        words[0x5128u] = 0x7001u;
        bytes[0x7000u] = 2u;
        words[0x5118u] = 0x6000u;
        words[0x5120u] = 1u;
        words[0x6000u] = 0xd000u;
        words[0x6004u] = 0xb000u;
        first_override = 0xd000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read-ref");
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.emplace_back("write-ref");
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
        return 0x66u;
    }
    void FreeNodes(std::uint32_t tree,
                   std::uint32_t root) override {
        Check(tree == 0x5114u && root == 0xb000u,
              "bulk tree owner and root");
        events.emplace_back("free-nodes");
    }
    std::uint32_t NextNode(std::uint32_t& iterator) override {
        Check(next_index < next.size(), "iterator bound");
        const auto old = iterator;
        iterator = next[next_index++];
        events.emplace_back("next");
        return old;
    }
    void DeleteNode(std::uint32_t tree,
                    std::uint32_t node) override {
        Check(tree == 0x5114u, "delete tree owner");
        events.emplace_back("delete");
        deleted.push_back(node);
    }
    void FirstElement(std::uint32_t) override {
        throw std::runtime_error("unexpected other first-element API");
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t value) override {
        events.emplace_back("clear-nil");
        nil = value;
    }
    std::uint32_t SharedNilUsers() override { return users; }
    void SetSharedNilUsers(std::uint32_t value) override {
        events.emplace_back("users");
        users = value;
    }
    void DestructSound(std::uint32_t address) override {
        Check(address == 0x8004u, "sound subobject offset");
        events.emplace_back("sound");
    }
    void DestructLateStruct(std::uint32_t address) override {
        Check(address == 0x51dcu, "late struct offset");
        events.emplace_back("late");
    }
    void DestructXml(std::uint32_t address) override {
        Check(address == 0x514cu, "XML offset");
        events.emplace_back("xml");
    }
    void DestructEffects(std::uint32_t manager) override {
        Check(manager == 0x5000u, "effects owner");
        events.emplace_back("effects");
    }
    std::uint32_t GetFirstElement(
        std::uint32_t tree) override {
        Check(tree == 0x5114u, "first element tree");
        events.emplace_back("first");
        return first_override;
    }
    std::uint32_t DestructEventTree(
        std::uint32_t tree) override {
        Check(tree == 0x5104u, "event tree offset");
        events.emplace_back("event-tree");
        return 0xabcdu;
    }
};

void CheckBulkAndFinalNil() {
    Runtime runtime;
    Check(pal4::UiFrameManagerSingleton::DestructManager(
              0x5000u, runtime) == 0xabcdu,
          "final event tree return");
    Check(runtime.events == std::vector<std::string>{
              "sound", "free:32768", "late", "free:36864",
              "xml", "effects", "free:40960", "read-ref",
              "write-ref", "first", "free-nodes", "first",
              "free:24576", "users", "clear-nil",
              "free:49152", "event-tree"},
          "full destructor order");
    Check(runtime.words.at(0x5000u) == 0x8443a4u &&
              runtime.words.at(0x51a4u) == 0u &&
              runtime.words.at(0x518cu) == 0u &&
              runtime.words.at(0x5190u) == 0u &&
              runtime.words.at(0x5194u) == 0u &&
              runtime.words.at(0x5138u) == 0u &&
              runtime.words.at(0x513cu) == 0u &&
              runtime.words.at(0x5140u) == 0u &&
              runtime.words.at(0x5128u) == 0u &&
              runtime.words.at(0x512cu) == 0u &&
              runtime.words.at(0x5130u) == 0u &&
              runtime.words.at(0x5118u) == 0u &&
              runtime.words.at(0x5120u) == 0u &&
              runtime.nil == 0u && runtime.users == 0u,
          "all owned fields cleared");
    Check(runtime.bytes.at(0x7000u) == 1u,
          "shared name reference decremented");
}

void CheckIncrementalDespiteCount() {
    Runtime runtime;
    runtime.first_override = 0xe000u;
    runtime.next = {0x6000u};
    runtime.bytes[0x7000u] = 0xffu;
    runtime.users = 2u;
    Check(pal4::UiFrameManagerSingleton::DestructManager(
              0x5000u, runtime) == 0xabcdu,
          "incremental destructor return");
    Check(runtime.deleted ==
              std::vector<std::uint32_t>{0xd000u} &&
              runtime.nil == 0xc000u && runtime.users == 1u,
          "mismatched first element forces node deletion");
    Check(runtime.events == std::vector<std::string>{
              "sound", "free:32768", "late", "free:36864",
              "xml", "effects", "free:40960", "read-ref",
              "free:28672", "first", "next", "delete",
              "free:24576", "users", "event-tree"},
          "incremental path and unshared name release");
}

void CheckNullAndEmpty() {
    Runtime runtime;
    runtime.words[0x51a4u] = 0u;
    runtime.words[0x518cu] = 0u;
    runtime.words[0x5138u] = 0u;
    runtime.words[0x5128u] = 0u;
    runtime.words[0x5120u] = 0u;
    runtime.words[0x6000u] = 0x6000u;
    runtime.nil = 0u;
    Check(pal4::UiFrameManagerSingleton::DestructManager(
              0x5000u, runtime) == 0xabcdu,
          "empty destructor return");
    Check(runtime.events == std::vector<std::string>{
              "late", "free:0", "xml", "effects", "free:0",
              "free:24576", "users", "clear-nil", "event-tree"},
          "empty destructor still frees null native buffers");
}

}  // namespace

int main() {
    try {
        CheckBulkAndFinalNil();
        CheckIncrementalDespiteCount();
        CheckNullAndEmpty();
        std::cout << "ui_frame_manager_full_destruct_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
