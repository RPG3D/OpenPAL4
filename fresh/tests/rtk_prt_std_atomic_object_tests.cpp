#include "pal4/rtk_prt_std_atomic_object.h"

#include <array>
#include <bit>
#include <limits>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::uint32_t, 10u> kDerived{
    0x84a7ccu, 0x84a7c0u, 0x84a7b4u, 0x84a7a8u, 0x84a7a0u,
    0x84a794u, 0x84a784u, 0x84a778u, 0x84a760u, 0x84a754u};
constexpr std::array<std::uint32_t, 10u> kBase{
    0x84a1dcu, 0x84a1d0u, 0x84a1a0u, 0x84a194u, 0x84a1c8u,
    0x84a1bcu, 0x84a1acu, 0x84a830u, 0x84a408u, 0x84a178u};

void CheckFacets(const pal4::RtkPrtStdAtomicObjectState& state,
                 const std::array<std::uint32_t, 10u>& expected,
                 const char* message) {
    for (std::uint32_t i = 0u; i != expected.size(); ++i)
        Check(state.words[5u + i] == expected[i], message);
}

struct Runtime final : pal4::IRtkPrtStdAtomicObjectRuntime {
    pal4::RtkPrtStdAtomicObjectState* state = nullptr;
    std::uint32_t count = 7u;
    bool throw_on_read = false;
    std::vector<std::uint32_t> float_reads;
    std::uint32_t count_writes = 0u;
    std::uint32_t resource_reads = 0u;

    std::uint32_t SystemInstance() override { return 0x5000u; }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        if (address == 0x302cu) {
            ++resource_reads;
            return 0x87654321u;
        }
        Check(address == 0x50a4u, "instance-count field +164");
        Check(state != nullptr && state->words[0u] == 0x84a7d8u,
              "count change follows derived vtable installation");
        CheckFacets(*state, kDerived, "derived facets precede count change");
        if (throw_on_read)
            throw std::runtime_error("native count read failed");
        return count;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x50a4u, "instance-count write field");
        count = value;
        ++count_writes;
    }
    float ReadFloat(const std::uint32_t address) override {
        float_reads.push_back(address);
        if (address == 0x3118u) return 1.25f;
        Check(address == 0x3114u, "second duration field +276");
        return 2.5f;
    }
};

void TestLifecycle() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.state = &state;
    Check(&pal4::RtkPrtStdAtomicObject::Construct(state, runtime) == &state &&
              state.words[0u] == 0x84a7d8u && runtime.count == 8u &&
              runtime.count_writes == 1u && state.words[23u] == 0u,
          "constructor installs derived vtable, zeros resource and counts");
    CheckFacets(state, kDerived, "constructor installs ten derived facets");
    for (std::uint32_t i = 1u; i != 5u; ++i)
        Check(state.words[i] == 0u, "base reset clears owner prefix");
    Check(state.words[15u] == 0xaaaaaaaau &&
              state.words[24u] == 0xaaaaaaaau,
          "constructor preserves untouched storage");
    Check(pal4::RtkPrtStdAtomicObject::ClassName() ==
              "RtkPrtStdAtomicObject", "class string from native vtable");

    state.words[23u] = 0x3000u;
    Check(pal4::RtkPrtStdAtomicObject::ResourceProperty(state, runtime) == 0u &&
              runtime.resource_reads == 0u,
          "inactive resource property does not read native storage");
    state.words[1u] = 0x100u;
    Check(pal4::RtkPrtStdAtomicObject::ResourceProperty(state, runtime) == 0u &&
              runtime.resource_reads == 0u,
          "resource gate uses low activity byte");
    state.words[1u] = 0x101u;
    Check(pal4::RtkPrtStdAtomicObject::ResourceProperty(state, runtime) ==
                  0x87654321u && runtime.resource_reads == 1u,
          "active resource property reads resource plus 44");
    Check(pal4::RtkPrtStdAtomicObject::GetDuration(state, runtime) == 3.75 &&
              runtime.float_reads == std::vector<std::uint32_t>{
                  0x3118u, 0x3114u},
          "duration reads +280 then +276 and sums float values");
    pal4::RtkPrtStdAtomicObject::Destroy(state, runtime);
    Check(state.words[0u] == 0x84b240u && runtime.count == 7u &&
              runtime.count_writes == 2u && state.words[23u] == 0x3000u,
          "destructor decrements count and preserves resource word");
    CheckFacets(state, kBase, "destructor restores ten base facet vtables");
}

void TestConstructorUnwind() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.state = &state;
    runtime.throw_on_read = true;
    bool threw = false;
    try {
        (void)pal4::RtkPrtStdAtomicObject::Construct(state, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.words[0u] == 0x84b240u &&
              state.words[23u] == 0u && runtime.count_writes == 0u,
          "constructor rethrows after restoring base on count failure");
    CheckFacets(state, kBase, "constructor unwind restores base facets");
}

void TestUpdate() {
    struct UpdateRuntime final : pal4::IRtkPrtStdAtomicUpdateRuntime {
        std::uint32_t reads = 0u;
        std::uint32_t calls = 0u;
        std::uint32_t ReadWord(const std::uint32_t address) override {
            Check(address == 0x5030u, "system resource +48 reread");
            ++reads;
            return 0x7000u + reads;
        }
        std::uint8_t UpdateState(
            const std::uint32_t resource,
            const double elapsed,
            const std::uint32_t argument,
            const std::uint8_t mode) override {
            ++calls;
            Check(resource == 0x3000u && argument == 0x7000u + calls &&
                      elapsed == static_cast<double>(
                          std::bit_cast<float>(0x3b03126fu)) &&
                      mode == 0u,
                  "own resource, per-iteration argument, float multiplier and mode");
            return static_cast<std::uint8_t>(calls == 500u ? 0x7bu : 0x22u);
        }
    } runtime;
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[4u] = 0x5000u;
    state.words[23u] = 0x3000u;
    state.words[22u] = std::bit_cast<std::uint32_t>(1.0f);
    Check(pal4::RtkPrtStdAtomicObject::Update(state, runtime) == 0u &&
              runtime.calls == 0u, "inactive update skips native service");
    state.words[1u] = 1u;
    for (const float value : {
             0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN()}) {
        state.words[22u] = std::bit_cast<std::uint32_t>(value);
        Check(pal4::RtkPrtStdAtomicObject::Update(state, runtime) == 0u &&
                  runtime.calls == 0u,
              "zero, negative and NaN time skip update loop");
    }
    state.words[22u] = std::bit_cast<std::uint32_t>(1.0f);
    Check(pal4::RtkPrtStdAtomicObject::Update(state, runtime) == 0x7bu &&
              runtime.reads == 500u && runtime.calls == 500u,
          "positive time performs exactly 500 updates and returns last byte");
}

}  // namespace

int main() {
    TestLifecycle();
    TestConstructorUnwind();
    TestUpdate();
}
