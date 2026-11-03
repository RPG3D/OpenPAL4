#include "pal4/gob_effect.h"
#include "pal4/gob_scene_package.h"

namespace pal4 {

std::uint32_t GobEffect::TypeKey() { return 8u; }

std::uint32_t GobEffect::Create(IGobEffectRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobEffect::Construct(
    const std::uint32_t object, IGobEffectConstructRuntime& runtime) {
    runtime.ConstructBase(object);
    try {
        runtime.ConstructEffectHandle(object + kEffectHandleOffset);
        try {
            runtime.InitializeEmptyString(object + kStringOffset);
        } catch (...) {
            runtime.ResetEffectHandle(object + kEffectHandleOffset);
            throw;
        }
    } catch (...) {
        runtime.DestructBase(object);
        throw;
    }
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x170u, 0u);
    runtime.WriteWord(object + 0x1f0u, 1u);
    runtime.WriteWord(object + 0x1f8u, 0u);
    return object;
}

void GobEffect::Destruct(
    const std::uint32_t object, IGobEffectConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.CleanupString(object + kStringOffset);
    runtime.ResetEffectHandle(object + kEffectHandleOffset);
    runtime.DestructBase(object);
}

std::uint32_t GobEffect::CleanupResources(
    const std::uint32_t object, const std::uint32_t context,
    IGobEffectCleanupRuntime& runtime) {
    const auto handle = object + kEffectHandleOffset;
    if (runtime.EffectNeedsUpdate(handle))
        runtime.HandleEffectEvent(handle, context, 1u);
    return runtime.CleanupBase(object, context);
}

std::uint32_t GobEffect::HandleEventIfEnabled(std::uint32_t object,std::uint32_t argument,IGobEffectRuntime& runtime){
    class Frame final : public IGobEffectFrameRuntime {
        IGobEffectRuntime& runtime;
    public:
        explicit Frame(IGobEffectRuntime& r):runtime(r){}
        std::uint32_t ReadWord(std::uint32_t a)override{return runtime.ReadWord(a);}
        std::uint32_t RenderBase(std::uint32_t o,std::uint32_t c)override{return runtime.RenderBase(o,c);}
    } frame(runtime);
    return HandleEventIfEnabled(object,argument,frame);
}
std::uint32_t GobEffect::HandleEventIfEnabled(std::uint32_t object,std::uint32_t argument,IGobEffectFrameRuntime& runtime){
    const auto enabled=runtime.ReadWord(object+0x170u);return enabled?runtime.RenderBase(object,argument):enabled;
}

std::uint32_t GobEffect::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobEffectRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto loaded = runtime.LoadBaseModel(
        object, first, path, fourth, fifth);
    if (loaded != 0u) {
        const auto document = runtime.ReadWord(object + 0x1c0u);
        const auto parameters = runtime.FindXmlChild(
            document, "PAL4-GameObject-Parameters");
        if (parameters != 0u) {
            const auto effect = runtime.FindXmlChild(
                parameters, "PAL4_GameObject-effect");
            if (effect != 0u)
                runtime.RemoveXmlChild(parameters, effect);
            (void)runtime.CreateXmlChild(
                parameters, "PAL4_GameObject-effect");
        }
    }
    runtime.CleanupScriptPackages();
    return loaded;
}

std::uint32_t GobEffect::TriggerSaverStateIfEnabled(
    const std::uint32_t object, IGobEffectSaverRuntime& runtime) {
    const auto saver = runtime.ReadWord(object + 0x1f4u);
    return saver == 0u ? saver :
        runtime.InitializeGameState(9u, 0u);
}

std::uint32_t GobEffect::GetConfigNode(
    const std::uint32_t object, IGobEffectRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters == 0u) return 0u;
    return runtime.FindXmlChild(parameters,
                                "PAL4_GameObject-effect");
}

