#include "pal4/game_vector_math.h"
#include "pal4/pal_actor_control_interaction.h"

#include <cmath>
#include <limits>

namespace pal4 {

ActorInteractionInput IActorControlInteractionRuntime::Input() { return {}; }
ActorInteractionPoint IActorControlInteractionRuntime::CursorClient() {
    return {};
}
ActorInteractionRay IActorControlInteractionRuntime::PixelRay(
    std::uint32_t, ActorInteractionPoint) { return {}; }
std::uint32_t IActorControlInteractionRuntime::NpcByRay(
    const ActorInteractionRay&) { return 0u; }
std::uint32_t IActorControlInteractionRuntime::DialogByRay(
    const ActorInteractionRay&) { return 0u; }
std::uint32_t IActorControlInteractionRuntime::EffectByRay(
    std::uint32_t, ActorInteractionPoint) { return 0u; }
std::uint32_t IActorControlInteractionRuntime::NpcByPosition(
    std::uint32_t) { return 0u; }
std::uint32_t IActorControlInteractionRuntime::DialogByPosition(
    std::uint32_t, float) { return 0u; }
std::vector<std::uint32_t>
IActorControlInteractionRuntime::EffectCandidates() { return {}; }
std::uint32_t IActorControlInteractionRuntime::ReadWord(
    std::uint32_t) { return 0u; }
float IActorControlInteractionRuntime::ReadFloat(std::uint32_t) {
    return 0.0f;
}
double IActorControlInteractionRuntime::ActorProperty(std::uint32_t) {
    return 0.0;
}
ActorControlEffectPoint IActorControlInteractionRuntime::EffectVertex(
    std::uint32_t, std::int32_t) { return {}; }
float IActorControlInteractionRuntime::PlanarMagnitude(
    ActorControlEffectPoint offset) {
    return static_cast<float>(GameVectorMath::ApproximateMagnitude(
        {offset.x,0.0f,offset.z}));
}
double IActorControlInteractionRuntime::SpatialMagnitude(
    ActorControlEffectPoint offset) {
    return GameVectorMath::ApproximateMagnitude({offset.x,offset.y,offset.z});
}
void IActorControlInteractionRuntime::SetCursor(std::string_view) {}
double IActorControlInteractionRuntime::NpcDistance(
    PalActorControlState& state, std::uint32_t npc_position) {
    return PalActorControlInteraction::DistanceToNpc(
        state, npc_position, *this);
}
void IActorControlInteractionRuntime::ExecuteScript(std::uint32_t script) {
    static_cast<void>(GameSectionExecutor::Execute(ReadCString(script), *this));
}
std::string IActorControlInteractionRuntime::ReadCString(
    std::uint32_t) { return {}; }
bool IActorControlInteractionRuntime::DialogEnabled(std::uint32_t) {
    return false;
}
bool IActorControlInteractionRuntime::DialogInRange(
    std::uint32_t, std::uint32_t, float) { return false; }
bool IActorControlInteractionRuntime::ProcessDialogEvent(
    PalActorControlState& state, std::uint32_t dialog) {
    return PalActorControlGameEvent::Process(state, dialog, *this);
}
void IActorControlInteractionRuntime::AttachDialog(
    std::uint32_t, std::uint32_t) {}
void IActorControlInteractionRuntime::TriggerEffect(std::uint32_t) {}

namespace {

bool SecondaryClick(IActorControlInteractionRuntime& runtime) {
    const auto input = runtime.Input();
    return input.secondary_count == 1 &&
           (input.secondary_mode & 0xfu) == 1u;
}

bool Click(IActorControlInteractionRuntime& runtime) {
    const auto primary = runtime.Input();
    if (primary.primary_count == 1 &&
        (primary.primary_mode & 0xfu) == 1u)
        return true;
    return SecondaryClick(runtime);
}

bool NpcAvailable(std::uint32_t npc,
                  IActorControlInteractionRuntime& runtime) {
    if (npc == 0u || runtime.ReadWord(npc + 0x1f0u) == 0u)
        return false;
    const auto data = runtime.ReadWord(npc + 0x654u);
    return data != 0u && runtime.ReadWord(data + 64u) != 0u;
}

void StartNpcScript(std::uint32_t npc,
                    IActorControlInteractionRuntime& runtime) {
    const auto data = runtime.ReadWord(npc + 0x654u);
    if (static_cast<std::int32_t>(runtime.ReadWord(data + 28u)) > 0)
        runtime.ExecuteScript(runtime.ReadWord(data + 24u));
}

void CommitDialog(PalActorControlState& state, std::uint32_t dialog,
                  IActorControlInteractionRuntime& runtime) {
    if (runtime.ProcessDialogEvent(state, dialog)) {
        state.words[21] = dialog;
        runtime.AttachDialog(dialog, state.words[1] + 172u);
    }
}

ActorControlEffectPoint ActorPosition(std::uint32_t actor,
                                      IActorControlInteractionRuntime& runtime) {
    return {runtime.ReadFloat(actor + 172u),
            runtime.ReadFloat(actor + 176u),
            runtime.ReadFloat(actor + 180u)};
}

float ClosestEffectVertex(std::uint32_t effect, std::uint32_t actor,
                          IActorControlInteractionRuntime& runtime) {
    float best = std::numeric_limits<float>::max();
    const auto begin = runtime.ReadWord(effect + 84u);
    if (begin == 0u) return best;
    const auto span = static_cast<std::int32_t>(
        runtime.ReadWord(effect + 88u) - begin);
    const auto count = span / 80;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto vertex = runtime.EffectVertex(effect, index);
        const auto actor_position = ActorPosition(actor, runtime);
        if (runtime.ActorProperty(actor) <
            std::fabs(static_cast<double>(vertex.y - actor_position.y)))
            continue;
        const ActorControlEffectPoint planar{
            vertex.x - actor_position.x, 0.0f,
            vertex.z - actor_position.z};
        const auto distance = runtime.PlanarMagnitude(planar);
        if (best > distance) best = distance;
    }
    return best;
}

void TriggerEffect(std::uint32_t effect,
                   IActorControlInteractionRuntime& runtime) {
    const auto action = runtime.ReadWord(effect + 64u);
    if (action != 0u) runtime.TriggerEffect(action);
}

class EffectSelectorAdapter final : public IActorControlEffectSelectorRuntime {
public:
    EffectSelectorAdapter(PalActorControlState& state,
                          std::uint32_t address,
                          IActorControlInteractionRuntime& runtime)
        : state_(state), address_(address), runtime_(runtime) {}

