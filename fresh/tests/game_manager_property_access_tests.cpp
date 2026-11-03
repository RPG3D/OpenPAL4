#include "pal4/game_manager_property_access.h"

#include <bit>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::GameManagerPropertyRuntimeStub {
    std::uint32_t getter_slot{};
    std::uint32_t setter_slot{};
    std::int32_t expected_index{-1};
    bool got{};
    bool set{};
    float float_value{};
    std::uint32_t integer_value{};

    double GetFloat(std::uint32_t vtable, std::uint32_t slot,
                    pal4::GameManagerPropertyState& state,
                    std::int32_t index) override {
        Check(vtable == 0x1234u && slot == getter_slot &&
                  index == expected_index, "float getter call");
        got = true;
        state.words[0] = 0x5678u;
        return 0.1;
    }
    std::uint32_t SetFloat(std::uint32_t vtable, std::uint32_t slot,
                           pal4::GameManagerPropertyState& state,
                           std::int32_t index, float value) override {
        Check(got && vtable == 0x1234u && state.words[0] == 0x5678u &&
                  slot == setter_slot && index == expected_index,
              "float setter reuses the cached vtable");
        set = true;
        float_value = value;
        return 0xfeedu;
    }
    std::uint32_t GetInteger(std::uint32_t vtable, std::uint32_t slot,
                             pal4::GameManagerPropertyState& state,
                             std::int32_t index) override {
        Check(vtable == 0x1234u && slot == 0x58u &&
                  index == expected_index, "integer getter call");
        got = true;
        state.words[0] = 0x5678u;
        return 0xfffffffeu;
    }
    std::uint32_t SetInteger(std::uint32_t vtable, std::uint32_t slot,
                             pal4::GameManagerPropertyState& state,
                             std::int32_t index,
                             std::uint32_t value) override {
        Check(got && vtable == 0x1234u && state.words[0] == 0x5678u &&
                  slot == 0x5cu && index == expected_index,
              "integer setter reuses the cached vtable");
        set = true;
        integer_value = value;
        return 0xbeefu;
    }
};

using State = pal4::GameManagerPropertyState;
using Access = pal4::GameManagerPropertyAccess;

void TestFloatDirectAccess() {
    using Getter = double (*)(const State&, std::int32_t);
    using Setter = double (*)(State&, std::int32_t, float);
    struct Group { Getter get; Setter set; std::uint32_t offset; int count; };
    constexpr Group groups[] = {
        {Access::GetFloatArrayElement, Access::SetFloatArrayElement, 0x178u, 4},
        {Access::GetFloatGroup1, Access::SetFloatGroup1, 0x1b8u, 6},
        {Access::GetFloatGroup2, Access::SetFloatGroup2, 0x1a0u, 6},
        {Access::GetFloatGroup4, Access::SetFloatGroup4, 0x248u, 5},
        {Access::GetFloatGroup5, Access::SetFloatGroup5, 0x25cu, 7},
    };
    for (const auto& group : groups) {
        State state{};
        state.words.fill(0xa5a5a5a5u);
        for (int index = 0; index < group.count; ++index) {
            const float value = static_cast<float>(index) + 0.25f;
            Check(group.set(state, index, value) == static_cast<double>(value),
                  "valid float setter returns its input");
            Check(group.get(state, index) == static_cast<double>(value) &&
                      state.words[group.offset / 4u + index] ==
                          std::bit_cast<std::uint32_t>(value),
                  "valid float access uses the observed offset");
        }
        const auto before = state.words;
        Check(group.get(state, -1) == -1.0 &&
                  group.get(state, group.count) == group.count &&
                  group.set(state, -9, 3.0f) == -9.0 &&
                  group.set(state, group.count, 3.0f) == group.count &&
                  state.words == before,
              "invalid signed indexes return as doubles without writes");
    }
}

void TestFloatAdjustments() {
    using Adjuster = std::uint32_t (*)(State&, std::int32_t, float,
                                        pal4::IGameManagerPropertyRuntime&);
    struct Group { Adjuster adjust; std::uint32_t getter; std::uint32_t setter; };
    constexpr Group groups[] = {
        {Access::AdjustFloatArrayElement, 0x2cu, 0x3cu},
        {Access::AdjustFloatGroup1, 0x70u, 0x74u},
        {Access::AdjustFloatGroup2, 0x64u, 0x68u},
        {Access::AdjustFloatGroup4, 0x28u, 0x38u},
        {Access::AdjustFloatGroup5, 0x7cu, 0x80u},
    };
    for (const auto& group : groups) {
        State state{};
        state.words[0] = 0x1234u;
        Runtime runtime{};
        runtime.getter_slot = group.getter;
        runtime.setter_slot = group.setter;
        runtime.expected_index = 2;
        Check(group.adjust(state, 2, 0.2f, runtime) == 0xfeedu &&
                  runtime.got && runtime.set &&
                  runtime.float_value == static_cast<float>(0.1 +
                      static_cast<double>(0.2f)),
              "float adjustment rounds once to float and forwards result");
    }
}

