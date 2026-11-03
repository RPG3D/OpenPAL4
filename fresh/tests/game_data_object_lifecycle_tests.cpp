#include "pal4/game_data_object_lifecycle.h"

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
    std::vector<std::string> calls;

    void InitializeOuterBase(std::uint32_t object) override {
        calls.push_back("outer " + std::to_string(object));
    }
    void InitializeInnerBase(std::uint32_t object) override {
        calls.push_back("inner " + std::to_string(object));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        words[address] = value;
    }
};

void BothEmbeddedStates() {
    Runtime runtime;
    Check(pal4::GameDataObjectLifecycle::ConstructBaseStates(
              1000u, runtime) == 1000u &&
              runtime.calls.front() == "outer 1004" &&
              runtime.calls[1] == "inner 1004" &&
              runtime.words[1004u + 0x148u] == 6u &&
              runtime.words[1004u + 0x14cu] == 0xffffffffu &&
              runtime.words[1340u + 0x148u] == 6u &&
              runtime.words[1340u + 0x14cu] == 0xffffffffu &&
              runtime.words[1000u] == 0x843270u &&
              runtime.calls.back() == "write 1000 8663664",
          "both embedded constructors finish before base vtable write");

    const auto& calls = runtime.calls;
    const auto find = [&](const std::string& entry) {
        for (std::size_t i = 0; i < calls.size(); ++i)
            if (calls[i] == entry) return i;
        return calls.size();
    };
    Check(find("outer 1004") < find("inner 1004") &&
              find("inner 1004") < find("outer 1340") &&
              find("outer 1340") < find("inner 1340") &&
              find("inner 1340") < find("write 1000 8663664"),
          "embedded states and final vtable follow native order");
}

void BaseVtableOnly() {
    Runtime runtime;
    pal4::GameDataObjectLifecycle::InitializeVtable(1000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "write 1000 8663664"},
          "base initializer writes only vtable 0x843270");
}

}  // namespace

int main() {
    try {
        BothEmbeddedStates();
        BaseVtableOnly();
        std::cout << "game data object lifecycle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