    ActorControlEffectPoint Vertex(
        ActorControlEffectObject&, std::int32_t index) override {
        return runtime_.EffectVertex(address_, index);
    }
    ActorControlEffectPoint ActorPosition() override {
        return ::pal4::ActorPosition(state_.words[1], runtime_);
    }
    float VerticalTolerance() override {
        return static_cast<float>(runtime_.ActorProperty(state_.words[1]));
    }
    float Magnitude(ActorControlEffectPoint offset) override {
        return runtime_.PlanarMagnitude(offset);
    }
    void SelectEffect(ActorControlEffectObject&) override {
        state_.words[22] = address_;
    }

private:
    PalActorControlState& state_;
    std::uint32_t address_;
    IActorControlInteractionRuntime& runtime_;
};

void SearchEffectList(PalActorControlState& state, float& best,
                      IActorControlInteractionRuntime& runtime) {
    for (const auto address : runtime.EffectCandidates()) {
        ActorControlEffectObject effect{};
        for (std::uint32_t index = 0; index < effect.words.size(); ++index)
            effect.words[index] = runtime.ReadWord(address + index * 4u);
        EffectSelectorAdapter adapter(state, address, runtime);
        static_cast<void>(PalActorControlEffectSelector::Visit(
            effect, best, adapter));
    }
}

}  // namespace

