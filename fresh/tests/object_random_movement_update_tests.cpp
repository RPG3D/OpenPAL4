#include "pal4/object_random_movement_update.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectRandomMovementUpdateRuntime {
    pal4::ObjectRandomMovementState* state = nullptr;
    std::vector<std::string> calls;
    std::uint32_t captured_member = 0;
    std::uint32_t notified_member = 0;

    pal4::BehaviorVector3 ProcessCollision(
        pal4::ObjectRandomMovementState& object, std::uint32_t input,
        float delta) override {
        calls.emplace_back("collision");
        Check(&object == state && input == 0x7777u && delta == 0.25f,
              "collision receives the two stack arguments");
        return {8.0f, 9.0f, 10.0f};
    }
    void UpdateBase(pal4::ObjectRandomMovementState& object,
                    std::uint32_t input, float delta,
                    const pal4::BehaviorVector3& collision_result) override {
        calls.emplace_back("base");
        Check(&object == state && input == 0x7777u && delta == 0.25f &&
                  collision_result ==
                      pal4::BehaviorVector3{8.0f, 9.0f, 10.0f},
              "base update receives collision result and original arguments");
    }
    std::uint32_t MemberVtable(std::uint32_t member) override {
        calls.emplace_back("member_vtable");
        captured_member = member;
        return 0x9999u;
    }
    std::uint32_t PositionToken(
        pal4::ObjectRandomMovementState& object) override {
        calls.emplace_back("position");
        Check(&object == state, "position virtual uses the same object");
        state->words[42] = 0x2222u;
        return 0xaaaau;
    }
    std::uint32_t NotifyMember(std::uint32_t vtable,
                                std::uint32_t member,
                                std::uint32_t position) override {
        calls.emplace_back("notify");
        notified_member = member;
        Check(vtable == 0x9999u && position == 0xaaaau &&
                  (state->words[46] & 0xff00u) == 0xa500u,
              "member callback uses captured vtable before activity write");
        return 0xbbbbu;
    }
};

void TestActive() {
    pal4::ObjectRandomMovementState state;
    state.words[42] = 0x1111u;
    state.words[46] = 0xabcda55au;
    state.words[47] = std::bit_cast<std::uint32_t>(1.0f);
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ObjectRandomMovementUpdate::Update(
              state, 0x7777u, 0.25f, 0x1234u, runtime) == 0xbbbbu &&
              state.words[46] == 0xabcd015au &&
              runtime.captured_member == 0x1111u &&
              runtime.notified_member == 0x2222u,
          "positive duration propagates member result and sets activity byte");
    Check(runtime.calls ==
              std::vector<std::string>{"collision", "base", "member_vtable",
                                       "position", "notify"},
          "active path retains assembly call order");
}

void TestInactiveAndUnordered() {
    pal4::ObjectRandomMovementState state;
    state.words[46] = 0xabcda55au;
    Runtime runtime;
    runtime.state = &state;
    for (float duration : {0.0f, -1.0f}) {
        state.words[47] = std::bit_cast<std::uint32_t>(duration);
        Check(pal4::ObjectRandomMovementUpdate::Update(
                  state, 0x7777u, 0.25f, 0x1234u, runtime) == 0x1234u &&
                  state.words[46] == 0xabcda55au && runtime.calls.empty(),
              "nonpositive duration keeps the activity byte and exits");
    }
    state.words[42] = 0x1111u;
    state.words[47] = std::bit_cast<std::uint32_t>(
        std::numeric_limits<float>::quiet_NaN());
    Check(pal4::ObjectRandomMovementUpdate::Update(
              state, 0x7777u, 0.25f, 0x1234u, runtime) == 0xbbbbu &&
              runtime.calls.size() == 5u,
          "unordered x87 comparison enters the movement branch");
}

}  // namespace

int main() {
    TestActive();
    TestInactiveAndUnordered();
    pal4::ObjectRandomMovementUpdateRuntimeStub stub;
    pal4::ObjectRandomMovementState state;
    state.words[47] = std::bit_cast<std::uint32_t>(1.0f);
    Check(pal4::ObjectRandomMovementUpdate::Update(
              state, 0u, 0.0f, 123u, stub) == 0u,
          "default runtime links");
    std::cout << "2 random movement update cases passed\n";
}
