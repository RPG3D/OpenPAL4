#include "pal4/pal_actor_control_blend_tail_config.h"

namespace pal4 {

void IActorControlXmlFacetRuntime::Free(ActorControlXmlWriteFacet*) {}

void ActorControlXmlWriteFacet::ResetVtable() {
    words[0u] = 0x8426acu;
}

ActorControlXmlWriteFacet* ActorControlXmlWriteFacet::Delete(
    const std::uint32_t flags, IActorControlXmlFacetRuntime& runtime) {
    ResetVtable();
    if ((flags & 1u) != 0u) {
        runtime.Free(this);
    }
    return this;
}

void IActorControlBlendTailConfigRuntime::InitializeDocument(
    ActorControlXmlDocument&, std::string_view, std::uint32_t) {}
void IActorControlBlendTailConfigRuntime::SetErrorFlag(
    ActorControlXmlAttribute&, bool) {}
void IActorControlBlendTailConfigRuntime::SetExceptionMessage(
    ActorControlXmlAttribute&, std::string_view) {}
std::uint32_t IActorControlBlendTailConfigRuntime::FindAttribute(
    ActorControlXmlLookup& lookup, const ActorControlXmlAttribute&) {
    return lookup.words[1u];
}
void IActorControlBlendTailConfigRuntime::SetAttributeInt(
    std::uint32_t, const std::uint32_t*) {}
std::uint32_t IActorControlBlendTailConfigRuntime::InitializeAttribute(
    ActorControlXmlAttribute&, std::string_view,
    const std::uint32_t*) { return 0u; }
void IActorControlBlendTailConfigRuntime::AddAttribute(
    ActorControlXmlDocument&, std::uint32_t) {}
void IActorControlBlendTailConfigRuntime::DestroyAttribute(
    ActorControlXmlAttribute&) {}
void IActorControlBlendTailConfigRuntime::InitializeWriteFacet(
    ActorControlXmlWriteFacet&, std::uint32_t) {}
std::uint8_t IActorControlBlendTailConfigRuntime::WriteConfig(
    ActorControlXmlDocument&, ActorControlXmlWriteFacet&) { return 0u; }
void IActorControlBlendTailConfigRuntime::ResetWriteFacetVtable(
    ActorControlXmlWriteFacet& facet, std::uint32_t vtable) {
    (void)vtable;
    facet.ResetVtable();
}
void IActorControlBlendTailConfigRuntime::DestroyDocument(
    ActorControlXmlDocument&) {}

std::uint8_t PalActorControlBlendTailConfig::Write(
    const PalActorControlState& state, const std::uint32_t argument,
    IActorControlBlendTailConfigRuntime& runtime) {
    ActorControlXmlDocument document{};
    ActorControlXmlAttribute attribute{};
    ActorControlXmlLookup lookup{};
    ActorControlXmlWriteFacet facet{};
    const auto* value = state.words.data() + 3u;
    constexpr std::string_view kName = "m_isNeedBlendTailAngle";
    bool document_ready = false;
    bool key_ready = false;
    bool attribute_ready = false;
    bool facet_ready = false;

    try {
        runtime.InitializeDocument(document, "Actor-Controller", 0u);
        document_ready = true;
        attribute.words[0u] = argument & 0xffu;
        runtime.SetErrorFlag(attribute, false);
        key_ready = true;
        runtime.SetExceptionMessage(attribute, kName);
        const auto found = runtime.FindAttribute(lookup, attribute);
        runtime.SetErrorFlag(attribute, true);
        key_ready = false;

        if (found == lookup.words[1u] || found + 0x1cu == 0u) {
            const auto added = runtime.InitializeAttribute(
                attribute, kName, value);
            attribute_ready = true;
            runtime.AddAttribute(document, added);
            runtime.DestroyAttribute(attribute);
            attribute_ready = false;
        } else {
            runtime.SetAttributeInt(found + 0x1cu, value);
        }

        runtime.InitializeWriteFacet(facet, argument);
        facet_ready = true;
        const auto result = runtime.WriteConfig(document, facet);
        runtime.ResetWriteFacetVtable(facet, 0x8426acu);
        facet_ready = false;
        runtime.DestroyDocument(document);
        document_ready = false;
        return result;
    } catch (...) {
        if (facet_ready) {
            runtime.ResetWriteFacetVtable(facet, 0x8426acu);
        }
        if (attribute_ready) {
            runtime.DestroyAttribute(attribute);
        } else if (key_ready) {
            runtime.SetErrorFlag(attribute, true);
        }
        if (document_ready) {
            runtime.DestroyDocument(document);
        }
        throw;
    }
}

}  // namespace pal4
