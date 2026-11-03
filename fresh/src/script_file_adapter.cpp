#include "pal4/script_file_adapter.h"

namespace pal4 {

ScriptFileAdapterState& ScriptFileAdapter::Construct(
    ScriptFileAdapterState& state, const std::uint32_t handle) {
    state.vtable = kVtable;
    state.handle = handle;
    return state;
}

std::uint32_t ScriptFileAdapter::Destroy(
    ScriptFileAdapterState& state, const std::uint32_t object,
    const std::uint32_t flags) {
    ResetBase(state);
    if ((flags & 1u) != 0u) runtime_.Free(object);
    return object;
}

void ScriptFileAdapter::ResetBase(ScriptFileAdapterState& state) {
    state.handle = 0u;
    state.vtable = kBaseVtable;
}

std::uint32_t ScriptFileAdapter::GetSize(
    const ScriptFileAdapterState& state) {
    const auto script_data = runtime_.GetPackedScriptData();
    if (script_data != 0u)
        return runtime_.GetPackedScriptLength(script_data, state.handle);

    const auto original_position = runtime_.GetPosition(state.handle);
    (void)runtime_.Seek(state.handle, 0, 2u);
    const auto size = runtime_.GetPosition(state.handle);
    (void)runtime_.Seek(state.handle,
                        static_cast<std::int32_t>(original_position), 0u);
    return size;
}

std::uint32_t ScriptFileAdapter::GetPosition(
    const ScriptFileAdapterState& state) {
    return runtime_.GetPosition(state.handle);
}

bool ScriptFileAdapter::Read(const ScriptFileAdapterState& state,
                             const std::uint32_t buffer,
                             const std::uint32_t bytes) {
    return runtime_.Read(buffer, bytes, 1u, state.handle) == 1u;
}

bool ScriptFileAdapter::Write(const ScriptFileAdapterState& state,
                              const std::uint32_t buffer,
                              const std::uint32_t bytes) {
    return runtime_.Write(buffer, bytes, 1u, state.handle) == 1u;
}

}  // namespace pal4
