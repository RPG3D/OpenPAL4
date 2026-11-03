#pragma once

#include "pal4/global_system.h"
#include "pal4/pal_actor_control_blend_tail_config.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct GameConfigXmlDocument {
    std::array<std::uint32_t, 15> words{};
};
static_assert(sizeof(GameConfigXmlDocument) == 0x3cu);

class IGameConfigProcessRuntime {
public:
    virtual ~IGameConfigProcessRuntime() = default;
    virtual void SetErrorFlag(GlobalSystemText& text,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(GlobalSystemText& text,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void CopyLiteral(std::uint32_t destination,
                             std::string_view value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void CheckDataReady(std::uint32_t stream,
                                const GlobalSystemText& marker) = 0;
    virtual void InitializeDocument(GameConfigXmlDocument& document,
                                    std::string_view root,
                                    std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t GameDataCollection() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeGlobalCollection() = 0;
    virtual void ForEach(std::uint32_t source,
                         std::uint32_t object_member,
                         GameConfigXmlDocument& document) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(
        GameConfigXmlDocument& document, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChild(
        std::uint32_t parent, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void InitializeFacet(ActorControlXmlWriteFacet& facet,
                                 std::uint32_t stream) = 0;
    virtual void WriteConfig(GameConfigXmlDocument& document,
                             ActorControlXmlWriteFacet& facet) = 0;
    [[nodiscard]] virtual std::uint8_t SerializeHostServer(
        std::uint32_t host_server, std::uint32_t stream) = 0;
    virtual void DestroyDocument(GameConfigXmlDocument& document) = 0;
    virtual void ReleaseText(GlobalSystemText& text) = 0;
};

class GameConfigProcessRuntimeStub : public IGameConfigProcessRuntime {
public:
    void SetErrorFlag(GlobalSystemText&, std::uint8_t) override;
    bool AllocateText(GlobalSystemText&, std::uint32_t,
                      std::uint8_t) override;
    void CopyLiteral(std::uint32_t, std::string_view) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    void CheckDataReady(std::uint32_t,
                        const GlobalSystemText&) override;
    void InitializeDocument(GameConfigXmlDocument&, std::string_view,
                            std::uint32_t) override;
    std::uint32_t GameDataCollection() override;
    std::uint32_t InitializeGlobalCollection() override;
    void ForEach(std::uint32_t, std::uint32_t,
                 GameConfigXmlDocument&) override;
    std::uint32_t CreateNode(GameConfigXmlDocument&,
                             std::string_view) override;
    std::uint32_t CreateChild(std::uint32_t,
                              std::string_view) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void InitializeFacet(ActorControlXmlWriteFacet&,
                         std::uint32_t) override;
    void WriteConfig(GameConfigXmlDocument&,
                     ActorControlXmlWriteFacet&) override;
    std::uint8_t SerializeHostServer(std::uint32_t,
                                     std::uint32_t) override;
    void DestroyDocument(GameConfigXmlDocument&) override;
    void ReleaseText(GlobalSystemText&) override;
};

class GameConfigProcessor final {
public:
    // IDA 0x43fc10.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t object, std::uint32_t stream,
        std::uint8_t incoming_stack_byte,
        IGameConfigProcessRuntime& runtime);
};

}  // namespace pal4
