#include "pal4/atomic_bounding_box_builder.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IAtomicBoundsInitRuntime {
    static constexpr std::uint32_t record = 0x1000u;
    static constexpr std::uint32_t clump = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t vertex_result{0x3333u};
    std::uint32_t position_result{0x4444u};

    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
        events.emplace_back("write");
    }
    [[nodiscard]] std::uint32_t ForEachAtomicVertexBounds(
        const std::uint32_t source,
        const std::uint32_t destination) override {
        Check(source == clump && destination == record,
              "first traversal uses transformed-vertex callback on base box");
        Check(writes.size() == 6u,
              "initial bounds are written before first traversal");
        events.emplace_back("vertices");
        return vertex_result;
    }
    [[nodiscard]] std::uint32_t ForEachAtomicPosition(
        const std::uint32_t source,
        const std::uint32_t destination) override {
        Check(source == clump && destination == record + 48u,
              "second traversal uses position callback on embedded sphere");
        events.emplace_back("positions");
        return position_result;
    }
};

}  // namespace

int main() {
    Runtime missing;
    Check(pal4::AtomicBoundingBoxBuilder::InitializeClumpBounds(
              Runtime::record, 0u, missing) == 0u &&
              missing.events.empty(),
          "null clump leaves record alone and has defined zero return");

    Runtime runtime;
    Check(pal4::AtomicBoundingBoxBuilder::InitializeClumpBounds(
              Runtime::record, Runtime::clump, runtime) ==
              runtime.position_result &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {Runtime::record + 8u, 0xff7fffffu},
                      {Runtime::record + 4u, 0xff7fffffu},
                      {Runtime::record, 0xff7fffffu},
                      {Runtime::record + 20u, 0x7f7fffffu},
                      {Runtime::record + 16u, 0x7f7fffffu},
                      {Runtime::record + 12u, 0x7f7fffffu}}) &&
              runtime.events == std::vector<std::string>({
                  "write", "write", "write", "write", "write", "write",
                  "vertices", "positions"}),
          "initializes six extremes and forwards second traversal result");
    pal4::AtomicBoundsInitRuntimeStub stub;
    Check(pal4::AtomicBoundingBoxBuilder::InitializeClumpBounds(
              Runtime::record, Runtime::clump, stub) == 0u,
          "RenderWare traversal stub links and runs");
    return EXIT_SUCCESS;
}
