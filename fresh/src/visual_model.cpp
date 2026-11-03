#include "pal4/visual_model.h"

namespace pal4 {

int VisualModel::SetVector(VisualModelState& model,
                           VisualModelVector& destination,
                           VisualModelVector value, bool additive,
                           IVisualModelRuntime& runtime) {
    if (additive) {
        destination.x += value.x;
        destination.y += value.y;
        destination.z += value.z;
    } else {
        destination = value;
    }
    return UpdateFrameTransform(model, runtime);
}

int VisualModel::SetRotation(VisualModelState& model,
                             VisualModelVector value, bool additive,
                             IVisualModelRuntime& runtime) {
    return SetVector(model, model.rotation, value, additive, runtime);
}

int VisualModel::SetScale(VisualModelState& model,
                          VisualModelVector value, bool additive,
                          IVisualModelRuntime& runtime) {
    return SetVector(model, model.scale, value, additive, runtime);
}

int VisualModel::SetPosition(VisualModelState& model,
                             VisualModelVector value, bool additive,
                             IVisualModelRuntime& runtime) {
    return SetVector(model, model.position, value, additive, runtime);
}

int VisualModel::SetMaterialFlags(VisualModelState& model,
                                  std::uint32_t flags,
                                  IVisualModelRuntime& runtime) {
    model.material_flags = flags;
    if ((model.material_flags >> 24u) == 0xffu)
        model.material_flags &= 0xfeffffffu;
    return runtime.SetMaterialFlagsForAllAtomics(
        model.clump, model.material_flags);
}

void VisualModel::Destroy(VisualModelState& model,
                          IVisualModelDestroyRuntime& runtime) {
    if (model.active == 0u) return;
    const auto clump = model.clump;
    model.active = 0u;
    const auto world = runtime.FindClumpWorld(clump);
    if (world != 0u) runtime.RemoveClumpFromWorld(world, clump);
    if (model.clump != 0u) {
        runtime.ClearClumpFrameLink(model.clump);
        runtime.DestroyClump(model.clump);
        model.clump = 0u;
    }
    if (model.frame != 0u) {
        runtime.DestroyFrame(model.frame);
        model.frame = 0u;
    }
}

int VisualModel::UpdateFrameTransform(
    const VisualModelState& model, IVisualModelRuntime& runtime) {
    const auto frame = model.frame;
    if (frame == 0u) return 0;
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, model.scale, 0u);
    runtime.RotateFrame(frame, 0x84b380u, model.rotation.x, 2u);
    runtime.RotateFrame(frame, 0x84b38cu, model.rotation.y, 2u);
    runtime.RotateFrame(frame, 0x84b398u, model.rotation.z, 2u);
    runtime.TranslateFrame(frame, model.position, 2u);
    return runtime.UpdateFrameObjects(frame);
}

int VisualModel::Render(const VisualModelState& model,
                        std::uint32_t first, std::uint32_t second,
                        IVisualModelRenderRuntime& runtime) {
    if (model.active == 0u) return 0;
    const auto previous_27 = runtime.GetRenderState(27u);
    const auto previous_19 = runtime.GetRenderState(19u);
    const auto previous_20 = runtime.GetRenderState(20u);
    runtime.SetRenderState(27u, 1u);
    runtime.SetRenderState(19u, 5u);
    runtime.SetRenderState(20u, model.property != 0u ? 2u : 6u);
    const auto previous_7 = runtime.GetRenderState(7u);
    const auto previous_14 = runtime.GetRenderState(14u);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 0u);
    const auto previous_device_20 = runtime.GetDeviceState(20u);
    runtime.SetDeviceState(20u, 1u);
    runtime.RenderClump(model.clump);
    runtime.SetDeviceState(20u, previous_device_20);
    runtime.SetDeviceState(1u, 0u);
    runtime.SetRenderState(7u, previous_7);
    runtime.SetRenderState(14u, previous_14);
    runtime.SetRenderState(27u, previous_27);
    runtime.SetRenderState(19u, previous_19);
    runtime.SetRenderState(20u, previous_20);
    runtime.SetTextureStage(first, second, 0u);
    runtime.SetTextureStage(first, second, 1u);
    (void)runtime.ClearStreamSource(0u);
    return runtime.ClearStreamSource(1u);
}

}  // namespace pal4