double PalActorControlInteraction::DistanceToNpc(
    PalActorControlState& state, std::uint32_t npc_position,
    IActorControlInteractionRuntime& runtime) {
    const auto actor = state.words[1];
    if (actor == 0u || npc_position == 0u)
        return static_cast<double>(std::numeric_limits<float>::max());
    const ActorControlEffectPoint offset{
        runtime.ReadFloat(npc_position) - runtime.ReadFloat(actor + 172u),
        runtime.ReadFloat(npc_position + 4u) -
            runtime.ReadFloat(actor + 176u),
        runtime.ReadFloat(npc_position + 8u) -
            runtime.ReadFloat(actor + 180u)};
    return runtime.SpatialMagnitude(offset);
}

bool PalActorControlInteraction::Handle(
    PalActorControlState& state, std::uint32_t camera_internal,
    IActorControlInteractionRuntime& runtime) {
    auto& words = state.words;
    const auto actor = words[1];
    if (actor == 0u) return false;
    const auto blocking = runtime.Input();
    if (blocking.blocking_count > 0 &&
        (blocking.blocking_mode & 0xfu) != 0u)
        return false;

    if (words[20] != 0u || words[21] != 0u || words[22] != 0u) {
        const auto primary = runtime.Input();
        if ((primary.primary_count == 0 &&
             primary.primary_release != 0u) ||
            ([&runtime] {
                const auto secondary = runtime.Input();
                return secondary.secondary_count == 0 &&
                       secondary.secondary_release != 0u;
            }())) {
            words[20] = words[21] = words[22] = 0u;
        }
        return true;
    }

    const auto point = runtime.CursorClient();
    const auto ray = runtime.PixelRay(camera_internal, point);
    const auto npc = runtime.NpcByRay(ray);
    const auto dialog = runtime.DialogByRay(ray);
    const auto effect = runtime.EffectByRay(camera_internal, point);
    const bool npc_available = NpcAvailable(npc, runtime);
    const bool dialog_available = dialog != 0u &&
        runtime.ReadWord(dialog + 0x184u) != 0u &&
        runtime.DialogEnabled(dialog);
    const bool effect_available = effect != 0u &&
        runtime.ReadWord(effect + 4u) == 5u;

    if (npc_available) {
        runtime.SetCursor("talk.ani");
        if (Click(runtime)) {
            const auto data = runtime.ReadWord(npc + 0x654u);
            if (runtime.NpcDistance(state, npc + 172u) <
                runtime.ReadFloat(data + 68u)) {
                words[20] = npc;
                StartNpcScript(npc, runtime);
                return true;
            }
        }
    } else {
        runtime.SetCursor(dialog_available || effect_available
                              ? "pickup.ani" : "default.cur");
        if (SecondaryClick(runtime)) {
            const auto nearby = runtime.NpcByPosition(actor + 172u);
            if (nearby != 0u &&
                runtime.ReadWord(runtime.ReadWord(nearby + 0x654u) + 64u) != 0u) {
                words[20] = nearby;
                StartNpcScript(nearby, runtime);
                return true;
            }
        }
    }

    if (dialog_available) {
        if (Click(runtime) && runtime.DialogInRange(
                dialog, actor + 172u,
                static_cast<float>(runtime.ActorProperty(actor)))) {
            CommitDialog(state, dialog, runtime);
            return true;
        }
    } else if (SecondaryClick(runtime)) {
        const auto nearby = runtime.DialogByPosition(
            actor + 172u,
            static_cast<float>(runtime.ActorProperty(actor)));
        if (nearby != 0u && runtime.DialogEnabled(nearby)) {
            CommitDialog(state, nearby, runtime);
            return true;
        }
    }

    if (effect_available) {
        if (!Click(runtime)) return false;
        const auto best = ClosestEffectVertex(effect, actor, runtime);
        if (!(runtime.ReadFloat(actor + 236u) * 2.0f > best))
            return false;
        words[22] = effect;
        TriggerEffect(effect, runtime);
        return true;
    }
    if (!SecondaryClick(runtime)) return false;
    float best = runtime.ReadFloat(actor + 236u) * 2.0f;
    SearchEffectList(state, best, runtime);
    if (words[22] == 0u) return false;
    TriggerEffect(words[22], runtime);
    return true;
}

}  // namespace pal4
