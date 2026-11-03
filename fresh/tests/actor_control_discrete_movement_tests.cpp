#include "pal4/actor_control_discrete_movement.h"
#include "pal4/game_resource_manager.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kActor = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.00002f;
}

void CheckVector(const pal4::ActorControlVec3& actual,
                 float x, float y, float z, const char* message) {
    Check(Near(actual.x, x) && Near(actual.y, y) && Near(actual.z, z),
          message);
}

struct Runtime final : pal4::ActorControlDiscreteMovementRuntimeStub {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    bool camera_active = true;
    bool input_active = false;
    bool movement_allowed = true;
    bool mutate_movement = false;
    std::string current_animation = "idle";
    std::string assigned_animation;
    float assigned_speed = -1.0f;
    pal4::ActorControlVec3 displacement{};
    pal4::ActorControlVec3 updated_position{};
    pal4::ActorControlVec3 checked_position{};
    pal4::ActorControlVec3 written_position{};
    pal4::ActorControlVec3 minimap_position{};
    pal4::ActorControlVec3 rotation{};
    unsigned property_reads = 0;
    unsigned tag_reads = 0;
    unsigned angle_calls = 0;
    double actor_property = 4.0;
    std::uint32_t rotate_enabled = 99u;

    Runtime() {
        memory[kActor + 0x14u] = 0x2000u;
        memory[0x2004u] = 0x3000u;
        memory[kActor + 0xacu] = std::bit_cast<std::uint32_t>(10.0f);
        memory[kActor + 0xb0u] = std::bit_cast<std::uint32_t>(20.0f);
        memory[kActor + 0xb4u] = std::bit_cast<std::uint32_t>(30.0f);
        memory[0x4020u] = std::bit_cast<std::uint32_t>(3.0f);
        memory[0x4024u] = std::bit_cast<std::uint32_t>(4.0f);
        memory[0x4028u] = std::bit_cast<std::uint32_t>(0.0f);
        memory[kActor + 0xa0u] = std::bit_cast<std::uint32_t>(7.0f);
        memory[kActor + 0xa8u] = std::bit_cast<std::uint32_t>(9.0f);
        memory[0x8000u] = std::bit_cast<std::uint32_t>(0.6f);
        memory[0x8004u] = std::bit_cast<std::uint32_t>(0.0f);
        memory[0x8008u] = std::bit_cast<std::uint32_t>(0.8f);
    }
    std::uint32_t CameraSystem() override {
        calls.emplace_back("camera_system");
        return 0x6000u;
    }
    bool CameraActive(std::uint32_t system) override {
        Check(system == 0x6000u, "active check uses camera system");
        calls.emplace_back("camera_active");
        return camera_active;
    }
    void GetActiveCamera(std::uint32_t system) override {
        Check(system == 0x6000u, "camera read uses second system result");
        calls.emplace_back("get_camera");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x5024u) {
            ++tag_reads;
            return tag_reads == 1u ? 0xaau : 0xbbu;
        }
        Check(memory.contains(address), "expected actor or transform read");
        return memory[address];
    }
    std::uint32_t ObjectTransform(std::uint32_t object) override {
        Check(object == 0x3000u, "nested actor object reaches transform");
        calls.emplace_back("transform");
        return 0x4000u;
    }
    double ActorProperty(std::uint32_t actor) override {
        Check(actor == kActor, "property receives actor");
        ++property_reads;
        calls.emplace_back("property");
        return actor_property;
    }
    std::uint32_t Controller() override {
        calls.emplace_back("controller");
        return 0x5000u;
    }
    void Normalize(pal4::ActorControlVec3& vector) override {
        calls.emplace_back("normalize");
        CheckVector(vector, 3.0f, 4.0f, 0.0f,
                    "RenderWare receives transform row");
        vector = {0.6f, 0.8f, 0.0f};
    }
    void SetAnimationSpeed(std::uint32_t animation, float speed) override {
        Check(animation == kActor + 0x170u, "animation speed address");
        calls.emplace_back("speed");
        assigned_speed = speed;
    }
    std::string AnimationName(std::uint32_t animation) override {
        Check(animation == kActor + 0x170u, "animation name address");
        calls.emplace_back("animation_name");
        return current_animation;
    }
    void SetAnimationName(std::uint32_t animation,
                          std::string_view name) override {
        Check(animation == kActor + 0x170u, "animation setter address");
        calls.emplace_back("set_animation");
        assigned_animation = name;
    }
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t flag) override {
        Check(animation == kActor + 0x170u && flag == 1u,
              "animation change sets flag one");
        calls.emplace_back("flag");
    }
    double AngleFromDot(float value) override {
        Check(Near(value, 0.8f), "yaw helper receives direction z");
        ++angle_calls;
        calls.emplace_back("angle");
        return 1.0;
    }
    bool AnyInputActive(
        const pal4::ActorControlInputComponentState&) override {
        calls.emplace_back("input_active");
        return input_active;
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("adapter");
        return 0x7000u;
    }
    void PreprocessMovement(std::uint32_t adapter,
                            pal4::ActorControlMovementGlobals& globals,
                            pal4::ActorControlVec3& movement,
                            std::uint32_t object,
                            std::uint32_t first, std::uint32_t second,
                            std::uint32_t third) override {
        Check(adapter == 0x7000u && object == 0x2000u &&
                  first == 1u && second == 1u && third == 1u,
              "preprocess receives adapter, actor object and three flags");
        Check(globals.upper_tag == 0xaau && globals.lower_tag == 0xbbu,
              "preprocess sees both independently read global tags");
        calls.emplace_back("preprocess");
        displacement = movement;
        if (mutate_movement) movement.x += 0.25f;
    }
    void UpdateMovement(std::uint32_t adapter,
                        pal4::ActorControlVec3& position,
                        float lower, float upper,
                        std::uint32_t enabled) override {
        Check(adapter == 0x7000u && Near(lower, 1.4f) &&
                  Near(upper, 2.12f) && enabled == 1u,
              "update receives property-scaled radii and enable flag");
        calls.emplace_back("update");
        updated_position = position;
        if (mutate_movement) position.y += 1.0f;
    }
    bool CanMove(std::uint32_t adapter,
                 pal4::ActorControlVec3& position,
                 float lower, float upper) override {
        Check(adapter == 0x7000u && Near(lower, 1.4f) &&
                  Near(upper, 2.12f),
              "collision receives the same radii");
        calls.emplace_back("can_move");
        checked_position = position;
        if (mutate_movement) position.z += 0.5f;
        return movement_allowed;
    }
    void SetActorPosition(std::uint32_t actor,
                          const pal4::ActorControlVec3& position) override {
        Check(actor == kActor, "position write receives actor");
        calls.emplace_back("set_position");
        written_position = position;
        memory[kActor + 0xa0u] = std::bit_cast<std::uint32_t>(8.0f);
        memory[kActor + 0xa8u] = std::bit_cast<std::uint32_t>(10.0f);
    }
    void RotateActor(std::uint32_t actor,
                     const pal4::ActorControlVec3& value,
                     std::uint32_t enabled) override {
        Check(actor == kActor, "rotation receives actor");
        calls.emplace_back("rotate");
        rotation = value;
        rotate_enabled = enabled;
    }
    void UpdateMinimap(const pal4::ActorControlVec3& position,
                       std::uint32_t grid) override {
        Check(grid == kActor + 0xa0u, "minimap grid address");
        calls.emplace_back("minimap");
        minimap_position = position;
    }
    void UpdateFloorAttributes() override {
        calls.emplace_back("floor");
    }
};

