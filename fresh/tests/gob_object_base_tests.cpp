#include "pal4/gob_object_base.h"
#include "pal4/native_game_vector_math.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& name) {
    const auto found = std::find(calls.begin(), calls.end(), name);
    if (found == calls.end()) throw std::runtime_error("missing " + name);
    return static_cast<std::size_t>(found - calls.begin());
}

struct Runtime final : pal4::IGobObjectBaseRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool fail_empty_string = false;
    bool fail_name_node = false;
    bool effect_needs_update = false;
    bool effect_ready = false;
    std::uint32_t allocation = 0x9000u;
    std::uint32_t initialized_document = 0xa000u;
    std::uint32_t node_document = 0xffffffffu;
    std::uint32_t render_argument = 0u;
    float update_delta = 0.0f;
    std::map<std::uint32_t, std::string> strings;
    std::uint32_t load_result = 1u;
    bool variant_allowed = true;
    std::uint32_t texture = 0xc000u;
    std::string texture_path;
    std::uint32_t loaded_variant = 0u;
    std::map<std::string, std::uint32_t> attributes;
    std::map<std::uint32_t, float> float_attributes;
    std::map<std::uint32_t, std::int32_t> int_attributes;
    std::vector<std::string> requested_attributes;
    std::vector<float> converted_floats;
    float applied_scale = 0.0f;
    std::uint32_t applied_mode = 0xffffffffu;
    std::vector<std::uint32_t> stream_buffers = {
        0x2100u, 0x2200u, 0x2300u, 0x2400u, 0x2500u};
    std::size_t next_stream_buffer = 0u;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> stream_reads;
    std::vector<std::uint32_t> freed_stream_buffers;
    std::uint32_t stream_type = 1u;
    std::uint32_t record_context = 0u;
    std::string script_name;
    bool checked_xml = false;
    float interaction_threshold = 20.0f;
    bool dialog_needs_update = false;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "gob base constructor address");
        calls.emplace_back("base construct");
    }
    void BaseDestruct(std::uint32_t address) override {
        Check(address == object, "gob base destructor address");
        calls.emplace_back("base destruct");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void InitializeEmptyString(std::uint32_t record) override {
        Check(record == object + 0x178u,
              "gob empty string record");
        calls.emplace_back("initialize empty string");
        if (fail_empty_string)
            throw std::runtime_error("empty string failed");
    }
    void CleanupString(std::uint32_t record) override {
        Check(record == object + 0x178u,
              "gob cleanup string record");
        calls.emplace_back("cleanup string");
    }
    void ConstructNameNode(std::uint32_t handle) override {
        Check(handle == object + 0x1ccu,
              "gob name node handle");
        calls.emplace_back("construct name node");
        if (fail_name_node)
            throw std::runtime_error("name node failed");
    }
    void ResetEffect(std::uint32_t handle) override {
        Check(handle == object + 0x1ccu,
              "gob reset effect handle");
        calls.emplace_back("reset effect");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x1ccu,
              "gob effect update handle");
        calls.emplace_back("effect needs update");
        return effect_needs_update;
    }
    void HandleEffectEvent(std::uint32_t handle,
                           std::uint32_t context,
                           std::uint32_t mode) override {
        Check(handle == object + 0x1ccu &&
                  context == 0x7000u && mode == 1u,
              "gob effect event arguments");
        calls.emplace_back("handle effect event");
    }
    bool EffectReadyToShow(std::uint32_t handle) override {
        Check(handle == object + 0x1d4u,
              "gob ready effect handle");
        calls.emplace_back("effect ready");
        return effect_ready;
    }
    std::uint32_t RemoveEffect(std::uint32_t handle) override {
        Check(handle == object + 0x1d4u,
              "gob remove effect handle");
        calls.emplace_back("remove effect");
        return 0x1234u;
    }
    std::uint32_t CleanupClump(std::uint32_t address) override {
        Check(address == object, "gob cleanup clump object");
        calls.emplace_back("cleanup clump");
        return 0x1111u;
    }
    void DestructXmlDocument(std::uint32_t document) override {
        Check(document == 0x8000u, "gob XML document");
        calls.emplace_back("destruct XML");
    }
    std::uint32_t FreeMemory(std::uint32_t memory) override {
        Check(memory == 0x8000u, "gob XML allocation");
        calls.emplace_back("free XML");
        return 0x2222u;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 0x3cu, "gob XML allocation size");
        calls.emplace_back("allocate XML");
        return allocation;
    }
    std::uint32_t InitializeXmlDocument(
        std::uint32_t memory, std::string_view root,
        std::uint32_t mode) override {
        Check(memory == 0x9000u && root == "PAL4-GameObject" &&
                  mode == 0u,
              "gob XML document arguments");
        calls.emplace_back("initialize XML");
        return initialized_document;
    }
    std::uint32_t CreateXmlNode(
        std::uint32_t document,
        std::string_view node_name) override {
        Check(node_name == "PAL4-GameObject-Parameters",
              "gob XML node name");
        node_document = document;
        calls.emplace_back("create XML node");
        return 0xb000u;
    }
    std::uint32_t RenderClump(std::uint32_t address,
                              std::uint32_t argument) override {
        Check(address == object, "gob render object");
        render_argument = argument;
        calls.emplace_back("render clump");
        return 0x3333u;
    }
    std::uint32_t UpdateClump(std::uint32_t address,
                              float delta) override {
        Check(address == object, "gob update object");
        update_delta = delta;
        calls.emplace_back("update clump");
        return 0x4444u;
    }
    std::uint32_t LoadClump(std::uint32_t address,
                            std::uint32_t first,
                            std::uint32_t second,
                            std::uint32_t third) override {
        Check(address == object && first == 1u && second == 2u &&
                  third == 3u,
              "gob clump load arguments");
        calls.emplace_back("load clump");
        return load_result;
    }
    void HideUserDataAtomics(std::uint32_t clump) override {
        Check(clump == 0x6000u, "gob RenderWare clump handle");
        calls.emplace_back("hide user data atomics");
    }
    std::string ReadCString(std::uint32_t address) override {
        calls.emplace_back("read string");
        return strings.at(address);
    }
    bool CanUseVariant(std::uint32_t plugin_context,
                       std::uint32_t variant_name) override {
        Check(plugin_context == 0x5000u && variant_name == 0x3000u,
              "gob variant plugin arguments");
        calls.emplace_back("can use variant");
        return variant_allowed;
    }
    std::uint32_t FindTexture(std::string_view path) override {
        texture_path = path;
        calls.emplace_back("find texture");
        return texture;
    }
    void LoadTexture(std::uint32_t address,
                     std::uint32_t variant_name,
                     std::uint32_t handle) override {
        Check(address == object && variant_name == 0x3000u &&
                  handle == texture,
              "gob texture load arguments");
        loaded_variant = variant_name;
        calls.emplace_back("load texture");
    }
    std::uint32_t FindConfigAttribute(
        std::uint32_t document, std::string_view name) override {
        Check(document == 0x8000u, "gob config document");
        requested_attributes.emplace_back(name);
        calls.emplace_back("find config attribute");
        const auto found = attributes.find(std::string(name));
        return found == attributes.end() ? 0u : found->second;
    }
    float ConfigAttributeToFloat(std::uint32_t attribute,
                                 float fallback) override {
        Check(fallback == 1.0f, "gob float attribute default");
        calls.emplace_back("parse float attribute");
        return float_attributes.at(attribute);
    }
    std::int32_t ConfigAttributeToInt(
        std::uint32_t attribute, std::int32_t fallback) override {
        Check(fallback == 1 || fallback == 0 || fallback == -1,
              "gob integer attribute default");
        calls.emplace_back("parse integer attribute");
        return int_attributes.at(attribute);
    }
    std::int32_t FloatToInteger(float value) override {
        converted_floats.push_back(value);
        calls.emplace_back("convert float to integer");
        return static_cast<std::int32_t>(value);
    }
    void ApplyScale(std::uint32_t address, float scale,
                    std::uint32_t mode) override {
        Check(address == object, "gob scale object");
        applied_scale = scale;
        applied_mode = mode;
        calls.emplace_back("apply scale");
    }
    std::uint32_t ReadStreamBuffer(std::uint32_t stream) override {
        Check(stream == 0x7000u &&
                  next_stream_buffer < stream_buffers.size(),
              "gob stream buffer read");
        calls.emplace_back("read stream buffer");
        return stream_buffers[next_stream_buffer++];
    }
    void ReadStreamBytes(std::uint32_t stream,
                         std::uint32_t destination,
                         std::uint32_t size) override {
        Check(stream == 0x7000u, "gob stream byte source");
        stream_reads.emplace_back(destination, size);
        if (destination == object + 0x184u) words[destination] = 7u;
        calls.emplace_back("read stream bytes");
    }
    std::uint32_t InitializeFromRecord(
        std::uint32_t address, std::uint32_t name,
        std::uint32_t directory, std::uint32_t model,
        std::uint32_t variant, std::uint32_t context) override {
        Check(address == object && name == 0x2100u &&
                  directory == 0x2200u && model == 0x2300u &&
                  variant == 0x2400u,
              "gob record name, directory, DFF model and variant");
        record_context = context;
        calls.emplace_back("initialize from record");
        return 1u;
    }
    void AssignScriptName(std::uint32_t record,
                          std::string_view name) override {
        Check(record == object + 0x178u, "gob script string record");
        script_name = name;
        calls.emplace_back("assign script name");
    }
    std::uint32_t ReadStreamType(std::uint32_t stream) override {
        Check(stream == 0x7000u, "gob stream type source");
        calls.emplace_back("read stream type");
        return stream_type;
    }
    void CheckRecordXml(std::uint32_t stream,
                        std::uint32_t document) override {
        Check(stream == 0x7000u && document == 0x8000u,
              "gob stream XML document");
        checked_xml = true;
        calls.emplace_back("check record XML");
    }
    void FinalizeRecord(std::uint32_t address) override {
        Check(address == object, "gob finalize record object");
        calls.emplace_back("finalize record");
    }
    void ApplyPosition(std::uint32_t address,
                       std::uint32_t vector,
                       std::uint32_t mode) override {
        Check(address == object && vector == object + 0xacu && mode == 0u,
              "gob record position");
        calls.emplace_back("apply position");
    }
    void ApplyRotation(std::uint32_t address,
                       std::uint32_t vector,
                       std::uint32_t mode) override {
        Check(address == object && vector == object + 0xa0u && mode == 0u,
              "gob record rotation");
        calls.emplace_back("apply rotation");
    }
    void ApplyRecordVisibility(std::uint32_t address,
                               std::uint32_t visible) override {
        Check(address == object && visible == 7u,
              "gob record visibility");
        calls.emplace_back("apply record visibility");
    }
    void FreeStreamBuffer(std::uint32_t buffer) override {
        freed_stream_buffers.push_back(buffer);
        calls.emplace_back("free stream buffer");
    }
    std::uint32_t ProcessMarker() override {
        calls.emplace_back("process marker");
        return 0x5678u;
    }
    float InteractionThreshold(std::uint32_t address) override {
        Check(address == object, "gob interaction threshold object");
        calls.emplace_back("interaction threshold");
        return interaction_threshold;
    }
    double PlanarMagnitude(float x, float z) override {
        return pal4::NativeGameApproximateMagnitude({x, 0.0f, z});
    }
    bool DialogNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x1ccu, "gob dialog handle");
        calls.emplace_back("dialog needs update");
        return dialog_needs_update;
    }
    void UpdateDialogContent(std::uint32_t handle) override {
        Check(handle == object + 0x1ccu, "gob update dialog handle");
        calls.emplace_back("update dialog content");
    }
    void AddActiveSceneObject(std::uint32_t address,
                              float value) override {
        Check(address == object && value == 1.0f,
              "gob active scene object arguments");
        calls.emplace_back("add active scene object");
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GobObjectBase::Construct(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.words.at(Runtime::object) == 0x848a00u &&
              runtime.words.at(Runtime::object + 0x1d4u) ==
                  0xffffffffu &&
              runtime.words.at(Runtime::object + 0x1b4u) ==
                  0xffffffffu &&
              runtime.words.at(Runtime::object + 0x1c0u) == 0u &&
              runtime.words.at(Runtime::object + 0x184u) == 1u &&
              runtime.words.at(Runtime::object + 0x18cu) == 1u &&
              runtime.words.at(Runtime::object + 0x198u) == 1u &&
              runtime.words.at(Runtime::object + 0x19cu) ==
                  std::bit_cast<std::uint32_t>(20.0f) &&
              runtime.words.at(Runtime::object + 0x1acu) ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              IndexOf(runtime.calls, "base construct") <
                  IndexOf(runtime.calls, "initialize empty string") &&
              IndexOf(runtime.calls, "initialize empty string") <
                  IndexOf(runtime.calls, "construct name node"),
          "gob constructor defaults and order");
    runtime.calls.clear();
    pal4::GobObjectBase::Destruct(Runtime::object, runtime);
    Check(runtime.calls.front() == "write word" &&
              IndexOf(runtime.calls, "reset effect") <
                  IndexOf(runtime.calls, "cleanup string") &&
              runtime.calls.back() == "base destruct",
          "gob destructor order");
    Runtime failed_string;
    failed_string.fail_empty_string = true;
    bool thrown = false;
    try {
        (void)pal4::GobObjectBase::Construct(Runtime::object,
                                              failed_string);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed_string.calls.back() == "base destruct" &&
              std::find(failed_string.calls.begin(),
                        failed_string.calls.end(),
                        "cleanup string") == failed_string.calls.end(),
          "gob empty string constructor failure");
    Runtime failed_node;
    failed_node.fail_name_node = true;
    thrown = false;
    try {
        (void)pal4::GobObjectBase::Construct(Runtime::object,
                                              failed_node);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed_node.calls[failed_node.calls.size() - 2u] ==
                        "cleanup string" &&
              failed_node.calls.back() == "base destruct",
          "gob name node constructor failure");
}