void TestIntegerAccessAndAdjustment() {
    State state{};
    state.words.fill(0xa5a5a5a5u);
    for (int index = 0; index < 6; ++index) {
        const auto value = 100u + static_cast<std::uint32_t>(index);
        Check(Access::SetIntegerGroup3(state, index, value) == value &&
                  Access::GetIntegerGroup3(state, index) == value &&
                  state.words[0x188u / 4u + index] == value,
              "integer property uses all six observed slots");
    }
    const auto before = state.words;
    Check(Access::GetIntegerGroup3(state, -1) == 0xffffffffu &&
              Access::GetIntegerGroup3(state, 6) == 6u &&
              Access::SetIntegerGroup3(state, -9, 3u) == 0xfffffff7u &&
              Access::SetIntegerGroup3(state, 6, 3u) == 6u &&
              state.words == before,
          "integer invalid indexes preserve signed bit patterns");

    state.words[0] = 0x1234u;
    Runtime runtime{};
    runtime.expected_index = 3;
    Check(Access::AdjustIntegerGroup3(state, 3, 5u, runtime) == 0xbeefu &&
              runtime.got && runtime.set && runtime.integer_value == 3u,
          "integer adjustment wraps at 32 bits and forwards setter result");
}

void TestFixedBoundedProperties() {
    using Getter = std::uint32_t (*)(const State&);
    using Setter = std::uint32_t (*)(State&, std::int32_t);
    struct Field { Getter get; Setter set; std::uint32_t word; std::int32_t limit; };
    constexpr Field fields[] = {
        {Access::Get158, Access::Set158, 158u, 6},
        {Access::Get159, Access::Set159, 159u, 7},
        {Access::Get160, Access::Set160, 160u, 7},
        {Access::Get161, Access::Set161, 161u, 2},
        {Access::Get162, Access::Set162, 162u, 4},
        {Access::Get163, Access::Set163, 163u, 2},
        {Access::Get164, Access::Set164, 164u, 5},
        {Access::Get165, Access::Set165, 165u, 3},
        {Access::Get166, Access::Set166, 166u, 6},
    };
    for (const auto& field : fields) {
        State state{};
        state.words.fill(0xa5a5a5a5u);
        Check(field.get(state) == 0xa5a5a5a5u,
              "getter reads its fixed offset");
        for (int value = 0; value < field.limit; ++value)
            Check(field.set(state, value) == static_cast<std::uint32_t>(value) &&
                      field.get(state) == static_cast<std::uint32_t>(value),
                  "setter accepts every in-range value");
        const auto before = state.words;
        Check(field.set(state, -1) == 0xffffffffu &&
                  field.set(state, field.limit) ==
                      static_cast<std::uint32_t>(field.limit) &&
                  state.words == before &&
                  state.words[field.word] ==
                      static_cast<std::uint32_t>(field.limit - 1),
              "setter returns invalid values without changing any word");
    }
}

void TestDirectFloatAdjustments() {
    using Getter = double (*)(const State&, std::int32_t);
    using Setter = double (*)(State&, std::int32_t, float);
    using Adjuster = double (*)(State&, std::int32_t, float);
    struct Group { Getter get; Setter set; Adjuster adjust;
                   std::uint32_t offset; int count; };
    constexpr Group groups[] = {
        {Access::GetFloatGroup6, Access::SetFloatGroup6,
         Access::AdjustFloatGroup6, 0x1e4u, 6},
        {Access::GetFloatGroup7, Access::SetFloatGroup7,
         Access::AdjustFloatGroup7, 0x1fcu, 7},
        {Access::GetFloatGroup8, Access::SetFloatGroup8,
         Access::AdjustFloatGroup8, 0x218u, 2},
    };
    constexpr float initial = 0.1f;
    constexpr float delta = 0.2f;
    const double unrounded = static_cast<double>(initial) +
                             static_cast<double>(delta);
    for (const auto& group : groups) {
        State state{};
        state.words.fill(0xa5a5a5a5u);
        for (int index = 0; index < group.count; ++index) {
            Check(group.set(state, index, initial) ==
                      static_cast<double>(initial) &&
                      group.get(state, index) == static_cast<double>(initial),
                  "direct float group reads and writes each observed slot");
            Check(group.adjust(state, index, delta) == unrounded &&
                      group.get(state, index) ==
                          static_cast<double>(static_cast<float>(unrounded)) &&
                      state.words[group.offset / 4u + index] ==
                          std::bit_cast<std::uint32_t>(
                              static_cast<float>(unrounded)),
                  "direct adjustment returns unrounded sum but stores float");
        }
        const auto before = state.words;
        Check(group.get(state, -1) == -1.0 &&
                  group.get(state, group.count) == group.count &&
                  group.set(state, -1, 9.0f) == -1.0 &&
                  group.set(state, group.count, 9.0f) == group.count &&
                  group.adjust(state, -1, delta) == -1.0 &&
                  group.adjust(state, group.count, delta) == group.count &&
                  state.words == before,
              "invalid direct float indexes leave the object unchanged");
    }
}

}  // namespace

int main() {
    TestFloatDirectAccess();
    TestFloatAdjustments();
    TestIntegerAccessAndAdjustment();
    TestFixedBoundedProperties();
    TestDirectFloatAdjustments();
}
