#include "pal4/inventory_recent_snapshot.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IInventoryRecentSnapshotRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation{0x5000u};

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return allocation;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("write-byte:" + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
};

Runtime MakeRuntime(std::uint32_t begin, std::uint32_t end) {
    Runtime runtime;
    runtime.bytes[0x106cu] = 0x81u;
    runtime.words[0x1070u] = begin;
    runtime.words[0x1074u] = end;
    return runtime;
}

void TestCopy() {
    auto runtime = MakeRuntime(0x3000u, 0x300cu);
    runtime.words[0x3000u] = 7u;
    runtime.words[0x3004u] = 11u;
    runtime.words[0x3008u] = 13u;
    runtime.bytes[0x2001u] = 0xaau;
    Check(pal4::InventoryRecentSnapshot::Copy(
              0x1000u, 0x2000u, runtime) == 0x2000u &&
              runtime.bytes.at(0x2000u) == 0x81u &&
              runtime.bytes.at(0x2001u) == 0xaau &&
              runtime.words.at(0x2004u) == 0x5000u &&
              runtime.words.at(0x5000u) == 7u &&
              runtime.words.at(0x5004u) == 11u &&
              runtime.words.at(0x5008u) == 13u &&
              runtime.words.at(0x2008u) == 0x500cu &&
              runtime.words.at(0x200cu) == 0x500cu,
          "copy preserves flag byte, DWORD sequence and both end pointers");
    Check(runtime.calls == std::vector<std::string>{
              "read-byte:4204", "write-byte:8192", "read:4208",
              "read:4212", "allocate:12", "write:8196",
              "read:12288", "write:20480", "read:12292",
              "write:20484", "read:12296", "write:20488",
              "write:8200", "write:8204"},
          "source and output operations follow the machine-code order");
}

void TestEmptyAndAllocationFailure() {
    auto runtime = MakeRuntime(0u, 0u);
    Check(pal4::InventoryRecentSnapshot::Copy(
              0x1000u, 0x2000u, runtime) == 0x2000u &&
              runtime.words.at(0x2004u) == 0x5000u &&
              runtime.words.at(0x2008u) == 0x5000u &&
              runtime.words.at(0x200cu) == 0x5000u &&
              runtime.calls[4] == "allocate:0",
          "zero count still requests a zero-byte allocation");
    runtime = MakeRuntime(0x3000u, 0x3008u);
    runtime.allocation = 0u;
    runtime.words[0x3004u] = 11u;
    Check(pal4::InventoryRecentSnapshot::Copy(
              0x1000u, 0x2000u, runtime) == 0x2000u &&
              runtime.words.at(0x2004u) == 0u &&
              runtime.words.at(0x2008u) == 8u &&
              runtime.words.at(0x200cu) == 8u &&
              runtime.words.find(0x3000u) == runtime.words.end() &&
              runtime.words.at(4u) == 11u,
          "failed allocation skips first element then writes second to address four");
}

}  // namespace

int main() {
    TestCopy();
    TestEmptyAndAllocationFailure();
    std::cout << "Inventory recent snapshot tests passed\n";
}