void CheckCleanupAndVisibility() {
    Runtime runtime;
    runtime.words[Runtime::object + 0x1c0u] = 0x8000u;
    runtime.effect_needs_update = true;
    runtime.effect_ready = true;
    Check(pal4::GobObjectBase::CleanupResources(
              Runtime::object, 0x7000u, runtime) == 0x2222u &&
              runtime.words.at(Runtime::object + 0x1c0u) == 0u &&
              IndexOf(runtime.calls, "handle effect event") <
                  IndexOf(runtime.calls, "remove effect") &&
              IndexOf(runtime.calls, "remove effect") <
                  IndexOf(runtime.calls, "cleanup clump") &&
              IndexOf(runtime.calls, "cleanup clump") <
                  IndexOf(runtime.calls, "destruct XML") &&
              IndexOf(runtime.calls, "destruct XML") <
                  IndexOf(runtime.calls, "free XML"),
          "gob cleanup effect clump and XML order");
    Runtime no_document;
    no_document.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobObjectBase::CleanupResources(
              Runtime::object, 0x7000u, no_document) == 0x1111u &&
              std::find(no_document.calls.begin(),
                        no_document.calls.end(),
                        "free XML") == no_document.calls.end(),
          "gob cleanup without optional resources");
    Runtime hidden;
    hidden.words[Runtime::object + 0x184u] = 0u;
    hidden.words[Runtime::object + 0x1c4u] = 0u;
    Check(pal4::GobObjectBase::RenderIfVisible(
              Runtime::object, 7u, hidden) == 0u &&
              pal4::GobObjectBase::UpdateClumpIfVisible(
                  Runtime::object, 0.5f, hidden) == 0u &&
              hidden.calls.empty(), "gob hidden render gates");
    for (const auto offset : {0x184u, 0x1c4u}) {
        Runtime visible;
        visible.words[Runtime::object + 0x184u] = 0u;
        visible.words[Runtime::object + 0x1c4u] = 0u;
        visible.words[Runtime::object + offset] = 1u;
        Check(pal4::GobObjectBase::RenderIfVisible(
                  Runtime::object, 7u, visible) == 0x3333u &&
                  pal4::GobObjectBase::UpdateClumpIfVisible(
                      Runtime::object, 0.5f, visible) == 0x4444u &&
                  visible.render_argument == 7u &&
                  visible.update_delta == 0.5f,
              "gob either visibility flag enables render and update");
    }
}

