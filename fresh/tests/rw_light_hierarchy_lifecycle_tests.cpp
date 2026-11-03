#include "pal4/rw_light_hierarchy_lifecycle.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IRwLightHierarchyRuntime {
public:
    class LightsRuntime final : public pal4::IRwLightRuntime {
    public:
        explicit LightsRuntime(std::vector<std::string>& calls)
            : calls_(calls) {}
        std::uint32_t primary = 0x100u;
        std::uint32_t secondary = 0x200u;
        std::uint32_t frame = 0x300u;
        std::map<std::uint32_t, std::uint32_t> frames;
        std::size_t rotation_count = 0;

        std::uint32_t CreateLight(std::uint32_t type) override {
            calls_.push_back(type == 2 ? "create-primary" : "create-secondary");
            Check(type == 1 || type == 2, "known light type");
            return type == 2 ? primary : secondary;
        }
        std::uint32_t CreateFrame() override {
            calls_.push_back("create-frame");
            return frame;
        }
        void RotateFrame(std::uint32_t object,
                         const std::array<float, 3>& axis,
                         float angle, std::uint32_t mode) override {
            calls_.push_back("rotate");
            constexpr std::array<float, 3> angles{-60.0f, 20.0f, 0.0f};
            Check(object == frame && rotation_count < 3u &&
                      angle == angles[rotation_count] &&
                      axis[rotation_count] == 1.0f &&
                      mode == (rotation_count == 0u ? 0u : 2u),
                  "secondary light uses observed axes and angles");
            ++rotation_count;
        }
        void TranslateFrame(std::uint32_t object,
                            const std::array<float, 3>& position,
                            std::uint32_t mode) override {
            calls_.push_back("translate");
            Check(object == frame && mode == 2u &&
                      position == std::array<float, 3>{0, 0, 0},
                  "secondary frame uses origin translation");
        }
        void SetFrame(std::uint32_t light,
                      std::uint32_t value) override {
            calls_.push_back(value == 0 ? "clear-frame" : "set-frame");
            if (value != 0)
                Check(light == secondary && value == frame,
                      "secondary light receives created frame");
        }
        std::uint32_t SetColor(
            std::uint32_t light,
            const std::array<float, 4>& color) override {
            calls_.push_back(light == primary ? "color-primary" :
                                               "color-secondary");
            Check(color == std::array<float, 4>{1, 1, 1, 1},
                  "both lights receive opaque white");
            return light;
        }
        std::uint32_t SetRadius(std::uint32_t light,
                                std::uint32_t bits) override {
            calls_.push_back("radius");
            Check(light == secondary && bits == 0x461c4000u,
                  "secondary radius uses native float bits");
            return light;
        }
        std::uint32_t SetConeAngle(std::uint32_t light,
                                   float angle) override {
            calls_.push_back("cone");
            Check(light == secondary && angle == 0.78539819f,
                  "secondary cone angle is pi over four");
            return light;
        }
        std::uint32_t WorldOfLight(std::uint32_t light) override {
            calls_.push_back(light == primary ? "world-primary" :
                                               "world-secondary");
            return 0;
        }
        void RemoveLight(std::uint32_t, std::uint32_t) override {
            calls_.push_back("remove-light");
        }
        std::uint32_t ReadWord(std::uint32_t address) override {
            calls_.push_back("read-frame");
            return frames[address];
        }
        void DestroyFrame(std::uint32_t object) override {
            calls_.push_back("destroy-frame");
            Check(object == frame, "only the secondary frame is destroyed");
        }
        void DestroyLight(std::uint32_t light) override {
            calls_.push_back(light == primary ? "destroy-primary" :
                                               "destroy-secondary");
        }

    private:
        std::vector<std::string>& calls_;
    };

    Runtime() : lights(calls) {}
    std::vector<std::string> calls;
    LightsRuntime lights;
    std::uint32_t active_lights_result = 0x77u;

    pal4::IRwLightRuntime& Lights() override { return lights; }
    std::uint32_t SetActiveNormalMapLights(
        std::uint32_t secondary,
        std::uint32_t primary) override {
        calls.push_back("active_lights");
        Check((secondary == lights.secondary && primary == lights.primary) ||
                  (secondary == 0 && primary == 0),
              "normal-map binding receives secondary then primary or resets both");
        return active_lights_result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::RwLightHierarchyState state{};
    runtime.lights.primary = 0;
    Check(pal4::RwLightHierarchyLifecycle::Initialize(
              state, runtime) == 0u &&
              state.primary_light == 0 && state.secondary_light == 0 &&
              runtime.calls == std::vector<std::string>{"create-primary"},
          "primary creation failure stops before secondary work");

    runtime.calls.clear();
    runtime.lights.primary = 0x100u;
    runtime.lights.secondary = 0;
    Check(pal4::RwLightHierarchyLifecycle::Initialize(
              state, runtime) == 0u &&
              state.primary_light == 0x100u &&
              state.secondary_light == 0 &&
              runtime.calls == std::vector<std::string>{
                  "create-primary", "create-secondary"},
          "secondary creation failure retains primary like native code");

    runtime.calls.clear();
    runtime.lights.secondary = 0x200u;
    runtime.lights.frame = 0;
    Check(pal4::RwLightHierarchyLifecycle::Initialize(
              state, runtime) == 0u &&
              state.primary_light == 0x100u &&
              state.secondary_light == 0 &&
              runtime.calls == std::vector<std::string>{
                  "create-primary", "create-secondary",
                  "create-frame", "destroy-secondary"},
          "failed frame destroys secondary light but retains primary");

    runtime.calls.clear();
    runtime.lights.frame = 0x300u;
    runtime.lights.rotation_count = 0;
    Check(pal4::RwLightHierarchyLifecycle::Initialize(
              state, runtime) == 1u &&
              state.primary_light == 0x100u &&
              state.secondary_light == 0x200u &&
              runtime.lights.rotation_count == 3u &&
              runtime.calls == std::vector<std::string>{
                  "create-primary", "create-secondary", "create-frame",
                  "rotate", "rotate", "rotate", "translate", "set-frame",
                  "color-primary", "color-secondary", "radius", "cone",
                  "active_lights"},
          "successful initialization follows native light setup order");

    runtime.calls.clear();
    runtime.lights.frames[0x104u] = 0;
    runtime.lights.frames[0x204u] = 0x300u;
    Check(pal4::RwLightHierarchyLifecycle::Shutdown(
              state, runtime) == 0x77u &&
              state.primary_light == 0 && state.secondary_light == 0 &&
              runtime.calls == std::vector<std::string>{
                  "world-primary", "read-frame", "clear-frame",
                  "destroy-primary", "world-secondary", "read-frame",
                  "clear-frame", "destroy-frame", "destroy-secondary",
                  "active_lights"},
          "shutdown destroys primary then secondary and resets hierarchy");
    runtime.calls.clear();
    Check(pal4::RwLightHierarchyLifecycle::Shutdown(
              state, runtime) == 0x77u &&
              runtime.calls == std::vector<std::string>{"active_lights"},
          "repeat shutdown still resets hierarchy parameters");

    std::cout << "rw_light_hierarchy_lifecycle_tests passed\n";
}
