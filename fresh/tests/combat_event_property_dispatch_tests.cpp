#include "pal4/combat_event_property_dispatch.h"

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

struct Runtime final : pal4::ICombatEventPropertyDispatchRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::int32_t, std::uint32_t> events;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t LookupEvent(std::int32_t id) override {
        calls.push_back("lookup " + std::to_string(id));
        return events[id];
    }
    std::int32_t ApplyEvent(std::uint32_t event, std::uint32_t target,
                            std::uint32_t scale_bits) override {
        calls.push_back("apply " + std::to_string(event) + " " +
                        std::to_string(target) + " " +
                        std::to_string(scale_bits));
        return static_cast<std::int32_t>(event + 100u);
    }
};

void Prepare(Runtime& runtime) {
    const float ids[9]{1.9f, 0.0f, -2.9f, 3.0f, 0.0f,
                       4.0f, 0.0f, 5.0f, 6.0f};
    for (std::uint32_t i = 0u; i < 9u; ++i)
        runtime.words[1000u + 20u + i * 4u] =
            std::bit_cast<std::uint32_t>(ids[i]);
    runtime.events[1] = 11u;
    runtime.events[-2] = 22u;
    runtime.events[6] = 66u;
}

void AllNineAndFinalResult() {
    Runtime runtime;
    Prepare(runtime);
    Check(pal4::CombatEventPropertyDispatch::Process(
              1000u, 300u, runtime) == 166 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "convert 1.900000", "lookup 1",
                  "apply 11 300 1065353216",
                  "read 1024", "convert 0.000000", "lookup 0",
                  "read 1028", "convert -2.900000", "lookup -2",
                  "apply 22 300 1065353216",
                  "read 1032", "convert 3.000000", "lookup 3",
                  "read 1036", "convert 0.000000", "lookup 0",
                  "read 1040", "convert 4.000000", "lookup 4",
                  "read 1044", "convert 0.000000", "lookup 0",
                  "read 1048", "convert 5.000000", "lookup 5",
                  "read 1052", "convert 6.000000", "lookup 6",
                  "apply 66 300 1065353216"},
          "all nine IDs are queried and hits receive raw 1.0f bits");

    Runtime final_miss;
    Prepare(final_miss);
    final_miss.events[6] = 0u;
    Check(pal4::CombatEventPropertyDispatch::Process(
              1000u, 300u, final_miss) == 0 &&
              final_miss.calls.back() == "lookup 6",
          "final lookup miss overrides an earlier dispatch result");
}

void ClearAllNine() {
    Runtime runtime;
    Prepare(runtime);
    Check(pal4::CombatEventPropertyDispatch::Clear(
              1000u, 300u, runtime) == 166 &&
              runtime.calls.size() == 30u &&
              runtime.calls[3] == "apply 11 300 0" &&
              runtime.calls[10] == "apply 22 300 0" &&
              runtime.calls.back() == "apply 66 300 0",
          "clear scans all nine IDs and applies zero bits to every hit");

    Runtime final_miss;
    Prepare(final_miss);
    final_miss.events[6] = 0u;
    Check(pal4::CombatEventPropertyDispatch::Clear(
              1000u, 300u, final_miss) == 0 &&
              final_miss.calls.back() == "lookup 6",
          "clear returns final lookup miss instead of earlier apply result");
}

}  // namespace

int main() {
    try {
        AllNineAndFinalResult();
        ClearAllNine();
        std::cout << "combat event property dispatch tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
