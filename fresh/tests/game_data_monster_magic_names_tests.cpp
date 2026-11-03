#include "pal4/game_data_monster_magic_names.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMonsterMagicNamesRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed;
    std::uint32_t fail_allocation_at = 0;
    std::uint32_t registered_callback = 0;
    int registration_result = -3;

    std::uint8_t InitialSeed() override {
        calls.emplace_back("seed");
        return 0x5a;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void AssignText(std::uint32_t address,
                    std::string_view text) override {
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(words[address + step] == 0,
                  "assigned text must start with empty pointers");
        names[address] = text;
        calls.push_back("assign " + std::to_string(address));
    }
    bool AllocateText(std::uint32_t address,
                       std::string_view text) override {
        calls.push_back("allocate text " + std::to_string(address));
        for (std::uint32_t step : {4u, 8u, 12u})
            words[address + step] = 0;
        if (address == fail_allocation_at)
            return false;
        names[address] = text;
        return true;
    }
    void DestroyText(std::uint32_t address) override {
        destroyed.push_back(address);
        calls.push_back("destroy " + std::to_string(address));
    }
    int RegisterAtexit(std::uint32_t callback_address) override {
        registered_callback = callback_address;
        calls.emplace_back("register");
        return registration_result;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected name operation missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckInitializationAndRegistration() {
    Runtime runtime;
    Check(pal4::GameDataMonsterMagicNames::InitializeAndRegister(runtime) ==
              -3 &&
              runtime.registered_callback == 0x4dd720 &&
              runtime.calls.front() == "seed" &&
              runtime.calls.back() == "register",
          "five names initialize before destructor registration");
    for (std::uint32_t i = 0; i < 5; ++i) {
        const auto address = 0x8eed50 + i * 16;
        Check(runtime.bytes[address] == 0x5a &&
                  runtime.names[address] ==
                      "MONSTER_MAGIC_" + std::to_string(i + 1),
              "all five names use exact IDA literals and contiguous addresses");
    }
    Check(At(runtime, "assign 9366864") <
              At(runtime, "assign 9366880") &&
              At(runtime, "assign 9366880") <
                  At(runtime, "assign 9366896") &&
              At(runtime, "assign 9366896") <
                  At(runtime, "allocate text 9366912") &&
              At(runtime, "allocate text 9366912") <
                  At(runtime, "allocate text 9366928") &&
              At(runtime, "allocate text 9366928") <
                  At(runtime, "register"),
          "three assignment paths precede two allocation paths and atexit");

    Runtime failed;
    failed.fail_allocation_at = 0x8eed90;
    pal4::GameDataMonsterMagicNames::Initialize(failed);
    Check(failed.names.size() == 4 &&
              failed.names.find(0x8eed90) == failed.names.end() &&
              failed.calls.back() == "allocate text 9366928",
          "last text allocation failure leaves earlier names available");
}

void CheckFieldInitializationAndRegistration() {
    Runtime runtime;
    Check(pal4::GameDataMonsterMagicNames::InitializeFieldsAndRegister(runtime) ==
              -3 &&
              runtime.registered_callback == 0x4dd970 &&
              runtime.calls.front() == "seed" &&
              runtime.calls.back() == "register",
          "six fields initialize before their exact atexit callback");

    const std::vector<std::string> expected{
        "MA_NAME", "MA_ICON", "MA_MODEL", "MA_TEXTURE", "MA_DESC",
        "EFF_FINAL_HIT"};
    for (std::uint32_t i = 0; i < expected.size(); ++i) {
        const auto address = 0x8eecf0 + i * 16;
        Check(runtime.bytes[address] == 0x5a &&
                  runtime.names[address] == expected[i],
              "field address, seed and literal must match IDA");
        const auto operation = std::string(i == 0 || i == 5 ? "assign " :
                                                            "allocate text ") +
                               std::to_string(address);
        const auto index = At(runtime, operation);
        if (i != 0) {
            const auto previous_address = address - 16;
            const auto previous_operation =
                std::string(i == 1 ? "assign " : "allocate text ") +
                std::to_string(previous_address);
            Check(At(runtime, previous_operation) < index,
                  "field operations must follow address order");
        }
        Check(index < At(runtime, "register"),
              "field assignment must precede registration");
    }
    for (std::uint32_t address : {0x8eecf0u, 0x8eed40u}) {
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[address + step] == 0,
                  "direct-assignment field has empty pointer triplet");
    }

    Runtime failed;
    failed.fail_allocation_at = 0x8eed20;
    pal4::GameDataMonsterMagicNames::InitializeFields(failed);
    Check(failed.names.size() == 5 &&
              failed.names.find(0x8eed20) == failed.names.end() &&
              failed.names.at(0x8eed40) == "EFF_FINAL_HIT",
          "middle allocation failure does not skip later fields");
}

void CheckFieldDestruction() {
    Runtime runtime;
    pal4::GameDataMonsterMagicNames::DestroyFields(runtime);
    Check(runtime.destroyed == std::vector<std::uint32_t>{
                                   0x8eed40, 0x8eed30, 0x8eed20,
                                   0x8eed10, 0x8eed00, 0x8eecf0},
          "six field texts are destroyed in reverse address order");
}

void CheckDropFieldGroup() {
    Runtime runtime;
    Check(pal4::GameDataMonsterMagicNames::InitializeDropFieldsAndRegister(
              runtime) == -3 &&
              runtime.registered_callback == 0x4ddb00,
          "drop-field destructor registration forwards CRT result");
    const std::vector<std::string> expected{
        "MA_MAX_DROP_MONEY", "MA_MIN_DROP_MONEY", "MA_ATK_ADDED_RAGE"};
    for (std::uint32_t i = 0; i < expected.size(); ++i) {
        const auto address = 0x8eecc0 + i * 16;
        Check(runtime.bytes[address] == 0x5a &&
                  runtime.names[address] == expected[i] &&
                  At(runtime, "allocate text " + std::to_string(address)) <
                      At(runtime, "register"),
              "drop-field literals, addresses and order match IDA");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[address + step] == 0,
                  "drop-field text pointer triplet is cleared");
    }
    Check(At(runtime, "allocate text 9366720") <
              At(runtime, "allocate text 9366736") &&
              At(runtime, "allocate text 9366736") <
                  At(runtime, "allocate text 9366752"),
          "drop-field allocation follows address order");

    Runtime failed;
    failed.fail_allocation_at = 0x8eecd0;
    pal4::GameDataMonsterMagicNames::InitializeDropFields(failed);
    Check(failed.names.size() == 2 &&
              failed.names.find(0x8eecd0) == failed.names.end() &&
              failed.names.at(0x8eece0) == "MA_ATK_ADDED_RAGE",
          "drop-field allocation failure does not skip later fields");

    Runtime destroyed;
    pal4::GameDataMonsterMagicNames::DestroyDropFields(destroyed);
    Check(destroyed.destroyed ==
              std::vector<std::uint32_t>{0x8eece0, 0x8eecd0, 0x8eecc0},
          "drop fields are destroyed in reverse order");
}