void CheckXmlNode() {
    Runtime runtime;
    runtime.words[Runtime::object + 0x1c0u] = 0x8000u;
    Check(pal4::GobObjectBase::CreateParametersNode(
              Runtime::object, runtime) == 0xb000u &&
              runtime.words.at(Runtime::object + 0x1c0u) ==
                  0xa000u && runtime.node_document == 0xa000u &&
              IndexOf(runtime.calls, "destruct XML") <
                  IndexOf(runtime.calls, "free XML") &&
              IndexOf(runtime.calls, "free XML") <
                  IndexOf(runtime.calls, "allocate XML") &&
              IndexOf(runtime.calls, "initialize XML") <
                  IndexOf(runtime.calls, "create XML node"),
          "gob replaces old parameters document");
    Runtime empty_allocation;
    empty_allocation.words[Runtime::object + 0x1c0u] = 0u;
    empty_allocation.allocation = 0u;
    Check(pal4::GobObjectBase::CreateParametersNode(
              Runtime::object, empty_allocation) == 0xb000u &&
              empty_allocation.node_document == 0u &&
              empty_allocation.words.at(Runtime::object + 0x1c0u) == 0u &&
              std::find(empty_allocation.calls.begin(),
                        empty_allocation.calls.end(),
                        "initialize XML") == empty_allocation.calls.end(),
          "gob null allocation forwards null document to node call");
}

