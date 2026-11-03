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

struct Runtime final : pal4::IUiFrameManagerTreeDestructRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::uint32_t> deleted;
    std::uint32_t nil = 0x9000u;
    std::uint32_t users = 2u;
    std::vector<std::uint32_t> walk;
    std::size_t walk_index = 0u;

    Runtime() {
        words[0x1014u] = 0x2001u;
        bytes[0x2000u] = 2u;
        words[0x1004u] = 0x3000u;
        words[0x3000u] = 0x3000u;
        words[0x3004u] = 0x4000u;
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
        return 0x79u;
    }
    void FreeNodes(std::uint32_t tree,
                   std::uint32_t root) override {
        Check(tree == 0x1000u && root == 0x4000u,
              "bulk owner and root");
        events.emplace_back("free-nodes");
    }
    std::uint32_t NextNode(std::uint32_t& iterator) override {
        Check(walk_index < walk.size(), "iterator bound");
        const auto old = iterator;
        iterator = walk[walk_index++];
        events.emplace_back("next");
        return old;
    }
    void DeleteNode(std::uint32_t tree,
                    std::uint32_t node) override {
        Check(tree == 0x1000u, "delete owner");
        events.emplace_back("delete");
        deleted.push_back(node);
    }
    void FirstElement(std::uint32_t tree) override {
        Check(tree == 0x1000u, "first element owner");
        events.emplace_back("first-element");
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t value) override {
        events.emplace_back("clear-nil");
        nil = value;
    }
    std::uint32_t SharedNilUsers() override { return users; }
    void SetSharedNilUsers(std::uint32_t count) override {
        events.emplace_back("decrement-users");
        users = count;
    }
};

void CheckBulkShared() {
    Runtime runtime;
    runtime.words[0x100cu] = 2u;
    Check(pal4::UiFrameManagerSingleton::DestructEmbeddedTree(
              0x1000u, runtime) == 1u,
          "nonlast nil user returns decremented count");
    Check(runtime.events == std::vector<std::string>{
              "read-ref", "write-ref", "free-nodes", "free:12288",
              "decrement-users"},
          "bulk release order");
    Check(runtime.bytes.at(0x2000u) == 1u &&
              runtime.words.at(0x1004u) == 0u &&
              runtime.words.at(0x100cu) == 0u &&
              runtime.words.at(0x1014u) == 0u &&
              runtime.words.at(0x1018u) == 0u &&
              runtime.words.at(0x101cu) == 0u &&
              runtime.nil == 0x9000u,
          "bulk fields and shared nil retained");
}

void CheckInconsistentEmptyCount() {
    Runtime runtime;
    runtime.words[0x100cu] = 0u;
    runtime.words[0x3000u] = 0x4100u;
    runtime.walk = {0x4200u, 0x3000u};
    runtime.bytes[0x2000u] = 0u;
    runtime.users = 1u;
    Check(pal4::UiFrameManagerSingleton::DestructEmbeddedTree(
              0x1000u, runtime) == 0x79u,
          "last nil user's free result");
    Check(runtime.deleted ==
              std::vector<std::uint32_t>{0x4100u, 0x4200u} &&
              runtime.nil == 0u && runtime.users == 0u,
          "zero-count nodes deleted and nil released");
    Check(runtime.events == std::vector<std::string>{
              "read-ref", "free:8192", "next", "delete",
              "next", "delete", "free:12288",
              "decrement-users", "clear-nil", "free:36864"},
          "incremental release order");
}

void CheckEmptyNoNil() {
    Runtime runtime;
    runtime.words[0x1014u] = 0u;
    runtime.words[0x100cu] = 0u;
    runtime.users = 1u;
    runtime.nil = 0u;
    Check(pal4::UiFrameManagerSingleton::DestructEmbeddedTree(
              0x1000u, runtime) == 0u,
          "empty shared nil return");
    Check(runtime.events == std::vector<std::string>{
              "free:12288", "decrement-users", "clear-nil"},
          "empty tree cleanup");
}

void CheckEventTree() {
    Runtime bulk;
    bulk.words[0x100cu] = 1u;
    Check(pal4::UiFrameManagerSingleton::DestructEventTree(
              0x1000u, bulk) == 1u,
          "event tree shared nil users");
    Check(bulk.events == std::vector<std::string>{
              "free-nodes", "first-element", "free:12288",
              "decrement-users"},
          "event tree refreshes first element before head free");
    Check(bulk.words.at(0x3004u) == 0x9000u &&
              bulk.words.at(0x3008u) == 0x3000u,
          "event tree head reset");

    Runtime incremental;
    incremental.words[0x100cu] = 0u;
    incremental.words[0x3000u] = 0x4100u;
    incremental.walk = {0x3000u};
    Check(pal4::UiFrameManagerSingleton::DestructEventTree(
              0x1000u, incremental) == 1u &&
              incremental.deleted ==
                  std::vector<std::uint32_t>{0x4100u},
          "event tree incremental release");
    Check(incremental.events == std::vector<std::string>{
              "next", "delete", "free:12288",
              "decrement-users"},
          "zero-count event tree skips first element refresh");
}

void CheckAuxiliaryTree() {
    Runtime bulk;
    bulk.words[0x1014u] = 0u;
    bulk.words[0x100cu] = 2u;
    Check(pal4::UiFrameManagerSingleton::DestructAuxiliaryTree(
              0x1000u, bulk) == 1u,
          "auxiliary tree keeps nonfinal shared nil");
    Check(bulk.events == std::vector<std::string>{
              "free-nodes", "free:12288", "decrement-users"},
          "auxiliary tree bulk release skips name and first-element refresh");
    Check(bulk.words.at(0x3000u) == 0x3000u &&
              bulk.words.at(0x3004u) == 0x9000u &&
              bulk.words.at(0x3008u) == 0x3000u &&
              bulk.words.at(0x1004u) == 0u &&
              bulk.words.at(0x100cu) == 0u,
          "auxiliary tree resets head and fields");

    Runtime empty;
    empty.words[0x1014u] = 0u;
    empty.words[0x100cu] = 0u;
    empty.users = 1u;
    Check(pal4::UiFrameManagerSingleton::DestructAuxiliaryTree(
              0x1000u, empty) == 0x79u &&
              empty.events == std::vector<std::string>{
                  "free:12288", "decrement-users", "clear-nil",
                  "free:36864"},
          "empty final user frees shared nil and returns free result");

    Runtime inconsistent;
    inconsistent.words[0x1014u] = 0u;
    inconsistent.words[0x100cu] = 0u;
    inconsistent.words[0x3000u] = 0x4100u;
    inconsistent.walk = {0x4200u, 0x3000u};
    Check(pal4::UiFrameManagerSingleton::DestructAuxiliaryTree(
              0x1000u, inconsistent) == 1u &&
              inconsistent.deleted ==
                  std::vector<std::uint32_t>{0x4100u, 0x4200u} &&
              inconsistent.events == std::vector<std::string>{
                  "next", "delete", "next", "delete", "free:12288",
                  "decrement-users"},
          "zero count with nodes follows incremental deletion path");
}

}  // namespace

int main() {
    try {
        CheckBulkShared();
        CheckInconsistentEmptyCount();
        CheckEmptyNoNil();
        CheckEventTree();
        CheckAuxiliaryTree();
        std::cout << "ui_frame_manager_tree_destruct_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