void GobEffect::ReadConfig(
    const std::uint32_t object, IGobEffectRuntime& runtime) {
    runtime.ApplyBaseParameters(object);
    const auto node = GetConfigNode(object, runtime);
    if (node == 0u) return;
    class NodeValues final : public IGobEffectConfigValueRuntime {
    public:
        NodeValues(std::uint32_t node, IGobEffectRuntime& runtime)
            : node_(node), runtime_(runtime) {}
        std::string ReadString(std::string_view name,
                               std::string_view fallback) override {
            return runtime_.ReadXmlString(node_, name, fallback);
        }
        std::int32_t ReadInt(std::string_view name,
                             std::int32_t fallback) override {
            const auto attribute = runtime_.FindXmlAttribute(node_, name);
            return attribute == 0u ? fallback :
                runtime_.XmlAttributeToInt(attribute, fallback);
        }
        void AssignEffectName(std::uint32_t record,
                              std::string_view name) override {
            runtime_.AssignEffectName(record, name);
        }
        void WriteWord(std::uint32_t address,
                       std::uint32_t value) override {
            runtime_.WriteWord(address, value);
        }
    private:
        std::uint32_t node_;
        IGobEffectRuntime& runtime_;
    } values(node, runtime);
    ApplyConfigValues(object, values);
}

void GobEffect::ApplyConfigValues(
    const std::uint32_t object, IGobEffectConfigValueRuntime& runtime) {
    runtime.AssignEffectName(object + kStringOffset,
        runtime.ReadString("PAL4_GameObject-effect-name", ""));
    runtime.WriteWord(object + 0x1f0u, static_cast<std::uint32_t>(
        runtime.ReadInt("PAL4_GameObject-effect-times", 0)));
    runtime.WriteWord(object + 0x1f4u, static_cast<std::uint32_t>(
        runtime.ReadInt("PAL4_GameObject-effect-saver", 0)));
    runtime.WriteWord(object + 0x1f8u, static_cast<std::uint32_t>(
        runtime.ReadInt("PAL4_GameObject-effect-distance-clip", 1)));
}

std::uint32_t GobEffect::UpdateClumpTransform(
    const std::uint32_t object, const std::uint32_t transform,
    const std::uint32_t mode, IGobEffectRuntime& runtime) {
    return runtime.UpdateClumpTransform(object, transform, mode);
}

std::uint8_t GobEffect::SetVisibleAndCreateEffect(
    const std::uint32_t object, const std::uint32_t context,
    const std::uint32_t visible, IGobEffectVisibilityRuntime& runtime) {
    runtime.StoreWord(object + 0x184u, visible);
    const auto handle = object + kEffectHandleOffset;
    if (visible == 0u) {
        if (!runtime.EffectNeedsUpdate(handle)) return 0u;
        return runtime.UpdateDialogContent(handle);
    }
    if (runtime.EffectNeedsUpdate(handle))
        runtime.HandleEffectEvent(handle, context, 1u);
    const auto created = runtime.CreateEffect(
        runtime.EffectName(object + kStringOffset),
        runtime.ReadWord(object + 0x1f0u),
        runtime.ReadWord(object + 0x1f8u) != 0u);
    runtime.StoreWord(handle, created.first);
    runtime.StoreWord(handle + 4u, created.second);
    runtime.ResetTemporaryEffect();
    if (!runtime.EffectNeedsUpdate(handle)) return 0u;
    const auto clump = runtime.ReadWord(object + 0x14u);
    runtime.UpdateEffectProperties(
        handle, runtime.ReadWord(clump + 4u));
    return runtime.UpdateEffectState(handle);
}

std::uint32_t GobEffect::CreateStateSnapshot(
    const std::uint32_t object, IGobEffectSnapshotRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    const auto snapshot = runtime.Allocate(0x18u);
    if (snapshot == 0u) return 0u;
    try {
        runtime.ConstructSnapshot(snapshot);
        runtime.AssignSnapshotName(
            snapshot, runtime.ReadCString(runtime.ReadWord(object + 4u)));
        runtime.WriteWord(snapshot + 0x14u,
                          runtime.ReadWord(object + 0x184u));
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
    return snapshot;
}

void GobEffect::ApplyStateSnapshot(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobEffectSnapshotRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u ||
        runtime.SnapshotType(snapshot) != TypeKey()) return;
    runtime.SetBaseVisible(
        object, runtime.ReadWord(snapshot + 0x14u));
}

}  // namespace pal4
