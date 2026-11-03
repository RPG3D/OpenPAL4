#include "pal4/spatial_grid.h"

#include <array>
#include <bit>
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
        std::exit(1);
    }
}

class Runtime final : public pal4::ISpatialGridRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("alloc:" + std::to_string(bytes));
        if (bytes == sizeof(pal4::SpatialGridState))
            return fail_header ? 0u : 0x1000u;
        return fail_buckets ? 0u : 0x2000u;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return address == 0x1000u ? 0xaau : 0xbbu;
    }
    pal4::SpatialGridState& GridAt(std::uint32_t address) override {
        Check(address == 0x1000u, "grid lookup uses allocated header");
        return grid;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address >= 0x2000u && address < 0x2060u &&
                  (address & 3u) == 0u && value == 0u,
              "bucket initializer writes aligned zeros");
        writes.push_back(address);
    }

    pal4::SpatialGridState grid{};
    std::vector<std::string> events;
    std::vector<std::uint32_t> writes;
    bool fail_header = false;
    bool fail_buckets = false;
};

pal4::SpatialGridConfig Config() {
    return {{10.0f, 20.0f, 30.0f},
            {8.0f, 6.0f, 4.0f},
            {2u, 3u, 4u}};
}

void Lifecycle() {
    Runtime runtime;
    const auto address = pal4::SpatialGrid::Create(Config(), runtime);
    const auto& grid = runtime.grid;
    Check(address == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "alloc:44", "alloc:96"} &&
              runtime.writes.size() == 24u &&
              runtime.writes.front() == 0x2000u &&
              runtime.writes.back() == 0x205cu &&
              grid.origin_bits == std::array<std::uint32_t, 3u>{
                  0x41200000u, 0x41a00000u, 0x41f00000u} &&
              grid.extent_bits == std::array<std::uint32_t, 3u>{
                  0x41000000u, 0x40c00000u, 0x40800000u} &&
              grid.dimensions ==
                  std::array<std::uint32_t, 3u>{2u, 3u, 4u} &&
              grid.buckets == 0x2000u && grid.overflow_head == 0u,
          "create allocates 44-byte header and zeroes 24 bucket heads");
    runtime.events.clear();
    Check(pal4::SpatialGrid::Destroy(address, runtime) == 0xaau &&
              runtime.events == std::vector<std::string>{
                  "free:8192", "free:4096"},
          "destroy frees buckets before header and forwards header result");
    Runtime failed_header;
    failed_header.fail_header = true;
    Check(pal4::SpatialGrid::Create(Config(), failed_header) == 0u &&
              failed_header.events ==
                  std::vector<std::string>{"alloc:44"},
          "header allocation failure leaves no resources");
    Runtime failed_buckets;
    failed_buckets.fail_buckets = true;
    Check(pal4::SpatialGrid::Create(Config(), failed_buckets) == 0u &&
              failed_buckets.events == std::vector<std::string>{
                  "alloc:44", "alloc:96", "free:4096"} &&
              failed_buckets.writes.empty(),
          "bucket allocation failure frees header without invalid writes");
}

void BucketSelection() {
    Runtime runtime;
    (void)pal4::SpatialGrid::Initialize(runtime.grid, Config(), runtime);
    const auto& grid = runtime.grid;
    const auto find = [&](float x, float y, float z) {
        return pal4::SpatialGrid::BucketAddress(
            grid, 0x1000u, x, y, z);
    };
    Check(find(10.0f, 20.0f, 30.0f) == 0x2000u &&
              find(14.1f, 22.1f, 31.1f) == 0x2044u &&
              find(17.99f, 25.99f, 33.99f) == 0x205cu,
          "3D positions flatten as x-major, then y, then z");
    Check(find(9.99f, 20.0f, 30.0f) == 0x1028u &&
              find(10.0f, 19.99f, 30.0f) == 0x1028u &&
              find(10.0f, 20.0f, 29.99f) == 0x1028u &&
              find(18.0f, 20.0f, 30.0f) == 0x1028u &&
              find(10.0f, 26.0f, 30.0f) == 0x1028u &&
              find(10.0f, 20.0f, 34.0f) == 0x1028u,
          "all six out-of-range faces use the embedded overflow head");
    Check(find(std::numeric_limits<float>::quiet_NaN(),
               20.0f, 30.0f) == 0x1028u,
          "NaN coordinate is safely mapped to overflow");
    auto invalid = grid;
    invalid.extent_bits[0] = std::bit_cast<std::uint32_t>(0.0f);
    Check(pal4::SpatialGrid::BucketAddress(
              invalid, 0x1000u, 10.0f, 20.0f, 30.0f) == 0x1028u,
          "zero extent does not perform undefined integer conversion");
}

}  // namespace

int main() {
    Lifecycle();
    BucketSelection();
    std::cout << "Spatial grid tests passed\n";
}
