#include "pal4/pal_actor_control_blend_tail_config.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IActorControlBlendTailConfigRuntime {
    const pal4::PalActorControlState* state{};
    std::vector<std::string> calls;
    std::uint32_t found{0x1200u};
    std::uint32_t end{0x1300u};
    std::uint32_t argument{0x1234abcdu};
    bool throw_on_add{};
    bool throw_on_write{};

    void InitializeDocument(pal4::ActorControlXmlDocument& document,
                            std::string_view root,
                            std::uint32_t flag) override {
        Check(root == "Actor-Controller" && flag == 0u &&
                  document.words[0u] == 0u,
              "document root and initialization flag");
        calls.emplace_back("document");
    }
    void SetErrorFlag(pal4::ActorControlXmlAttribute& attribute,
                      bool flag) override {
        if (!flag) {
            Check((attribute.words[0u] & 0xffu) == (argument & 0xffu),
                  "key seed uses low argument byte");
        }
        calls.emplace_back(flag ? "flag_true" : "flag_false");
    }
    void SetExceptionMessage(pal4::ActorControlXmlAttribute&,
                             std::string_view name) override {
        Check(name == "m_isNeedBlendTailAngle" && name.size() == 22u,
              "blend-tail attribute name");
        calls.emplace_back("key_name");
    }
    std::uint32_t FindAttribute(pal4::ActorControlXmlLookup& lookup,
                                const pal4::ActorControlXmlAttribute&) override {
        lookup.words[1u] = end;
        calls.emplace_back("find"); return found;
    }
    void SetAttributeInt(std::uint32_t value_field,
                         const std::uint32_t* value) override {
        Check(value_field == found + 0x1cu &&
                  value == state->words.data() + 3u && *value == 1u,
              "existing attribute value offset and pointer");
        calls.emplace_back("set_int");
    }
    std::uint32_t InitializeAttribute(
        pal4::ActorControlXmlAttribute&, std::string_view name,
        const std::uint32_t* value) override {
        Check(name == "m_isNeedBlendTailAngle" &&
                  value == state->words.data() + 3u && *value == 1u,
              "new attribute name and value pointer");
        calls.emplace_back("new_attribute"); return 0xa000u;
    }
    void AddAttribute(pal4::ActorControlXmlDocument&,
                      std::uint32_t attribute) override {
        Check(attribute == 0xa000u, "new attribute handle");
        calls.emplace_back("add_attribute");
        if (throw_on_add) throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::ActorControlXmlAttribute&) override {
        calls.emplace_back("destroy_attribute");
    }
    void InitializeWriteFacet(pal4::ActorControlXmlWriteFacet& facet,
                              std::uint32_t value) override {
        Check(value == argument, "facet receives full argument");
        facet.words[0u] = 0x11111111u;
        calls.emplace_back("facet");
    }
    std::uint8_t WriteConfig(pal4::ActorControlXmlDocument&,
                             pal4::ActorControlXmlWriteFacet& facet) override {
        Check(facet.words[0u] == 0x11111111u,
              "write occurs before facet vtable reset");
        calls.emplace_back("write");
        if (throw_on_write) throw std::runtime_error("write");
        return 0x7au;
    }
    void ResetWriteFacetVtable(pal4::ActorControlXmlWriteFacet& facet,
                               std::uint32_t vtable) override {
        Check(vtable == 0x8426acu, "facet final vtable address");
        facet.words[0u] = vtable;
        calls.emplace_back("reset_facet");
    }
    void DestroyDocument(pal4::ActorControlXmlDocument&) override {
        calls.emplace_back("destroy_document");
    }
};

struct FacetRuntime final : pal4::IActorControlXmlFacetRuntime {
    pal4::ActorControlXmlWriteFacet* freed{};
    void Free(pal4::ActorControlXmlWriteFacet* facet) override {
        Check(facet->words[0u] == 0x8426acu,
              "facet vtable reset precedes free");
        freed = facet;
    }
};

}  // namespace

int main() {
    pal4::ActorControlXmlWriteFacet direct_facet{};
    direct_facet.words[0u] = 0xffffffffu;
    direct_facet.words[1u] = 0x12345678u;
    direct_facet.ResetVtable();
    Check(direct_facet.words[0u] == 0x8426acu &&
              direct_facet.words[1u] == 0x12345678u,
          "facet reset writes only the vtable word");
    FacetRuntime facet_runtime;
    direct_facet.words[0u] = 0xffffffffu;
    Check(direct_facet.Delete(2u, facet_runtime) == &direct_facet &&
              direct_facet.words[0u] == 0x8426acu &&
              facet_runtime.freed == nullptr,
          "deleting destructor ignores non-low flag bits");
    direct_facet.words[0u] = 0xffffffffu;
    Check(direct_facet.Delete(3u, facet_runtime) == &direct_facet &&
              facet_runtime.freed == &direct_facet &&
              direct_facet.words[1u] == 0x12345678u,
          "deleting destructor conditionally frees after vtable reset");

    pal4::PalActorControlState state{};
    state.words[3u] = 1u;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::PalActorControlBlendTailConfig::Write(
              state, runtime.argument, runtime) == 0x7au &&
              runtime.calls == std::vector<std::string>{
                  "document", "flag_false", "key_name", "find",
                  "flag_true", "set_int", "facet", "write", "reset_facet",
                  "destroy_document"},
          "existing XML attribute update and cleanup order");

    runtime.calls.clear();
    runtime.found = runtime.end;
    Check(pal4::PalActorControlBlendTailConfig::Write(
              state, runtime.argument, runtime) == 0x7au &&
              runtime.calls == std::vector<std::string>{
                  "document", "flag_false", "key_name", "find",
                  "flag_true", "new_attribute", "add_attribute",
                  "destroy_attribute", "facet", "write", "reset_facet",
                  "destroy_document"},
          "tree-end lookup inserts XML attribute");

    runtime.calls.clear();
    runtime.found = 0xffffffe4u;
    Check(pal4::PalActorControlBlendTailConfig::Write(
              state, runtime.argument, runtime) == 0x7au &&
              runtime.calls[5u] == "new_attribute",
          "node whose value-field address wraps to zero inserts attribute");

    runtime.calls.clear();
    runtime.throw_on_add = true;
    try {
        (void)pal4::PalActorControlBlendTailConfig::Write(
            state, runtime.argument, runtime);
        Check(false, "add exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "document", "flag_false", "key_name", "find",
                  "flag_true", "new_attribute", "add_attribute",
                  "destroy_attribute", "destroy_document"},
              "attribute and document cleanup after insertion failure");
    }

    runtime.calls.clear();
    runtime.throw_on_add = false;
    runtime.throw_on_write = true;
    runtime.found = 0x1200u;
    try {
        (void)pal4::PalActorControlBlendTailConfig::Write(
            state, runtime.argument, runtime);
        Check(false, "write exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "document", "flag_false", "key_name", "find",
                  "flag_true", "set_int", "facet", "write", "reset_facet",
                  "destroy_document"},
              "facet and document cleanup after write failure");
    }
}
