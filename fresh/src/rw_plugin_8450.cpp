#include "pal4/rw_plugin_8450.h"

#include <bit>

namespace pal4 {

bool RwPlugin8450::Register(RwPlugin8450State& state,
                            IRwPlugin8450Lifecycle& runtime) {
    using Callback = RwPlugin8450Callback;
    state.engine_offset = runtime.RegisterEngine(
        8, 8450, Callback::EngineConstruct, Callback::EngineDestruct);
    if (state.engine_offset < 0)
        return false;

    state.atomic_offset = runtime.RegisterAtomic(
        12, 8450, Callback::AtomicConstruct,
        Callback::AtomicDestruct, Callback::AtomicCopy);
    if (state.atomic_offset < 0)
        return false;

    return runtime.RegisterAtomicStream(
        8450, Callback::StreamReadIdentity,
        Callback::StreamWriteIdentity,
        Callback::StreamSize) == state.atomic_offset;
}

std::uint32_t RwPlugin8450::EngineConstruct(
    const std::uint32_t object, const RwPlugin8450State& state,
    IRwPlugin8450Lifecycle& runtime) {
    if (state.engine_offset > 0) {
        const auto address = runtime.EngineBase() +
                             static_cast<std::uint32_t>(state.engine_offset);
        runtime.WriteWord(address, 0);
        runtime.WriteWord(address + 4, 1);
    }
    return object;
}

std::uint32_t RwPlugin8450::EngineDestruct(
    const std::uint32_t object, const RwPlugin8450State& state,
    IRwPlugin8450Lifecycle& runtime) {
    if (state.engine_offset > 0) {
        const auto address = runtime.EngineBase() +
                             static_cast<std::uint32_t>(state.engine_offset);
        runtime.WriteWord(address, 0);
        runtime.WriteWord(address + 4, 0);
    }
    return object;
}

std::uint32_t RwPlugin8450::AtomicConstruct(
    const std::uint32_t object, const RwPlugin8450State& state,
    IRwPlugin8450Lifecycle& runtime) {
    if (state.atomic_offset > 0) {
        const auto address = object +
                             static_cast<std::uint32_t>(state.atomic_offset);
        runtime.WriteWord(address, 0);
        runtime.WriteWord(address + 4, 0);
        runtime.WriteWord(address + 8, 1);
    }
    return object;
}

std::uint32_t RwPlugin8450::AtomicDestruct(
    const std::uint32_t object, const RwPlugin8450State& state,
    IRwPlugin8450Lifecycle& runtime) {
    return AtomicConstruct(object, state, runtime);
}

std::uint32_t RwPlugin8450::AtomicCopy(
    const std::uint32_t target, const std::uint32_t source,
    const RwPlugin8450State& state, IRwPlugin8450Lifecycle& runtime) {
    const auto offset = static_cast<std::uint32_t>(state.atomic_offset);
    for (std::uint32_t index = 0; index < 3; ++index)
        runtime.WriteWord(target + offset + 4 * index,
                          runtime.ReadWord(source + offset + 4 * index));
    return target;
}

std::uint32_t RwPlugin8450::ProcessClump(
    const std::uint32_t clump, IRwPlugin8450Runtime& runtime) {
    if (!clump)
        return clump;
    return runtime.ForAllAtomics(clump, RwPlugin8450Callback::ProcessAtomic, 0);
}

std::uint32_t RwPlugin8450::ProcessAtomic(
    const std::uint32_t atomic, const RwPlugin8450State& state,
    IRwPlugin8450Runtime& runtime) {
    const auto old_callback = runtime.ReadWord(atomic + 72);
    const auto render_callback = runtime.RenderCallbackAddress();
    if (old_callback != render_callback &&
        !runtime.ReadWord(atomic +
                          static_cast<std::uint32_t>(state.atomic_offset) + 4)) {
        static_cast<void>(SetPluginData(atomic, old_callback, state, runtime));
        runtime.WriteWord(atomic + 72, render_callback);
        if (!render_callback)
            runtime.WriteWord(atomic + 72,
                              runtime.DefaultRenderCallbackAddress());
        static_cast<void>(SetMaterialColor(atomic, 0xffffffffu, runtime));
    }
    return atomic;
}

std::optional<std::uint32_t> RwPlugin8450::SetPluginData(
    const std::uint32_t atomic, const std::uint32_t callback,
    const RwPlugin8450State& state, IRwPlugin8450Runtime& runtime) {
    if (state.atomic_offset <= 0)
        return std::nullopt;
    if (state.engine_offset <= 0)
        return static_cast<std::uint32_t>(state.engine_offset);
    const auto address = atomic +
                         static_cast<std::uint32_t>(state.atomic_offset);
    runtime.WriteWord(address, callback);
    runtime.WriteWord(address + 4, 1);
    runtime.WriteWord(address + 8, 1);
    return atomic;
}

std::int32_t RwPlugin8450::SetMaterialColor(
    const std::uint32_t atomic, const std::uint32_t color,
    IRwPlugin8450Runtime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24);
    const auto flags = runtime.ReadWord(geometry + 8);
    const auto count = static_cast<std::int32_t>(runtime.ReadWord(geometry + 36));
    runtime.WriteWord(geometry + 8, flags | 0x40u);
    if (count <= 0)
        return 0;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto materials = runtime.ReadWord(geometry + 32);
        const auto material = runtime.ReadWord(
            materials + 4 * static_cast<std::uint32_t>(index));
        runtime.WriteWord(material + 4, color);
    }
    return count;
}

