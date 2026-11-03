#pragma once

#include "pal4/pal_actor_control.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorControlXmlDocument {
    std::array<std::uint32_t, 5> words{};
};
struct ActorControlXmlAttribute {
    std::array<std::uint32_t, 7> words{};
};
struct ActorControlXmlLookup {
    std::array<std::uint32_t, 10> words{};
};
struct ActorControlXmlWriteFacet;
class IActorControlXmlFacetRuntime {
public:
    virtual ~IActorControlXmlFacetRuntime() = default;
    virtual void Free(ActorControlXmlWriteFacet* facet);
};
struct ActorControlXmlWriteFacet {
    std::array<std::uint32_t, 2> words{};
    // IDA 0x427150.
    void ResetVtable();
    // IDA 0x427460.
    [[nodiscard]] ActorControlXmlWriteFacet* Delete(
        std::uint32_t flags, IActorControlXmlFacetRuntime& runtime);
};

static_assert(sizeof(ActorControlXmlDocument) == 20u);
static_assert(sizeof(ActorControlXmlAttribute) == 28u);
static_assert(sizeof(ActorControlXmlLookup) == 40u);
static_assert(sizeof(ActorControlXmlWriteFacet) == 8u);

class IActorControlBlendTailConfigRuntime {
public:
    virtual ~IActorControlBlendTailConfigRuntime() = default;
    virtual void InitializeDocument(ActorControlXmlDocument& document,
                                    std::string_view root, std::uint32_t flag);
    virtual void SetErrorFlag(ActorControlXmlAttribute& attribute, bool flag);
    virtual void SetExceptionMessage(ActorControlXmlAttribute& attribute,
                                     std::string_view name);
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        ActorControlXmlLookup& lookup,
        const ActorControlXmlAttribute& attribute);
    virtual void SetAttributeInt(std::uint32_t value_field,
                                 const std::uint32_t* value);
    [[nodiscard]] virtual std::uint32_t InitializeAttribute(
        ActorControlXmlAttribute& attribute, std::string_view name,
        const std::uint32_t* value);
    virtual void AddAttribute(ActorControlXmlDocument& document,
                              std::uint32_t attribute);
    virtual void DestroyAttribute(ActorControlXmlAttribute& attribute);
    virtual void InitializeWriteFacet(ActorControlXmlWriteFacet& facet,
                                      std::uint32_t argument);
    [[nodiscard]] virtual std::uint8_t WriteConfig(
        ActorControlXmlDocument& document,
        ActorControlXmlWriteFacet& facet);
    virtual void ResetWriteFacetVtable(ActorControlXmlWriteFacet& facet,
                                       std::uint32_t vtable);
    virtual void DestroyDocument(ActorControlXmlDocument& document);
};

class PalActorControlBlendTailConfig final {
public:
    // IDA 0x427010. Only the low return byte of Config_Write is retained.
    [[nodiscard]] static std::uint8_t Write(
        const PalActorControlState& state, std::uint32_t argument,
        IActorControlBlendTailConfigRuntime& runtime);
};

}  // namespace pal4
