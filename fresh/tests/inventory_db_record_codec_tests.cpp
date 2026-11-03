#include "pal4/inventory_db_record_codec.h"

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

struct Runtime final : pal4::IInventoryDbRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> incoming;
    std::vector<std::uint32_t> outgoing;
    std::vector<std::string> calls;
    std::size_t next = 0u;
    std::uint32_t found = 0x8000u;
    std::uint32_t write_result = 0xabcdef01u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t ReadStreamWord(std::uint32_t stream) override {
        Check(stream == 0x4000u && next < incoming.size(),
              "read stream arguments");
        calls.emplace_back("stream read");
        return incoming[next++];
    }
    std::uint32_t WriteStreamWord(std::uint32_t stream,
                                  std::uint32_t value) override {
        Check(stream == 0x4000u, "write stream address");
        calls.emplace_back("stream write");
        outgoing.push_back(value);
        return write_result;
    }
    std::uint32_t FindGameData(std::uint32_t id) override {
        Check(id == 3072u, "GameDB lookup ID");
        calls.emplace_back("lookup");
        return found;
    }
};

void CheckRead() {
    Runtime runtime;
    runtime.incoming = {17u, 29u, 3072u};
    Check(pal4::InventoryDbRecordCodec::Read(
              0x5000u, 0x4000u, runtime) &&
              runtime.words[0x5004u] == 17u &&
              runtime.words[0x5008u] == 29u &&
              runtime.words[0x5000u] == 0x8000u &&
              runtime.next == 3u &&
              runtime.calls == std::vector<std::string>{
                  "stream read", "write:20484", "stream read",
                  "write:20488", "stream read", "lookup",
                  "write:20480"},
          "reader stores fields before resolving GameDB ID");

    Runtime missing;
    missing.incoming = {0u, 0xffffffffu, 3072u};
    missing.found = 0u;
    Check(!pal4::InventoryDbRecordCodec::Read(
              0x5000u, 0x4000u, missing) &&
              missing.words[0x5004u] == 0u &&
              missing.words[0x5008u] == 0xffffffffu &&
              missing.words[0x5000u] == 0u,
          "missing GameDB record returns false after storing both fields");
}

void CheckWrite() {
    Runtime runtime;
    runtime.words = {{0x5000u, 0x8000u}, {0x5004u, 17u},
                     {0x5008u, 29u}, {0x8000u, 3072u}};
    Check(pal4::InventoryDbRecordCodec::Write(
              0x5000u, 0x4000u, runtime) == 0xabcdef01u &&
              runtime.outgoing ==
                  std::vector<std::uint32_t>{17u, 29u, 3072u} &&
              runtime.calls == std::vector<std::string>{
                  "read:20484", "stream write", "read:20488",
                  "stream write", "read:20480", "read:32768",
                  "stream write"},
          "writer emits two fields then pointed-to ID and final return");
}

}  // namespace

int main() {
    try {
        CheckRead();
        CheckWrite();
        std::cout << "inventory_db_record_codec_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
