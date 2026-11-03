#include "pal4/gob_action.h"
#include "pal4/gob_scene_package.h"

namespace pal4 {

std::uint32_t GobAction::TypeKey() { return 6u; }

std::uint32_t GobAction::Create(IGobActionRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobAction::Construct(
    const std::uint32_t object, IGobActionConstructRuntime& runtime) {
    runtime.ConstructBase(object);
    try {
        runtime.InitializeAnimation(object + kAnimationOffset);
    } catch (...) {
        runtime.DestructBase(object);
        throw;
    }
    runtime.WriteWord(object + kAnimationOffset, kAnimationVtable);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x258u, 1u);
    runtime.WriteWord(object + 0x25cu, 0u);
    runtime.WriteWord(object + 0x260u, 0u);
    return object;
}

void GobAction::Destruct(
    const std::uint32_t object, IGobActionConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + kAnimationOffset, kAnimationVtable);
    runtime.DestructAnimation(object + kAnimationOffset);
    runtime.DestructBase(object);
}

std::uint32_t GobAction::CleanupResources(
    const std::uint32_t object, const std::uint32_t context,
    IGobActionCleanupRuntime& runtime) {
    runtime.CleanupAnimation(object + kAnimationOffset);
    return runtime.CleanupBase(object, context);
}

std::uint32_t GobAction::HandleEvent(std::uint32_t object,std::uint32_t argument,IGobActionRuntime& runtime){
    class Frame final : public IGobActionFrameRuntime {
        IGobActionRuntime& runtime;
    public:
        explicit Frame(IGobActionRuntime& r):runtime(r){}
        void RenderBase(std::uint32_t o,std::uint32_t c)override{runtime.RenderBase(o,c);}
        std::uint32_t ProcessAnimation(std::uint32_t o,std::uint32_t c)override{return runtime.ProcessAnimation(o,c);}
    } frame(runtime);
    return HandleEvent(object,argument,frame);
}
std::uint32_t GobAction::HandleEvent(std::uint32_t object,std::uint32_t argument,IGobActionFrameRuntime& runtime){
    runtime.RenderBase(object,argument);return runtime.ProcessAnimation(object+kAnimationOffset,argument);
}

std::uint32_t GobAction::Update(
    const std::uint32_t object, const float delta,
    IGobActionRuntime& runtime) {
    class Facet final:public IGobActionUpdateRuntime {
        IGobActionRuntime& r;
    public:
        explicit Facet(IGobActionRuntime& value):r(value){}
        void UpdateBase(std::uint32_t o,float d) override{r.UpdateBase(o,d);}
        void UpdateInterpolator(std::uint32_t s,float d) override{r.UpdateInterpolator(s,d);}
        std::uint32_t AnimationFlag(std::uint32_t s) override{return r.AnimationFlag(s);}
        std::uint32_t ReadWord(std::uint32_t a) override{return r.ReadWord(a);}
        void WriteWord(std::uint32_t a,std::uint32_t v) override{r.WriteWord(a,v);}
        void SetAnimationTime(std::uint32_t s,float t) override{r.SetAnimationTime(s,t);}
        std::uint32_t ClearAnimationFlag(std::uint32_t s,std::uint32_t f) override{return r.ClearAnimationFlag(s,f);}
    } facet(runtime);
    return Update(object,delta,facet);
}
std::uint32_t GobAction::Update(std::uint32_t object,float delta,IGobActionUpdateRuntime& runtime) {
    runtime.UpdateBase(object, delta);
    runtime.UpdateInterpolator(object + kAnimationOffset, delta);
    auto result = runtime.AnimationFlag(object + kAnimationOffset);
    if (result == 0u) return result;
    result = runtime.ReadWord(object + 0x25cu);
    if (static_cast<std::int32_t>(result) <= 0) return result;
    --result;
    runtime.WriteWord(object + 0x25cu, result);
    if (result != 0u) return result;
    if (runtime.ReadWord(object + 0x260u) == 0u)
        runtime.SetAnimationTime(object + kAnimationOffset, 0.0f);
    return runtime.ClearAnimationFlag(object + kAnimationOffset, 1u);
}

std::uint32_t GobAction::InitializeSceneAndAnimation(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view model_path,
    const std::uint32_t animation_data,
    const std::uint32_t final_argument,
    IGobActionRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(model_path, runtime);

    const auto loaded = runtime.LoadBaseModel(
        object, first, model_path, animation_data, final_argument);
    if (loaded != 0u) {
        const auto clump = runtime.ReadWord(object + 0x14u);
        runtime.HideHookUserDataAtomics(clump);
        runtime.LoadAnimationData(object + kAnimationOffset, clump,
                                  model_path, animation_data);
        runtime.SetAnimationFlag(object + kAnimationOffset, 1u);

        const auto document = runtime.ReadWord(object + 0x1c0u);
        const auto parameters = runtime.FindXmlChild(
            document, "PAL4-GameObject-Parameters");
        if (parameters != 0u) {
            const auto action = runtime.FindXmlChild(
                parameters, "PAL4_GameObject-action");
            if (action != 0u)
                runtime.RemoveXmlChild(parameters, action);
            (void)runtime.CreateXmlChild(
                parameters, "PAL4_GameObject-action");
        }
    }
    runtime.CleanupScriptPackages();
    return loaded;
}

