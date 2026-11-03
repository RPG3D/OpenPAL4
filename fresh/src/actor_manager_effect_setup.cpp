#include "pal4/actor_manager_effect_setup.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::uint32_t kBottomHandle = 868u;
constexpr std::uint32_t kLeftHandle = 876u;
constexpr std::uint32_t kRightHandle = 884u;

void StoreLoadedEffect(std::uint32_t address, ActorEffectHandle handle,
                       IActorManagerEffectLoadRuntime& runtime) {
    runtime.StoreEffect(address, handle);
    runtime.ResetTemporary(handle);
}

void LoadHand(std::uint32_t actor, std::uint32_t offset,
              std::string_view name, std::string_view hook,
              IActorManagerEffectLoadRuntime& runtime) {
    StoreLoadedEffect(actor + offset,
                      runtime.LoadNamedEffect(actor, name, hook), runtime);
}

}  // namespace

void ActorManagerEffectSetup::Initialize(
    std::uint32_t actor, std::uint32_t context,
    IActorManagerEffectSetupRuntime& runtime) {
    runtime.WriteByte(actor + 892u, 0u);
    runtime.WriteWord(actor + 832u, 0u);
    runtime.WriteWord(actor + 828u, 1u);
    runtime.InvokeActorSlot(actor, 24u, context);
    runtime.InvokeActorSlot(actor, 584u, 0u);
    runtime.InvokeActorSlot(actor, 276u, 0u);
    runtime.InvokeActorSlot(actor, 572u, context);
    runtime.InvokeActorSlot(actor, 396u, 1u);
    runtime.InvokeActorSlot(actor, 548u, 0u);
    RefreshAttachments(actor,runtime);
}
void ActorManagerEffectSetup::RefreshAttachments(std::uint32_t actor,IActorManagerEffectSetupRuntime& runtime) {
    for (const auto offset : std::array{kBottomHandle, kLeftHandle,
                                        kRightHandle}) {
        runtime.ResetEffect(actor, actor + offset);
    }

    const auto child = runtime.ReadWord(actor + 808u);
    if (child == 0u) return;
    LoadAttachments(actor, child, runtime);
}

void ActorManagerEffectSetup::LoadAttachments(
    const std::uint32_t actor, const std::uint32_t child,
    IActorManagerEffectLoadRuntime& runtime) {
    StoreLoadedEffect(actor + kBottomHandle,
                      runtime.LoadBottomEffect(actor, child + 496u,
                                               "hook_ef_bottom"), runtime);

    const auto model_id = runtime.ModelId(actor);
    if (model_id == 2598u || model_id == 2599u) {
        LoadHand(actor, kLeftHandle,
                 model_id == 2598u ? "H_B20" : "H_B21",
                 "hook_ef_bottom", runtime);
        return;
    }

    std::string_view resource;
    switch (model_id) {
        case 2592u: resource = "EFF20"; break;
        case 2603u: resource = "EFF21"; break;
        case 2604u: resource = "EFF22"; break;
        case 2605u: resource = "EFF23"; break;
        case 2606u: resource = "EFF24"; break;
        default: return;
    }
    LoadHand(actor, kLeftHandle, resource, "hook_ef_lhand", runtime);
    LoadHand(actor, kRightHandle, resource, "hook_ef_rhand", runtime);
}

}  // namespace pal4