pal4::ActorControlInputComponentState InputState(std::uint32_t animation_mode) {
    pal4::ActorControlInputComponentState state;
    state.words[1] = std::bit_cast<std::uint32_t>(8.0f);
    state.words[2] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[10] = animation_mode;
    return state;
}

bool Has(const Runtime& runtime, std::string_view call) {
    for (const auto& entry : runtime.calls)
        if (entry == call) return true;
    return false;
}

unsigned Count(const Runtime& runtime, std::string_view call) {
    unsigned count = 0;
    for (const auto& entry : runtime.calls)
        if (entry == call) ++count;
    return count;
}

void CheckCommon(const Runtime& runtime,
                 const pal4::ActorControlMovementGlobals& globals) {
    Check(runtime.property_reads == 3u && runtime.tag_reads == 2u &&
              Count(runtime, "camera_system") == 2u,
          "active actor reads camera twice, property thrice and tags twice");
    CheckVector(globals.lower, 10.0f, 21.5f, 30.0f,
                "lower collision point");
    CheckVector(globals.upper, 10.0f, 22.5f, 30.0f,
                "upper collision point");
}

void TestObservedM20CollisionBounds() {
    // Pair187: original/native positions and actor+DC agree. Original PC24
    // bounds already differ before the first physical displacement fork.
    struct Sample {float y;std::uint32_t lower,upper;};
    for(const auto sample : {Sample{3.591279983520508f,1107485242u,1112576016u},
                              Sample{3.6765193939208984f,1107507588u,1112598362u},
                              Sample{3.800333023071289f,1107540044u,1112630818u}}) {
        auto state=InputState(0u);Runtime runtime;
        runtime.actor_property=77.6790542602539;
        runtime.memory[kActor+0xb0u]=std::bit_cast<std::uint32_t>(sample.y);
        pal4::ActorControlMovementGlobals globals{};
        Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
                  state,globals,kActor,3u,.017f,runtime)==1u,
              "captured M20 command prepares collision bounds");
        Check(std::bit_cast<std::uint32_t>(globals.lower.y)==sample.lower &&
                  std::bit_cast<std::uint32_t>(globals.upper.y)==sample.upper,
              "M20 collision sphere centers preserve original intermediate rounding");
    }
}