std::uint32_t GobAction::ResetAnimation(
    const std::uint32_t object, IGobActionResetRuntime& runtime) {
    runtime.ResetAnimationSlot(object);
    runtime.SetAnimationTime(object + kAnimationOffset, 0.0f);
    return runtime.ClearAnimationFlag(object + kAnimationOffset, 0u);
}

std::uint32_t GobAction::GetOrCreateActionConfigNode(
    const std::uint32_t object, IGobActionRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters == 0u) return 0u;
    auto action = runtime.FindXmlChild(
        parameters, "PAL4_GameObject-action");
    if (action != 0u) return action;
    action = runtime.CreateXmlChild(
        parameters, "PAL4_GameObject-action");
    runtime.WriteXmlInt(action,
                        "PAL4_GameObject-action-default-play", 1);
    runtime.WriteXmlInt(action,
                        "PAL4_GameObject-action-play-times", 0);
    runtime.WriteXmlInt(action,
                        "PAL4_GameObject-action-holding-end", 0);
    return action;
}

void GobAction::SetVisibleAndAnimationState(
    const std::uint32_t object, const std::uint32_t visible,
    IGobActionVisibilityRuntime& runtime) {
    runtime.SetBaseVisible(object, visible);
    if (visible == 0u) return;
    const auto state = object + kAnimationOffset;
    if (runtime.ReadWord(object + 0x25cu) == 0u) {
        (void)runtime.ClearAnimationFlag(state, 1u);
        const auto time = runtime.ReadWord(object + 0x260u) != 0u ?
            runtime.AnimationLength(state) : 0.0f;
        runtime.SetAnimationTime(state, time);
        return;
    }
    const auto flag = runtime.ReadWord(object + 0x258u) == 0u ? 1u : 0u;
    (void)runtime.ClearAnimationFlag(state, flag);
}

void GobAction::ReadConfig(
    const std::uint32_t object, IGobActionRuntime& runtime) {
    runtime.ApplyBaseParameters(object);
    const auto node = GetOrCreateActionConfigNode(object, runtime);
    if (node == 0u) return;
    class Values final : public IGobActionConfigValueRuntime {
    public:
        Values(IGobActionRuntime& runtime, std::uint32_t node)
            : runtime_(runtime), node_(node) {}
        std::int32_t ReadInt(std::string_view name, std::int32_t fallback) override {
            const auto attribute = runtime_.FindXmlAttribute(node_, name);
            return attribute ? runtime_.XmlAttributeToInt(attribute, fallback) : fallback;
        }
        void WriteWord(std::uint32_t address, std::uint32_t value) override {
            runtime_.WriteWord(address, value);
        }
        std::uint32_t ClearAnimationFlag(std::uint32_t state, std::uint32_t flag) override {
            return runtime_.ClearAnimationFlag(state, flag);
        }
    private:
        IGobActionRuntime& runtime_;
        std::uint32_t node_;
    } values(runtime, node);
    ApplyConfigValues(object, values);
}

void GobAction::ApplyConfigValues(
    const std::uint32_t object, IGobActionConfigValueRuntime& runtime) {
    const auto default_play = runtime.ReadInt("PAL4_GameObject-action-default-play", 1);
    runtime.WriteWord(object + 0x258u, static_cast<std::uint32_t>(default_play));
    runtime.WriteWord(object + 0x25cu, static_cast<std::uint32_t>(
        runtime.ReadInt("PAL4_GameObject-action-play-times", 0)));
    runtime.WriteWord(object + 0x260u, static_cast<std::uint32_t>(
        runtime.ReadInt("PAL4_GameObject-action-holding-end", 0)));
    const auto flag = default_play == 0 ? 1u : 0u;
    (void)runtime.ClearAnimationFlag(object + kAnimationOffset, flag);
}

std::uint32_t GobAction::CreateStateSnapshot(
    const std::uint32_t object, IGobActionStateRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    const auto snapshot = runtime.Allocate(0x1cu);
    if (snapshot == 0u) return 0u;
    try {
        runtime.ConstructSnapshot(snapshot);
        runtime.AssignSnapshotName(
            snapshot, runtime.ReadCString(runtime.ReadWord(object + 4u)));
        runtime.WriteWord(snapshot + 0x14u,
                          runtime.ReadWord(object + 0x184u));
        runtime.WriteWord(snapshot + 0x18u,
                          runtime.ReadWord(object + 0x25cu));
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
    return snapshot;
}

void GobAction::ApplyStateSnapshot(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobActionStateRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u ||
        runtime.SnapshotType(snapshot) != TypeKey()) return;
    SetVisibleAndAnimationState(
        object, runtime.ReadWord(snapshot + 0x14u), runtime);
    const auto play_times = runtime.ReadWord(snapshot + 0x18u);
    runtime.WriteWord(object + 0x25cu, play_times);
    const auto state = object + kAnimationOffset;
    if (play_times != 0u) {
        const auto flag = runtime.ReadWord(object + 0x258u) == 0u ? 1u : 0u;
        (void)runtime.ClearAnimationFlag(state, flag);
        return;
    }
    (void)runtime.ClearAnimationFlag(state, 1u);
    const auto time = runtime.ReadWord(object + 0x260u) != 0u ?
        runtime.AnimationLength(state) : 0.0f;
    runtime.SetAnimationTime(state, time);
}

}  // namespace pal4