void CheckDataTypeGroup() {
    Runtime runtime;
    Check(pal4::GameDataMonsterMagicNames::InitializeDataTypesAndRegister(
              runtime) == -3 &&
              runtime.registered_callback == 0x4ddc90,
          "MDT destructor registration forwards CRT result");
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto address = 0x8eec80 + i * 16;
        const auto operation =
            std::string(i % 2 == 0 ? "allocate text " : "assign ") +
            std::to_string(address);
        Check(runtime.bytes[address] == 0x5a &&
                  runtime.names[address] == "MDT_" + std::to_string(i + 1) &&
                  At(runtime, operation) < At(runtime, "register"),
              "MDT literal, address and allocation mode match IDA");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[address + step] == 0,
                  "MDT text pointer triplet is cleared");
        if (i != 0) {
            const auto previous =
                std::string((i - 1) % 2 == 0 ? "allocate text " : "assign ") +
                std::to_string(address - 16);
            Check(At(runtime, previous) < At(runtime, operation),
                  "MDT initialization follows address order");
        }
    }
    Runtime failed;
    failed.fail_allocation_at = 0x8eec80;
    pal4::GameDataMonsterMagicNames::InitializeDataTypes(failed);
    Check(failed.names.size() == 3 &&
              failed.names.find(0x8eec80) == failed.names.end() &&
              failed.names.at(0x8eecb0) == "MDT_4",
          "MDT allocation failure does not skip later fields");

    Runtime destroyed;
    pal4::GameDataMonsterMagicNames::DestroyDataTypes(destroyed);
    Check(destroyed.destroyed == std::vector<std::uint32_t>{
                                     0x8eecb0, 0x8eeca0, 0x8eec90,
                                     0x8eec80},
          "MDT fields are destroyed in reverse order");
}

void CheckDestruction() {
    Runtime runtime;
    pal4::GameDataMonsterMagicNames::Destroy(runtime);
    Check(runtime.destroyed == std::vector<std::uint32_t>{
                                   0x8eed90, 0x8eed80, 0x8eed70,
                                   0x8eed60, 0x8eed50},
          "vector destructor releases five 16-byte texts in reverse order");
}

}  // namespace

int main() {
    try {
        CheckInitializationAndRegistration();
        CheckDestruction();
        CheckFieldInitializationAndRegistration();
        CheckFieldDestruction();
        CheckDropFieldGroup();
        CheckDataTypeGroup();
        std::cout << "game_data_monster_magic_names_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
