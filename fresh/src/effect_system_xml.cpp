#include "pal4/effect_system.h"

#include <string>

namespace pal4 {
namespace {

constexpr EffectSystemHandle kInvalidHandle{0xffffffffu, 0u};

class FileResources final {
public:
    explicit FileResources(IEffectSystemXmlFileRuntime& runtime)
        : runtime_(runtime) { runtime_.BeginScriptScope(); }
    ~FileResources() {
        if (file_ != 0u) runtime_.CloseFile(file_);
        if (xml_ != 0u) runtime_.DestroyXmlDocument(xml_);
        if (adapter_ != 0u) runtime_.DestroyFileAdapter(adapter_);
        runtime_.EndScriptScope();
    }

    void Open(std::string_view path) { file_ = runtime_.OpenFile(path); }
    [[nodiscard]] bool IsOpen() const { return file_ != 0u; }
    void CreateParser() {
        adapter_ = runtime_.CreateFileAdapter(file_);
        xml_ = runtime_.CreateXmlDocument();
    }
    [[nodiscard]] bool Parse() const {
        return runtime_.ParseXml(adapter_, xml_);
    }
    [[nodiscard]] std::uint32_t Xml() const { return xml_; }

private:
    IEffectSystemXmlFileRuntime& runtime_;
    std::uint32_t file_{};
    std::uint32_t adapter_{};
    std::uint32_t xml_{};
};

}  // namespace

EffectSystemHandle EffectSystem::LoadEffectFromXml(
    std::uint32_t system, std::uint32_t xml,
    std::uint32_t context, IEffectSystemXmlRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return kInvalidHandle;

    auto effect = runtime.CreatePrimaryEffect(xml, context, system);
    if (effect == 0u)
        effect = runtime.CreateSecondaryEffect(xml, context, system);
    if (effect == 0u) return kInvalidHandle;

    const auto id = runtime.ReadWord(system + 8u);
    runtime.WriteWord(system + 8u, id + 1u);
    runtime.InsertXmlEffect(system + 228u, id, effect);
    return {id, system};
}

EffectSystemHandle EffectSystem::LoadEffectFromXmlFile(
    std::uint32_t system, std::string_view name,
    std::uint32_t context, IEffectSystemXmlFileRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return kInvalidHandle;

    FileResources resources(runtime);
    std::string path = "gamedata\\effect\\script\\";
    path += name;
    path += ".hef";
    resources.Open(path);
    if (!resources.IsOpen()) return kInvalidHandle;
    resources.CreateParser();
    if (!resources.Parse()) return kInvalidHandle;
    return LoadEffectFromXml(system, resources.Xml(), context, runtime);
}

}  // namespace pal4
