#include "pal4/actor_control_database.h"

namespace pal4 {

std::uint32_t ActorControlDatabaseRuntimeStub::FreeAddress(void*) {
    return 0u;
}
void ActorControlDatabaseRuntimeStub::ConstructResourceManager(
    std::uint32_t*) {}
void ActorControlDatabaseRuntimeStub::ConstructNameNode(
    std::uint32_t*) {}
void ActorControlDatabaseRuntimeStub::DestructNameNode(
    std::uint32_t*) {}
void ActorControlDatabaseRuntimeStub::DestructResourceManager(
    std::uint32_t*) {}
void ActorControlDatabaseRuntimeStub::ConcatenatePath(
    ActorControlPathBuffer&, std::uint32_t, std::uint32_t) {}
void ActorControlDatabaseRuntimeStub::AppendPath(
    ActorControlPathBuffer&, std::string_view) {}
std::uint32_t ActorControlDatabaseRuntimeStub::GameInstance() { return 0u; }
void ActorControlDatabaseRuntimeStub::LoadClump(
    std::uint32_t*, std::string_view, std::uint32_t, std::string_view,
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}
void ActorControlDatabaseRuntimeStub::ReleasePath(ActorControlPathBuffer&) {}
std::uint32_t ActorControlDatabaseRuntimeStub::HAnimFrameId(
    std::uint32_t) { return 0u; }
std::uint32_t ActorControlDatabaseRuntimeStub::GameArenaManager() {
    return 0u;
}
void ActorControlDatabaseRuntimeStub::RemoveClump(
    std::uint32_t, std::uint32_t) {}
void ActorControlDatabaseRuntimeStub::CleanupClump(
    std::uint32_t*, std::uint32_t) {}
bool ActorControlDatabaseRuntimeStub::NameNodeNeedsUpdate(
    std::uint32_t*) { return false; }
void ActorControlDatabaseRuntimeStub::HandleNameEvent(
    std::uint32_t*, std::uint32_t, std::uint32_t) {}
std::uint32_t ActorControlDatabaseRuntimeStub::UiFrameManager() {
    return 0u;
}
void ActorControlDatabaseRuntimeStub::ProcessUiEvents(
    std::uint32_t, std::uint32_t,
    std::array<std::uint32_t, 2>&) {}
std::uint32_t ActorControlDatabaseRuntimeStub::CreateProgressBar(
    std::uint32_t) { return 0u; }
void ActorControlDatabaseRuntimeStub::SetProgressBarVisible(
    std::uint32_t, bool) {}

std::uint32_t* ActorControlBase::Delete(
    std::uint32_t* object, std::uint32_t flags,
    IActorControlDatabaseRuntime& runtime) {
    object[0] = kVtable;
    if ((flags & 1u) != 0u)
        static_cast<void>(runtime.FreeAddress(object));
    return object;
}

void ActorControlBase::Construct(std::uint32_t* object) {
    object[0] = kVtable;
}

void ActorControlBase::RestoreVtable(std::uint32_t* object) {
    object[0] = kVtable;
}

std::uint32_t ActorControlBase::SetMember(
    std::uint32_t* object, std::uint32_t value) {
    object[2] = value;
    return value;
}

ActorControlDatabasePrefix& ActorControlDatabase::Construct(
    ActorControlDatabasePrefix& state,
    IActorControlDatabaseRuntime& runtime) {
    auto* const words = state.words.data();
    words[0] = ActorControlBase::kVtable;
    runtime.ConstructResourceManager(words + 35u);
    runtime.ConstructNameNode(words + 128u);
    words[28] = 0u;
    words[30] = 0u;
    words[31] = 1u;
    words[32] = 1u;
    words[33] = 0u;
    words[34] = 1u;
    words[127] = 0u;
    words[0] = kVtable;
    return state;
}

void ActorControlDatabase::Destruct(
    ActorControlDatabasePrefix& state,
    IActorControlDatabaseRuntime& runtime) {
    auto* const words = state.words.data();
    words[0] = kVtable;
    runtime.DestructNameNode(words + 128u);
    runtime.DestructResourceManager(words + 35u);
    words[0] = ActorControlBase::kVtable;
}

ActorControlDatabasePrefix* ActorControlDatabase::Destroy(
    ActorControlDatabasePrefix* state, std::uint32_t flags,
    IActorControlDatabaseRuntime& runtime) {
    Destruct(*state, runtime);
    if ((flags & 1u) != 0u)
        static_cast<void>(runtime.FreeAddress(state));
    return state;
}

std::uint32_t ActorControlDatabase::LoadDummyClump(
    ActorControlDatabasePrefix& state,
    IActorControlDatabaseRuntime& runtime) {
    ActorControlPathBuffer path{};
    runtime.ConcatenatePath(path, kPathFirstGlobal, kPathSecondGlobal);
    try {
        runtime.AppendPath(path, "JDumy\\");
        const auto game = runtime.GameInstance();
        runtime.LoadClump(state.words.data() + 35u,
                          "ACTCTRL-JDumy", path.data, "JDumy",
                          game + 0x1acu, 0u, 0u, 1u);
        state.words[28] = 0u;
        state.words[29] = 0u;
        state.words[30] = 0u;
        state.words[31] = 1u;
        state.words[32] = 1u;
        state.words[34] = 1u;
    } catch (...) {
        runtime.ReleasePath(path);
        throw;
    }
    runtime.ReleasePath(path);
    return 1u;
}

std::uint32_t ActorControlDatabase::UnloadClump(
    ActorControlDatabasePrefix& state, std::uint32_t event_context,
    IActorControlDatabaseRuntime& runtime) {
    auto* const words = state.words.data();
    const auto clump = words[40];
    if (clump != 0u && runtime.HAnimFrameId(clump) != 0u)
        runtime.RemoveClump(runtime.GameArenaManager(), clump);
    const auto game = runtime.GameInstance();
    runtime.CleanupClump(words + 35u, game + 0x1acu);
    if (runtime.NameNodeNeedsUpdate(words + 128u))
        runtime.HandleNameEvent(words + 128u, event_context, 1u);
    return 1u;
}

std::uint32_t ActorControlDatabase::ResetDialog(
    ActorControlDatabasePrefix& state, std::uint32_t event_context,
    std::uint32_t value, IActorControlDatabaseRuntime& runtime) {
    auto* const words = state.words.data();
    words[28] = 0u;
    words[30] = 0u;
    words[31] = 1u;
    words[127] = value;
    if (runtime.NameNodeNeedsUpdate(words + 128u))
        runtime.HandleNameEvent(words + 128u, event_context, 1u);
    return 1u;
}

std::uint32_t ActorControlDatabase::ResetWithUi(
    ActorControlDatabasePrefix& state, std::uint32_t event_context,
    std::uint32_t value, IActorControlDatabaseRuntime& runtime) {
    std::array<std::uint32_t, 2> scratch{};
    runtime.ProcessUiEvents(runtime.UiFrameManager(), 0u, scratch);
    runtime.SetProgressBarVisible(
        runtime.CreateProgressBar(runtime.UiFrameManager()), false);
    auto* const words = state.words.data();
    runtime.RemoveClump(runtime.GameArenaManager(), words[40]);
    words[28] = 0u;
    words[29] = 0u;
    words[31] = 1u;
    words[30] = 0u;
    words[127] = value;
    if (runtime.NameNodeNeedsUpdate(words + 128u))
        runtime.HandleNameEvent(words + 128u, event_context, 1u);
    return 1u;
}

std::uint32_t ActorControlDatabase::GetState28(
    const ActorControlDatabasePrefix& state) {
    return state.words[28];
}

}  // namespace pal4
