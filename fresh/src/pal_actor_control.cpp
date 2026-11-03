#include "pal4/pal_actor_control.h"
#include "pal4/actor_control_database.h"
#include "pal4/actor_control_input_component.h"
#include "pal4/actor_control_motion.h"
#include "pal4/pal_actor_control_loop.h"

#include <bit>
#include <cstring>
#include <initializer_list>

namespace pal4 {

std::uint32_t PalActorControl::SetControlWord(
    PalActorControlState& state, std::uint32_t value) {
    state.words[32] = value;
    return value;
}

std::uint32_t PalActorControl::GetControlWord(
    const PalActorControlState& state) {
    return state.words[32];
}

std::uint32_t PalActorControl::GetMode(
    const PalActorControlState& state) {
    return state.words[33];
}

void PalActorControlRuntimeStub::ConstructGameComponent(std::uint32_t* storage) {
    ActorControlInputComponentState view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    class Runtime final : public IActorControlInputComponentRuntime {
    public:
        explicit Runtime(IPalActorControlRuntime& owner) : owner_(owner) {}
        std::int32_t GameConfigPrimary() override {
            return owner_.GameConfig(0u);
        }
        void SetAnimationFlag(std::uint32_t, std::uint32_t) override {}
        void HandleInputEvents(ActorControlInputComponentState&,
                               std::uint32_t, float) override {}
        void UpdateInputObject(ActorControlInputComponentState&,
                               std::uint32_t, float) override {}
    private:
        IPalActorControlRuntime& owner_;
    } runtime(*this);
    static_cast<void>(ActorControlInputComponent::Construct(view, runtime));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::ConstructDatabase(std::uint32_t* storage) {
    ActorControlDatabasePrefix view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    ActorControlDatabaseRuntimeStub runtime;
    static_cast<void>(ActorControlDatabase::Construct(view, runtime));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::ConstructMotion(std::uint32_t* storage) {
    ActorControlMotionState view;
    std::memcpy(&view, storage, sizeof(view));
    static_cast<void>(ActorControlMotion::Construct(view));
    std::memcpy(storage, &view, sizeof(view));
}
void PalActorControlRuntimeStub::ConstructDNameNode(std::uint32_t*) {}
void PalActorControlRuntimeStub::ConstructNamedNode(
    std::uint32_t*, std::string_view) {}
void PalActorControlRuntimeStub::DestructDatabase(std::uint32_t* storage) {
    ActorControlDatabasePrefix view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    ActorControlDatabaseRuntimeStub runtime;
    ActorControlDatabase::Destruct(view, runtime);
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::DestructDNameNode(std::uint32_t*) {}
void PalActorControlRuntimeStub::ResetMemberVtable(std::uint32_t*) {}
void PalActorControlRuntimeStub::DestructNamedNode(std::uint32_t*) {}
void PalActorControlRuntimeStub::SetNamedNodeErrorFlag(
    std::uint32_t*, std::uint8_t) {}
std::int32_t PalActorControlRuntimeStub::GameConfig(std::uint32_t) {
    return 1;
}
std::uint32_t PalActorControlRuntimeStub::Logger() { return 0u; }
std::uint32_t PalActorControlRuntimeStub::Log(
    std::uint32_t, std::string_view, std::string_view,
    std::uint32_t) { return 0u; }
void PalActorControlRuntimeStub::PrepareScripts() {}
void PalActorControlRuntimeStub::CleanupScripts() {}
void PalActorControlRuntimeStub::InitializeComponent(std::uint32_t* storage) {
    ActorControlInputComponentState view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    static_cast<void>(ActorControlInputComponent::Initialize(view));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::InitializeDatabase(
    std::uint32_t* storage) {
    ActorControlDatabasePrefix view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    ActorControlDatabaseRuntimeStub runtime;
    static_cast<void>(ActorControlDatabase::LoadDummyClump(view, runtime));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::InitializeMotion(std::uint32_t* storage) {
    ActorControlMotionState view;
    std::memcpy(&view, storage, sizeof(view));
    static_cast<void>(ActorControlMotion::Reset(view));
    std::memcpy(storage, &view, sizeof(view));
}
void PalActorControlRuntimeStub::ShutdownComponent(std::uint32_t*) {}
void PalActorControlRuntimeStub::ShutdownDatabase(
    std::uint32_t* storage, std::uint32_t event_context) {
    ActorControlDatabasePrefix view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    ActorControlDatabaseRuntimeStub runtime;
    static_cast<void>(ActorControlDatabase::UnloadClump(
        view, event_context, runtime));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::ShutdownMotion(std::uint32_t* storage) {
    ActorControlMotionState view;
    std::memcpy(&view, storage, sizeof(view));
    static_cast<void>(ActorControlMotion::Reset(view));
    std::memcpy(storage, &view, sizeof(view));
}
std::uint8_t PalActorControlRuntimeStub::ResetController(
    PalActorControlState& state) {
    IPalActorControlLoopRuntime runtime;
    return PalActorControlLoop::ClearActor(state, runtime);
}
void PalActorControlRuntimeStub::UpdateComponent(std::uint32_t*) {}
void PalActorControlRuntimeStub::UpdateDatabase(std::uint32_t*) {}
std::uint32_t PalActorControlRuntimeStub::UpdateMotion(
    std::uint32_t*) { return 1u; }
void PalActorControlRuntimeStub::ProcessComponentInput(
    std::uint32_t* storage, std::uint32_t actor) {
    ActorControlInputComponentState view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    ActorControlInputComponentRuntimeStub runtime;
    static_cast<void>(ActorControlInputComponent::ResetActor(view, actor, runtime));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::ProcessDatabaseInput(
    std::uint32_t* storage, std::uint32_t actor,
    std::uint32_t event_context) {
    ActorControlDatabasePrefix view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    ActorControlDatabaseRuntimeStub runtime;
    static_cast<void>(ActorControlDatabase::ResetDialog(
        view, event_context, actor, runtime));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
std::uint32_t PalActorControlRuntimeStub::ProcessMotionInput(
    std::uint32_t* storage, std::uint32_t actor) {
    ActorControlMotionState view;
    std::memcpy(&view, storage, sizeof(view));
    const auto result = ActorControlMotion::ResetWithArgument(view, actor);
    std::memcpy(storage, &view, sizeof(view));
    return result;
}
std::uint32_t PalActorControlRuntimeStub::SetActorAnimationFlag(
    std::uint32_t, std::uint32_t) { return 0u; }
void PalActorControlRuntimeStub::ProcessComponentInputWithUi(
    std::uint32_t* storage, std::uint32_t actor) {
    ActorControlInputComponentState view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    static_cast<void>(ActorControlInputComponent::ResetActorWithoutAnimation(
        view, actor));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::ProcessDatabaseInputWithUi(
    std::uint32_t* storage, std::uint32_t actor,
    std::uint32_t event_context) {
    ActorControlDatabasePrefix view;
    std::memcpy(view.words.data(), storage, sizeof(view));
    ActorControlDatabaseRuntimeStub runtime;
    static_cast<void>(ActorControlDatabase::ResetWithUi(
        view, event_context, actor, runtime));
    std::memcpy(storage, view.words.data(), sizeof(view));
}
void PalActorControlRuntimeStub::ProcessMotionInputWithUi(
    std::uint32_t*, std::uint32_t) {}

PalActorControlState& PalActorControl::Construct(
    PalActorControlState& state, IPalActorControlRuntime& runtime) {
    auto* const words = state.words.data();
    int stage = -1;
    try {
        runtime.ConstructGameComponent(words + 23u);
        stage = 0;
        runtime.ConstructDatabase(words + 35u);
        stage = 1;
        runtime.ConstructMotion(words + 165u);
        stage = 2;
        runtime.ConstructDNameNode(words + 186u);
        stage = 3;
        runtime.ConstructDNameNode(words + 203u);
        stage = 4;
        words[205] = kFirstMemberVtable;
        words[206] = 0u;
        stage = 5;
        words[207] = kSecondMemberVtable;
        words[208] = 0u;
        stage = 6;
        runtime.ConstructNamedNode(words + 209u, "FL");
        stage = 7;
        runtime.ConstructNamedNode(words + 216u, "FR");
        stage = 8;
        runtime.ConstructDNameNode(words + 279u);

        words[0] = 1u;
        words[1] = 0u;
        words[2] = 0x43160000u;
        words[3] = 1u;
        words[4] = 1u;
        words[5] = 0u;
        words[7] = 0u;
        words[8] = 0u;
        words[9] = 0x41900000u;
        words[10] = 0u;
        words[11] = 0u;
        const auto second_config = runtime.GameConfig(1u);
        words[12] = std::bit_cast<std::uint32_t>(
            static_cast<float>(40.0 / second_config));
        const auto first_config = runtime.GameConfig(0u);
        for (const auto index : {14u, 15u, 16u, 17u, 18u,
                                 20u, 21u, 22u, 6u,
                                 183u, 184u, 185u, 188u,
                                 195u, 202u, 223u})
            words[index] = 0u;
        words[13] = std::bit_cast<std::uint32_t>(
            static_cast<float>(120.0 / first_config));
    } catch (...) {
        if (stage >= 8) runtime.DestructNamedNode(words + 216u);
        if (stage >= 7) runtime.DestructNamedNode(words + 209u);
        if (stage >= 6) runtime.ResetMemberVtable(words + 207u);
        if (stage >= 5) runtime.ResetMemberVtable(words + 205u);
        if (stage >= 4) runtime.DestructDNameNode(words + 203u);
        if (stage >= 3) runtime.DestructDNameNode(words + 186u);
        if (stage >= 2) words[165] = 0x8425d4u;
        if (stage >= 1) runtime.DestructDatabase(words + 35u);
        if (stage >= 0) words[23] = 0x8425d4u;
        throw;
    }
    return state;
}

void PalActorControl::Destruct(PalActorControlState& state,
                               IPalActorControlRuntime& runtime) {
    auto* const words = state.words.data();
    runtime.DestructDNameNode(words + 279u);
    runtime.SetNamedNodeErrorFlag(words + 217u, 1u);
    words[216] = 0x8425a0u;
    runtime.SetNamedNodeErrorFlag(words + 210u, 1u);
    words[209] = 0x8425a0u;
    words[207] = 0x8425a0u;
    words[205] = 0x8425a0u;
    runtime.DestructDNameNode(words + 203u);
    runtime.DestructDNameNode(words + 186u);
    words[165] = 0x8425d4u;
    runtime.DestructDatabase(words + 35u);
    words[23] = 0x8425d4u;
}

std::uint32_t PalActorControl::Initialize(
    PalActorControlState& state, std::uint32_t base_address,
    IPalActorControlRuntime& runtime) {
    constexpr std::string_view kSource =
        "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\ActorController\\palActorController.cpp";
    constexpr std::string_view kBegin =
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  palActorController initializing begin...\n\n\n";
    constexpr std::string_view kDone =
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  palActorController initialize ok\n\n\n";
    static_cast<void>(runtime.Log(runtime.Logger(), kBegin, kSource, 79u));
    runtime.PrepareScripts();
    auto* const words = state.words.data();
    bool scripts_active = true;
    try {
        words[183] = 0u;
        words[223] = 0u;
        words[6] = 0u;
        runtime.InitializeComponent(words + 23u);
        runtime.InitializeDatabase(words + 35u);
        runtime.InitializeMotion(words + 165u);
        words[206] = base_address + 140u;
        words[208] = base_address + 140u;
        words[215] = kFloorSoundCallback;
        words[222] = kFloorSoundCallback;
        scripts_active = false;
        runtime.CleanupScripts();
    } catch (...) {
        if (scripts_active) {
            try { runtime.CleanupScripts(); } catch (...) {}
        }
        throw;
    }
    for (const auto index : {184u, 185u, 188u, 195u, 202u, 223u})
        words[index] = 0u;
    return runtime.Log(runtime.Logger(), kDone, kSource, 108u);
}

std::uint8_t PalActorControl::Shutdown(
    PalActorControlState& state, std::uint32_t event_context,
    IPalActorControlRuntime& runtime) {
    auto* const words = state.words.data();
    runtime.ShutdownComponent(words + 23u);
    runtime.ShutdownDatabase(words + 35u, event_context);
    runtime.ShutdownMotion(words + 165u);
    const auto result = runtime.ResetController(state);
    words[0] = 0u;
    return result;
}

std::uint32_t PalActorControl::UpdateComponents(
    PalActorControlState& state, IPalActorControlRuntime& runtime) {
    auto* const words = state.words.data();
    runtime.UpdateComponent(words + 23u);
    runtime.UpdateDatabase(words + 35u);
    return runtime.UpdateMotion(words + 165u);
}

std::uint32_t PalActorControl::ProcessInput(
    PalActorControlState& state, std::uint32_t event_context,
    IPalActorControlRuntime& runtime) {
    auto* const words = state.words.data();
    const auto actor = words[1];
    words[183] = 0u;
    runtime.ProcessComponentInput(words + 23u, actor);
    runtime.ProcessDatabaseInput(words + 35u, words[1], event_context);
    const auto result = runtime.ProcessMotionInput(words + 165u, words[1]);
    if (words[1] != 0u)
        return runtime.SetActorAnimationFlag(words[1], 1u);
    return result;
}

std::uint32_t PalActorControl::ProcessInputAndClearVector(
    PalActorControlState& state, std::uint32_t event_context,
    IPalActorControlRuntime& runtime) {
    auto* const words = state.words.data();
    runtime.ProcessComponentInputWithUi(words + 23u, words[1]);
    runtime.ProcessDatabaseInputWithUi(words + 35u, words[1],
                                       event_context);
    runtime.ProcessMotionInputWithUi(words + 165u, words[1]);
    auto result = words[1];
    if (result != 0u)
        result = runtime.SetActorAnimationFlag(result, 1u);
    words[20] = 0u;
    words[21] = 0u;
    words[22] = 0u;
    return result;
}

}  // namespace pal4
