#include "pal4/grid_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IGridManagerVelocityRuntime {
public:
    float ReadSpeed(pal4::GridManagerState&) override {
        ++reads;
        return speed;
    }
    std::uint32_t ApplyDirection(
        pal4::GridManagerState&,
        const pal4::GridVector3& value) override {
        ++applies;
        direction = value;
        return 0xaau;
    }
    float speed = 2.0f;
    std::uint32_t reads = 0u;
    std::uint32_t applies = 0u;
    pal4::GridVector3 direction{};
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::GridManagerState state{};
    Check(pal4::GridManager::SetVelocityDirection(
              state, {2.0f, 4.0f, 6.0f}, runtime) == 0xaau &&
              runtime.reads == 2u && runtime.applies == 1u &&
              runtime.direction == pal4::GridVector3{1.0f, 2.0f, 3.0f},
          "positive speed normalizes input and forwards virtual result");
    runtime.speed = 0.0f;
    runtime.reads = 0u;
    Check(pal4::GridManager::SetVelocityDirection(
              state, {2.0f, 4.0f, 6.0f}, runtime) == 0u &&
              runtime.reads == 1u && runtime.applies == 1u,
          "zero speed skips division and direction dispatch");
    runtime.speed = -1.0f;
    runtime.reads = 0u;
    Check(pal4::GridManager::SetVelocityDirection(
              state, {2.0f, 4.0f, 6.0f}, runtime) == 0u &&
              runtime.reads == 1u,
          "negative speed also skips dispatch");
    std::cout << "Grid manager velocity tests passed\n";
}
