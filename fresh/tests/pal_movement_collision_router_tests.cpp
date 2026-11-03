#include "pal4/pal_movement_collision_router.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalMovementCollisionRuntime {
    std::vector<std::string> calls;
    std::uint32_t first_return = 11u;
    std::uint32_t second_return = 22u;
    std::uint32_t third_return = 33u;
    std::uint32_t first_hit = 0u;
    std::uint32_t second_hit = 0u;
    std::uint32_t third_hit = 0u;

    static void Verify(std::uint32_t object, std::uint32_t first,
                       std::uint32_t second, std::uint32_t third,
                       std::uint32_t zero_one, std::uint32_t zero_two) {
        Check(object == 0x1000u && first == 0x2000u &&
                  second == 0x3000u && third == 0x4000u &&
                  zero_one == 0u && zero_two == 0u,
              "collision calls preserve object, vectors and zero flags");
    }
    std::uint32_t CheckFirst(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t& hit, std::uint32_t zero_one,
        std::uint32_t zero_two) override {
        Verify(object, first, second, third, zero_one, zero_two);
        Check(hit == 0u, "first stage sees cleared hit marker");
        calls.emplace_back("first");
        hit = first_hit;
        return first_return;
    }
    std::uint32_t CheckSecond(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t& hit, std::uint32_t zero_one,
        std::uint32_t zero_two) override {
        Verify(object, first, second, third, zero_one, zero_two);
        Check(hit == 0u, "second stage only runs without a hit");
        calls.emplace_back("second");
        hit = second_hit;
        return second_return;
    }
    std::uint32_t CheckThird(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t extra,
        std::uint32_t third, std::uint32_t& hit,
        std::uint32_t zero_one, std::uint32_t zero_two) override {
        Verify(object, first, second, third, zero_one, zero_two);
        Check(hit == 0u && extra == 0x5000u,
              "third stage receives extra argument only after no hit");
        calls.emplace_back("third");
        hit = third_hit;
        return third_return;
    }
};

std::uint32_t Run(std::uint32_t first, std::uint32_t second,
                  std::uint32_t third, Runtime& runtime) {
    return pal4::PalMovementCollisionRouter::CheckStages(
        0x1000u, first, second, third, 0x2000u, 0x3000u,
        0x4000u, 0x5000u, runtime);
}

void TestDisabledAndIndividualStages() {
    Runtime runtime;
    Check(Run(0u, 0u, 0u, runtime) == 0u && runtime.calls.empty(),
          "all disabled stages return zero");
    Check(Run(8u, 0u, 0u, runtime) == 11u &&
              runtime.calls == std::vector<std::string>{"first"},
          "nonzero first enable runs only first stage");
    runtime.calls.clear();
    Check(Run(0u, 4u, 0u, runtime) == 22u &&
              runtime.calls == std::vector<std::string>{"second"},
          "second stage can run without first");
    runtime.calls.clear();
    Check(Run(0u, 0u, 2u, runtime) == 33u &&
              runtime.calls == std::vector<std::string>{"third"},
          "third stage can run alone with extra argument");
}

void TestMarkerControlsLaterStages() {
    Runtime runtime;
    runtime.first_hit = 1u;
    Check(Run(1u, 1u, 1u, runtime) == 11u &&
              runtime.calls == std::vector<std::string>{"first"},
          "first hit suppresses second and third stages");
    runtime.calls.clear();
    runtime.first_hit = 0u;
    runtime.second_hit = 0x80000000u;
    Check(Run(1u, 1u, 1u, runtime) == 22u &&
              runtime.calls ==
                  std::vector<std::string>{"first", "second"},
          "second hit suppresses third stage");
    runtime.calls.clear();
    runtime.second_hit = 0u;
    Check(Run(1u, 1u, 1u, runtime) == 33u &&
              runtime.calls ==
                  std::vector<std::string>{"first", "second", "third"},
          "without hits all three stages run and last return wins");

    runtime.calls.clear();
    runtime.first_return = 0xffffffffu;
    runtime.second_return = 0u;
    Check(Run(1u, 1u, 0u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"first", "second"},
          "nonzero result alone does not set the hit marker");
}

}  // namespace

int main() {
    TestDisabledAndIndividualStages();
    TestMarkerControlsLaterStages();
    pal4::PalMovementCollisionRuntimeStub runtime;
    Check(pal4::PalMovementCollisionRouter::CheckStages(
              0u, 1u, 1u, 1u, 0u, 0u, 0u, 0u, runtime) == 0u,
          "linkable collision stub executes all enabled stages");
}