void CheckLoadAndInitialize() {
    Runtime failed;
    failed.load_result = 0u;
    Check(pal4::GobObjectBase::LoadAndInitialize(
              Runtime::object, 1u, 2u, 3u, 0x3000u,
              failed) == 0u &&
              failed.calls ==
                  (std::vector<std::string>{"load clump"}),
          "gob failed clump load short circuits");
    Runtime plain;
    plain.words[Runtime::object + 0x14u] = 0x6000u;
    plain.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobObjectBase::LoadAndInitialize(
              Runtime::object, 1u, 2u, 3u, 0u,
              plain) == 1u &&
              IndexOf(plain.calls, "load clump") <
                  IndexOf(plain.calls, "hide user data atomics") &&
              IndexOf(plain.calls, "hide user data atomics") <
                  IndexOf(plain.calls, "allocate XML") &&
              std::find(plain.calls.begin(), plain.calls.end(),
                        "find texture") == plain.calls.end(),
          "gob plain clump creates XML without variant");
    Runtime variant;
    variant.words[Runtime::object + 0x14u] = 0x6000u;
    variant.words[Runtime::object + 0x10u] = 0x5000u;
    variant.words[Runtime::object + 8u] = 0x4000u;
    variant.words[Runtime::object + 0x1c0u] = 0u;
    variant.strings[0x3000u] = "JQB1";
    variant.strings[0x4000u] = "N01/";
    Check(pal4::GobObjectBase::LoadAndInitialize(
              Runtime::object, 1u, 2u, 3u, 0x3000u,
              variant) == 1u &&
              variant.texture_path == "N01/JQB1.png" &&
              variant.loaded_variant == 0x3000u &&
              IndexOf(variant.calls, "find texture") <
                  IndexOf(variant.calls, "load texture") &&
              IndexOf(variant.calls, "load texture") <
                  IndexOf(variant.calls, "allocate XML"),
          "gob variant texture path and load order");
    Runtime missing_plugin;
    missing_plugin.words[Runtime::object + 0x14u] = 0x6000u;
    missing_plugin.words[Runtime::object + 0x10u] = 0u;
    missing_plugin.words[Runtime::object + 0x1c0u] = 0u;
    missing_plugin.strings[0x3000u] = "JQB1";
    Check(pal4::GobObjectBase::LoadAndInitialize(
              Runtime::object, 1u, 2u, 3u, 0x3000u,
              missing_plugin) == 1u &&
              std::find(missing_plugin.calls.begin(),
                        missing_plugin.calls.end(),
                        "can use variant") == missing_plugin.calls.end(),
          "gob skips variant without plugin context");
    Runtime missing_texture;
    missing_texture.words = variant.words;
    missing_texture.words[Runtime::object + 0x1c0u] = 0u;
    missing_texture.strings = variant.strings;
    missing_texture.texture = 0u;
    Check(pal4::GobObjectBase::LoadAndInitialize(
              Runtime::object, 1u, 2u, 3u, 0x3000u,
              missing_texture) == 1u &&
              missing_texture.loaded_variant == 0u &&
              IndexOf(missing_texture.calls, "find texture") <
                  IndexOf(missing_texture.calls, "allocate XML"),
          "gob missing texture still creates parameters XML");
}

