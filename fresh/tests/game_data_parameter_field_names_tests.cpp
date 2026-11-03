#include "pal4/game_data_parameter_field_names.h"

#include <array>
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

struct ExpectedField {
    std::string_view text;
    bool reset;
    bool allocate;
};

struct ExpectedGroup {
    pal4::ParameterFieldGroup group;
    std::uint32_t base;
    std::uint32_t destructor;
    std::vector<ExpectedField> fields;
};

std::vector<ExpectedGroup> Groups() {
    using pal4::ParameterFieldGroup;
    return {
        {ParameterFieldGroup::Attack, 0x8eef90, 0x4e1200,
         {{"PRA_NAME", false, false}, {"PRA_DESCRIPTION", true, true},
          {"PRA_PROPERTY", true, true}, {"PRA_EF2", true, true},
          {"PRA_EF3", true, true}, {"PRA_EF4", true, false}}},
        {ParameterFieldGroup::Product, 0x8eef40, 0x4e13c0,
         {{"PRA_PRODUCT_ID", false, false},
          {"PRA_NEED_POTENTIAL", false, false},
          {"PRA_NEED_MAGIC", false, false},
          {"PRA_SKILL_ID", true, true}, {"PRA_PRICE", true, true}}},
        {ParameterFieldGroup::Progression, 0x8eeec0, 0x4e16b0,
         {{"PRI_MAX_HP", true, true}, {"PRI_ADDED_RAGE", true, true},
          {"PRI_MAX_MP", true, true}, {"PRI_WU", true, true},
          {"PRI_DEFENCE", true, true}, {"PRI_SPEED", true, true},
          {"PRI_LUCKY", true, false},
          {"PRI_MAGIC_ATTACK", true, false}}},
        {ParameterFieldGroup::Special, 0x8eee80, 0x4e1840,
         {{"PSA_ADDED_CRITICAL", false, true},
          {"PSA_FEND_OFF", false, false},
          {"PSA_ADDED_HITTING", false, true},
          {"PSA_PAY_FOR_SPARE", false, false}}},
        {ParameterFieldGroup::Elements, 0x8eee20, 0x4e1a90,
         {{"PAA_PHYSICAL", false, false}, {"PAA_WATER", true, true},
          {"PAA_FIRE", true, true}, {"PAA_THUNDER", true, true},
          {"PAA_AIR", true, true}, {"PAA_EARTH", true, false}}},
        {ParameterFieldGroup::Materials, 0x8eede0, 0x4e1c20,
         {{"P_MAT_1", false, true}, {"P_MAT_2", false, false},
          {"P_MAT_3", false, true}, {"P_MAT_4", false, false}}},
    };
}

struct Runtime final : pal4::IGameDataParameterFieldNamesRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed;
    std::uint32_t callback = 0;
    std::uint32_t fail_at = 0;
    int result = -19;

    std::uint8_t InitialSeed() override {
        calls.emplace_back("seed");
        return 0x73;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void ResetText(std::uint32_t address) override {
        for (std::uint32_t offset : {4u, 8u, 12u})
            words[address + offset] = 0;
        calls.push_back("reset " + std::to_string(address));
    }
    void AssignText(std::uint32_t address,
                    std::string_view text) override {
        Check(words[address + 4] == 0 && words[address + 8] == 0 &&
                  words[address + 12] == 0,
              "assigned text must start empty");
        names[address] = text;
        calls.push_back("assign " + std::to_string(address));
    }
    bool AllocateText(std::uint32_t address,
                      std::string_view text) override {
        Check(words[address + 4] == 0 && words[address + 8] == 0 &&
                  words[address + 12] == 0,
              "allocated text must start empty");
        calls.push_back("allocate " + std::to_string(address));
        if (address == fail_at) return false;
        names[address] = text;
        return true;
    }
    void DestroyText(std::uint32_t address) override {
        destroyed.push_back(address);
        calls.push_back("destroy " + std::to_string(address));
    }
    int RegisterAtexit(std::uint32_t address) override {
        callback = address;
        calls.emplace_back("register");
        return result;
    }
};

void CheckGroup(const ExpectedGroup& group) {
    Runtime runtime;
    Check(pal4::GameDataParameterFieldNames::InitializeAndRegister(
              group.group, runtime) == -19 &&
              runtime.callback == group.destructor &&
              runtime.calls.front() == "seed" &&
              runtime.calls.back() == "register" &&
              runtime.names.size() == group.fields.size(),
          "group initializes before registering its destructor");
    std::size_t call = 1;
    for (std::size_t i = 0; i < group.fields.size(); ++i) {
        const auto address = group.base + static_cast<std::uint32_t>(i) * 16;
        const auto& expected = group.fields[i];
        Check(runtime.bytes[address] == 0x73 &&
                  runtime.names[address] == expected.text,
              "field seed, address and IDA literal");
        Check(runtime.calls[call++] == "byte " + std::to_string(address),
              "field seed precedes initialization");
        if (expected.reset) {
            Check(runtime.calls[call++] == "reset " + std::to_string(address),
                  "field uses native text reset path");
        } else {
            for (std::uint32_t offset : {4u, 8u, 12u})
                Check(runtime.calls[call++] ==
                          "word " + std::to_string(address + offset),
                      "field directly clears its three words");
        }
        Check(runtime.calls[call++] ==
                  std::string(expected.allocate ? "allocate " : "assign ") +
                      std::to_string(address),
              "field uses IDA assignment or allocation path");
    }
    Check(call + 1 == runtime.calls.size(),
          "group has no extra initialization operations");

    runtime.calls.clear();
    pal4::GameDataParameterFieldNames::Destroy(group.group, runtime);
    Check(runtime.destroyed.size() == group.fields.size(),
          "destructor visits every text field");
    for (std::size_t i = 0; i < group.fields.size(); ++i)
        Check(runtime.destroyed[i] ==
                  group.base +
                      static_cast<std::uint32_t>(group.fields.size() - 1 - i) * 16,
              "destructor walks contiguous text fields backward");
}

void CheckAllocationFailure() {
    Runtime runtime;
    runtime.fail_at = 0x8eefb0;
    pal4::GameDataParameterFieldNames::Initialize(
        pal4::ParameterFieldGroup::Attack, runtime);
    Check(runtime.names.size() == 5 &&
              runtime.names.find(runtime.fail_at) == runtime.names.end() &&
              runtime.names.at(0x8eefe0) == "PRA_EF4" &&
              runtime.callback == 0,
          "failed allocation does not stop later fields or register atexit");
}

}  // namespace

int main() {
    try {
        const auto groups = Groups();
        for (const auto& group : groups) CheckGroup(group);
        CheckAllocationFailure();
        std::cout << "game_data_parameter_field_names_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
