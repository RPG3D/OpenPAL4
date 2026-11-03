#include "pal4/inventory_base_storage.h"

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

struct Runtime final : pal4::IInventoryBaseStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> writes;
    std::uint32_t fail_element = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        if (address == fail_element)
            throw std::runtime_error("element construction");
        bytes[address] = value;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
};

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::InventoryBaseStorage::Construct(
              0x5000u, 0x37u, 0x82u, runtime) == 0x5000u &&
              runtime.words[0x5000u] ==
                  pal4::InventoryBaseStorage::kVtable &&
              runtime.words[0x5004u] == 1u &&
              runtime.bytes[0x506cu] == 0x82u &&
              runtime.words[0x5070u] == 0u &&
              runtime.words[0x5074u] == 0u &&
              runtime.words[0x5078u] == 0u,
          "constructor sets header and tail fields");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto element = 0x5008u + index * 16u;
        Check(runtime.bytes[element] == 0x37u &&
                  runtime.words[element + 4u] == 0u &&
                  runtime.words[element + 8u] == 0u &&
                  runtime.words[element + 12u] == 0u &&
                  runtime.words[0x5058u + index * 4u] == 0u,
              "constructor initializes each of five elements and counters");
    }
    Check(runtime.writes[0] == 0x5004u &&
              runtime.writes[1] == 0x5000u &&
              runtime.writes[2] == 0x500cu,
          "header precedes array construction");
}

void CheckReset() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto element = 0x5008u + index * 16u;
        runtime.words[element + 4u] = 0x6000u + index * 0x100u;
        runtime.words[element + 8u] =
            runtime.words[element + 4u] + 24u;
        runtime.words[0x5058u + index * 4u] = index + 1u;
    }
    runtime.words[0x5070u] = 0x8000u;
    runtime.words[0x5074u] = 0x8030u;
    Check(pal4::InventoryBaseStorage::Reset(0x5000u, runtime) ==
              0x8030u &&
              runtime.words[0x5074u] == 0x8000u,
          "reset returns old tail end and resets it to begin");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto element = 0x5008u + index * 16u;
        Check(runtime.words[element + 8u] ==
                  runtime.words[element + 4u] &&
                  runtime.words[0x5058u + index * 4u] == 0u,
              "reset clears all five element ends and counters");
    }
}

void CheckConstructionUnwind() {
    Runtime runtime;
    runtime.fail_element = 0x5018u;
    bool threw = false;
    try {
        (void)pal4::InventoryBaseStorage::Construct(
            0x5000u, 0x37u, 0x82u, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "element construction";
    }
    Check(threw && runtime.freed == std::vector<std::uint32_t>{0u} &&
              runtime.words[0x500cu] == 0u &&
              runtime.words[0x5010u] == 0u &&
              runtime.words[0x5014u] == 0u &&
              !runtime.bytes.contains(0x5028u),
          "failed second element unwinds only the completed first element");
}

void CheckCleanup() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto element = 0x5008u + index * 16u;
        runtime.words[element + 4u] = 0x6000u + index * 0x100u;
        runtime.words[element + 8u] =
            runtime.words[element + 4u] + 12u;
        runtime.words[element + 12u] =
            runtime.words[element + 4u] + 32u;
    }
    runtime.words[0x5070u] = 0x8000u;
    runtime.words[0x5074u] = 0x8010u;
    runtime.words[0x5078u] = 0x8040u;
    pal4::InventoryBaseStorage::Cleanup(0x5000u, runtime);
    Check(runtime.freed ==
              std::vector<std::uint32_t>{0x8000u, 0x6400u, 0x6300u,
                                         0x6200u, 0x6100u, 0x6000u} &&
              runtime.words[0x5000u] ==
                  pal4::InventoryBaseStorage::kBaseVtable &&
              runtime.words[0x5004u] == 1u &&
              runtime.words[0x5070u] == 0u &&
              runtime.words[0x5074u] == 0u &&
              runtime.words[0x5078u] == 0u,
          "cleanup releases tail and elements in reverse order");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto element = 0x5008u + index * 16u;
        Check(runtime.words[element + 4u] == 0u &&
                  runtime.words[element + 8u] == 0u &&
                  runtime.words[element + 12u] == 0u,
              "cleanup clears every element pointer triplet");
    }
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckConstructionUnwind();
        CheckReset();
        CheckCleanup();
        std::cout << "inventory_base_storage_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