void TestGatesAndUnknownCommand() {
    auto state = InputState(0u);
    pal4::ActorControlMovementGlobals globals{};
    Runtime runtime;
    runtime.camera_active = false;
    Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
              state, globals, kActor, 1u, 0.25f, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "camera_system", "camera_active"},
          "inactive camera exits before actor work");
    runtime.calls.clear();
    runtime.camera_active = true;
    Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
              state, globals, 0u, 1u, 0.25f, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "camera_system", "camera_active"},
          "null actor exits after camera-active check");
    runtime.calls.clear();
    Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
              state, globals, kActor, 99u, 0.25f, runtime) == 0u,
          "unknown command returns zero");
    CheckCommon(runtime, globals);
    Check(!Has(runtime, "animation_name") && !Has(runtime, "adapter") &&
              !Has(runtime, "minimap"),
          "unknown command still prepares state but has no action");
}

void TestForwardModesAndMovement() {
    for (const auto mode : {0u, 1u, 2u, 3u}) {
        auto state = InputState(mode);
        pal4::ActorControlMovementGlobals globals{};
        Runtime runtime;
        runtime.mutate_movement = true;
        Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
                  state, globals, kActor, 1u, 0.25f, runtime) == 1u,
              "forward command returns one");
        CheckCommon(runtime, globals);
        Check(runtime.assigned_animation ==
                  (mode == 0u ? "C02" : mode < 3u ? "C03" : ""),
              "forward animation follows component mode");
        Check(Count(runtime, "speed") == (mode < 3u ? 1u : 0u) &&
                  (mode >= 3u || Near(runtime.assigned_speed,
                                       mode == 2u ? 1.4f : 1.0f)),
              "forward speed flag follows mode");
        CheckVector(runtime.displacement, 1.2f, 0.0f, 0.0f,
                    "forward movement uses normalized horizontal heading");
        CheckVector(runtime.updated_position, 11.45f, 20.0f, 30.0f,
                    "preprocess mutation affects candidate position");
        CheckVector(runtime.checked_position, 11.45f, 21.0f, 30.0f,
                    "update mutation reaches collision check");
        CheckVector(runtime.written_position, 11.45f, 21.0f, 30.5f,
                    "collision mutation reaches actor write");
        CheckVector(runtime.minimap_position, 11.45f, 21.0f, 30.5f,
                    "accepted movement updates minimap");
        Check(Count(runtime, "adapter") == 3u &&
                  Count(runtime, "floor") == 1u,
              "forward path gets adapter three times and updates floor");
        if (mode == 0u)
            Check(runtime.calls == std::vector<std::string>{
                      "camera_system", "camera_active", "camera_system",
                      "get_camera", "transform", "property", "property",
                      "property", "controller", "controller", "normalize",
                      "speed", "animation_name", "set_animation", "flag",
                      "adapter", "preprocess", "adapter", "update",
                      "adapter", "can_move", "set_position", "minimap",
                      "floor"},
                  "forward calls preserve the observed service order");
    }
}

