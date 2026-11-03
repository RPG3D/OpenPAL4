#include "pal4/ai_action_task.h"

namespace pal4 {

std::uint32_t AiActionLocaleFacet::Construct(
    const std::uint32_t facet, const std::uint32_t references,
    IAiActionLifecycleRuntime& runtime) {
    runtime.WriteWord(facet, kVtable);
    runtime.WriteWord(facet + 4u, references);
    return facet;
}

std::uint32_t AiActionDerivedLocaleFacet::Construct(
    const std::uint32_t facet, const std::uint32_t references,
    IAiActionLifecycleRuntime& runtime) {
    (void)AiActionLocaleFacet::Construct(facet, references, runtime);
    runtime.WriteWord(facet, kVtable);
    return facet;
}

void AiActionDerivedLocaleFacet::Destruct(
    const std::uint32_t facet, IAiActionLifecycleRuntime& runtime) {
    runtime.WriteWord(facet, kVtable);
    runtime.WriteWord(facet, AiActionLocaleFacet::kVtable);
}

std::uint32_t AiActionSecondaryLocaleFacet::Construct(
    const std::uint32_t facet, const std::uint32_t references,
    IAiActionLifecycleRuntime& runtime) {
    (void)AiActionLocaleFacet::Construct(facet, references, runtime);
    runtime.WriteWord(facet, kVtable);
    return facet;
}

void AiActionSecondaryLocaleFacet::Destruct(
    const std::uint32_t facet, IAiActionLifecycleRuntime& runtime) {
    runtime.WriteWord(facet, kVtable);
    runtime.WriteWord(facet, AiActionLocaleFacet::kVtable);
}

bool AiActionTask::Process(
    const std::uint32_t task, IAiActionTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 4u);
    if (actor == 0u) return false;
    runtime.PrepareActor(actor);
    const auto target = runtime.SelectTarget(task);
    if (target == 0u) return false;

    const auto roll = static_cast<float>(
        static_cast<double>(runtime.RandomNumber() % 1000u) *
        static_cast<double>(0.001f));
    double cumulative = 0.0;
    for (std::uint32_t index = 0u; index < kBranches.size(); ++index) {
        const auto& branch = kBranches[index];
        cumulative += runtime.ReadFloat(target + branch.weight_offset);
        const bool selected = index + 1u == kBranches.size()
                                  ? !(roll > cumulative)
                                  : roll <= cumulative;
        if (!selected) continue;

        const auto ai_type = static_cast<std::int32_t>(
            runtime.ReadWord(target + 12u));
        const auto state = static_cast<std::int32_t>(
            runtime.ReadWord(target + 8u));
        if ((ai_type >= 4 || state >= 8) &&
            (runtime.ReadByte(0x978b10u) & 1u) == 0u) {
            runtime.WriteByte(0x978b10u,
                              runtime.ReadByte(0x978b10u) | 1u);
            runtime.InitializeErrorString();
            runtime.RegisterErrorStringCleanup();
        }

        runtime.LogAction(task, "cbActorAi::takeAction",
                          branch.message, runtime.AnimationId(task));
        return runtime.PostEvent(runtime.DispatchAction(
            task, branch.virtual_slot, target));
    }
    return false;
}

}  // namespace pal4
