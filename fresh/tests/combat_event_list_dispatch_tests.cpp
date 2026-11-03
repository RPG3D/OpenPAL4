#include "pal4/combat_event_list_dispatch.h"

#include <bit>
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

struct Runtime final : pal4::ICombatEventListDispatchRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    std::int32_t DispatchId(std::uint32_t target, std::int32_t id) override {
        calls.push_back("dispatch " + std::to_string(target) + " " +
                        std::to_string(id));
        return id == 6 ? 83 : 17;
    }
};

void AllNineIncludingZero() {
    Runtime runtime;
    const float ids[9]{1.9f, 0.0f, -2.9f, 3.0f, 0.0f,
                       4.0f, 0.0f, 5.0f, 6.0f};
    for (std::uint32_t i = 0u; i < 9u; ++i)
        runtime.words[1000u + 20u + i * 4u] =
            std::bit_cast<std::uint32_t>(ids[i]);
    Check(pal4::CombatEventListDispatch::Process(
              1000u, 300u, runtime) == 83 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "convert 1.900000", "dispatch 300 1",
                  "read 1024", "convert 0.000000", "dispatch 300 0",
                  "read 1028", "convert -2.900000", "dispatch 300 -2",
                  "read 1032", "convert 3.000000", "dispatch 300 3",
                  "read 1036", "convert 0.000000", "dispatch 300 0",
                  "read 1040", "convert 4.000000", "dispatch 300 4",
                  "read 1044", "convert 0.000000", "dispatch 300 0",
                  "read 1048", "convert 5.000000", "dispatch 300 5",
                  "read 1052", "convert 6.000000", "dispatch 300 6"},
          "all nine IDs are dispatched in order, including zeros");

    Runtime second;
    second.words = runtime.words;
    Check(pal4::CombatEventListDispatch::ProcessSecond(
              1000u, 300u, second) == 83 &&
              second.calls == runtime.calls,
          "second native entry preserves the same nine-ID behavior");
}

}  // namespace

int main() {
    try {
        AllNineIncludingZero();
        std::cout << "combat event list dispatch tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
