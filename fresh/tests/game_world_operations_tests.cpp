#include "pal4/game_world_operations.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldOperationsRuntime {
public:
    void DestroyLight(std::uint32_t light) override {
        calls.push_back("destroy_light");
        destroyed_light = light;
    }
    std::uint32_t CreateLight() override {
        calls.push_back("create_light");
        return created_light;
    }
    void SetLightColor(
        std::uint32_t light,
        const std::array<std::uint32_t, 4>& rgba_bits) override {
        Check(light == created_light, "color receives newly created light");
        calls.push_back("color");
        color = rgba_bits;
    }
    void SetLightRadius(std::uint32_t light,
                        std::uint32_t radius_bits) override {
        Check(light == created_light &&
                  radius_bits == pal4::GameWorldOperations::kLightRadiusBits,
              "light radius uses native float bits");
        calls.push_back("radius");
    }
    void SetLightConeAngle(std::uint32_t light,
                           std::uint32_t angle_bits) override {
        Check(light == created_light &&
                  angle_bits == pal4::GameWorldOperations::kLightConeAngleBits,
              "cone angle uses native float bits");
        calls.push_back("cone");
    }
    void AddLight(std::uint32_t world, std::uint32_t light) override {
        Check(world == 100 && light == created_light,
              "new light joins active world");
        calls.push_back("add_light");
    }
    void RemoveCamera(std::uint32_t world,
                      std::uint32_t camera) override {
        Check(world == 100 && camera == 200,
              "old camera detaches from world");
        calls.push_back("remove_camera");
    }
    void AddCamera(std::uint32_t world,
                   std::uint32_t camera) override {
        added_camera_world = world;
        added_camera = camera;
        calls.push_back("add_camera");
    }
    std::uint32_t GetClumpWorld(std::uint32_t clump) override {
        Check(clump == 300, "owner query receives requested clump");
        calls.push_back("get_owner");
        return clump_owner;
    }
    void AddClump(std::uint32_t world,
                  std::uint32_t clump) override {
        Check(world == 100 && clump == 300,
              "clump joins active world");
        calls.push_back("add_clump");
    }
    void RemoveClump(std::uint32_t world,
                     std::uint32_t clump) override {
        Check(world == 100 && clump == 300,
              "clump detaches from its active world");
        calls.push_back("remove_clump");
    }

    std::vector<std::string> calls;
    std::uint32_t created_light = 600;
    std::uint32_t destroyed_light = 0xffffffffu;
    std::uint32_t added_camera_world = 0xffffffffu;
    std::uint32_t added_camera = 0xffffffffu;
    std::uint32_t clump_owner = 0;
    std::array<std::uint32_t, 4> color{};
};

void LightAndCamera() {
    pal4::GameWorldObjectState state;
    state.words[4] = 100;
    state.words[5] = 500;
    state.words[11] = 200;
    const std::array<std::uint32_t, 4> color{
        0x3f800000u, 0x3f000000u, 0x3e800000u, 0x3f800000u};
    Runtime runtime;
    Check(!pal4::GameWorldOperations::AddLight(state, color, runtime) &&
              !pal4::GameWorldOperations::SetCamera(state, 300, runtime) &&
              runtime.calls.empty(), "unready world bypasses light and camera");
    state.words[12] = 1;
    Check(pal4::GameWorldOperations::AddLight(state, color, runtime) &&
              state.words[5] == 600 && runtime.destroyed_light == 500 &&
              runtime.color == color &&
              runtime.calls == std::vector<std::string>{
                  "destroy_light", "create_light", "color", "radius", "cone",
                  "add_light"},
          "light replacement follows RenderWare call order");
    runtime.calls.clear();
    runtime.created_light = 0;
    Check(!pal4::GameWorldOperations::AddLight(state, color, runtime) &&
              state.words[5] == 0 &&
              runtime.calls == std::vector<std::string>{
                  "destroy_light", "create_light"},
          "creation failure keeps the old light destroyed");
    runtime.calls.clear();
    Check(pal4::GameWorldOperations::SetCamera(state, 300, runtime) &&
              state.words[11] == 300 &&
              runtime.calls == std::vector<std::string>{
                  "remove_camera", "add_camera"} &&
              runtime.added_camera_world == 100 && runtime.added_camera == 300,
          "camera switch removes the old camera before adding new one");
    runtime.calls.clear();
    state.words[4] = 0;
    Check(pal4::GameWorldOperations::SetCamera(state, 0, runtime) &&
              state.words[11] == 0 &&
              runtime.calls == std::vector<std::string>{"add_camera"} &&
              runtime.added_camera_world == 0,
          "native camera setter still calls add with a zero world pointer");
}

void ClumpMembership() {
    pal4::GameWorldObjectState state;
    state.words[4] = 100;
    Runtime runtime;
    Check(!pal4::GameWorldOperations::AddClump(state, 300, runtime) &&
              !pal4::GameWorldOperations::RemoveClump(state, 300, runtime) &&
              runtime.calls.empty(), "unready world bypasses clump operations");
    state.words[12] = 1;
    Check(!pal4::GameWorldOperations::AddClump(state, 0, runtime) &&
              !pal4::GameWorldOperations::RemoveClump(state, 0, runtime) &&
              runtime.calls.empty(), "null clump bypasses ownership query");
    runtime.clump_owner = 100;
    Check(!pal4::GameWorldOperations::AddClump(state, 300, runtime) &&
              pal4::GameWorldOperations::RemoveClump(state, 300, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "get_owner", "get_owner", "remove_clump"},
          "same-world clump is removable but not added twice");
    runtime.calls.clear();
    runtime.clump_owner = 0;
    Check(pal4::GameWorldOperations::AddClump(state, 300, runtime) &&
              !pal4::GameWorldOperations::RemoveClump(state, 300, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "get_owner", "add_clump", "get_owner"},
          "unowned clump may be added but not removed");
    runtime.calls.clear();
    runtime.clump_owner = 200;
    Check(pal4::GameWorldOperations::AddClump(state, 300, runtime) &&
              !pal4::GameWorldOperations::RemoveClump(state, 300, runtime),
          "foreign-owned clump follows native add and remove conditions");
}

}  // namespace

int main() {
    LightAndCamera();
    ClumpMembership();
}