void CheckApplyParameters() {
    const std::vector<std::string> keys = {
        "PAL4-GameObject-object-scale",
        "PAL4-GameObject-object-auto-disappear",
        "PAL4-GameObject-object-camera-collide",
        "PAL4-GameObject-object-research-num",
        "PAL4_GameObject-object-height-limit",
        "PAL4-GameObject-object-store",
        "PAL4-GameObject-object-clip",
        "PAL4-GameObject-object-marker",
        "PAL4-GameObject-object-validfor",
        "PAL4-GameObject-object-hide",
        "PAL4-GameObject-object-hint"};
    Runtime absent;
    absent.words[Runtime::object + 0x1c0u] = 0u;
    pal4::GobObjectBase::ApplyParameters(Runtime::object, absent);
    Check(absent.calls.empty(), "gob absent config does nothing");

    Runtime defaults;
    defaults.words[Runtime::object + 0x1c0u] = 0x8000u;
    pal4::GobObjectBase::ApplyParameters(Runtime::object, defaults);
    Check(defaults.requested_attributes == keys &&
              defaults.words.at(Runtime::object + 0x1acu) ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              defaults.words.at(Runtime::object + 0x190u) == 1u &&
              defaults.words.at(Runtime::object + 0x194u) == 0u &&
              defaults.words.at(Runtime::object + 0x198u) == 1u &&
              defaults.words.at(Runtime::object + 0x1a0u) == 1u &&
              defaults.words.at(Runtime::object + 0x1a4u) == 1u &&
              defaults.words.at(Runtime::object + 0x1a8u) == 1u &&
              defaults.words.at(Runtime::object + 0x1b0u) == 0u &&
              defaults.words.at(Runtime::object + 0x1b4u) == 0xffffffffu &&
              defaults.words.at(Runtime::object + 0x1b8u) == 0u &&
              defaults.words.at(Runtime::object + 0x1bcu) == 0xffffffffu &&
              defaults.converted_floats ==
                  (std::vector<float>{1.0f, 1.0f, 1.0f}) &&
              defaults.applied_scale == 1.0f &&
              defaults.applied_mode == 0u &&
              defaults.calls.back() == "apply scale",
          "gob missing attribute defaults and order");

    Runtime configured;
    configured.words[Runtime::object + 0x1c0u] = 0x8000u;
    for (std::size_t i = 0; i < keys.size(); ++i)
        configured.attributes[keys[i]] =
            static_cast<std::uint32_t>(i + 1u);
    configured.float_attributes = {
        {1u, 1.75f}, {4u, 3.25f}, {5u, -2.75f}, {6u, 4.5f}};
    configured.int_attributes = {
        {2u, 0}, {3u, 1}, {7u, 0}, {8u, 9},
        {9u, -3}, {10u, 1}, {11u, 12}};
    pal4::GobObjectBase::ApplyParameters(Runtime::object, configured);
    Check(configured.requested_attributes == keys &&
              configured.words.at(Runtime::object + 0x1acu) ==
                  std::bit_cast<std::uint32_t>(1.75f) &&
              configured.words.at(Runtime::object + 0x190u) == 0u &&
              configured.words.at(Runtime::object + 0x194u) == 1u &&
              configured.words.at(Runtime::object + 0x198u) == 3u &&
              configured.words.at(Runtime::object + 0x1a0u) ==
                  static_cast<std::uint32_t>(-2) &&
              configured.words.at(Runtime::object + 0x1a4u) == 4u &&
              configured.words.at(Runtime::object + 0x1a8u) == 0u &&
              configured.words.at(Runtime::object + 0x1b0u) == 9u &&
              configured.words.at(Runtime::object + 0x1b4u) ==
                  static_cast<std::uint32_t>(-3) &&
              configured.words.at(Runtime::object + 0x1b8u) == 1u &&
              configured.words.at(Runtime::object + 0x1bcu) == 12u &&
              configured.converted_floats ==
                  (std::vector<float>{3.25f, -2.75f, 4.5f}) &&
              configured.applied_scale == 1.75f &&
              configured.calls.back() == "apply scale",
          "gob configured values and float conversion");
}