void TestBackwardAndRejectedMovement() {
    auto state = InputState(0u);
    pal4::ActorControlMovementGlobals globals{};
    Runtime runtime;
    runtime.movement_allowed = false;
    runtime.current_animation = "c02";
    Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
              state, globals, kActor, 2u, 0.25f, runtime) == 1u,
          "backward command returns one even when collision rejects it");
    CheckVector(runtime.displacement, -0.6f, 0.0f, 0.0f,
                "backward movement reverses and halves heading");
    CheckVector(runtime.checked_position, 9.4f, 20.0f, 30.0f,
                "backward candidate reaches collision check");
    Check(!Has(runtime, "speed") && !Has(runtime, "set_animation") &&
              !Has(runtime, "flag") && !Has(runtime, "set_position") &&
              !Has(runtime, "minimap") && !Has(runtime, "floor"),
          "matching animation and rejected movement skip writes");
    Check(runtime.calls == std::vector<std::string>{
              "camera_system", "camera_active", "camera_system",
              "get_camera", "transform", "property", "property",
              "property", "controller", "controller", "normalize",
              "animation_name", "adapter", "preprocess", "adapter",
              "update", "adapter", "can_move"},
          "rejected backward movement preserves call order");
}

void TestTurns() {
    for (const auto command : {3u, 4u}) {
        for (const bool input_active : {false, true}) {
            auto state = InputState(0u);
            pal4::ActorControlMovementGlobals globals{};
            Runtime runtime;
            runtime.input_active = input_active;
            Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
                      state, globals, kActor, command, 0.25f,
                      runtime) == 1u,
                  "turn commands return one");
            CheckCommon(runtime, globals);
            CheckVector(runtime.rotation, 0.0f,
                        command == 3u ? 1.0f : -1.0f, 0.0f,
                        "turn direction uses component turn speed");
            Check(runtime.rotate_enabled == 1u,
                  "discrete turn passes enable flag one");
            CheckVector(runtime.minimap_position, 10.0f, 20.0f, 30.0f,
                        "turn updates minimap at original position");
            Check(Has(runtime, "input_active") &&
                      Has(runtime, "rotate") && Has(runtime, "minimap") &&
                      !Has(runtime, "adapter") && !Has(runtime, "floor"),
                  "turn skips movement adapter and floor update");
            Check(Has(runtime, "animation_name") == !input_active,
                  "active input prevents turn animation change");
        }
    }
}