std::uint32_t RwPlugin8450::RenderAtomic(
    const std::uint32_t atomic, const RwPlugin8450State& state,
    IRwPlugin8450Runtime& runtime) {
    const auto engine_data = runtime.EngineBase() +
                             static_cast<std::uint32_t>(state.engine_offset);
    if (runtime.ReadWord(engine_data + 4)) {
        const double distance = CalculateDistanceToCamera(atomic, runtime);
        const double limit = std::bit_cast<float>(runtime.ReadWord(engine_data));
        const auto plugin_flags = atomic +
                                  static_cast<std::uint32_t>(state.atomic_offset) + 8;
        if (distance >= limit) {
            runtime.WriteWord(plugin_flags, 0);
            return atomic;
        }

        const double difference = limit - distance;
        if (difference <= 50.0) {
            const double scaled =
                difference * std::bit_cast<float>(0x3ca3d70au) *
                std::bit_cast<float>(0x437f0000u) + 0.5;
            const auto intensity = static_cast<std::uint8_t>(
                runtime.FloatToLong(scaled));
            const auto color = static_cast<std::uint32_t>(intensity) *
                               0x01010101u;
            static_cast<void>(SetMaterialColor(atomic, color, runtime));
            runtime.WriteWord(plugin_flags, 0);
        } else if (!runtime.ReadWord(plugin_flags)) {
            static_cast<void>(SetMaterialColor(atomic, 0xffffffffu, runtime));
            runtime.WriteWord(plugin_flags, 1);
        }
    }

    const auto callback = GetPluginData(atomic, state, runtime);
    if (callback)
        runtime.InvokeCallback(callback, atomic);
    return atomic;
}

std::uint32_t RwPlugin8450::GetPluginData(
    const std::uint32_t atomic, const RwPlugin8450State& state,
    IRwPlugin8450Runtime& runtime) {
    if (state.atomic_offset <= 0 || state.engine_offset <= 0)
        return 0;
    return runtime.ReadWord(atomic +
                            static_cast<std::uint32_t>(state.atomic_offset));
}