void CheckLoadRecord() {
    Runtime absent;
    Check(pal4::GobObjectBase::LoadRecord(
              Runtime::object, 0x4000u, 0u, absent) == 0u &&
              absent.calls.empty(), "gob null record stream");

    Runtime loaded;
    loaded.words[Runtime::object + 0x1c0u] = 0x8000u;
    loaded.strings[0x2500u] = "onTouch";
    Check(pal4::GobObjectBase::LoadRecord(
              Runtime::object, 0x4000u, 0x7000u, loaded) ==
              0x2500u &&
              loaded.record_context == 0x4000u &&
              loaded.script_name == "onTouch" &&
              loaded.next_stream_buffer == 5u &&
              loaded.stream_reads ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{
                      {Runtime::object + 0xacu, 0xcu},
                      {Runtime::object + 0xa0u, 0xcu},
                      {Runtime::object + 0x184u, 4u},
                      {Runtime::object + 0x188u, 4u},
                      {Runtime::object + 0x18cu, 4u},
                      {Runtime::object + 0x19cu, 4u}}) &&
              loaded.checked_xml &&
              loaded.freed_stream_buffers ==
                  (std::vector<std::uint32_t>{
                      0x2100u, 0x2200u, 0x2300u, 0x2400u, 0u}) &&
              IndexOf(loaded.calls, "initialize from record") <
                  IndexOf(loaded.calls, "assign script name") &&
              IndexOf(loaded.calls, "assign script name") <
                  IndexOf(loaded.calls, "check record XML") &&
              IndexOf(loaded.calls, "check record XML") <
                  IndexOf(loaded.calls, "finalize record") &&
              IndexOf(loaded.calls, "finalize record") <
                  IndexOf(loaded.calls, "apply position") &&
              IndexOf(loaded.calls, "apply position") <
                  IndexOf(loaded.calls, "apply rotation") &&
              IndexOf(loaded.calls, "apply rotation") <
                  IndexOf(loaded.calls, "apply record visibility") &&
              loaded.calls.back() == "free stream buffer",
          "gob record data layout and load order");

    for (const auto type : {0u, 2u, 3u}) {
        Runtime alternate;
        alternate.words[Runtime::object + 0x1c0u] = 0x8000u;
        alternate.strings[0x2500u] = "";
        alternate.stream_type = type;
        (void)pal4::GobObjectBase::LoadRecord(
            Runtime::object, 0u, 0x7000u, alternate);
        Check(alternate.checked_xml == (type == 2u),
              "gob XML check only for stream types one and two");
    }
}

