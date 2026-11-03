#include "pal4/game_config_loader.h"

#include "pal4/range_no_op_callback.h"

#include <bit>
#include <cstdlib>
#include <utility>

namespace pal4 {

void GameConfigLoadRuntimeStub::SetErrorFlag(GlobalSystemText&,
                                             std::uint8_t) {}
void GameConfigLoadRuntimeStub::FillMarker(std::uint32_t,
                                           GlobalSystemText&) {}
std::int32_t GameConfigLoadRuntimeStub::CompareMarker(
    const GlobalSystemText&, std::string_view) { return 1; }
void GameConfigLoadRuntimeStub::InitializeDocument(
    GameConfigXmlDocument&, std::string_view, std::uint32_t) {}
void GameConfigLoadRuntimeStub::InitializeFacet(
    ActorControlXmlWriteFacet& facet, std::uint32_t) {
    facet.ResetVtable();
}
void GameConfigLoadRuntimeStub::ReadConfig(
    ActorControlXmlWriteFacet&, GameConfigXmlDocument&) {}
std::uint32_t GameConfigLoadRuntimeStub::GameDataCollection() { return 0u; }
std::uint32_t GameConfigLoadRuntimeStub::InitializeGlobalCollection() {
    return GlobalSystem::kCollectionAddress;
}
void GameConfigLoadRuntimeStub::SetProperties(
    std::uint32_t, std::uint32_t, GameConfigXmlDocument&) {}
std::uint32_t GameConfigLoadRuntimeStub::FindAttribute(
    GameConfigXmlDocument&, std::string_view) { return 0u; }
std::int32_t GameConfigLoadRuntimeStub::AttributeToInt(
    std::uint32_t, std::int32_t& initial_value) {
    return initial_value;
}
void GameConfigLoadRuntimeStub::SetPlayerProperty(
    std::uint32_t, std::uint32_t, std::int32_t) {}
void GameConfigLoadRuntimeStub::DeserializeHostServer(
    std::uint32_t, std::uint32_t) {}
std::uint32_t GameConfigLoadRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void GameConfigLoadRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t GameConfigLoadRuntimeStub::FindChild(
    GameConfigXmlDocument&, std::string_view) { return 0u; }
std::uint32_t GameConfigLoadRuntimeStub::ChildByIndex(
    std::uint32_t, std::int32_t&) { return 0u; }
const char* GameConfigLoadRuntimeStub::ChildText(std::uint32_t) {
    return nullptr;
}
void GameConfigLoadRuntimeStub::InsertForbiddenSkill(
    std::uint32_t, std::uint32_t, std::uint32_t, std::int32_t) {}
void GameConfigLoadRuntimeStub::ProcessPlayerCollections(
    std::uint32_t) {}
void GameConfigLoadRuntimeStub::FinalizePlayerScript(
    std::uint32_t) {}
void GameConfigLoadRuntimeStub::ProcessScript(std::uint32_t) {}
void GameConfigLoadRuntimeStub::DestroyDocument(
    GameConfigXmlDocument&) {}
void GameConfigLoadRuntimeStub::ReleaseText(GlobalSystemText&) {}

std::uint8_t GameConfigLoader::Load(
    std::uint32_t object, std::uint32_t stream,
    std::uint8_t incoming_stack_byte,
    IGameConfigLoadRuntime& runtime) {
    GlobalSystemText marker{};
    marker.marker = incoming_stack_byte;
    runtime.SetErrorFlag(marker, 0u);
    bool player_ready = false;
    bool basic_ready = false;
    bool facet_ready = false;
    GameConfigXmlDocument player{};
    GameConfigXmlDocument basic{};
    ActorControlXmlWriteFacet facet{};

    try {
        runtime.FillMarker(stream, marker);
        if (runtime.CompareMarker(marker, "PR_SERIALIZE_FLAG") != 0) {
            runtime.SetErrorFlag(marker, 1u);
            return 0u;
        }

        runtime.InitializeDocument(player, "Player", 0u);
        player_ready = true;
        runtime.InitializeDocument(basic, "BASIC", 0u);
        basic_ready = true;
        runtime.InitializeFacet(facet, stream);
        facet_ready = true;
        runtime.ReadConfig(facet, basic);
        runtime.ReadConfig(facet, player);
        runtime.SetProperties(runtime.GameDataCollection(),
                              object + 0x658u, basic);
        runtime.SetProperties(runtime.InitializeGlobalCollection(),
                              object, player);

        for (const auto [name, property] :
             {std::pair{"CA_HP", 7u}, std::pair{"CA_MP", 9u}}) {
            std::int32_t value = 0;
            const auto attribute = runtime.FindAttribute(player, name);
            if (attribute != 0u)
                value = runtime.AttributeToInt(attribute, value);
            runtime.SetPlayerProperty(object + 0x654u, property, value);
        }
        runtime.DeserializeHostServer(object + 0x938u, stream);

        const auto begin = runtime.ReadDword(object + 0x9ecu);
        const auto end = runtime.ReadDword(object + 0x9f0u);
        RangeNoOpCallback::Invoke(begin, end);
        runtime.WriteDword(object + 0x9f0u, begin);
        const auto forbidden = runtime.FindChild(player, "ForbidenSkill");
        if (forbidden != 0u) {
            const auto count = std::bit_cast<std::int32_t>(
                runtime.ReadDword(forbidden + 0x38u));
            for (std::int32_t index = 0; index < count; ++index) {
                const auto child = runtime.ChildByIndex(forbidden, index);
                const auto* text = runtime.ChildText(child);
                const auto value = std::atoi(text == nullptr ? "" : text);
                runtime.InsertForbiddenSkill(object + 0x9e8u,
                                             runtime.ReadDword(object + 0x9f0u),
                                             1u, value);
            }
        }

        runtime.ProcessPlayerCollections(object);
        runtime.FinalizePlayerScript(object + 0x654u);
        runtime.ProcessScript(object);
        facet.ResetVtable();
        facet_ready = false;
        runtime.DestroyDocument(basic);
        basic_ready = false;
        runtime.DestroyDocument(player);
        player_ready = false;
        runtime.ReleaseText(marker);
        return 1u;
    } catch (...) {
        if (facet_ready)
            facet.ResetVtable();
        if (basic_ready)
            runtime.DestroyDocument(basic);
        if (player_ready)
            runtime.DestroyDocument(player);
        runtime.ReleaseText(marker);
        throw;
    }
}

}  // namespace pal4
