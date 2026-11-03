#include "pal4/combat_effect_slot.h"

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

constexpr std::uint32_t kSlot = 0x1000u;

struct Runtime final : pal4::ICombatEffectSlotRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool throw_on_name = false;

    void ConstructNameNode(std::uint32_t slot) override {
        Check(slot == kSlot, "name node starts at slot base");
        calls.push_back("name");
        if (throw_on_name) throw std::runtime_error("name");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        words[address] = value;
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::CombatEffectSlot::Construct(kSlot, runtime) == kSlot &&
              runtime.bytes.at(kSlot + 8u) == 0u &&
              runtime.words.at(kSlot + 12u) == 0u &&
              runtime.words.at(kSlot + 16u) == 0u &&
              runtime.words.at(kSlot + 20u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "name", "byte:4104", "word:4108",
                  "word:4112", "word:4116"},
          "effect slot initializes name, byte and three words in IDA order");
    Check(runtime.bytes.size() == 1u && runtime.words.size() == 3u,
          "constructor leaves DNameNode-owned first eight bytes to helper");
}

void NameFailure() {
    Runtime runtime;
    runtime.throw_on_name = true;
    bool caught = false;
    try {
        (void)pal4::CombatEffectSlot::Construct(kSlot, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{"name"} &&
              runtime.bytes.empty() && runtime.words.empty(),
          "failed name construction does not write subsequent fields");
}

}  // namespace

int main() {
    Construction();
    NameFailure();
    std::cout << "Combat effect slot tests passed\n";
}
