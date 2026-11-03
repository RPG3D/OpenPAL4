#include "pal4/pal_movement_primary_collision.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalMovementPrimaryCollisionRuntime {
    std::vector<std::string> calls;
    std::uint32_t last_field70 = 0u;
    std::uint32_t last_field6c = 0u;
    std::uint32_t return_value = 0x89abcdefu;
    float source_height = 20.0f;
    float output_height = 20.0f;

    std::uint32_t CheckObjectIntersection(
        std::uint32_t first, float second, float third,
        std::uint32_t fourth, std::uint32_t field70,
        std::uint32_t field6c) override {
        calls.emplace_back("options");
        Check(first == 0x1000u && second == 1.25f &&
                  third == -2.5f && fourth == 0x2000u,
              "options wrapper forwards collision arguments");
        last_field70 = field70;
        last_field6c = field6c;
        return return_value;
    }
    std::uint32_t CheckHeightIntersection(
        std::uint32_t source, float second, float third,
        std::array<float, 3>& output,
        std::uint32_t zero_one, std::uint32_t zero_two) override {
        calls.emplace_back("height");
        Check(source == 0x4000u && second == 1.25f &&
                  third == -2.5f && zero_one == 0u &&
                  zero_two == 0u,
              "height wrapper forwards source, floats and two zeros");
        output = {3.0f, output_height, 5.0f};
        return return_value;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("source_y");
        Check(address == 0x4004u,
              "height validation reads source at plus four");
        return source_height;
    }
};

void TestOptionalFieldAddresses() {
    for (const auto use70 : {0u, 9u}) {
        for (const auto use6c : {0u, 8u}) {
            Runtime runtime;
            Check(pal4::PalMovementPrimaryChild::CheckIntersectionOptions(
                      0x3000u, use70, use6c, 0x1000u,
                      1.25f, -2.5f, 0x2000u, runtime) ==
                      0x89abcdefu &&
                      runtime.last_field70 ==
                          (use70 ? 0x3070u : 0u) &&
                      runtime.last_field6c ==
                          (use6c ? 0x306cu : 0u) &&
                      runtime.calls == std::vector<std::string>{"options"},
                  "both optional output addresses follow independent flags");
        }
    }
}

void TestHeightThreshold() {
    Runtime runtime;
    runtime.output_height = -100.0f;
    Check(pal4::PalMovementPrimaryChild::ValidateIntersectionHeight(
              0x4000u, 1.25f, -2.5f, 0u, runtime) ==
              0x89abcdefu &&
              runtime.calls == std::vector<std::string>{"height"},
          "disabled height check returns raw result without source read");

    for (const auto output_y : {10.0f, 15.0f, 29.0f, 30.0f}) {
        runtime.calls.clear();
        runtime.output_height = output_y;
        Check(pal4::PalMovementPrimaryChild::ValidateIntersectionHeight(
                  0x4000u, 1.25f, -2.5f, 1u, runtime) ==
                  0x89abcdefu &&
                  runtime.calls ==
                      std::vector<std::string>{"height", "source_y"},
              "distance at or below ten preserves raw collision result");
    }
    runtime.calls.clear();
    runtime.output_height = 9.0f;
    Check(pal4::PalMovementPrimaryChild::ValidateIntersectionHeight(
              0x4000u, 1.25f, -2.5f, 1u, runtime) == 0u,
          "distance above ten rejects collision result");

    runtime.output_height = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::PalMovementPrimaryChild::ValidateIntersectionHeight(
              0x4000u, 1.25f, -2.5f, 1u, runtime) ==
              0x89abcdefu,
          "unordered height comparison preserves raw result");
    runtime.output_height = 20.0f;
    runtime.source_height = std::numeric_limits<float>::infinity();
    Check(pal4::PalMovementPrimaryChild::ValidateIntersectionHeight(
              0x4000u, 1.25f, -2.5f, 1u, runtime) == 0u,
          "infinite height difference exceeds the threshold");
    runtime.return_value = 0u;
    runtime.source_height = 20.0f;
    runtime.calls.clear();
    Check(pal4::PalMovementPrimaryChild::ValidateIntersectionHeight(
              0x4000u, 1.25f, -2.5f, 1u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"height", "source_y"},
          "enabled validation reads height even after a zero collision result");
}

}  // namespace

int main() {
    TestOptionalFieldAddresses();
    TestHeightThreshold();
    pal4::PalMovementPrimaryCollisionRuntimeStub runtime;
    Check(pal4::PalMovementPrimaryChild::CheckIntersectionOptions(
              0u, 0u, 0u, 0u, 0.0f, 0.0f, 0u, runtime) == 0u &&
              pal4::PalMovementPrimaryChild::ValidateIntersectionHeight(
                  0u, 0.0f, 0.0f, 0u, runtime) == 0u,
          "linkable collision stubs return zero");
}