void TestMouseTurn() {
    for (const auto input : {2, -2}) {
        for (const bool active : {false, true}) {
            auto state = InputState(0u);
            state.words[3] = std::bit_cast<std::uint32_t>(3.5f);
            Runtime runtime;
            runtime.memory[0x9000u] = std::bit_cast<std::uint32_t>(input);
            runtime.input_active = active;
            runtime.current_animation = "idle";
            const auto original = state.words;
            Check(pal4::ActorControlInputComponent::ApplyMouseTurn(
                      state, kActor, 0x9000u, 0xdeadbeefu,
                      runtime) == 1u,
                  "mouse turn returns one and ignores trailing argument");
            Check(state.words == original, "mouse turn leaves component intact");
            CheckVector(runtime.rotation, 0.0f,
                        input > 0 ? -7.0f : 7.0f, 0.0f,
                        "signed input rotates with negative component scale");
            Check(runtime.rotate_enabled == 1u,
                  "mouse turn passes enable flag one");
            CheckVector(runtime.minimap_position, 10.0f, 20.0f, 30.0f,
                        "mouse turn uses actor position for minimap");
            Check(Count(runtime, "camera_system") == 0u &&
                      Count(runtime, "adapter") == 0u &&
                      Count(runtime, "property") == 0u &&
                      runtime.calls.front() == "input_active" &&
                      runtime.calls.back() == "minimap",
                  "mouse turn bypasses camera and movement setup");
            Check(Has(runtime, "animation_name") == !active &&
                      (active || runtime.assigned_animation == "C01"),
                  "idle mouse turn chooses C01 only when no input is active");
        }
    }
    auto state = InputState(0u);
    state.words[3] = std::bit_cast<std::uint32_t>(3.5f);
    Runtime runtime;
    runtime.memory[0x9000u] = 1u;
    runtime.current_animation = "c01";
    Check(pal4::ActorControlInputComponent::ApplyMouseTurn(
              state, kActor, 0x9000u, 0u, runtime) == 1u &&
              Has(runtime, "animation_name") &&
              !Has(runtime, "set_animation") && !Has(runtime, "flag"),
          "case-insensitive C01 match avoids animation restart");
}

void TestVectorMovement() {
    for (const auto mode : {0u, 1u, 2u, 3u}) {
        auto state = InputState(mode);
        pal4::ActorControlMovementGlobals globals{};
        Runtime runtime;
        const bool negative_x = mode == 2u;
        if (negative_x)
            runtime.memory[0x8000u] =
                std::bit_cast<std::uint32_t>(-0.6f);
        Check(pal4::ActorControlInputComponent::DispatchVectorMovement(
                  state, globals, kActor, 0x8000u, 0.25f,
                  runtime) == 1u,
              "vector movement returns one");
        Check(runtime.property_reads == 3u && runtime.tag_reads == 2u &&
                  Count(runtime, "camera_system") == 1u &&
                  Count(runtime, "get_camera") == 1u &&
                  !Has(runtime, "camera_active") &&
                  !Has(runtime, "normalize"),
              "vector route reads camera without activity gate or normalization");
        CheckVector(globals.lower, 10.0f, 21.5f, 30.0f,
                    "vector route lower collision point");
        CheckVector(globals.upper, 10.0f, 22.5f, 30.0f,
                    "vector route upper collision point");
        Check(globals.upper_tag == 0xaau && globals.lower_tag == 0xbbu,
              "vector route fills the second global pair");
        Check(runtime.assigned_animation ==
                  (mode == 0u ? "C02" : mode < 3u ? "C03" : "") &&
                  !Has(runtime, "speed"),
              "vector route chooses animation without speed change");
        CheckVector(runtime.displacement, 6.0f, 8.0f, 0.0f,
                    "vector route uses raw transform row times dt-speed");
        CheckVector(runtime.written_position, 16.0f, 28.0f, 30.0f,
                    "accepted vector route writes candidate position");
        const auto radians = negative_x ? 6.283185307179586 - 1.0 : 1.0;
        const auto degrees = static_cast<float>(
            radians * static_cast<double>(
                std::bit_cast<float>(0x42652ee1u)));
        CheckVector(runtime.rotation, 8.0f, degrees, 10.0f,
                    "yaw uses input sign and post-position actor fields");
        Check(runtime.rotate_enabled == 0u && runtime.angle_calls == 1u &&
                  Count(runtime, "adapter") == 3u &&
                  Has(runtime, "floor") && !Has(runtime, "minimap"),
              "vector route rotates with flag zero and updates floor only");
        if (mode == 0u)
            Check(runtime.calls == std::vector<std::string>{
                      "camera_system", "get_camera", "transform",
                      "property", "property", "property", "controller",
                      "controller", "animation_name", "set_animation",
                      "flag", "adapter", "preprocess", "adapter",
                      "update", "adapter", "can_move", "set_position",
                      "angle", "rotate", "floor"},
                  "vector route preserves original service order");
    }

    auto state = InputState(0u);
    pal4::ActorControlMovementGlobals globals{};
    Runtime rejected;
    rejected.movement_allowed = false;
    Check(pal4::ActorControlInputComponent::DispatchVectorMovement(
              state, globals, kActor, 0x8000u, 0.25f,
              rejected) == 1u &&
              rejected.angle_calls == 0u &&
              !Has(rejected, "set_position") &&
              !Has(rejected, "rotate") &&
              !Has(rejected, "floor"),
          "rejected vector collision skips position, yaw and floor");
}

