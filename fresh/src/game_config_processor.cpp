#include "pal4/game_config_processor.h"

#include <bit>
#include <string>

namespace pal4 {

void GameConfigProcessRuntimeStub::SetErrorFlag(GlobalSystemText&,
                                                std::uint8_t) {}
bool GameConfigProcessRuntimeStub::AllocateText(GlobalSystemText&,
                                                std::uint32_t,
                                                std::uint8_t) { return false; }
void GameConfigProcessRuntimeStub::CopyLiteral(std::uint32_t,
                                                std::string_view) {}
void GameConfigProcessRuntimeStub::WriteByte(std::uint32_t,
                                             std::uint8_t) {}
void GameConfigProcessRuntimeStub::CheckDataReady(
    std::uint32_t, const GlobalSystemText&) {}
void GameConfigProcessRuntimeStub::InitializeDocument(
    GameConfigXmlDocument&, std::string_view, std::uint32_t) {}
std::uint32_t GameConfigProcessRuntimeStub::GameDataCollection() {
    return 0u;
}
std::uint32_t GameConfigProcessRuntimeStub::InitializeGlobalCollection() {
    return GlobalSystem::kCollectionAddress;
}
void GameConfigProcessRuntimeStub::ForEach(
    std::uint32_t, std::uint32_t, GameConfigXmlDocument&) {}
std::uint32_t GameConfigProcessRuntimeStub::CreateNode(
    GameConfigXmlDocument&, std::string_view) { return 0u; }
std::uint32_t GameConfigProcessRuntimeStub::CreateChild(
    std::uint32_t, std::string_view) { return 0u; }
std::uint32_t GameConfigProcessRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void GameConfigProcessRuntimeStub::InitializeFacet(
    ActorControlXmlWriteFacet& facet, std::uint32_t) {
    facet.ResetVtable();
}
void GameConfigProcessRuntimeStub::WriteConfig(
    GameConfigXmlDocument&, ActorControlXmlWriteFacet&) {}
std::uint8_t GameConfigProcessRuntimeStub::SerializeHostServer(
    std::uint32_t, std::uint32_t) { return 1u; }
void GameConfigProcessRuntimeStub::DestroyDocument(
    GameConfigXmlDocument&) {}
void GameConfigProcessRuntimeStub::ReleaseText(GlobalSystemText&) {}

std::uint8_t GameConfigProcessor::Process(
    std::uint32_t object, std::uint32_t stream,
    std::uint8_t incoming_stack_byte,
    IGameConfigProcessRuntime& runtime) {
    GlobalSystemText marker{};
    marker.marker = incoming_stack_byte;
    runtime.SetErrorFlag(marker, 0u);
    constexpr std::string_view kMarker = "PR_SERIALIZE_FLAG";
    if (runtime.AllocateText(marker,
                             static_cast<std::uint32_t>(kMarker.size()), 1u)) {
        runtime.CopyLiteral(marker.buffer.data, kMarker);
        marker.buffer.length = static_cast<std::uint32_t>(kMarker.size());
        runtime.WriteByte(marker.buffer.data + marker.buffer.length, 0u);
    }

    GameConfigXmlDocument basic{};
    GameConfigXmlDocument player{};
    ActorControlXmlWriteFacet facet{};
    bool basic_ready = false;
    bool player_ready = false;
    bool facet_ready = false;
    try {
        runtime.CheckDataReady(stream, marker);
        runtime.InitializeDocument(basic, "BASIC", 0u);
        basic_ready = true;
        runtime.ForEach(runtime.GameDataCollection(), object + 0x658u,
                        basic);
        runtime.InitializeDocument(player, "Player", 0u);
        player_ready = true;
        runtime.ForEach(runtime.InitializeGlobalCollection(), object,
                        player);

        const auto forbidden = runtime.CreateNode(player, "ForbidenSkill");
        for (std::uint32_t index = 0u;; ++index) {
            const auto begin = runtime.ReadDword(object + 0x9ecu);
            if (begin == 0u)
                break;
            const auto end = runtime.ReadDword(object + 0x9f0u);
            const auto size = std::bit_cast<std::uint32_t>(
                std::bit_cast<std::int32_t>(end - begin) >> 2);
            if (index >= size)
                break;
            const auto value = std::bit_cast<std::int32_t>(
                runtime.ReadDword(begin + index * 4u));
            static_cast<void>(runtime.CreateChild(forbidden,
                                                  std::to_string(value)));
        }

        runtime.InitializeFacet(facet, stream);
        facet_ready = true;
        runtime.WriteConfig(basic, facet);
        runtime.WriteConfig(player, facet);
        const auto result = runtime.SerializeHostServer(object + 0x938u,
                                                        stream);
        facet.ResetVtable();
        facet_ready = false;
        runtime.DestroyDocument(player);
        player_ready = false;
        runtime.DestroyDocument(basic);
        basic_ready = false;
        runtime.ReleaseText(marker);
        return result;
    } catch (...) {
        if (facet_ready)
            facet.ResetVtable();
        if (player_ready)
            runtime.DestroyDocument(player);
        if (basic_ready)
            runtime.DestroyDocument(basic);
        runtime.ReleaseText(marker);
        throw;
    }
}

}  // namespace pal4
