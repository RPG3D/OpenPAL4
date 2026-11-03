#include "pal4/game_data_class_fields.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataClassFieldsRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    bool allocate_text = true;

    bool InitializeDirectText(std::uint32_t slot, std::uint8_t seed,
                              std::string_view text) override {
        Check(slot == 0 && seed == 0x34 && text == "ID",
              "direct text slot, seed and literal");
        calls.emplace_back("direct ID");
        return allocate_text;
    }
    void ResetText(std::uint32_t slot, std::uint8_t seed,
                   std::uint8_t flag) override {
        Check(seed == 0x34 && flag == 0, "reset seed and flag");
        calls.emplace_back("reset " + std::to_string(slot));
    }
    bool AssignAllocatedText(std::uint32_t slot,
                             std::string_view text) override {
        Check((slot == 0 && text == "CLASS") ||
                  (slot == 1 && text == "TYPE"),
              "allocated field literal and slot");
        calls.emplace_back("allocate " + std::string(text));
        return allocate_text;
    }
    void CreateEffect(std::uint32_t object, std::uint32_t slot,
                      std::uint32_t offset) override {
        Check(object == 0x1234 && slot == offset / 8,
              "Effect object and temporary text slot");
        calls.emplace_back("effect " + std::to_string(offset));
    }
    void ReleaseText(std::uint32_t slot) override {
        calls.emplace_back("release " + std::to_string(slot));
    }
    void SetErrorFlag(std::uint32_t slot, std::uint8_t flag) override {
        Check(slot == 1 && flag == 1, "final TYPE text flag");
        calls.emplace_back("flag");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words[address];
    }
};

void CheckInitialization(bool allocation_succeeds) {
    Runtime runtime;
    runtime.allocate_text = allocation_succeeds;
    Check(pal4::GameDataClassFields::Initialize(0x1234, runtime) == 1,
          "field initializer returns one");
    const auto expected = allocation_succeeds
        ? std::vector<std::string>{
              "direct ID", "effect 0", "release 0", "reset 0",
              "allocate CLASS", "effect 4", "release 0", "reset 1",
              "allocate TYPE", "effect 8", "flag"}
        : std::vector<std::string>{
              "direct ID", "effect 0", "reset 0", "allocate CLASS",
              "effect 4", "reset 1", "allocate TYPE", "effect 8", "flag"};
    Check(runtime.calls == expected,
          "three Effect fields and temporary text lifetime keep IDA order");
}

void CheckBounds() {
    Runtime runtime;
    runtime.words[0x1238] = 4;
    runtime.words[0x123c] = 6;
    Check(pal4::GameDataClassFields::MembersInRange(0x1234, runtime),
          "inclusive maximums are accepted");
    runtime.calls.clear();
    runtime.words[0x1238] = 5;
    Check(!pal4::GameDataClassFields::MembersInRange(0x1234, runtime) &&
              runtime.calls == std::vector<std::string>{"read 4664"},
          "first out-of-range member short-circuits the second read");
    runtime.words[0x1238] = 0xffffffff;
    Check(!pal4::GameDataClassFields::MembersInRange(0x1234, runtime),
          "member comparison is unsigned");
    runtime.words[0x1238] = 4;
    runtime.words[0x123c] = 7;
    Check(!pal4::GameDataClassFields::MembersInRange(0x1234, runtime),
          "second upper bound is six");
}

}  // namespace

int main() {
    try {
        CheckInitialization(true);
        CheckInitialization(false);
        CheckBounds();
        std::cout << "game_data_class_fields_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
