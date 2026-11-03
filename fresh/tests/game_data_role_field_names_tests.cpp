#include "pal4/game_data_role_field_names.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataRoleFieldNamesRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed;
    std::uint8_t seed = 0x5a;
    std::uint32_t failed_allocation = 0;
    int atexit_result = -17;
    unsigned seed_calls = 0;

    std::uint8_t InitialSeed() override {
        ++seed_calls;
        calls.emplace_back("seed");
        return seed;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    bool AllocateText(std::uint32_t address,
                      std::string_view text) override {
        calls.push_back("allocate " + std::to_string(address) + " " +
                        std::string(text));
        return address != failed_allocation;
    }
    void AssignText(std::uint32_t address,
                    std::string_view text) override {
        calls.push_back("assign " + std::to_string(address) + " " +
                        std::string(text));
    }
    void DestroyText(std::uint32_t address) override {
        destroyed.push_back(address);
        calls.push_back("destroy " + std::to_string(address));
    }
    int RegisterAtexit(std::uint32_t callback) override {
        Check(callback == 0x4e7840, "registered destructor address");
        calls.emplace_back("atexit");
        return atexit_result;
    }
};

void CheckInitialize() {
    Runtime r;
    const auto base = 0x8ef1c8u;
    r.failed_allocation = base + 2 * 16;
    Check(pal4::GameDataRoleFieldNames::InitializeAndRegister(r) == -17 &&
              r.seed_calls == 1 && r.calls.back() == "atexit",
          "field initialization precedes atexit and forwards signed result");
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto address = base + i * 16;
        Check(r.bytes[address] == 0x5a &&
                  r.words[address + 4] == 0 &&
                  r.words[address + 8] == 0 &&
                  r.words[address + 12] == 0,
              "each 16-byte text starts with seed and zeroed storage");
    }
    Check(r.calls[5] == "allocate 9368008 RA_NAME" &&
              r.calls[10] == "assign 9368024 RA_ICON" &&
              r.calls[15] == "allocate 9368040 RA_MODEL" &&
              r.calls[20] == "assign 9368056 RA_TEXTURE",
          "RA names use direct allocation for name/model and assignment for icon/texture");
    pal4::GameDataRoleFieldNames::Destroy(r);
    Check(r.destroyed == std::vector<std::uint32_t>{
              base + 48, base + 32, base + 16, base},
          "four text slots destroy in reverse order");
}

}  // namespace

int main() {
    try {
        CheckInitialize();
        std::cout << "game_data_role_field_names_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
