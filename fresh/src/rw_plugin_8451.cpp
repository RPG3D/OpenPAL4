#include "pal4/rw_plugin_8451.h"

namespace pal4 {

bool RwPlugin8451::Register(RwPlugin8451State& state,
                            IRwPlugin8451Lifecycle& runtime) {
    using Callback = RwPlugin8451Callback;
    state.engine_offset = runtime.RegisterEngine(
        1, 8451, Callback::Identity, Callback::Identity);
    if (state.engine_offset < 0)
        return false;
    state.geometry_offset = runtime.RegisterGeometry(
        8, 8451, Callback::GeometryConstruct,
        Callback::GeometryDestruct, Callback::GeometryCopy);
    if (state.geometry_offset < 0)
        return false;
    return runtime.RegisterGeometryStream(
        8451, Callback::Identity, Callback::Identity,
        Callback::StreamSize) == state.geometry_offset;
}

std::uint32_t RwPlugin8451::GeometryConstruct(
    const std::uint32_t object, const RwPlugin8451State& state,
    IRwPlugin8451Lifecycle& runtime) {
    if (state.geometry_offset > 0) {
        const auto address = object +
                             static_cast<std::uint32_t>(state.geometry_offset);
        runtime.WriteWord(address, 0);
        runtime.WriteWord(address + 4, 0);
    }
    return object;
}

std::uint32_t RwPlugin8451::GeometryDestruct(
    const std::uint32_t object, const RwPlugin8451State& state,
    IRwPlugin8451Lifecycle& runtime) {
    return GeometryConstruct(object, state, runtime);
}

std::uint32_t RwPlugin8451::GeometryCopy(
    const std::uint32_t target, const std::uint32_t source,
    const RwPlugin8451State& state, IRwPlugin8451Lifecycle& runtime) {
    const auto offset = static_cast<std::uint32_t>(state.geometry_offset);
    runtime.WriteWord(target + offset,
                      runtime.ReadWord(source + offset));
    runtime.WriteWord(target + offset + 4,
                      runtime.ReadWord(source + offset + 4));
    return target;
}

std::uint32_t RwPlugin8451::PluginSize() { return 8; }

std::uint32_t RwPlugin8451::HideClumpHookAtomics(
    const std::uint32_t clump, IRwPlugin8451Runtime& runtime) {
    return runtime.ForAllAtomics(
        clump, RwPlugin8451Callback::HideHookAtomic, 0);
}

std::uint32_t RwPlugin8451::HideHookAtomic(
    const std::uint32_t atomic, const RwPlugin8451State& state,
    IRwPlugin8451Runtime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24);
    const auto count = runtime.UserDataCount(geometry);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto array = runtime.UserDataArray(
            geometry, static_cast<std::uint32_t>(index));
        if (runtime.ReadWord(array + 4) != 3)
            continue;
        const auto text = runtime.UserDataText(
            array, static_cast<std::uint32_t>(index));
        if (text.size() <= 5 || text.substr(0, 5) != "hook_")
            continue;
        const auto plugin = atomic +
                            static_cast<std::uint32_t>(state.geometry_offset);
        if (runtime.ReadWord(plugin + 4))
            continue;
        const auto callback = runtime.HookRenderCallbackAddress();
        if (runtime.ReadWord(atomic + 72) == callback)
            continue;
        runtime.WriteWord(plugin + 4, 1);
        runtime.WriteWord(plugin, 0);
        runtime.WriteWord(atomic + 72, callback);
        if (!callback)
            runtime.WriteWord(atomic + 72,
                              runtime.DefaultRenderCallbackAddress());
    }
    return atomic;
}

std::uint32_t RwPlugin8451::RenderHiddenAtomic(
    const std::uint32_t atomic, IRwPlugin8451Runtime& runtime) {
    const auto frame = runtime.ReadWord(atomic + 4);
    if (runtime.FrameCount(frame) > 1)
        runtime.UpdateFrameObjects(frame);
    return atomic;
}

std::uint32_t RwPlugin8451::ProcessClumpUserData(
    const std::uint32_t clump, const std::string_view name,
    const std::uint32_t frame, IRwPlugin8451Runtime& runtime) {
    return runtime.ForAllAtomicsWithAttachment(
        clump, RwPlugin8451Callback::AttachFrameToAtomic, name, frame);
}

std::uint32_t RwPlugin8451::AttachFrameToMatchingAtomic(
    const std::uint32_t atomic, const std::string_view name,
    const std::uint32_t frame, const RwPlugin8451State& state,
    IRwPlugin8451Runtime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24);
    const auto count = runtime.UserDataCount(geometry);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto array = runtime.UserDataArray(
            geometry, static_cast<std::uint32_t>(index));
        if (runtime.ReadWord(array + 4) != 3)
            continue;
        auto text = runtime.UserDataText(
            array, static_cast<std::uint32_t>(index));
        if (text.size() > name.size())
            text = text.substr(0, name.size());
        if (text != name)
            continue;
        const auto plugin = atomic +
                            static_cast<std::uint32_t>(state.geometry_offset);
        if (!runtime.ReadWord(plugin + 4))
            continue;
        runtime.WriteWord(plugin, frame);
        if (frame) {
            if (runtime.ReadWord(frame + 4))
                runtime.RemoveFrameChild(frame);
            runtime.AddFrameChild(runtime.ReadWord(atomic + 4), frame);
        }
        return 0;
    }
    return atomic;
}

std::uint32_t RwPlugin8451::RemoveFrameChildAndClear(
    const std::uint32_t clump, const std::string_view name,
    const std::uint32_t frame, IRwPlugin8451Runtime& runtime) {
    if (frame && runtime.ReadWord(frame + 4))
        runtime.RemoveFrameChild(frame);
    return ProcessClumpUserData(clump, name, 0, runtime);
}

std::uint32_t RwPlugin8451::QueryClumpUserData(
    const std::uint32_t clump,
    const std::optional<std::string_view> name,
    IRwPlugin8451Runtime& runtime) {
    if (!name)
        return 0;
    std::uint32_t output = 0;
    runtime.ForAllAtomicsWithQuery(
        clump, RwPlugin8451Callback::QueryUserDataAtomic,
        *name, output);
    return output;
}

std::uint32_t RwPlugin8451::QueryAtomicUserData(
    const std::uint32_t atomic, const std::string_view name,
    std::uint32_t& output, IRwPlugin8451Runtime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24);
    const auto count = runtime.UserDataCount(geometry);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto array = runtime.UserDataArray(
            geometry, static_cast<std::uint32_t>(index));
        if (runtime.ReadWord(array + 4) != 3)
            continue;
        auto text = runtime.UserDataText(
            array, static_cast<std::uint32_t>(index));
        if (text.size() > name.size())
            text = text.substr(0, name.size());
        if (text == name) {
            output = atomic;
            return 0;
        }
    }
    return atomic;
}

}  // namespace pal4
