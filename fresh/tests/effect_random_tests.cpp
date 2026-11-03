#include "pal4/effect_random.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectRandomRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> vtable_writes;
    std::uint32_t singleton = 0;
    std::uint32_t time = 0x12345678u;
    unsigned time_calls = 0;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        if (address == 0x1000u)
            vtable_writes.push_back(value);
    }
    std::uint32_t Singleton() override { return singleton; }
    void SetSingleton(std::uint32_t pointer) override {
        singleton = pointer;
    }
    std::uint32_t CurrentTime() override {
        ++time_calls;
        return time;
    }
    void FreeObject(std::uint32_t object) override {
        Check(singleton == 0u && words[object] == 0x84b238u,
              "deleting destructor clears singleton before free");
        freed.push_back(object);
    }
};

void TestLifecycle() {
    Runtime runtime;
    Check(pal4::EffectRandom::Construct(0x1000u, 7u, runtime) == 0x1000u &&
              runtime.singleton == 0x1000u &&
              runtime.words[0x1004u] == 7u && runtime.time_calls == 0u &&
              runtime.vtable_writes ==
                  std::vector<std::uint32_t>{0x84b238u, 0x84b234u},
          "explicit seed and base-to-derived construction");
    bool threw = false;
    try {
        (void)pal4::EffectRandom::Construct(0x2000u, 1u, runtime);
    } catch (const std::logic_error&) {
        threw = true;
    }
    Check(threw && runtime.singleton == 0x1000u &&
              runtime.words[0x2000u] == 0x84b238u,
          "duplicate singleton throws after base vtable write");
    Check(pal4::EffectRandom::Destruct(0x1000u, runtime) == 0x1000u &&
              runtime.singleton == 0u &&
              runtime.vtable_writes.back() == 0x84b238u,
          "destruction restores base vtable and clears singleton");
    threw = false;
    try {
        (void)pal4::EffectRandom::Destruct(0x1000u, runtime);
    } catch (const std::logic_error&) {
        threw = true;
    }
    Check(threw && runtime.singleton == 0u, "absent singleton throws");
    (void)pal4::EffectRandom::Construct(0x1000u, 0u, runtime);
    Check(runtime.words[0x1004u] == runtime.time && runtime.time_calls == 1u,
          "zero seed selects current time once");
}

void TestSequence() {
    Runtime runtime;
    (void)pal4::EffectRandom::Construct(0x1000u, 0xffffffffu, runtime);
    auto expected = 1194211693u * 0xffffffffu + 12345u;
    Check(pal4::EffectRandom::NextBounded(0x1000u, 0u, runtime) == 0u &&
              runtime.words[0x1004u] == expected,
          "zero modulus advances 32-bit seed and returns zero");
    expected = 1194211693u * expected + 12345u;
    Check(pal4::EffectRandom::NextBounded(0x1000u, 7u, runtime) ==
              expected % 7u &&
              runtime.words[0x1004u] == expected,
          "bounded value uses unsigned wrapped recurrence");
    expected = 1194211693u * expected + 12345u;
    Check(pal4::EffectRandom::NextUnit(0x1000u, runtime) ==
              static_cast<double>(expected % 65536u) / 65536.0,
          "unit value uses low sixteen bits");
    expected = 1194211693u * expected + 12345u;
    const double unit = static_cast<double>(expected % 65536u) / 65536.0;
    const float minimum = -1.0f;
    const float maximum = 16777216.0f;
    Check(pal4::EffectRandom::NextRange(0x1000u, minimum, maximum,
                                        runtime) ==
              static_cast<double>(minimum) + unit * 16777217.0 &&
              runtime.words[0x1004u] == expected,
          "range result uses x87-style endpoint arithmetic");
    Check(pal4::EffectRandom::NextRange(0x1000u, 5.0f, -3.0f,
                                        runtime) >= -3.0 &&
              pal4::EffectRandom::NextRange(0x1000u, 5.0f, -3.0f,
                                            runtime) <= 5.0,
          "descending range stays within endpoints");
}

void TestDeletingDestructor() {
    Runtime runtime;
    (void)pal4::EffectRandom::Construct(0x1000u, 3u, runtime);
    Check(pal4::EffectRandom::Delete(0x1000u, 2u, runtime) == 0x1000u &&
              runtime.freed.empty() && runtime.singleton == 0u,
          "non-deleting flag only destructs the singleton");
    (void)pal4::EffectRandom::Construct(0x1000u, 3u, runtime);
    Check(pal4::EffectRandom::Delete(0x1000u, 3u, runtime) == 0x1000u &&
              runtime.freed == std::vector<std::uint32_t>{0x1000u},
          "low flag bit frees object after destruction");
    bool threw = false;
    try {
        (void)pal4::EffectRandom::Delete(0x1000u, 1u, runtime);
    } catch (const std::logic_error&) {
        threw = true;
    }
    Check(threw && runtime.freed.size() == 1u,
          "absent singleton throws before freeing storage");
}

}  // namespace

int main() {
    TestLifecycle();
    TestSequence();
    TestDeletingDestructor();
}