void CheckInteraction() {
    Runtime marker;
    marker.words[Runtime::object + 0x1b0u] = 0u;
    Check(pal4::GobObjectBase::ProcessMarkerIfSet(
              Runtime::object, marker) == 0u &&
              marker.calls.empty(), "gob missing marker");
    marker.words[Runtime::object + 0x1b0u] = 1u;
    Check(pal4::GobObjectBase::ProcessMarkerIfSet(
              Runtime::object, marker) == 0x5678u &&
              marker.calls.back() == "process marker",
          "gob marker processing");

    Runtime runtime;
    runtime.words[Runtime::object + 0xacu] =
        std::bit_cast<std::uint32_t>(10.0f);
    runtime.words[Runtime::object + 0xb0u] =
        std::bit_cast<std::uint32_t>(5.0f);
    runtime.words[Runtime::object + 0xb4u] =
        std::bit_cast<std::uint32_t>(20.0f);
    runtime.words[Runtime::object + 0x1a0u] = 0u;
    float distance = -1.0f;
    using Position = pal4::GobObjectBase::Position;
    Check(pal4::GobObjectBase::IsWithinInteractionBounds(
              Runtime::object, Position{13.0f, 99.0f, 24.0f},
              1.0f, &distance, runtime) &&
              distance > 4.99f && distance < 5.01f,
          "gob compares approximate length with range, not squared length");
    runtime.interaction_threshold = distance;
    Check(pal4::GobObjectBase::IsWithinInteractionBounds(
              Runtime::object, Position{13.0f, 99.0f, 24.0f},
              1.0f, nullptr, runtime),
          "gob horizontal boundary is inclusive");
    runtime.interaction_threshold = 0.0f;
    runtime.words[Runtime::object + 0x1a0u] = 1u;
    Check(pal4::GobObjectBase::IsWithinInteractionBounds(
              Runtime::object, Position{10.0f, 5.0f, 20.0f},
              1.0f, &distance, runtime) && distance == 0.0f,
          "gob vertical interval accepts interior");
    Check(!pal4::GobObjectBase::IsWithinInteractionBounds(
              Runtime::object, Position{10.0f, 4.0f, 20.0f},
              1.0f, nullptr, runtime) &&
              !pal4::GobObjectBase::IsWithinInteractionBounds(
                  Runtime::object, Position{10.0f, 6.0f, 20.0f},
                  1.0f, nullptr, runtime),
          "gob vertical boundaries are exclusive");
    runtime.interaction_threshold =
        std::numeric_limits<float>::quiet_NaN();
    Check(!pal4::GobObjectBase::IsWithinInteractionBounds(
              Runtime::object, Position{10.0f, 5.0f, 20.0f},
              1.0f, nullptr, runtime),
          "gob unordered threshold rejects");

    runtime.words[Runtime::object + 0x18cu] = 0u;
    runtime.words[Runtime::object + 0x198u] = 1u;
    Check(!pal4::GobObjectBase::IsResearchVisibleCandidate(
              Runtime::object, runtime),
          "gob research requires candidate flag");
    runtime.words[Runtime::object + 0x18cu] = 1u;
    runtime.words[Runtime::object + 0x198u] = 0u;
    Check(!pal4::GobObjectBase::IsResearchVisibleCandidate(
              Runtime::object, runtime),
          "gob research requires remaining uses");
    runtime.words[Runtime::object + 0x198u] = 0xffffffffu;
    Check(pal4::GobObjectBase::IsResearchVisibleCandidate(
              Runtime::object, runtime),
          "gob negative uses remain eligible");
}

