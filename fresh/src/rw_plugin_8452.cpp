#include "pal4/rw_plugin_8452.h"

#include <bit>

namespace pal4 {

bool RwPlugin8452::Register(RwPlugin8452State& state,
                            IRwPlugin8452Lifecycle& runtime) {
    using C = RwPlugin8452Callback;
    state.engine_offset = runtime.RegisterEngine(
        1u, kId, C::Identity, C::Identity);
    if (state.engine_offset < 0) return false;
    state.geometry_offset = runtime.RegisterGeometry(
        12u, kId, C::GeometryConstruct,
        C::GeometryDestruct, C::GeometryCopy);
    if (state.geometry_offset < 0) return false;
    return runtime.RegisterGeometryStream(
        kId, C::Identity, C::Identity,
        C::StreamSize) == state.geometry_offset;
}

std::uint32_t RwPlugin8452::GeometryConstruct(
    const std::uint32_t object, const RwPlugin8452State& state,
    IRwPlugin8452Lifecycle& runtime) {
    if (state.geometry_offset > 0) {
        const auto plugin = object +
                            static_cast<std::uint32_t>(state.geometry_offset);
        runtime.WriteWord(plugin, 0);
        runtime.WriteWord(plugin + 4u, 0);
        runtime.WriteWord(plugin + 8u, 0);
    }
    return object;
}

std::uint32_t RwPlugin8452::GeometryDestruct(
    const std::uint32_t object, const RwPlugin8452State& state,
    IRwPlugin8452Lifecycle& runtime) {
    return GeometryConstruct(object, state, runtime);
}

std::uint32_t RwPlugin8452::GeometryCopy(
    const std::uint32_t target, const std::uint32_t source,
    const RwPlugin8452State& state,
    IRwPlugin8452Lifecycle& runtime) {
    if (state.geometry_offset < 0) return target;
    const auto offset = static_cast<std::uint32_t>(state.geometry_offset);
    for (std::uint32_t field = 0; field < 12u; field += 4u)
        runtime.WriteWord(target + offset + field,
                          runtime.ReadWord(source + offset + field));
    return target;
}

std::uint32_t RwPlugin8452::PluginSize() { return 12u; }

std::uint32_t RwPlugin8452::ProcessClump(
    const std::uint32_t clump, IRwPlugin8452Runtime& runtime) {
    (void)runtime.ForAllAtomics(clump, RwPlugin8452Callback::ProcessAtomic);
    return 1u;
}

std::uint32_t RwPlugin8452::SetPluginData(
    const std::uint32_t atomic, const std::uint32_t callback,
    const RwPlugin8452State& state,
    IRwPlugin8452Runtime& runtime) {
    if (state.geometry_offset <= 0) return 0;
    if (state.engine_offset <= 0)
        return static_cast<std::uint32_t>(state.engine_offset);
    const auto plugin = atomic +
                        static_cast<std::uint32_t>(state.geometry_offset);
    runtime.WriteWord(plugin, callback);
    runtime.WriteWord(plugin + 4u, 0);
    runtime.WriteWord(plugin + 8u, 1);
    return atomic;
}

std::uint32_t RwPlugin8452::ProcessAtomic(
    const std::uint32_t atomic, const RwPlugin8452State& state,
    IRwPlugin8452Runtime& runtime) {
    if (state.geometry_offset < 0) return atomic;
    const auto plugin = atomic +
                        static_cast<std::uint32_t>(state.geometry_offset);
    if (runtime.ReadWord(plugin + 8u) != 0) return atomic;
    const auto custom = runtime.CustomRenderCallback();
    if (runtime.ReadWord(plugin) == custom) return atomic;
    (void)SetPluginData(atomic, runtime.ReadWord(atomic + 72u),
                        state, runtime);
    runtime.WriteWord(atomic + 72u,
                      custom != 0 ? custom : runtime.DefaultRenderCallback());
    return atomic;
}

std::uint32_t RwPlugin8452::RenderAtomic(
    const std::uint32_t atomic, const RwPlugin8452State& state,
    IRwPlugin8452Runtime& runtime) {
    if (state.geometry_offset < 0) return atomic;
    const auto camera = runtime.ActiveCamera();
    if (camera != 0) {
        const auto transform = runtime.GetObjectTransform(
            runtime.ReadWord(atomic + 4u));
        if ((runtime.ReadWord(atomic + 76u) & 2u) != 0)
            runtime.ResyncAtomicSphere(atomic);
        RwPlugin8452Sphere sphere{
            std::bit_cast<float>(runtime.ReadWord(atomic + 28u)),
            std::bit_cast<float>(runtime.ReadWord(atomic + 32u)),
            std::bit_cast<float>(runtime.ReadWord(atomic + 36u)),
            std::bit_cast<float>(runtime.ReadWord(atomic + 40u))};
        runtime.CalculateSphere(sphere, transform);
        const auto result = runtime.FrustumTestSphere(camera, sphere);
        runtime.WriteWord(
            atomic + static_cast<std::uint32_t>(state.geometry_offset) + 4u,
            static_cast<std::uint32_t>(result));
    }
    const auto callback = runtime.ReadWord(
        atomic + static_cast<std::uint32_t>(state.geometry_offset));
    runtime.InvokeRender(callback, atomic);
    return atomic;
}

std::uint32_t RwPlugin8452::ResetClump(
    const std::uint32_t clump, IRwPlugin8452Runtime& runtime) {
    (void)runtime.ForAllAtomics(clump, RwPlugin8452Callback::ResetAtomic);
    return 1u;
}

std::uint32_t RwPlugin8452::ResetAtomic(
    const std::uint32_t atomic, const RwPlugin8452State& state,
    IRwPlugin8452Runtime& runtime) {
    if (state.geometry_offset < 0) return atomic;
    const auto plugin = atomic +
                        static_cast<std::uint32_t>(state.geometry_offset);
    if (runtime.ReadWord(plugin + 8u) != 0) {
        const auto saved = runtime.ReadWord(plugin);
        runtime.WriteWord(atomic + 72u,
                          saved != 0 ? saved : runtime.DefaultRenderCallback());
        runtime.WriteWord(plugin + 4u, 0);
        runtime.WriteWord(plugin + 8u, 0);
    }
    return atomic;
}

std::uint32_t RwPlugin8452::GetFrustumResult(
    const std::uint32_t object, const RwPlugin8452State& state,
    IRwPlugin8452Runtime& runtime) {
    if (state.geometry_offset < 0) return 0;
    return runtime.ReadWord(
        object + static_cast<std::uint32_t>(state.geometry_offset) + 4u);
}

std::uint32_t RwPlugin8452::QueryClump(
    const std::uint32_t clump, IRwPlugin8452Runtime& runtime) {
    std::uint32_t result = 0;
    runtime.ForAllAtomicsQuery(
        clump, RwPlugin8452Callback::QueryAtomic, result);
    return result;
}

std::uint32_t RwPlugin8452::QueryAtomic(
    const std::uint32_t atomic, std::uint32_t& result,
    const RwPlugin8452State& state,
    IRwPlugin8452Runtime& runtime) {
    if (GetActiveFlag(atomic, state, runtime) == 0) {
        result = 1;
        return 0;
    }
    result = GetFrustumResult(atomic, state, runtime) != 0 ? 1u : 0u;
    return result != 0 ? 0u : atomic;
}

std::uint32_t RwPlugin8452::GetActiveFlag(
    const std::uint32_t object, const RwPlugin8452State& state,
    IRwPlugin8452Runtime& runtime) {
    if (state.geometry_offset < 0) return 0;
    return runtime.ReadWord(
        object + static_cast<std::uint32_t>(state.geometry_offset) + 8u);
}

}  // namespace pal4
