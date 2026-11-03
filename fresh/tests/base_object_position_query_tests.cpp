#include "pal4/base_object_position_query.h"

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

struct Runtime final : pal4::IBaseObjectPositionQueryRuntime {
    std::vector<char> events;
    std::uint32_t count{};
    std::uint32_t result{0x1234u};
    std::uint32_t data{0x5678u};

    std::uint32_t MaterialCount(std::uint32_t clump) override {
        Check(clump == 0x2000u, "material count uses object clump");
        events.push_back('M');
        return count;
    }
    std::pair<std::uint32_t, std::uint32_t> ProcessClumpCollision(
        const pal4::BaseObjectVector3& position,
        std::uint32_t clump) override {
        Check(clump == 0x2000u && position.x == 1.0f &&
                  position.y == 2.0f && position.z == 3.0f,
              "collision receives position and same object clump");
        events.push_back('C');
        return {result, data};
    }
};

void CheckQuery() {
    pal4::BaseObjectState state{};
    state.Write(0x14u, 0x2000u);
    const pal4::BaseObjectVector3 position{1.0f, 2.0f, 3.0f};
    Runtime runtime;
    std::uint32_t output = 0xaaaau;
    Check(pal4::BaseObjectPositionQuery::GetPositionAndData(
              state, position, &output, runtime) == 0u &&
              output == 0xaaaau && runtime.events == std::vector<char>{'M'},
          "zero material count returns before collision or output write");

    runtime.events.clear();
    runtime.count = 2u;
    Check(pal4::BaseObjectPositionQuery::GetPositionAndData(
              state, position, &output, runtime) == 0x1234u &&
              output == 0x5678u &&
              runtime.events == std::vector<char>{'M', 'C'},
          "positive count forwards collision result and third output word");

    runtime.events.clear();
    runtime.result = 0u;
    runtime.data = 0x9999u;
    Check(pal4::BaseObjectPositionQuery::GetPositionAndData(
              state, position, &output, runtime) == 0u &&
              output == 0x9999u,
          "optional output is written even when collision result is zero");
    runtime.events.clear();
    Check(pal4::BaseObjectPositionQuery::GetPositionAndData(
              state, position, nullptr, runtime) == 0u &&
              runtime.events == std::vector<char>{'M', 'C'},
          "null optional output still performs collision query");
}

}  // namespace

int main() { CheckQuery(); }
