#include "pal4/camera_controller_access.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraControllerAccessRuntime {
public:
    pal4::CameraSystemObjectState object{};
    pal4::CameraDataState camera{};
    std::uint32_t lookups = 0u;
    std::uint32_t sets = 0u;
    std::uint32_t last_camera = 0u;
    float last_distance = 0.0f;
    std::uint32_t searches = 0u;
    std::uint32_t missing_reports = 0u;

    pal4::CameraSystemObjectState& ObjectAt(
        std::uint32_t address) override {
        Check(address == 0x1234u, "active object address");
        ++lookups;
        return object;
    }
    void SetFarClip(std::uint32_t camera, float distance) override {
        ++sets;
        last_camera = camera;
        last_distance = distance;
    }
    pal4::CameraDataState& CameraDataAt(
        std::uint32_t address) override {
        Check(address == 0x5678u, "inner camera address");
        return camera;
    }
    std::optional<std::uint32_t> FindNamedObject(
        const pal4::CameraControllerState& state,
        std::uint32_t offset, const char* name) override {
        Check(offset == 0x10u && state.At(0x34u) == 0x1111u,
              "dynamic collection offset");
        ++searches;
        if (std::strcmp(name, "ExtraCamera") == 0) return 0x5555u;
        if (std::strcmp(name, "NullCamera") == 0) return 0u;
        return std::nullopt;
    }
    void ReportMissingCameraName(const char* name) override {
        Check(std::strcmp(name, "UnknownCamera") == 0,
              "missing camera logger receives name");
        ++missing_reports;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraControllerAccess access(runtime);
    pal4::CameraControllerState state;
    Check(access.ActiveObject(state) == 0u &&
              access.ActiveInternalId(state) == 0u &&
              access.ActiveCameraPointer(state) == 0u &&
              !access.IsMainCameraActive(state) &&
              runtime.lookups == 0u,
          "empty active slot stops all three access layers");
    Check(!access.SetFarClipPlane(state, 350.0f) &&
              runtime.lookups == 0u && runtime.sets == 0u,
          "missing active object skips native calls");

    state.At(0x30u) = 0x1234u;
    runtime.object.camera = 0x5678u;
    constexpr float distance = 123.75f;
    Check(access.SetFarClipPlane(state, distance) &&
              runtime.lookups == 1u && runtime.sets == 1u &&
              runtime.last_camera == 0x5678u &&
              std::bit_cast<std::uint32_t>(runtime.last_distance) ==
                  std::bit_cast<std::uint32_t>(distance),
          "active object forwards inner camera and exact distance");
    runtime.camera.At(0u) = 0xabcd1234u;
    Check(access.ActiveObject(state) == 0x1234u &&
              access.ActiveInternalId(state) == 0x5678u &&
              access.ActiveCameraPointer(state) == 0xabcd1234u,
          "active access unwraps object, inner camera and native handle");
    runtime.object.camera = 0u;
    Check(access.ActiveInternalId(state) == 0u &&
              access.ActiveCameraPointer(state) == 0u,
          "missing inner camera stops before native handle read");
    runtime.object.camera = 0x5678u;

    state.At(0x34u) = 0x1111u;
    state.At(0x38u) = 0x2222u;
    state.At(0x3cu) = 0x3333u;
    state.At(0x40u) = 0x4444u;
    Check(!access.IsMainCameraActive(state),
          "different main and active objects are inactive");
    Check(access.GetObjectByName(state, "MainCamera") == 0x1111u &&
              access.GetObjectByName(state, "ScriptCamera") == 0x2222u &&
              access.GetObjectByName(state, "UiCamera") == 0x4444u &&
              access.GetObjectByName(state, "CombatCamera") == 0x3333u &&
              runtime.searches == 0u,
          "built-in names map to four controller slots without tree lookup");
    Check(access.GetObjectByName(state, "ExtraCamera") == 0x5555u &&
              access.GetObjectByName(state, "maincamera") == 0u &&
              runtime.searches == 2u,
          "dynamic lookup handles unknown and case-distinct names");

    Check(access.SetActiveCameraByName(state, "MainCamera") &&
              state.At(0x30u) == 0x1111u &&
              access.IsMainCameraActive(state) &&
              access.SetActiveCameraByName(state, "ScriptCamera") &&
              state.At(0x30u) == 0x2222u &&
              access.SetActiveCameraByName(state, "UiCamera") &&
              state.At(0x30u) == 0x4444u &&
              access.SetActiveCameraByName(state, "CombatCamera") &&
              state.At(0x30u) == 0x3333u &&
              runtime.searches == 2u,
          "fixed names select slots without dynamic lookup");
    Check(access.SetActiveCameraByName(state, "ExtraCamera") &&
              state.At(0x30u) == 0x5555u &&
              access.SetActiveCameraByName(state, "NullCamera") &&
              state.At(0x30u) == 0u,
          "dynamic hit selects stored value including null");
    state.At(0x30u) = 0x1111u;
    Check(!access.SetActiveCameraByName(state, "UnknownCamera") &&
              state.At(0x30u) == 0x1111u &&
              runtime.missing_reports == 1u,
          "dynamic miss logs and preserves active camera");
    state.At(0x34u) = 0u;
    state.At(0x30u) = 0u;
    Check(!access.IsMainCameraActive(state),
          "two null slots do not count as active main camera");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
