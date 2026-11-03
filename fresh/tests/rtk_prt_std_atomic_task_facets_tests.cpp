#include "pal4/rtk_prt_std_atomic_task_facets.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Write final {
    std::uint32_t address;
    std::uint32_t value;
    bool byte;
    bool operator==(const Write&) const = default;
};

struct Runtime final : pal4::IRtkPrtStdAtomicTaskFacetRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> reads;
    std::vector<Write> writes;
    std::uint32_t frames = 0u;
    std::uint32_t conversions = 0u;
    std::uint8_t frame_result = 0x57u;
    double conversion_input = 0.0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        reads.push_back(address);
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back({address, value, false});
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        writes.push_back({address, value, true});
        bytes[address] = value;
    }
    std::uint8_t UpdateFrame(std::uint32_t resource) override {
        Check(resource == 0x3000u, "frame resource");
        ++frames;
        return frame_result;
    }
    std::int32_t FloatToInt(double value) override {
        conversion_input = value;
        ++conversions;
        return static_cast<std::int32_t>(value);
    }
};

void TestPositionAndFlag() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[23u] = 0x3000u;
    Runtime runtime;
    std::array<std::uint32_t, 3> output{7u, 8u, 9u};
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetPositionWords(
              state, {1u, 2u, 3u}, runtime) == 0u &&
              pal4::RtkPrtStdAtomicTaskFacets::DisableResourceFlag(
                  state, runtime) == 0u &&
              pal4::RtkPrtStdAtomicTaskFacets::GetPositionWords(
                  state, output, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{7u, 8u, 9u} &&
              runtime.writes.empty(),
          "inactive task facets leave resource and outputs intact");
    state.words[1u] = 1u;
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetPositionWords(
              state, {0x11u, 0x22u, 0x33u}, runtime) == 0x57u &&
              runtime.frames == 1u &&
              runtime.writes == std::vector<Write>{
                  {0x3070u, 0x11u, false}, {0x3074u, 0x22u, false},
                  {0x3078u, 0x33u, false}},
          "track facet writes raw position words and updates frame");
    Check(pal4::RtkPrtStdAtomicTaskFacets::GetPositionWords(
              state, output, runtime) == 0x33u &&
              output == std::array<std::uint32_t, 3>{0x11u, 0x22u, 0x33u},
          "aim facet reads resource position and returns last low byte");
    runtime.bytes[0x3001u] = 0u;
    const auto writes_before = runtime.writes.size();
    Check(pal4::RtkPrtStdAtomicTaskFacets::DisableResourceFlag(
              state, runtime) == 0u &&
              runtime.writes.size() == writes_before,
          "zero resource flag remains unchanged");
    runtime.bytes[0x3001u] = 1u;
    Check(pal4::RtkPrtStdAtomicTaskFacets::DisableResourceFlag(
              state, runtime) == 0u &&
              runtime.writes.back() == Write{0x3001u, 0u, true},
          "nonzero resource flag plus one is cleared");
    const std::array<std::uint32_t, 3> target{
        0x11111111u, 0x12345678u, 0x33333333u};
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetTargetWords(
              state, target) == 0x78u &&
              state.words[15u] == target[0] &&
              state.words[16u] == target[1] &&
              state.words[17u] == target[2],
          "aim facet caches target at owner plus 60 and returns second byte");
}

void TestEmissionTime() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[23u] = 0x3000u;
    Runtime runtime;
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetEmissionTime(
              state, 2.0f, 1u, 0x12345678u, runtime) == 0x12345600u &&
              runtime.reads.empty(),
          "inactive timer clears entry EAX low byte");
    state.words[1u] = 1u;
    runtime.words[0x30c8u] = 100u;
    runtime.words[0x30d0u] = 30u;
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetEmissionTime(
              state, 2.0f, 0x101u, 0u, runtime) == 35u &&
              runtime.reads == std::vector<std::uint32_t>{0x30d0u, 0x30c8u} &&
              runtime.conversion_input == 35.0 &&
              runtime.writes == std::vector<Write>{
                  {0x3005u, 0u, true}, {0x3008u, 0u, false},
                  {0x3010u, 1u, true}, {0x3018u, 30u, false},
                  {0x3014u, 35u, false}, {0x3020u, 0u, false},
                  {0x3024u, 0u, false}},
          "remaining-emission mode writes native counters and converts quotient");
    runtime.reads.clear();
    runtime.writes.clear();
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetEmissionTime(
              state, 4.0f, 0x100u, 0u, runtime) == 25u &&
              runtime.reads == std::vector<std::uint32_t>{0x30c8u} &&
              runtime.conversion_input == 25.0 &&
              runtime.writes == std::vector<Write>{
                  {0x3010u, 0u, true}, {0x3014u, 0u, false},
                  {0x3005u, 1u, true}, {0x300cu, 100u, false},
                  {0x3008u, 25u, false}} && runtime.conversions == 2u,
          "full-emission mode writes alternate counters and uses mode low byte");
}

void TestPositionSetter() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[23u] = 0x3000u;
    Runtime runtime;
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetPosition(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0u &&
              runtime.writes.empty(),
          "inactive aim position setter preserves resource");
    state.words[1u] = 1u;
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetPosition(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0x57u &&
              runtime.writes == std::vector<Write>{
                  {0x3070u, std::bit_cast<std::uint32_t>(1.0f), false},
                  {0x3074u, std::bit_cast<std::uint32_t>(2.0f), false},
                  {0x3078u, std::bit_cast<std::uint32_t>(3.0f), false}} &&
              runtime.frames == 1u,
          "absolute aim position writes float triple and updates frame");
    runtime.writes.clear();
    runtime.reads.clear();
    Check(pal4::RtkPrtStdAtomicTaskFacets::SetPosition(
              state, {4.0f, -1.0f, 0.5f}, true, runtime) == 0x57u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x3070u, 0x3074u, 0x3078u} &&
              std::bit_cast<float>(runtime.words[0x3070u]) == 5.0f &&
              std::bit_cast<float>(runtime.words[0x3074u]) == 1.0f &&
              std::bit_cast<float>(runtime.words[0x3078u]) == 3.5f &&
              runtime.frames == 2u,
          "relative aim position adds components before frame update");
}

}  // namespace

int main() {
    TestPositionAndFlag();
    TestEmissionTime();
    TestPositionSetter();
}
