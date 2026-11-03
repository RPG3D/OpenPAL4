#include "pal4/effect_handle.h"

namespace pal4 {

EffectSystemHandleWords& EffectHandle::CopyData(
    EffectSystemHandleWords& destination,
    const EffectSystemHandleWords& source, std::uint32_t owner) {
    destination.id = source.id;
    destination.system = owner;
    return destination;
}

void EffectHandle::Reset(EffectSystemHandleWords& handle) {
    handle.id = 0xffffffffu;
    handle.system = 0u;
}

void EffectHandle::ResetThunk(EffectSystemHandleWords& handle) {
    Reset(handle);
}

bool EffectHandle::DialogNeedsUpdate(
    const EffectSystemHandleWords& handle, IEffectHandleRuntime& runtime) {
    if (handle.system == 0u) return false;
    const auto system = runtime.InitializeSystem();
    if (!runtime.Validate(system, handle.system)) return false;

    const auto selected = SelectedNode(handle, runtime);
    return selected != runtime.ReadWord(handle.system + 16u);
}

std::uint32_t EffectHandle::SelectedNode(
    const EffectSystemHandleWords& handle,
    IEffectHandleRuntime& runtime) {
    const auto tree = handle.system + 12u;
    const auto candidate = runtime.FindElement(tree, handle);
    const auto head = runtime.ReadWord(tree + 4u);
    return candidate == head || handle.id < runtime.ReadWord(candidate + 12u)
        ? head : candidate;
}

std::uint32_t EffectHandle::RegisteredDialog(
    const EffectSystemHandleWords& handle,
    IEffectHandlePropertyRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    return selected == 0u ||
                   selected == runtime.ReadWord(handle.system + 16u)
        ? 0u : runtime.ReadWord(selected + 16u);
}

std::uint32_t EffectHandle::RegisteredNode(
    const EffectSystemHandleWords& handle,
    IEffectHandlePropertyRuntime& runtime) {
    if (handle.system == 0u) return 0u;
    const auto system = runtime.RegisterSystem();
    if (!runtime.ContainsOwner(system, handle.system)) return 0u;
    return SelectedNode(handle, runtime);
}

std::string EffectHandle::GetName(
    const EffectSystemHandleWords& handle,
    IEffectHandlePropertyRuntime& runtime) {
    const auto dialog = RegisteredDialog(handle, runtime);
    return dialog == 0u ? std::string{} :
           runtime.ReadNameAttribute(dialog + 20u);
}

std::uint32_t EffectHandle::GetFrame(
    const EffectSystemHandleWords& handle,
    IEffectHandlePropertyRuntime& runtime) {
    const auto dialog = RegisteredDialog(handle, runtime);
    return dialog == 0u ? 0u : runtime.ReadWord(dialog + 172u);
}

std::uint8_t EffectHandle::SetVector(
    const EffectSystemHandleWords& handle,
    EffectHandleVectorProperty property,
    const std::array<float, 3u>& value, bool additive,
    IEffectHandleMutationRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    if (selected == 0u) return 0u;
    if (selected == runtime.ReadWord(handle.system + 16u))
        return static_cast<std::uint8_t>(selected);
    return runtime.ApplyVector(property, runtime.ReadWord(selected + 16u),
                               value, additive);
}

std::uint8_t EffectHandle::SetRotation(
    const EffectSystemHandleWords& handle,
    const std::array<float, 3u>& value, bool additive,
    IEffectHandleMutationRuntime& runtime) {
    return SetVector(handle, EffectHandleVectorProperty::Rotation,
                     value, additive, runtime);
}

std::uint8_t EffectHandle::SetScale(
    const EffectSystemHandleWords& handle,
    const std::array<float, 3u>& value, bool additive,
    IEffectHandleMutationRuntime& runtime) {
    return SetVector(handle, EffectHandleVectorProperty::Scale,
                     value, additive, runtime);
}

std::uint8_t EffectHandle::SetPosition(
    const EffectSystemHandleWords& handle,
    const std::array<float, 3u>& value, bool additive,
    IEffectHandleMutationRuntime& runtime) {
    return SetVector(handle, EffectHandleVectorProperty::Position,
                     value, additive, runtime);
}

std::uint8_t EffectHandle::SetParentFrame(
    const EffectSystemHandleWords& handle, std::uint32_t frame,
    IEffectHandleMutationRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    if (selected == 0u) return 0u;
    if (selected == runtime.ReadWord(handle.system + 16u))
        return static_cast<std::uint8_t>(selected);
    return runtime.ApplyParentFrame(runtime.ReadWord(selected + 16u), frame);
}

std::uint8_t EffectHandle::SetVisibility(
    const EffectSystemHandleWords& handle, std::uint32_t visibility,
    IEffectHandleMutationRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    if (selected == 0u) return 0u;
    if (selected == runtime.ReadWord(handle.system + 16u))
        return static_cast<std::uint8_t>(selected);
    return runtime.ApplyVisibility(runtime.ReadWord(selected + 16u),
                                   visibility);
}

std::int32_t EffectHandle::GetConfigInt(
    const EffectSystemHandleWords& handle, std::string_view key,
    IEffectHandleMutationRuntime& runtime) {
    const auto dialog = RegisteredDialog(handle, runtime);
    return dialog == 0u ? 0 : runtime.ReadConfigInt(dialog + 20u, key);
}

std::uint8_t EffectHandle::SetConfigInt(
    const EffectSystemHandleWords& handle, std::string_view key,
    std::int32_t value, IEffectHandleMutationRuntime& runtime) {
    const auto dialog = RegisteredDialog(handle, runtime);
    if (dialog == 0u) return 0u;
    runtime.WriteConfigInt(dialog + 20u, key, value);
    return 1u;
}

std::uint8_t EffectHandle::SetFacet(
    const EffectSystemHandleWords& handle,
    std::uint32_t first, std::uint32_t second,
    IEffectHandleMutationRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    if (selected == 0u) return 0u;
    if (selected == runtime.ReadWord(handle.system + 16u))
        return static_cast<std::uint8_t>(selected);
    return runtime.ApplyFacet(runtime.ReadWord(selected + 16u),
                              first, second);
}

std::uint8_t EffectHandle::PausePlayback(
    const EffectSystemHandleWords& handle,
    IEffectHandleMutationRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    if (selected == 0u) return 0u;
    if (selected == runtime.ReadWord(handle.system + 16u))
        return static_cast<std::uint8_t>(selected);
    return runtime.PausePlayback(runtime.ReadWord(selected + 16u));
}

void EffectHandle::Release(EffectSystemHandleWords& handle, bool force,
                           IEffectHandleReleaseRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    if (selected == 0u) return;
    if (selected == runtime.ReadWord(handle.system + 16u)) {
        Reset(handle);
        return;
    }

    const auto count = static_cast<std::int32_t>(runtime.ReadWord(selected + 20u));
    const auto next = force || count <= 0 ? 0 : count - 1;
    runtime.WriteWord(selected + 20u, static_cast<std::uint32_t>(next));
    if (next != 0) return;

    const auto owner = handle.system;
    const auto dialog = runtime.ReadWord(selected + 16u);
    runtime.ResetDialogRuntimeState(dialog);
    runtime.DispatchOwnerRelease(owner, handle, 0u);
    runtime.ReturnEffect(runtime.RegisterSystem() + 84u, dialog, 0u);
    runtime.EraseEntry(owner + 12u, selected);
    runtime.ProcessEffectList(owner);
    Reset(handle);
}

void EffectHandle::UpdateContent(
    EffectSystemHandleWords& handle,
    IEffectHandleContentRuntime& runtime) {
    const auto selected = RegisteredNode(handle, runtime);
    if (selected == 0u) return;
    if (selected == runtime.ReadWord(handle.system + 16u)) {
        Reset(handle);
        return;
    }

    const auto owner = handle.system;
    const auto dialog = runtime.ReadWord(selected + 16u);
    const auto frame = runtime.ReadWord(dialog + 172u);
    const auto transform = runtime.FrameTransform(frame);
    const auto time = runtime.StopDialogPlayback(dialog, true);
    runtime.SetFrameIdentity(frame);
    runtime.ApplyFrameTransform(frame, transform, 0u);
    runtime.UpdateFrameObjects(frame);
    runtime.DispatchOwnerRelease(owner, handle, 0u);
    runtime.EraseEntry(owner + 12u, selected);

    runtime.QueuePendingCompletion(owner, dialog, time);
    Reset(handle);
}

void IEffectHandleContentRuntime::QueuePendingCompletion(
    std::uint32_t owner, std::uint32_t dialog, float time) {
    auto& runtime = *this;
    const auto pending = owner + 28u;
    const auto head = runtime.ReadWord(pending + 4u);
    auto parent = head;
    auto child = runtime.ReadWord(head + 4u);
    bool insert_left = true;
    const auto nil = runtime.PendingNil();
    while (child != nil) {
        parent = child;
        insert_left = dialog < runtime.ReadWord(child + 12u);
        child = runtime.ReadWord(child + (insert_left ? 0u : 8u));
    }

    bool insert = runtime.ReadByte(pending + 8u) != 0u;
    if (!insert) {
        if (insert_left) {
            insert = parent == runtime.ReadWord(head) ||
                     runtime.ReadWord(runtime.Predecessor(parent) + 12u) <
                         dialog;
        } else {
            insert = parent == head || runtime.ReadWord(parent + 12u) < dialog;
        }
    }
    if (insert) runtime.InsertPending(pending, child, parent, dialog, time);
}

std::uint32_t EffectHandle::SelectedContextNode(
    const EffectSystemHandleWords& handle,
    IEffectHandleContextRuntime& runtime) {
    const auto tree = handle.system + 228u;
    const auto candidate = runtime.FindElement(tree, handle);
    const auto head = runtime.ReadWord(tree + 4u);
    return candidate == head || handle.id < runtime.ReadWord(candidate + 12u)
               ? head : candidate;
}

bool EffectHandle::IsReadyToShow(
    const EffectSystemHandleWords& handle,
    IEffectHandleContextRuntime& runtime) {
    if (handle.system == 0u || static_cast<std::int32_t>(handle.id) < 0)
        return false;
    if (!runtime.Validate(runtime.InitializeSystem(), handle.system))
        return false;
    return SelectedContextNode(handle, runtime) !=
           runtime.ReadWord(handle.system + 232u);
}

void EffectHandle::RemoveContext(
    EffectSystemHandleWords& handle,
    IEffectHandleContextRuntime& runtime) {
    if (handle.system == 0u || static_cast<std::int32_t>(handle.id) < 0)
        return;
    if (!runtime.ContainsOwner(runtime.RegisterSystem(), handle.system))
        return;
    const auto owner = handle.system;
    const auto selected = SelectedContextNode(handle, runtime);
    if (selected != runtime.ReadWord(owner + 232u)) {
        const auto dialog = runtime.ReadWord(selected + 16u);
        runtime.StopContextDialog(dialog);
        runtime.DestroyContextDialog(dialog);
        runtime.EraseContext(owner + 228u, selected);
    }
    Reset(handle);
}

void EffectHandle::DisplayAndCleanup(
    EffectSystemHandleWords& handle,
    IEffectHandleContextRuntime& runtime) {
    if (handle.system == 0u || static_cast<std::int32_t>(handle.id) < 0)
        return;
    if (!runtime.Validate(runtime.InitializeSystem(), handle.system))
        return;
    const auto owner = handle.system;
    const auto selected = SelectedContextNode(handle, runtime);
    if (selected != runtime.ReadWord(owner + 232u)) {
        const auto dialog = runtime.ReadWord(selected + 16u);
        const auto time = runtime.ContextDialogTime(dialog);
        runtime.PositionContextDialog(owner, dialog, time);
        runtime.EraseContext(owner + 228u, selected);
    }
    Reset(handle);
}

}  // namespace pal4
