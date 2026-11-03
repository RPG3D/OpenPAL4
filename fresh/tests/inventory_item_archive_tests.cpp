#include "pal4/inventory_item_archive.h"

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

struct Runtime final : pal4::IInventoryItemArchiveRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> game_data;
    std::vector<std::uint32_t> incoming;
    std::vector<std::uint32_t> outgoing;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> calls;
    std::size_t next = 0u;
    std::uint32_t next_allocation = 0xb000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t FindGameData(std::uint32_t id) override {
        return game_data[id];
    }
    std::uint32_t TemporaryRecordAddress() override {
        return 0xf000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        const auto address = next_allocation;
        next_allocation += 0x100u;
        return address;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t ReadStreamWord(std::uint32_t stream) override {
        Check(stream == 0x4000u && next < incoming.size(),
              "stream word read");
        return incoming[next++];
    }
    std::uint32_t WriteStreamWord(std::uint32_t stream,
                                  std::uint32_t value) override {
        Check(stream == 0x4000u, "stream word write");
        outgoing.push_back(value);
        return 1u;
    }
    void BeforeSerialize(std::uint32_t object,
                         std::uint32_t stream) override {
        Check(object == 0x5000u && stream == 0x4000u,
              "base serialize arguments");
        calls.emplace_back("before write");
    }
    void BeforeDeserialize(std::uint32_t object,
                           std::uint32_t stream) override {
        Check(object == 0x5000u && stream == 0x4000u,
              "base deserialize arguments");
        calls.emplace_back("before read");
    }
    void WriteStreamWords(std::uint32_t stream,
                          std::uint32_t begin,
                          std::uint32_t count) override {
        Check(stream == 0x4000u, "recent block writer stream");
        calls.emplace_back("write recent block");
        for (std::uint32_t index = 0u; index < count; ++index)
            outgoing.push_back(words[begin + index * 4u]);
    }
    void ReadStreamWords(std::uint32_t stream,
                         std::uint32_t begin,
                         std::uint32_t count) override {
        Check(stream == 0x4000u, "recent block reader stream");
        calls.emplace_back("read recent block");
        for (std::uint32_t index = 0u; index < count; ++index)
            words[begin + index * 4u] = ReadStreamWord(stream);
    }
};

void CheckSerialize() {
    Runtime runtime;
    runtime.words[0x500cu] = 0x9000u;
    runtime.words[0x5010u] = 0x900cu;
    runtime.words[0x9000u] = 0x8000u;
    runtime.words[0x9004u] = 11u;
    runtime.words[0x9008u] = 3u;
    runtime.words[0x8000u] = 100u;
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.words[0x5058u + index * 4u] = (index + 1u) * 10u;
    runtime.words[0x5070u] = 0xa000u;
    runtime.words[0x5074u] = 0xa008u;
    runtime.words[0xa000u] = 11u;
    runtime.words[0xa004u] = 22u;
    Check(pal4::InventoryItemArchive::Serialize(
              0x5000u, 0x4000u, runtime) == 1u &&
              runtime.outgoing == std::vector<std::uint32_t>{
                  1u, 11u, 3u, 100u, 10u,
                  0u, 20u, 0u, 30u, 0u, 40u, 0u, 50u,
                  2u, 11u, 22u, 0u} &&
              runtime.calls == std::vector<std::string>{
                  "before write", "write recent block"},
          "writer interleaves five slots and keys before recent block and empty tree");
}

void CheckDeserialize() {
    Runtime runtime;
    runtime.game_data[100u] = 0x8000u;
    runtime.incoming = {
        1u, 11u, 3u, 100u, 10u,
        0u, 20u, 0u, 30u, 0u, 40u, 0u, 50u,
        2u, 11u, 22u, 2u, 7u, 8u};
    Check(pal4::InventoryItemArchive::Deserialize(
              0x5000u, 0x4000u, runtime) == 1u &&
              runtime.next == runtime.incoming.size() &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{12u, 8u} &&
              runtime.freed == std::vector<std::uint32_t>{0u, 0u} &&
              runtime.words[0x500cu] == 0xb000u &&
              runtime.words[0x5010u] == 0xb00cu &&
              runtime.words[0xb000u] == 0x8000u &&
              runtime.words[0xb004u] == 11u &&
              runtime.words[0xb008u] == 3u &&
              runtime.words[0x5070u] == 0xb100u &&
              runtime.words[0x5074u] == 0xb108u &&
              runtime.words[0xb100u] == 11u &&
              runtime.words[0xb104u] == 22u &&
              runtime.calls == std::vector<std::string>{
                  "before read", "read recent block"},
          "reader restores five slots/keys/recent values and consumes tree IDs");
    for (std::uint32_t index = 0u; index < 5u; ++index)
        Check(runtime.words[0x5058u + index * 4u] ==
                  (index + 1u) * 10u,
              "reader restores each slot key");

    Runtime reuse;
    reuse.words[0x5070u] = 0xa000u;
    reuse.words[0x5074u] = 0xa008u;
    reuse.words[0x5078u] = 0xa010u;
    reuse.incoming = {0u, 1u, 0u, 2u, 0u, 3u, 0u, 4u,
                      0u, 5u, 2u, 31u, 32u, 0u};
    Check(pal4::InventoryItemArchive::Deserialize(
              0x5000u, 0x4000u, reuse) == 1u &&
              reuse.allocations.empty() &&
              reuse.words[0x5074u] == 0xa008u &&
              reuse.words[0xa000u] == 31u &&
              reuse.words[0xa004u] == 32u,
          "reader reuses recent-array capacity");

    Runtime missing;
    missing.incoming = {0u, 1u, 0u, 2u, 0u, 3u, 0u, 4u,
                        1u, 6u, 7u, 999u, 5u, 0u, 0u};
    Check(pal4::InventoryItemArchive::Deserialize(
              0x5000u, 0x4000u, missing) == 0u &&
              missing.next == missing.incoming.size() &&
              missing.words[0x5058u + 4u * 4u] == 5u,
          "last slot's failed GameDB lookup determines final return");
}

}  // namespace

int main() {
    try {
        CheckSerialize();
        CheckDeserialize();
        std::cout << "inventory_item_archive_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