double RwPlugin8450::CalculateDistanceToCamera(
    const std::uint32_t atomic, IRwPlugin8450Runtime& runtime) {
    if (runtime.ReadWord(atomic + 76) & 2u)
        runtime.ResyncAtomic(atomic);
    const auto object_transform = runtime.ObjectTransform(
        runtime.ReadWord(atomic + 4));
    const auto center = runtime.CalculatePosition(atomic + 28,
                                                   object_transform);
    const auto camera = runtime.ReadWord(runtime.EngineBase());
    const auto camera_transform = runtime.ObjectTransform(
        runtime.ReadWord(camera + 4));
    std::array<float, 3> delta{};
    for (std::uint32_t i = 0; i < 3; ++i) {
        const auto camera_position = std::bit_cast<float>(
            runtime.ReadWord(camera_transform + 48 + 4 * i));
        delta[i] = static_cast<float>(
            static_cast<double>(center[i]) - camera_position);
    }
    return runtime.ApproximateMagnitude(delta);
}

std::uint32_t RwPlugin8450::ResetClump(
    const std::uint32_t clump, IRwPlugin8450Runtime& runtime) {
    if (!clump)
        return clump;
    return runtime.ForAllAtomics(clump, RwPlugin8450Callback::ResetAtomic, 0);
}

std::uint32_t RwPlugin8450::ResetAtomic(
    const std::uint32_t atomic, const RwPlugin8450State& state,
    IRwPlugin8450Runtime& runtime) {
    const auto flags = atomic +
                       static_cast<std::uint32_t>(state.atomic_offset) + 4;
    if (runtime.ReadWord(flags)) {
        const auto callback = GetPluginData(atomic, state, runtime);
        runtime.WriteWord(atomic + 72, callback);
        if (!callback)
            runtime.WriteWord(atomic + 72,
                              runtime.DefaultRenderCallbackAddress());
        runtime.WriteWord(flags, 0);
        runtime.WriteWord(flags + 4, 0);
        static_cast<void>(SetMaterialColor(atomic, 0xffffffffu, runtime));
    }
    return atomic;
}

std::uint32_t RwPlugin8450::QueryClump(
    const std::uint32_t clump, IRwPlugin8450Runtime& runtime) {
    if (!clump)
        return clump;
    std::uint32_t output = 0;
    runtime.ForAllAtomicsWithOutput(clump,
                                    RwPlugin8450Callback::QueryAtomic,
                                    output);
    return output;
}

std::uint32_t RwPlugin8450::QueryAtomic(
    const std::uint32_t atomic, std::uint32_t& output,
    const RwPlugin8450State& state, IRwPlugin8450Runtime& runtime) {
    const auto address = atomic +
                         static_cast<std::uint32_t>(state.atomic_offset);
    if (!runtime.ReadWord(address + 4)) {
        output = 1;
        return 0;
    }
    output = runtime.ReadWord(address + 8);
    return output == 0 ? atomic : 0;
}

std::uint32_t RwPlugin8450::QueryDirect(
    const std::uint32_t object, const RwPlugin8450State& state,
    IRwPlugin8450Runtime& runtime) {
    if (!object)
        return object;
    std::uint32_t output = 0;
    static_cast<void>(QueryAtomic(object, output, state, runtime));
    return output;
}

std::uint32_t RwPlugin8450::SetEngineFloat(
    const float value, const RwPlugin8450State& state,
    IRwPlugin8450Runtime& runtime) {
    const auto base = runtime.EngineBase();
    runtime.WriteWord(base +
                          static_cast<std::uint32_t>(state.engine_offset),
                      std::bit_cast<std::uint32_t>(value));
    return base;
}

std::uint32_t RwPlugin8450::SetEngineInt(
    const std::uint32_t value, const RwPlugin8450State& state,
    IRwPlugin8450Runtime& runtime) {
    runtime.WriteWord(runtime.EngineBase() +
                          static_cast<std::uint32_t>(state.engine_offset) + 4,
                      value);
    return value;
}

}  // namespace pal4