int ReplayObservedTurns(const char* path) {
    std::ifstream input(path);
    std::string magic;
    std::uint32_t count{};
    Check(bool(input >> magic >> count) && magic=="PAL4_TURN_REPLAY_1" && count>0u && count<=1000000u,
          "turn replay requires a bounded versioned observed fixture");
    for(std::uint32_t index=0;index<count;++index){
        std::uint32_t side{},source_row{},command{},elapsed_bits{},speed_bits{},before_bits{},after_bits{};
        Check(bool(input>>side>>source_row>>command>>elapsed_bits>>speed_bits>>before_bits>>after_bits),
              "turn replay row is truncated or has invalid DWORD values");
        const auto elapsed=std::bit_cast<float>(elapsed_bits);
        const auto speed=std::bit_cast<float>(speed_bits);
        const auto before=std::bit_cast<float>(before_bits);
        const auto after=std::bit_cast<float>(after_bits);
        Check(side<=1u && (command==3u || command==4u) && std::isfinite(elapsed) && elapsed>0.f &&
              std::isfinite(speed) && speed>0.f && std::isfinite(before) && std::isfinite(after),
              "turn replay accepts only finite actual left/right samples");
        auto state=InputState(1u);state.words[2]=speed_bits;
        Runtime runtime;runtime.input_active=true;
        pal4::ActorControlMovementGlobals globals{};
        Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
                  state,globals,kActor,command,elapsed,runtime)==1u && runtime.rotate_enabled==1u,
              "observed turn must pass through production discrete dispatcher");
        // Reuse the production actor rotation/angle wrapping boundary. Frame
        // services are inert here: this fixture verifies raw angles, not RW pixels.
        pal4::GameResourceManager resource{};
        resource.words[5]=1u;resource.words[41]=before_bits;
        pal4::GameResourceManagerRuntimeStub frames;
        resource.UpdateClumpRotation({runtime.rotation.x,runtime.rotation.y,runtime.rotation.z},true,frames);
        if(resource.words[41]!=after_bits){
            std::cerr<<"turn replay mismatch side="<<side<<" source_row="<<source_row
                     <<" expected_bits="<<after_bits<<" actual_bits="<<resource.words[41]<<'\n';
            return 1;
        }
    }
    std::string extra;
    Check(!(input>>extra),"turn replay rejects trailing fixture data");
    std::cout<<"PASS: "<<count<<" observed turn outputs match production dispatcher/rotation bits\n";
    return 0;
}
}  // namespace

int main(int argc,char** argv) {
    if(argc==2)return ReplayObservedTurns(argv[1]);
    Check(argc==1,"usage: actor_control_discrete_movement_tests [observed-turn-fixture]");
    TestGatesAndUnknownCommand();
    TestForwardModesAndMovement();
    TestBackwardAndRejectedMovement();
    TestTurns();
    TestMouseTurn();
    TestVectorMovement();
    TestObservedM20CollisionBounds();

    auto state = InputState(0u);
    pal4::ActorControlMovementGlobals globals{};
    pal4::ActorControlDiscreteMovementRuntimeStub runtime;
    Check(pal4::ActorControlInputComponent::DispatchDiscreteMovement(
              state, globals, kActor, 1u, 0.25f, runtime) == 0u,
          "linkable runtime stub exits through inactive-camera gate");
}
