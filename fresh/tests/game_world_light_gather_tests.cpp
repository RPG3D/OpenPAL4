#include "pal4/game_world_light_gather.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldLightGatherRuntime {
public:
    std::uint32_t ForAllWorldSectors(
        std::uint32_t world, pal4::WorldSectorCallback callback,
        std::uint32_t user_data) override {
        Check(world == 0x9000u &&
                  callback == pal4::WorldSectorCallback::GatherLight &&
                  user_data == 0u,
              "gather passes world, light callback and null user data");
        ++calls;
        return result;
    }
    void ForAllSectorMeshes(std::uint32_t sector,
                            pal4::WorldMeshCallback callback,
                            std::uint32_t user_data) override {
        Check(sector == 0xa000u &&
                  callback == pal4::WorldMeshCallback::SetTriStripMethod &&
                  user_data == 0x77u,
              "sector callback passes mesh operation and user data");
        ++mesh_calls;
    }

    unsigned calls = 0;
    unsigned mesh_calls = 0;
    std::uint32_t result = 0x1234u;
};

}  // namespace

int main() {
    pal4::GameWorldObjectState state;
    Runtime runtime;
    Check(pal4::GameWorldLightGather::Gather(state, runtime) == 0u &&
              runtime.calls == 0u,
          "missing world bypasses traversal and returns zero");
    state.words[4] = 0x9000u;
    Check(pal4::GameWorldLightGather::Gather(state, runtime) == 0x1234u &&
              runtime.calls == 1u,
          "world traversal result is forwarded");
    runtime.result = 0u;
    Check(pal4::GameWorldLightGather::Gather(state, runtime) == 0u &&
              runtime.calls == 2u,
          "zero traversal result is forwarded");
    Check(pal4::GameWorldLightGather::GatherSectorMeshes(
              0xa000u, 0x77u, runtime) == 0xa000u &&
              runtime.mesh_calls == 1u,
          "sector callback returns original sector after mesh traversal");
}
