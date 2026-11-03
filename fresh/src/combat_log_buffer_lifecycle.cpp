#include "pal4/combat_log_buffer_lifecycle.h"

namespace pal4 {

std::int32_t CombatLogBufferLifecycle::CleanupBuffer(
    std::uint32_t buffer, ICombatLogBufferLifecycleRuntime& runtime) {
    const auto flags = runtime.ReadByte(buffer + 68u);
    runtime.WriteWord(buffer, 0x84814cu);
    std::int32_t result = flags;
    if ((flags & 1u) != 0u)
        result = runtime.Free(runtime.ReadWord(runtime.ReadWord(buffer + 12u)));
    const auto word_flags = runtime.ReadWord(buffer + 68u);
    runtime.WriteWord(buffer + 60u, 0u);
    runtime.WriteWord(buffer, 0x868310u);
    runtime.WriteWord(buffer + 68u, word_flags & ~1u);

    const auto facet = runtime.ReadWord(buffer + 52u);
    if (facet != 0u) {
        auto references = runtime.ReadWord(facet + 4u);
        result = static_cast<std::int32_t>(references);
        if (references != 0u && references != 0xffffffffu) {
            --references;
            runtime.WriteWord(facet + 4u, references);
            result = static_cast<std::int32_t>(references);
        }
        if (references == 0u)
            result = runtime.DestroyFacet(facet, 1u);
    }
    return result;
}

std::uint32_t CombatLogBufferLifecycle::CleanupStream(
    std::uint32_t ios, ICombatLogBufferLifecycleRuntime& runtime) {
    const auto stream = ios - 80u;
    const auto stream_vtable = runtime.ReadWord(stream);
    const auto stream_offset = runtime.ReadWord(stream_vtable + 4u);
    runtime.WriteWord(stream + stream_offset, 0x848148u);
    static_cast<void>(CleanupBuffer(ios - 76u, runtime));
    const auto result = runtime.ReadWord(stream);
    runtime.WriteWord(stream + runtime.ReadWord(result + 4u),
                      0x8682c8u);
    return result;
}

}  // namespace pal4