void CheckVisibilityAndUses() {
    Runtime visible;
    Check(pal4::GobObjectBase::SetVisible(
              Runtime::object, 0x123u, visible) == 0x23u &&
              visible.words.at(Runtime::object + 0x184u) == 0x123u &&
              std::find(visible.calls.begin(), visible.calls.end(),
                        "dialog needs update") == visible.calls.end(),
          "gob visible value bypasses effect cleanup");
    Runtime hidden;
    hidden.dialog_needs_update = true;
    hidden.effect_ready = true;
    Check(pal4::GobObjectBase::SetVisible(
              Runtime::object, 0u, hidden) == 0x34u &&
              hidden.words.at(Runtime::object + 0x184u) == 0u &&
              IndexOf(hidden.calls, "update dialog content") <
                  IndexOf(hidden.calls, "effect ready") &&
              IndexOf(hidden.calls, "effect ready") <
                  IndexOf(hidden.calls, "remove effect"),
          "gob hiding updates dialog and removes effect");
    Runtime hidden_no_effect;
    Check(pal4::GobObjectBase::SetVisible(
              Runtime::object, 0u, hidden_no_effect) == 0u &&
              std::find(hidden_no_effect.calls.begin(),
                        hidden_no_effect.calls.end(),
                        "remove effect") == hidden_no_effect.calls.end(),
          "gob hiding without effect");

    Runtime candidate;
    candidate.words[Runtime::object + 0x1b0u] = 1u;
    candidate.words[Runtime::object + 0x184u] = 1u;
    candidate.words[Runtime::object + 0x198u] = 1u;
    Check(pal4::GobObjectBase::IsMarkedVisibleWithUses(
              Runtime::object, candidate),
          "gob marked visible candidate");
    for (const auto offset : {0x1b0u, 0x184u, 0x198u}) {
        candidate.words[Runtime::object + offset] = 0u;
        Check(!pal4::GobObjectBase::IsMarkedVisibleWithUses(
                  Runtime::object, candidate),
              "gob candidate requires each flag");
        candidate.words[Runtime::object + offset] = 1u;
    }

    Runtime positive;
    positive.words[Runtime::object + 0x198u] = 2u;
    Check(pal4::GobObjectBase::ConsumeResearchUse(
              Runtime::object, positive) == 1u &&
              positive.words.at(Runtime::object + 0x198u) == 1u &&
              std::find(positive.calls.begin(), positive.calls.end(),
                        "add active scene object") == positive.calls.end(),
          "gob positive research use decrements");
    Runtime exhausted;
    exhausted.words[Runtime::object + 0x198u] = 1u;
    exhausted.words[Runtime::object + 0x190u] = 1u;
    exhausted.words[Runtime::object + 0x1b0u] = 1u;
    Check(pal4::GobObjectBase::ConsumeResearchUse(
              Runtime::object, exhausted) == 0x5678u &&
              exhausted.words.at(Runtime::object + 0x198u) == 0u &&
              IndexOf(exhausted.calls, "add active scene object") <
                  IndexOf(exhausted.calls, "process marker"),
          "gob exhausted research hides scene object then processes marker");
    Runtime unlimited;
    unlimited.words[Runtime::object + 0x198u] = 0xffffffffu;
    Check(pal4::GobObjectBase::ConsumeResearchUse(
              Runtime::object, unlimited) == 0xffffffffu &&
              unlimited.calls.empty(),
          "gob negative research use remains unlimited");
    Runtime zero;
    zero.words[Runtime::object + 0x198u] = 0u;
    zero.words[Runtime::object + 0x190u] = 0u;
    zero.words[Runtime::object + 0x1b0u] = 0u;
    Check(pal4::GobObjectBase::ConsumeResearchUse(
              Runtime::object, zero) == 0u &&
              zero.calls.empty(), "gob exhausted without side effects");

    Runtime getters;
    getters.words[Runtime::object + 0x190u] = 7u;
    getters.words[Runtime::object + 0x194u] = 8u;
    getters.words[Runtime::object + 0x19cu] =
        std::bit_cast<std::uint32_t>(20.5f);
    getters.words[Runtime::object + 0x1c0u] = 0x8000u;
    Check(pal4::GobObjectBase::AlwaysOne() == 1u &&
              pal4::GobObjectBase::AutoDisappear(
                  Runtime::object, getters) == 7u &&
              pal4::GobObjectBase::CameraCollide(
                  Runtime::object, getters) == 8u &&
              pal4::GobObjectBase::InteractionRangeThreshold(
                  Runtime::object, getters) == 20.5f &&
              pal4::GobObjectBase::ConfigNode(
                  Runtime::object, getters) == 0x8000u,
          "gob short vtable getters");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckCleanupAndVisibility();
        CheckXmlNode();
        CheckLoadAndInitialize();
        CheckApplyParameters();
        CheckLoadRecord();
        CheckInteraction();
        CheckVisibilityAndUses();
        std::cout << "gob_object_base_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
