#pragma once

#include "pal4/game_config_processor.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameConfigLoadRuntime {
public:
    virtual ~IGameConfigLoadRuntime() = default;
    virtual void SetErrorFlag(GlobalSystemText& marker,
                              std::uint8_t flag) = 0;
    virtual void FillMarker(std::uint32_t stream,
                            GlobalSystemText& marker) = 0;
    [[nodiscard]] virtual std::int32_t CompareMarker(
        const GlobalSystemText& marker,
        std::string_view expected) = 0;
    virtual void InitializeDocument(GameConfigXmlDocument& document,
                                    std::string_view root,
                                    std::uint32_t parent) = 0;
    virtual void InitializeFacet(ActorControlXmlWriteFacet& facet,
                                 std::uint32_t stream) = 0;
    virtual void ReadConfig(ActorControlXmlWriteFacet& facet,
                            GameConfigXmlDocument& document) = 0;
    [[nodiscard]] virtual std::uint32_t GameDataCollection() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeGlobalCollection() = 0;
    virtual void SetProperties(std::uint32_t source,
                               std::uint32_t object_member,
                               GameConfigXmlDocument& document) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        GameConfigXmlDocument& document,
        std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t AttributeToInt(
        std::uint32_t attribute, std::int32_t& initial_value) = 0;
    virtual void SetPlayerProperty(std::uint32_t script,
                                   std::uint32_t index,
                                   std::int32_t value) = 0;
    virtual void DeserializeHostServer(std::uint32_t host_server,
                                       std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindChild(
        GameConfigXmlDocument& document,
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ChildByIndex(
        std::uint32_t parent, std::int32_t& index) = 0;
    [[nodiscard]] virtual const char* ChildText(
        std::uint32_t child) = 0;
    virtual void InsertForbiddenSkill(std::uint32_t array,
                                      std::uint32_t end,
                                      std::uint32_t mode,
                                      std::int32_t value) = 0;
    virtual void ProcessPlayerCollections(std::uint32_t object) = 0;
    virtual void FinalizePlayerScript(std::uint32_t script) = 0;
    virtual void ProcessScript(std::uint32_t object) = 0;
    virtual void DestroyDocument(GameConfigXmlDocument& document) = 0;
    virtual void ReleaseText(GlobalSystemText& marker) = 0;
};

class GameConfigLoadRuntimeStub : public IGameConfigLoadRuntime {
public:
    void SetErrorFlag(GlobalSystemText&, std::uint8_t) override;
    void FillMarker(std::uint32_t, GlobalSystemText&) override;
    std::int32_t CompareMarker(const GlobalSystemText&,
                               std::string_view) override;
    void InitializeDocument(GameConfigXmlDocument&,
                            std::string_view, std::uint32_t) override;
    void InitializeFacet(ActorControlXmlWriteFacet&,
                         std::uint32_t) override;
    void ReadConfig(ActorControlXmlWriteFacet&,
                    GameConfigXmlDocument&) override;
    std::uint32_t GameDataCollection() override;
    std::uint32_t InitializeGlobalCollection() override;
    void SetProperties(std::uint32_t, std::uint32_t,
                       GameConfigXmlDocument&) override;
    std::uint32_t FindAttribute(GameConfigXmlDocument&,
                                std::string_view) override;
    std::int32_t AttributeToInt(std::uint32_t,
                                std::int32_t&) override;
    void SetPlayerProperty(std::uint32_t, std::uint32_t,
                           std::int32_t) override;
    void DeserializeHostServer(std::uint32_t,
                               std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    std::uint32_t FindChild(GameConfigXmlDocument&,
                            std::string_view) override;
    std::uint32_t ChildByIndex(std::uint32_t,
                               std::int32_t&) override;
    const char* ChildText(std::uint32_t) override;
    void InsertForbiddenSkill(std::uint32_t, std::uint32_t,
                              std::uint32_t, std::int32_t) override;
    void ProcessPlayerCollections(std::uint32_t) override;
    void FinalizePlayerScript(std::uint32_t) override;
    void ProcessScript(std::uint32_t) override;
    void DestroyDocument(GameConfigXmlDocument&) override;
    void ReleaseText(GlobalSystemText&) override;
};

class GameConfigLoader final {
public:
    // IDA 0x43fe10.
    [[nodiscard]] static std::uint8_t Load(
        std::uint32_t object, std::uint32_t stream,
        std::uint8_t incoming_stack_byte,
        IGameConfigLoadRuntime& runtime);
};

}  // namespace pal4
