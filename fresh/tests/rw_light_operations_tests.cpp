#include "pal4/rw_light_operations.h"

#include <array>
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

class Runtime final : public pal4::IRwLightRuntime {
public:
    std::uint32_t CreateLight(std::uint32_t type) override {
        events.push_back("light:" + std::to_string(type));
        return light_result;
    }
    std::uint32_t CreateFrame() override {
        events.push_back("frame");
        return frame_result;
    }
    void RotateFrame(std::uint32_t frame,
                     const std::array<float, 3>& axis,
                     float angle, std::uint32_t mode) override {
        Check(frame == 0x300, "rotation frame");
        const auto index = rotations++;
        Check(axis == std::array<float, 3>{
                  index == 0 ? 1.0f : 0.0f,
                  index == 1 ? 1.0f : 0.0f,
                  index == 2 ? 1.0f : 0.0f} &&
                  angle == static_cast<float>(index + 10) &&
                  mode == (index == 0 ? 0u : 2u),
              "axis, angle and mode order");
        events.push_back("rotate" + std::to_string(index));
    }
    void TranslateFrame(std::uint32_t frame,
                        const std::array<float, 3>& position,
                        std::uint32_t mode) override {
        Check(frame == 0x300 && position ==
                  std::array<float, 3>{1.0f, 2.0f, 3.0f} && mode == 2,
              "translation args");
        events.push_back("translate");
    }
    void SetFrame(std::uint32_t light, std::uint32_t frame) override {
        Check(light == 0x200, "set light frame");
        events.push_back("set_frame:" + std::to_string(frame));
    }
    std::uint32_t SetColor(
        std::uint32_t light, const std::array<float, 4>& color) override {
        Check(light == 0x200 && color ==
                  std::array<float, 4>{0.1f, 0.2f, 0.3f, 0.4f},
              "color forwarding");
        events.push_back("color");
        return 0x201;
    }
    std::uint32_t SetRadius(
        std::uint32_t light, std::uint32_t bits) override {
        Check(light == 0x200 && bits == 0x41200000u,
              "radius bit forwarding");
        events.push_back("radius");
        return 0x202;
    }
    std::uint32_t SetConeAngle(
        std::uint32_t light, float angle) override {
        Check(light == 0x200 && angle == 0.75f, "cone angle forwarding");
        events.push_back("cone");
        return 0x203;
    }
    std::uint32_t WorldOfLight(std::uint32_t light) override {
        Check(light == 0x200, "world lookup light");
        events.push_back("world");
        return world_result;
    }
    void RemoveLight(std::uint32_t world,
                     std::uint32_t light) override {
        Check(world == 0x900 && light == 0x200,
              "remove light args");
        events.push_back("remove");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x204, "frame word offset");
        events.push_back("read_frame");
        return attached_frame;
    }
    void DestroyFrame(std::uint32_t frame) override {
        Check(frame == 0x300, "destroy frame");
        events.push_back("destroy_frame");
    }
    void DestroyLight(std::uint32_t light) override {
        Check(light == 0x200, "destroy light");
        events.push_back("destroy_light");
    }

    std::uint32_t light_result = 0x200;
    std::uint32_t frame_result = 0x300;
    std::uint32_t world_result = 0x900;
    std::uint32_t attached_frame = 0x300;
    unsigned rotations = 0;
    std::vector<std::string> events;
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::RwLightRuntimeStub stub;
    Check(pal4::RwLightOperations::CreateTypeTwo(stub) == 0,
          "empty RenderWare light stub is usable");
    Check(pal4::RwLightOperations::CreateTypeTwo(runtime) == 0x200 &&
              runtime.events == std::vector<std::string>{"light:2"},
          "type-two helper delegates one create call");
    runtime.events.clear();
    Check(pal4::RwLightOperations::CreateWithFrame(
              {1.0f, 2.0f, 3.0f}, {10.0f, 11.0f, 12.0f}, runtime) ==
                  0x200 &&
              runtime.events == std::vector<std::string>{
                  "light:1", "frame", "rotate0", "rotate1", "rotate2",
                  "translate", "set_frame:768"},
          "light and frame creation with exact transform order");
    runtime.events.clear();
    runtime.light_result = 0;
    Check(pal4::RwLightOperations::CreateWithFrame(
              {1.0f, 2.0f, 3.0f}, {10.0f, 11.0f, 12.0f}, runtime) == 0 &&
              runtime.events == std::vector<std::string>{"light:1"},
          "light creation failure does not create frame");
    runtime.light_result = 0x200;
    runtime.frame_result = 0;
    runtime.events.clear();
    Check(pal4::RwLightOperations::CreateWithFrame(
              {1.0f, 2.0f, 3.0f}, {10.0f, 11.0f, 12.0f}, runtime) == 0 &&
              runtime.events == std::vector<std::string>{
                  "light:1", "frame", "destroy_light"},
          "frame creation failure destroys light");

    runtime.events.clear();
    Check(pal4::RwLightOperations::SetColor(
              0, {0.1f, 0.2f, 0.3f, 0.4f}, runtime) == 0 &&
              pal4::RwLightOperations::SetRadius(
                  0, 0x41200000u, runtime) == 0 &&
              pal4::RwLightOperations::SetConeAngle(
                  0, 0.75f, runtime) == 0 &&
              runtime.events.empty(),
          "null light skips all setters");
    Check(pal4::RwLightOperations::SetColor(
              0x200, {0.1f, 0.2f, 0.3f, 0.4f}, runtime) == 0x201 &&
              pal4::RwLightOperations::SetRadius(
                  0x200, 0x41200000u, runtime) == 0x202 &&
              pal4::RwLightOperations::SetConeAngle(
                  0x200, 0.75f, runtime) == 0x203 &&
              runtime.events == std::vector<std::string>{
                  "color", "radius", "cone"},
          "setter arguments and return values forwarded");

    runtime.events.clear();
    pal4::RwLightOperations::Destroy(0, runtime);
    Check(runtime.events.empty(), "null light destruction bypasses services");
    pal4::RwLightOperations::Destroy(0x200, runtime);
    Check(runtime.events == std::vector<std::string>{
              "world", "remove", "read_frame", "set_frame:0",
              "destroy_frame", "destroy_light"},
          "light destruction removes world and frame before light");
    runtime.events.clear();
    runtime.world_result = 0;
    runtime.attached_frame = 0;
    pal4::RwLightOperations::Destroy(0x200, runtime);
    Check(runtime.events == std::vector<std::string>{
              "world", "read_frame", "set_frame:0", "destroy_light"},
          "sparse light skips world and frame destruction");
}
