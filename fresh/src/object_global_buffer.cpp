#include "pal4/object_global_buffer.h"

namespace pal4 {

int ObjectGlobalBufferRuntimeStub::RegisterCleanup(std::uint32_t) {
    return 0;
}

void ObjectGlobalBufferRuntimeStub::Free(std::uint32_t) {}

void ObjectGlobalBuffer::InitializeMembers(
    ObjectGlobalBufferState& state, std::uint32_t ecx_value) {
    state.source_byte = static_cast<std::uint8_t>(ecx_value >> 24u);
    state.words.fill(0u);
}

int ObjectGlobalBuffer::InitializeAndRegister(
    ObjectGlobalBufferState& state, std::uint32_t ecx_value,
    IObjectGlobalBufferRuntime& runtime) {
    InitializeMembers(state, ecx_value);
    return runtime.RegisterCleanup(kCleanupAddress);
}

void ObjectGlobalBuffer::CleanupOnce(
    ObjectGlobalBufferState& state, IObjectGlobalBufferRuntime& runtime) {
    if ((state.cleanup_guard & 1u) != 0u) return;
    state.cleanup_guard = static_cast<std::uint8_t>(state.cleanup_guard | 1u);
    runtime.Free(state.words[0]);
    state.words.fill(0u);
}

}  // namespace pal4
