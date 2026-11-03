#include "pal4/pal_actor_control_config_initializer.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IActorControlConfigInitRuntime {
    pal4::PalActorControlState* state{};
    std::vector<std::string> calls;
    bool version_ok{};
    std::uint32_t attribute{};
    std::uint32_t actor_flag{};
    bool throw_on_sync{};
    std::uint32_t camera_count{};
    std::uint32_t scale_count{};

    void InitializeDocument(pal4::ActorControlXmlConfigDocument& document,
                            std::string_view root,
                            std::uint32_t flag) override {
        Check(root == "Actor-Controller" && flag == 0u &&
                  document.words[0u] == 0u,
              "config document construction");
        calls.emplace_back("document");
    }
    void InitializeFacet(pal4::ActorControlXmlWriteFacet& facet,
                         std::uint32_t argument) override {
        Check(argument == 0x12345678u, "facet full argument");
        facet.words[0u] = 0x11111111u;
        calls.emplace_back("facet");
    }
    bool CheckVersionAndType(pal4::ActorControlXmlWriteFacet& facet,
                             pal4::ActorControlXmlConfigDocument&) override {
        Check(facet.words[0u] == 0x11111111u,
              "version check precedes facet reset");
        calls.emplace_back("check"); return version_ok;
    }
    std::uint32_t FindAttribute(pal4::ActorControlXmlConfigDocument&,
                                std::string_view name) override {
        Check(name == "m_isNeedBlendTailAngle",
              "config attribute name");
        calls.emplace_back("find_attribute"); return attribute;
    }
    std::uint32_t AttributeToInt(std::uint32_t value,
                                 const std::uint32_t* fallback) override {
        Check(value == attribute && fallback != nullptr && *fallback == 1u,
              "attribute parser receives fallback one");
        calls.emplace_back("to_int"); return 7u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x91f0u, "actor readiness offset");
        calls.emplace_back("actor_flag"); return actor_flag;
    }
    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap_manager"); return 0x1000u;
    }
    void AddIcon(std::uint32_t manager, std::string_view name,
                 std::uint32_t index, std::string_view texture,
                 std::uint32_t first, std::uint32_t second) override {
        Check(manager == 0x1000u && name == "LeadingRole" && index == 0u &&
                  texture == "actor.png" && first == 0u && second == 0u,
              "leading-role icon args");
        calls.emplace_back("add_icon");
    }
    void SetActiveIcon(std::uint32_t manager, std::string_view name,
                       std::uint32_t index) override {
        Check(manager == 0x1000u && name == "LeadingRole" && index == 0u,
              "active icon args");
        calls.emplace_back("active_icon");
    }
    void UpdateGrid(std::uint32_t manager, std::uint32_t position,
                    std::uint32_t grid_state) override {
        Check(manager == 0x1000u && position == 0x90acu &&
                  grid_state == 0x90a0u, "grid offsets");
        calls.emplace_back("grid");
    }
    long double ActorScale(std::uint32_t actor) override {
        Check(actor == 0x9000u, "scale actor");
        calls.emplace_back("scale"); ++scale_count; return 10.0L;
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("movement_adapter"); return 0x2000u;
    }
    void UpdateMovement(std::uint32_t adapter, std::uint32_t position,
                        float first, float second,
                        std::uint32_t flag) override {
        Check(adapter == 0x2000u && position == 0x90acu && flag == 1u &&
                  first == static_cast<float>(10.0L * 0.34999999f) &&
                  second == static_cast<float>(10.0L * 0.52999997f),
              "movement scale and position");
        calls.emplace_back("movement");
    }
    std::uint32_t FloorAttributeManager() override {
        calls.emplace_back("floor_manager"); return 0x3000u;
    }
    void UpdateFloorAttributes(std::uint32_t manager) override {
        Check(manager == 0x3000u, "floor manager");
        calls.emplace_back("floor");
    }
    std::uint32_t CameraSubsystem() override {
        calls.emplace_back("camera_subsystem");
        return ++camera_count == 1u ? 0x4000u : 0x5000u;
    }
    std::uint32_t ActiveCamera(std::uint32_t subsystem) override {
        Check(subsystem == (camera_count == 1u ? 0x4000u : 0x5000u),
              "active camera subsystem");
        calls.emplace_back("active_camera");
        return subsystem + 0x100u;
    }
    pal4::ActorControlAudioPoint ReadPoint(std::uint32_t address) override {
        Check(address == (camera_count == 1u ? 0x4174u : 0x5150u),
              "camera point offsets");
        calls.emplace_back("camera_point");
        return camera_count == 1u ?
            pal4::ActorControlAudioPoint{2.0f, 3.0f, 4.0f} :
            pal4::ActorControlAudioPoint{5.0f, 6.0f, 7.0f};
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio_manager"); return 0x6000u;
    }
    std::uint32_t AudioDriver(std::uint32_t manager) override {
        Check(manager == 0x6000u, "audio manager address");
        calls.emplace_back("audio_driver"); return 0x7000u;
    }
    void SetAudioPosition(
        std::uint32_t driver,
        const pal4::ActorControlAudioPoint& point) override {
        Check(driver == 0x7000u &&
                  point == pal4::ActorControlAudioPoint{2.0f, 3.0f, -4.0f},
              "audio position negates camera Z");
        calls.emplace_back("audio_position");
    }
    void SetAudioOrientation(
        std::uint32_t driver,
        const pal4::ActorControlAudioPoint& direction,
        const pal4::ActorControlAudioPoint& up) override {
        Check(driver == 0x7000u &&
                  direction == pal4::ActorControlAudioPoint{5.0f, 6.0f, -7.0f} &&
                  up == pal4::ActorControlAudioPoint{0.0f, 1.0f, 0.0f} &&
                  std::signbit(up[2u]),
              "audio orientation and negative-zero up Z");
        calls.emplace_back("audio_orientation");
    }
    void ProcessInput(pal4::PalActorControlState& value) override {
        Check(&value == state && state->words[3u] ==
                  (attribute != 0u ? 7u : 1u),
              "input after config field write");
        calls.emplace_back("input");
    }
    void ProcessInputAndClearVector(
        pal4::PalActorControlState& value) override {
        Check(&value == state, "clear vector state");
        calls.emplace_back("input_clear");
    }
    std::uint32_t ControllerAccess() override {
        calls.emplace_back("access"); return 0x8000u;
    }
    void ResetAndSync(std::uint32_t access,
                       std::uint32_t flag) override {
        Check(access == 0x8000u && flag == 1u,
              "controller sync args");
        calls.emplace_back("sync");
        if (throw_on_sync) throw std::runtime_error("sync");
    }
    void ResetFacetVtable(pal4::ActorControlXmlWriteFacet& facet) override {
        facet.ResetVtable();
        Check(facet.words[0u] == 0x8426acu,
              "facet reset vtable");
        calls.emplace_back("reset_facet");
    }
    void DestroyDocument(pal4::ActorControlXmlConfigDocument&) override {
        calls.emplace_back("destroy_document");
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.state = &state;
    state.words[3u] = 99u;
    Check(pal4::PalActorControlConfigInitializer::Initialize(
              state, 0x12345678u, runtime) == 0u &&
              state.words[3u] == 99u &&
              runtime.calls == std::vector<std::string>{
                  "document", "facet", "check", "reset_facet",
                  "destroy_document"},
          "invalid config skips controller side effects");

    runtime.calls.clear();
    runtime.version_ok = true;
    Check(pal4::PalActorControlConfigInitializer::Initialize(
              state, 0x12345678u, runtime) == 1u &&
              state.words[3u] == 1u &&
              runtime.calls == std::vector<std::string>{
                  "document", "facet", "check", "find_attribute", "input",
                  "input_clear", "access", "sync", "reset_facet",
                  "destroy_document"},
          "missing attribute defaults to one; absent actor still syncs");

    runtime.calls.clear();
    state.words[1u] = 0x9000u;
    runtime.actor_flag = 0u;
    runtime.attribute = 0x4567u;
    Check(pal4::PalActorControlConfigInitializer::Initialize(
              state, 0x12345678u, runtime) == 1u &&
              state.words[3u] == 7u &&
              runtime.calls == std::vector<std::string>{
                  "document", "facet", "check", "find_attribute", "to_int",
                  "actor_flag", "input", "input_clear", "access", "sync",
                  "reset_facet", "destroy_document"},
          "disabled actor setup still processes input and sync");

    runtime.calls.clear();
    runtime.actor_flag = 1u;
    Check(pal4::PalActorControlConfigInitializer::Initialize(
              state, 0x12345678u, runtime) == 1u &&
              runtime.scale_count == 2u && runtime.camera_count == 2u &&
              runtime.calls == std::vector<std::string>{
                  "document", "facet", "check", "find_attribute", "to_int",
                  "actor_flag", "minimap_manager", "add_icon",
                  "minimap_manager", "active_icon", "minimap_manager",
                  "grid", "scale", "scale", "movement_adapter", "movement",
                  "floor_manager", "floor", "camera_subsystem",
                  "active_camera", "camera_point", "audio_manager",
                  "audio_driver", "audio_position", "camera_subsystem",
                  "active_camera", "camera_point", "audio_manager",
                  "audio_driver", "audio_orientation", "input", "input_clear",
                  "access", "sync", "reset_facet", "destroy_document"},
          "ready actor initializes minimap movement floor audio and input");

    runtime.calls.clear();
    runtime.throw_on_sync = true;
    runtime.camera_count = 0u;
    try {
        (void)pal4::PalActorControlConfigInitializer::Initialize(
            state, 0x12345678u, runtime);
        Check(false, "sync exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.size() >= 3u &&
                  runtime.calls[runtime.calls.size() - 3u] == "sync" &&
                  runtime.calls[runtime.calls.size() - 2u] == "reset_facet" &&
                  runtime.calls.back() == "destroy_document",
              "failed sync unwinds facet and document");
    }
}
