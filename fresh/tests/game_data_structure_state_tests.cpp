#include "pal4/game_data_structure_state.h"

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

struct Runtime final : pal4::IGameDataStructureStateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    void InitializeOuterBase(std::uint32_t object) override {
        Check(object == 0x2000, "outer base receives object");
        events.emplace_back("outer");
    }
    void InitializeInnerBase(std::uint32_t object) override {
        Check(object == 0x2000, "inner base receives object");
        events.emplace_back("inner");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write");
    }
};

void CheckDefaults(Runtime& runtime) {
    for (std::uint32_t offset = 0x90; offset < 0x148; offset += 4) {
        std::uint32_t expected = 0;
        if (offset == 0xdc) expected = 1;
        if ((offset >= 0xf4 && offset <= 0x100) ||
            (offset >= 0x108 && offset <= 0x120))
            expected = 0x3f800000;
        Check(runtime.words.at(0x2000 + offset) == expected,
              "every word in reset region has the IDA default");
    }
    Check(runtime.words.at(0x2148) == 6 &&
              runtime.words.at(0x214c) == 0xffffffff,
          "tail fields are six and minus one");
}

}  // namespace

int main() {
    try {
        Runtime direct;
        Check(pal4::GameDataStructureState::Initialize(0x2000, direct) ==
                  0x3f800000 && direct.events.front() == "inner",
              "initializer calls inner base and returns one-float bits");
        CheckDefaults(direct);
        Runtime constructed;
        Check(pal4::GameDataStructureState::Construct(0x2000, constructed) ==
                  0x2000 && constructed.events[0] == "outer" &&
                  constructed.events[1] == "inner",
              "constructor composes both base operations in order");
        CheckDefaults(constructed);
        std::cout << "game_data_structure_state_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
