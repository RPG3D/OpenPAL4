#include "pal4/base_object_callback_data.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectCollisionRuntime {
    std::vector<char> directions;
    bool hit_forward{};
    bool hit_reverse{};

    bool ProcessCollision(const pal4::BaseObjectVector3& first,
                          const pal4::BaseObjectVector3& second,
                          std::uint32_t argument) override {
        Check(argument == 0x9000u,
              "both collision queries forward third argument");
        if (first.x == 1.0f && second.x == 4.0f) {
            directions.push_back('F');
            return hit_forward;
        }
        Check(first.x == 4.0f && second.x == 1.0f,
              "reverse query swaps copied endpoint vectors");
        directions.push_back('R');
        return hit_reverse;
    }
};

void CheckPropertyAndPosition() {
    pal4::BaseObjectState state{};
    Check(pal4::BaseObjectCallbackData::SetProperty1544(
              state, 0x1000u, 0x12345678u) == 0x1000u &&
              state.Read(0x608u) == 0x12345678u,
          "property callback stores at +0x608 and returns target pointer");

    pal4::BaseObjectCallbackRecord record{};
    record.Write(0x608u, 0xaaaau);
    record.Write(0x618u, 0xbbbbu);
    record.Write(0x63cu, 0xccccu);
    const pal4::BaseObjectVector3 first{1.0f, 2.0f, 3.0f};
    const pal4::BaseObjectVector3 second{4.0f, -5.0f, 6.0f};
    Check(pal4::BaseObjectCallbackData::SetPositionAndTransformData(
              record, 0x2000u, first, second) == 0x2000u,
          "position callback returns target pointer");
    Check(record.Read(0x60cu) == std::bit_cast<std::uint32_t>(1.0f) &&
              record.Read(0x610u) ==
                  std::bit_cast<std::uint32_t>(
                      static_cast<float>(
                          2.0 + static_cast<double>(0.30000001192092896f))) &&
              record.Read(0x614u) == std::bit_cast<std::uint32_t>(3.0f) &&
              record.Read(0x624u) == 0xdcf01414u &&
              record.Read(0x630u) == std::bit_cast<std::uint32_t>(4.0f) &&
              record.Read(0x634u) ==
                  std::bit_cast<std::uint32_t>(
                      static_cast<float>(
                          -5.0 + static_cast<double>(0.30000001192092896f))) &&
              record.Read(0x638u) == std::bit_cast<std::uint32_t>(6.0f) &&
              record.Read(0x648u) == 0xdcf01414u,
          "position callback writes both vectors, raised Y and constants");
    Check(record.Read(0x608u) == 0xaaaau &&
              record.Read(0x618u) == 0xbbbbu &&
              record.Read(0x63cu) == 0xccccu,
          "position callback leaves adjacent record fields untouched");
}

void CheckCollisionBranches() {
    const pal4::BaseObjectVector3 first{1.0f, 2.0f, 3.0f};
    const pal4::BaseObjectVector3 second{4.0f, 5.0f, 6.0f};
    Runtime runtime;
    runtime.hit_forward = true;
    Check(!pal4::BaseObjectCallbackData::CheckCollision(
              first, second, 0x9000u, runtime) &&
              runtime.directions == std::vector<char>{'F'},
          "forward collision returns false without reverse query");
    runtime.directions.clear();
    runtime.hit_forward = false;
    runtime.hit_reverse = true;
    Check(!pal4::BaseObjectCallbackData::CheckCollision(
              first, second, 0x9000u, runtime) &&
              runtime.directions == std::vector<char>{'F', 'R'},
          "reverse collision also returns false");
    runtime.directions.clear();
    runtime.hit_reverse = false;
    Check(pal4::BaseObjectCallbackData::CheckCollision(
              first, second, 0x9000u, runtime) &&
              runtime.directions == std::vector<char>{'F', 'R'},
          "two collision misses return true");
}

}  // namespace

int main() {
    CheckPropertyAndPosition();
    CheckCollisionBranches();
}
