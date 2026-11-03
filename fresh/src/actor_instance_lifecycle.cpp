#include "pal4/actor_instance_lifecycle.h"

#include <array>

namespace pal4 {
namespace {

void InitializeComponentData(const std::uint32_t component,
                             IActorInstanceRuntime& runtime) {
    runtime.InitializeComponent(component);
    runtime.WriteWord(component + 392u, 0);
    runtime.WriteWord(component + 388u, 0xffffffffu);
    runtime.WriteWord(component + 396u, 0xffffffffu);
    runtime.WriteWord(component + 400u, 0);
    runtime.InitializeNameNode(component + 404u);
    for (const auto offset : std::array{412u, 428u, 444u}) {
        // The original copies an uninitialized stack byte to each flag.
        runtime.WriteByte(component + offset, 0);
        runtime.SetTextFlag(component + offset, 0);
    }
    runtime.WriteWord(component, 0x84499cu);
}

}  // namespace

std::uint32_t ActorInstanceLifecycle::Initialize(
    const std::uint32_t actor, IActorInstanceRuntime& runtime) {
    runtime.InitializeResourceManager(actor);
    runtime.InitializeObjectState(actor + 368u);
    runtime.WriteWord(actor + 492u, 0);
    runtime.WriteByte(actor + 496u, 0);
    runtime.WriteWord(actor + 500u, 0);
    runtime.WriteWord(actor + 504u, 0);
    runtime.WriteWord(actor + 508u, 0);
    runtime.WriteWord(actor + 512u, 0x8449c8u);
    runtime.InitializeTga(actor + 524u);
    InitializeComponentData(actor + 624u, runtime);
    InitializeComponentData(actor + 1088u, runtime);
    runtime.WriteWord(actor, 0x844960u);
    runtime.WriteWord(actor + 368u, 0x844954u);
    return actor;
}

const char* ActorInstanceLifecycle::EmptyErrorString() {
    return "";
}

std::uint32_t ActorInstanceLifecycle::CleanupComponentText(
    const std::uint32_t component, IActorInstanceRuntime& runtime) {
    runtime.ReleaseText(component + 444u);
    runtime.ReleaseText(component + 428u);
    runtime.ReleaseText(component + 412u);
    runtime.ResetEffect(component + 404u);
    return runtime.DestroyComponent(component);
}

std::uint32_t ActorInstanceLifecycle::ResetFrame(
    const std::uint32_t actor, IActorInstanceRuntime& runtime) {
    runtime.ResetActorProperties(actor + 624u);
    runtime.ResetActorProperties(actor + 1088u);
    runtime.CleanupObjectData(actor + 368u);
    const auto game = runtime.GameInstance();
    return runtime.CleanupClump(actor, game + 428u);
}

std::uint32_t ActorInstanceLifecycle::SetFrameFlags(
    const std::uint32_t actor, const std::uint32_t value,
    IActorInstanceRuntime& runtime) {
    runtime.WriteWord(actor + 492u, value);
    runtime.WriteWord(actor + 516u, value);
    return value;
}

void ActorInstanceLifecycle::Destroy(
    const std::uint32_t actor, IActorInstanceRuntime& runtime) {
    runtime.WriteWord(actor, 0x844960u);
    runtime.WriteWord(actor + 368u, 0x844954u);
    (void)ResetFrame(actor, runtime);
    for (const auto component : std::array{actor + 1088u,
                                           actor + 624u}) {
        for (const auto offset : std::array{444u, 428u, 412u})
            runtime.SetTextFlag(component + offset, 1u);
        runtime.ResetEffect(component + 404u);
        (void)runtime.DestroyComponent(component);
    }
    runtime.WriteWord(actor + 512u, 0x8425a0u);
    runtime.ReleaseText(actor + 496u);
    runtime.CleanupObjectState(actor + 368u);
    runtime.DestroyResourceManager(actor);
}

}  // namespace pal4
