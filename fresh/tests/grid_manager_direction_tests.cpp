#include "pal4/grid_manager.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IGridManagerDirectionRuntime {
public:
    float ReadParameter(const pal4::GridManagerState&,
                        const std::uint32_t slot) override {
        reads.push_back(slot);
        return slot == 120u ? limit_source : speed;
    }
    pal4::GridVector3 ReadNormal(
        const pal4::GridManagerState&) override {
        ++normal_reads;
        return {0.0f, 1.0f, 0.0f};
    }
    pal4::GridVector3 ReadZeroVector() override {
        return {0.0f, 0.0f, 0.0f};
    }
    float limit_source = 20.0f;
    float speed = 2.0f;
    std::vector<std::uint32_t> reads;
    std::uint32_t normal_reads = 0u;
};

}  // namespace

int main() {
    pal4::GridManagerState manager{};
    pal4::GridVector3 output{};
    const pal4::GridVector3 velocity{1.0f, 0.0f, 0.0f};
    Runtime runtime;
    Check(&pal4::GridManager::ConstrainVelocity(
              manager, output, velocity, runtime) == &output &&
              runtime.reads ==
                  std::vector<std::uint32_t>{120u, 100u, 100u} &&
              runtime.normal_reads == 1u &&
              output[0] > 0.0f && output[0] < 0.01f &&
              output[1] > 0.99f && output[1] < 1.01f &&
              std::abs(std::sqrt(output[0] * output[0] +
                                 output[1] * output[1]) - 1.0f) < 0.00001f,
          "slow nonzero velocity uses power threshold and normal");
    runtime.reads.clear();
    runtime.speed = 5.0f;
    Check(pal4::GridManager::ConstrainVelocity(
              manager, output, velocity, runtime) == velocity &&
              runtime.reads ==
                  std::vector<std::uint32_t>{120u, 100u} &&
              runtime.normal_reads == 1u,
          "speed above one-fifth limit passes through");
    runtime.reads.clear();
    runtime.speed = 2.0f;
    const pal4::GridVector3 zero{};
    Check(pal4::GridManager::ConstrainVelocity(
              manager, output, zero, runtime) == zero &&
              runtime.reads ==
                  std::vector<std::uint32_t>{120u, 100u} &&
              runtime.normal_reads == 1u,
          "zero velocity passes through without normal lookup");
    std::cout << "Grid manager direction tests passed\n";
}
