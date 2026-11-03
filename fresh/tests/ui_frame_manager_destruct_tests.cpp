#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerDestructRuntime {
    static constexpr std::uint32_t kManager = 0x1000u;
    static constexpr std::uint32_t kHeader = 0x2000u;
    static constexpr std::uint32_t kNil = 0x9000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::uint32_t> cursors;
    std::vector<std::uint32_t> deleted;

    Runtime() {
        words[kManager + 4u] = kHeader;
        words[kManager + 12u] = 0u;
        words[kManager + 20u] = 0u;
        words[kHeader] = kHeader;
        words[kHeader + 4u] = kNil;
        words[kHeader + 8u] = kHeader;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("reference");
    }
    std::uint32_t NilNode() override {
        return kNil;
    }
    void DestroyCursor(std::uint32_t cursor) override {
        cursors.push_back(cursor);
        events.emplace_back("destroy-cursor");
    }
    std::uint32_t FindMaxNode(std::uint32_t node) override {
        events.emplace_back("find-max");
        return node;
    }
    void FreeNodes(std::uint32_t manager,
                   std::uint32_t root) override {
        Check(manager == kManager && root == 0x2200u,
              "bulk free root");
        events.emplace_back("free-nodes");
    }
    void FirstElement(std::uint32_t manager) override {
        Check(manager == kManager, "first element owner");
        events.emplace_back("first-element");
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        Check(node == 0x2100u, "next cursor node");
        events.emplace_back("next-node");
        return kHeader;
    }
    void DeleteNode(std::uint32_t manager,
                    std::uint32_t node) override {
        Check(manager == kManager, "delete owner");
        deleted.push_back(node);
        events.emplace_back("delete-node");
    }
    std::uint8_t SetErrorFlag(std::uint32_t record,
                              std::uint32_t flag) override {
        Check(record == kManager + 16u && flag <= 1u,
              "name reset record");
        events.emplace_back(flag == 0u ?
                                "reset-shared" : "reset-owned");
        return static_cast<std::uint8_t>(0xa0u + flag);
    }
};

void CheckEmptyTreeAndOwnedName() {
    Runtime runtime;
    runtime.words[Runtime::kManager + 20u] = 0x4001u;
    runtime.bytes[0x4000u] = 0u;
    Check(pal4::UiFrameManagerSingleton::Destruct(
              Runtime::kManager, runtime) == 0xa1u &&
              runtime.cursors.empty() && runtime.deleted.empty() &&
              runtime.events ==
                  std::vector<std::string>{"reset-owned"},
          "empty tree and unshared empty name");
}

void CheckBulkTreeAndSharedName() {
    Runtime runtime;
    runtime.words[Runtime::kHeader] = 0x2100u;
    runtime.words[Runtime::kHeader + 4u] = 0x2200u;
    runtime.words[Runtime::kHeader + 8u] = 0x2200u;
    runtime.words[Runtime::kManager + 12u] = 2u;
    runtime.words[0x2100u + 4u] = 0x2200u;
    runtime.words[0x2100u + 8u] = Runtime::kNil;
    runtime.words[0x2100u + 28u] = 0x5000u;
    runtime.words[0x2200u + 4u] = Runtime::kHeader;
    runtime.words[0x2200u + 8u] = Runtime::kNil;
    runtime.words[0x2200u + 28u] = 0x5100u;
    runtime.words[Runtime::kManager + 20u] = 0x4001u;
    runtime.bytes[0x4000u] = 3u;
    Check(pal4::UiFrameManagerSingleton::Destruct(
              Runtime::kManager, runtime) == 0xa0u &&
              runtime.cursors ==
                  std::vector<std::uint32_t>{0x5000u,
                                             0x5100u} &&
              runtime.words.at(Runtime::kManager + 12u) == 0u &&
              runtime.words.at(Runtime::kHeader) ==
                  Runtime::kHeader &&
              runtime.words.at(Runtime::kHeader + 4u) ==
                  Runtime::kNil &&
              runtime.words.at(Runtime::kHeader + 8u) ==
                  Runtime::kHeader &&
              runtime.bytes.at(0x4000u) == 2u,
          "cursor traversal, bulk tree reset and shared name");
    Check(runtime.events ==
              std::vector<std::string>{
                  "destroy-cursor", "destroy-cursor",
                  "free-nodes", "write", "write", "write",
                  "write", "first-element", "reference",
                  "reset-shared"},
          "bulk cleanup order");
}

void CheckInconsistentZeroCount() {
    Runtime runtime;
    runtime.words[Runtime::kHeader] = 0x2100u;
    runtime.words[Runtime::kHeader + 4u] = 0x2100u;
    runtime.words[Runtime::kHeader + 8u] = 0x2100u;
    runtime.words[0x2100u + 4u] = Runtime::kHeader;
    runtime.words[0x2100u + 8u] = Runtime::kNil;
    runtime.words[0x2100u + 28u] = 0x5000u;
    Check(pal4::UiFrameManagerSingleton::Destruct(
              Runtime::kManager, runtime) == 0xa1u &&
              runtime.deleted ==
                  std::vector<std::uint32_t>{0x2100u} &&
              runtime.cursors ==
                  std::vector<std::uint32_t>{0x5000u},
          "zero count with node uses successor and delete path");
}

}  // namespace

int main() {
    try {
        CheckEmptyTreeAndOwnedName();
        CheckBulkTreeAndSharedName();
        CheckInconsistentZeroCount();
        std::cout << "ui_frame_manager_destruct_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
