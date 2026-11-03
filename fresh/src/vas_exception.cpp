#include "pal4/vas_exception.h"

namespace pal4 {

VasExceptionState& VasException::CopyConstruct(
    VasExceptionState& destination,
    const VasExceptionState& source,
    IVasExceptionRuntime& runtime) {
    runtime.CopyBase(destination, source);
    try {
        destination.words[3u] = (destination.words[3u] & 0xffffff00u) |
            (source.words[3u] & 0xffu);
        destination.words[4u] = 0u;
        destination.words[5u] = 0u;
        destination.words[6u] = 0u;
        const auto size = source.words[5u];

        if (&destination == &source) {
            runtime.DetachAndReset(destination);
            const auto remaining = destination.words[5u];
            if (remaining != 0u) {
                const auto buffer = destination.words[4u];
                runtime.CopyText(buffer, buffer + remaining, 0u);
                if (runtime.AllocateText(destination, 0u, 0u)) {
                    runtime.SetTextSize(destination, 0u);
                }
            }
            runtime.DetachAndReset(destination);
        } else {
            const auto source_buffer = source.words[4u] != 0u
                ? source.words[4u] : runtime.EmptyTextAddress();
            if (size != 0u && runtime.ReferenceByte(source_buffer) < 0xfeu) {
                runtime.SetSharedTextFlag(destination);
                destination.words[4u] = source_buffer;
                destination.words[5u] = source.words[5u];
                destination.words[6u] = source.words[6u];
                runtime.IncrementReferenceByte(source_buffer);
            } else if (runtime.AllocateText(destination, size, 1u)) {
                runtime.CopyText(destination.words[4u], source_buffer, size);
                destination.words[5u] = size;
                runtime.WriteByte(destination.words[4u] + size, 0u);
            }
        }
        destination.words[0u] = kVtable;
        return destination;
    } catch (...) {
        runtime.DestroyBase(destination);
        throw;
    }
}

void VasException::Destroy(VasExceptionState& state,
                           IVasExceptionRuntime& runtime) {
    const auto buffer = state.words[4u];
    if (buffer != 0u) {
        const auto count = runtime.ReferenceByte(buffer);
        if (count == 0u || count == 0xffu) {
            runtime.FreeText(buffer - 1u);
        } else {
            runtime.DecrementReferenceByte(buffer);
        }
    }
    state.words[4u] = 0u;
    state.words[5u] = 0u;
    state.words[6u] = 0u;
    runtime.DestroyBase(state);
}

}  // namespace pal4
