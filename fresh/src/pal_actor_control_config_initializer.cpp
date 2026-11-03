#include "pal4/pal_actor_control_config_initializer.h"

namespace pal4 {

void IActorControlConfigInitRuntime::InitializeDocument(
    ActorControlXmlConfigDocument&, std::string_view, std::uint32_t) {}
void IActorControlConfigInitRuntime::InitializeFacet(
    ActorControlXmlWriteFacet&, std::uint32_t) {}
bool IActorControlConfigInitRuntime::CheckVersionAndType(
    ActorControlXmlWriteFacet&, ActorControlXmlConfigDocument&) {
    return false;
}
std::uint32_t IActorControlConfigInitRuntime::FindAttribute(
    ActorControlXmlConfigDocument&, std::string_view) { return 0; }
std::uint32_t IActorControlConfigInitRuntime::AttributeToInt(
    std::uint32_t, const std::uint32_t* default_value) {
    return *default_value;
}
std::uint32_t IActorControlConfigInitRuntime::ReadWord(std::uint32_t) {
    return 0;
}
std::uint32_t IActorControlConfigInitRuntime::MinimapManager() { return 0; }
void IActorControlConfigInitRuntime::AddIcon(
    std::uint32_t, std::string_view, std::uint32_t, std::string_view,
    std::uint32_t, std::uint32_t) {}
void IActorControlConfigInitRuntime::SetActiveIcon(
    std::uint32_t, std::string_view, std::uint32_t) {}
void IActorControlConfigInitRuntime::UpdateGrid(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
long double IActorControlConfigInitRuntime::ActorScale(std::uint32_t) {
    return 0.0L;
}
std::uint32_t IActorControlConfigInitRuntime::MovementAdapter() { return 0; }
void IActorControlConfigInitRuntime::UpdateMovement(
    std::uint32_t, std::uint32_t, float, float, std::uint32_t) {}
std::uint32_t IActorControlConfigInitRuntime::FloorAttributeManager() {
    return 0;
}
void IActorControlConfigInitRuntime::UpdateFloorAttributes(std::uint32_t) {}
std::uint32_t IActorControlConfigInitRuntime::CameraSubsystem() { return 0; }
std::uint32_t IActorControlConfigInitRuntime::ActiveCamera(
    std::uint32_t) { return 0; }
ActorControlAudioPoint IActorControlConfigInitRuntime::ReadPoint(
    std::uint32_t) { return {}; }
std::uint32_t IActorControlConfigInitRuntime::AudioManager() { return 0; }
std::uint32_t IActorControlConfigInitRuntime::AudioDriver(
    std::uint32_t manager) { return ReadWord(manager + 0x1cu); }
void IActorControlConfigInitRuntime::SetAudioPosition(
    std::uint32_t, const ActorControlAudioPoint&) {}
void IActorControlConfigInitRuntime::SetAudioOrientation(
    std::uint32_t, const ActorControlAudioPoint&,
    const ActorControlAudioPoint&) {}
void IActorControlConfigInitRuntime::ProcessInput(
    PalActorControlState&) {}
void IActorControlConfigInitRuntime::ProcessInputAndClearVector(
    PalActorControlState&) {}
std::uint32_t IActorControlConfigInitRuntime::ControllerAccess() { return 0; }
void IActorControlConfigInitRuntime::ResetAndSync(
    std::uint32_t, std::uint32_t) {}
void IActorControlConfigInitRuntime::ResetFacetVtable(
    ActorControlXmlWriteFacet& facet) { facet.ResetVtable(); }
void IActorControlConfigInitRuntime::DestroyDocument(
    ActorControlXmlConfigDocument&) {}

std::uint8_t PalActorControlConfigInitializer::Initialize(
    PalActorControlState& state, const std::uint32_t argument,
    IActorControlConfigInitRuntime& runtime) {
    ActorControlXmlConfigDocument document{};
    ActorControlXmlWriteFacet facet{};
    bool document_ready = false;
    bool facet_ready = false;
    try {
        runtime.InitializeDocument(document, "Actor-Controller", 0u);
        document_ready = true;
        runtime.InitializeFacet(facet, argument);
        facet_ready = true;
        if (!runtime.CheckVersionAndType(facet, document)) {
            runtime.ResetFacetVtable(facet);
            facet_ready = false;
            runtime.DestroyDocument(document);
            document_ready = false;
            return 0u;
        }

        std::uint32_t blend_tail_angle = 1u;
        const auto attribute = runtime.FindAttribute(
            document, "m_isNeedBlendTailAngle");
        if (attribute != 0u) {
            blend_tail_angle = runtime.AttributeToInt(
                attribute, &blend_tail_angle);
        }
        state.words[3u] = blend_tail_angle;

        const auto actor = state.words[1u];
        if (actor != 0u && runtime.ReadWord(actor + 0x1f0u) != 0u) {
            runtime.AddIcon(runtime.MinimapManager(), "LeadingRole", 0u,
                            "actor.png", 0u, 0u);
            runtime.SetActiveIcon(runtime.MinimapManager(),
                                  "LeadingRole", 0u);
            runtime.UpdateGrid(runtime.MinimapManager(), actor + 0xacu,
                               actor + 0xa0u);

            const auto first_scale = static_cast<float>(
                runtime.ActorScale(actor) *
                static_cast<long double>(0.34999999f));
            const auto second_scale = static_cast<float>(
                runtime.ActorScale(actor) *
                static_cast<long double>(0.52999997f));
            runtime.UpdateMovement(runtime.MovementAdapter(), actor + 0xacu,
                                   first_scale, second_scale, 1u);
            runtime.UpdateFloorAttributes(runtime.FloorAttributeManager());

            const auto first_camera = runtime.ActiveCamera(
                runtime.CameraSubsystem());
            const auto first_point = runtime.ReadPoint(first_camera + 0x74u);
            const auto first_audio = runtime.AudioDriver(
                runtime.AudioManager());
            runtime.SetAudioPosition(first_audio,
                                     {first_point[0], first_point[1],
                                      -first_point[2]});

            const auto second_camera = runtime.ActiveCamera(
                runtime.CameraSubsystem());
            const auto second_point = runtime.ReadPoint(second_camera + 0x50u);
            const auto second_audio = runtime.AudioDriver(
                runtime.AudioManager());
            runtime.SetAudioOrientation(
                second_audio,
                {second_point[0], second_point[1], -second_point[2]},
                {0.0f, 1.0f, -0.0f});
        }

        runtime.ProcessInput(state);
        runtime.ProcessInputAndClearVector(state);
        runtime.ResetAndSync(runtime.ControllerAccess(), 1u);
        runtime.ResetFacetVtable(facet);
        facet_ready = false;
        runtime.DestroyDocument(document);
        document_ready = false;
        return 1u;
    } catch (...) {
        if (facet_ready) {
            runtime.ResetFacetVtable(facet);
        }
        if (document_ready) {
            runtime.DestroyDocument(document);
        }
        throw;
    }
}

}  // namespace pal4
