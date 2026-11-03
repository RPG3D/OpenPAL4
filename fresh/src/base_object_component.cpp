#include "pal4/base_object_component.h"

#include <bit>

namespace pal4 {

void IBaseObjectComponentRuntime::RemoveFromWorld(
    BaseObjectState& state) {
    BaseObjectComponent::RemoveFromWorld(state, *this);
}

std::uint32_t BaseObjectComponent::Initialize(
    const BaseObjectState& state, std::uint32_t,
    std::uint32_t, IBaseObjectComponentRuntime& runtime) {
    const auto component = state.Read(4u);
    if (component == 0u) return component;
    runtime.ClearAnimationFlag(component + 0x170u, 1u);
    return 1u;
}

std::uint32_t BaseObjectComponent::Process(
    const BaseObjectState& state, std::uint32_t animation_container,
    std::uint32_t output_address, IBaseObjectComponentRuntime& runtime) {
    const auto component = state.Read(4u);
    if (component == 0u) return 0u;
    const auto animation = runtime.FindAnimation(animation_container, "rps");
    if (animation != 0u) {
        const auto time = runtime.ReadDword(animation + 0xc0u);
        runtime.SetAnimationTime(component + 0x170u, time);
    }
    runtime.WriteDword(output_address + 0xc4u, 0u);
    return 1u;
}

std::uint32_t BaseObjectComponent::Setup(
    BaseObjectState& state, std::uint32_t label,
    std::string_view path, std::uint32_t model_address,
    std::uint8_t flag, IBaseObjectComponentRuntime& runtime) {
    const auto game = runtime.GameInstance();
    return runtime.LoadClump(state, label, path, model_address,
                             game + 0x1acu, flag, 0u, 1u) ? 1u : 0u;
}

std::uint32_t BaseObjectComponent::Cleanup(
    BaseObjectState& state, IBaseObjectComponentRuntime& runtime) {
    const auto game = runtime.GameInstance();
    runtime.CleanupClump(state, game + 0x1acu);
    return 1u;
}

std::uint32_t BaseObjectComponent::Construct(
    BaseObjectState& state, std::uint32_t address,
    IBaseObjectComponentRuntime& runtime) {
    runtime.ConstructResource(state);
    state.Write(0x174u, state.Read(0x174u) & 0xffffff00u);
    state.Write(0u, 0x843440u);
    return address;
}

std::uint32_t BaseObjectComponent::CleanupAndRelated(
    BaseObjectState& state, IBaseObjectComponentRuntime& runtime) {
    runtime.RemoveFromWorld(state);
    return Cleanup(state, runtime);
}

void BaseObjectComponent::Destroy(
    BaseObjectState& state, IBaseObjectComponentRuntime& runtime) {
    state.Write(0u, 0x843440u);
    (void)CleanupAndRelated(state, runtime);
    runtime.DestroyResource(state);
}

std::uint32_t BaseObjectComponent::Delete(
    BaseObjectState& state, std::uint32_t address,
    std::uint32_t flags, IBaseObjectComponentRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u) runtime.FreeObject(address);
    return address;
}

void BaseObjectComponent::CleanupResourceThunk(
    BaseObjectState& state, IBaseObjectComponentRuntime& runtime) {
    runtime.DestroyResource(state);
}

std::uint32_t BaseObjectComponent::LoadFromPrefixAndName(
    BaseObjectState& state, std::uint32_t attachment,
    std::string_view prefix, std::string_view model,
    std::uint32_t model_address,
    std::uint32_t, std::uint8_t flag,
    IBaseObjectComponentRuntime& runtime) {
    state.Write(0x170u, attachment);
    auto path = runtime.CreatePath();
    bool path_cleaned = false;
    const auto cleanup_path = [&] {
        path_cleaned = true;
        runtime.CleanupPath(path);
    };
    try {
        path.append(prefix);
        path.append(model);
        path.push_back('\\');
        const auto result = Setup(state, model_address, path,
                                  model_address, flag, runtime);
        cleanup_path();
        return result;
    } catch (...) {
        if (!path_cleaned) cleanup_path();
        throw;
    }
}

void BaseObjectComponent::Rotate(
    BaseObjectState& state, double elapsed,
    IBaseObjectComponentRuntime& runtime) {
    if ((state.Read(0x174u) & 0xffu) == 0u) return;
    const auto clump = state.Read(0x14u);
    if (clump == 0u) return;
    const auto frame = runtime.ReadDword(clump + 4u);
    for (const auto [offset, axis] : {
             std::pair{0x178u, 0x8a76b4u},
             std::pair{0x17cu, 0x8a76c0u},
             std::pair{0x180u, 0x8a76ccu}}) {
        const auto rate = std::bit_cast<float>(state.Read(offset));
        const auto angle = static_cast<float>(
            static_cast<double>(rate) * elapsed);
        runtime.RotateFrame(frame, axis, angle, 1u);
    }
    runtime.UpdateFrame(frame);
}

std::uint32_t BaseObjectComponent::AddToWorld(
    const BaseObjectState& state, std::uint32_t world,
    IBaseObjectComponentRuntime& runtime) {
    const auto clump = state.Read(0x14u);
    if (clump == 0u || world == 0u) return clump;
    return runtime.AddClumpToWorld(world, clump);
}

void BaseObjectComponent::RemoveFromWorld(
    BaseObjectState& state, IBaseObjectComponentRuntime& runtime) {
    const auto clump = state.Read(0x14u);
    if (clump == 0u) return;
    if (runtime.WorldFromClump(clump) != 0u) {
        const auto world = runtime.WorldFromClump(clump);
        runtime.RemoveClumpFromWorld(world, clump);
    }
}

std::uint32_t BaseObjectComponent::SetPosition(
    BaseObjectState& state, const BaseObjectVector3& position,
    IBaseObjectComponentRuntime& runtime) {
    const auto clump = state.Read(0x14u);
    if (clump == 0u) return clump;
    return runtime.UpdateClumpTransform(state, position, 0u);
}

std::uint32_t BaseObjectComponent::SetRotation(
    BaseObjectState& state, std::uint8_t flag,
    const BaseObjectVector3& rotation) {
    state.Write(0x174u,
                (state.Read(0x174u) & 0xffffff00u) | flag);
    state.Write(0x178u, std::bit_cast<std::uint32_t>(rotation.x));
    const auto y = std::bit_cast<std::uint32_t>(rotation.y);
    state.Write(0x17cu, y);
    state.Write(0x180u, std::bit_cast<std::uint32_t>(rotation.z));
    return y;
}

void BaseObjectComponent::SetScale(
    BaseObjectState& state, const BaseObjectVector3& scale,
    IBaseObjectComponentRuntime& runtime) {
    if (state.Read(0x14u) != 0u) {
        runtime.UpdateClumpPosition(state, scale, 0u);
    }
}

}  // namespace pal4
