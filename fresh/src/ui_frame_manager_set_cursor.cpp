#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace pal4 {

std::uint8_t UiFrameManagerSingleton::SetCursor(
    const std::uint32_t manager, const char* name,
    IUiFrameManagerSetCursorRuntime& runtime) {
    StringBufferState query{};
    (void)runtime.SetErrorFlag(query, 0u);
    runtime.AssignCString(query, name);
    const auto candidate = runtime.FindNode(manager, query);
    const auto end = runtime.TreeEnd(manager);
    const auto node =
        candidate != end &&
                runtime.CompareNodeName(query, candidate + 12u) >= 0
            ? candidate : end;
    const auto query_cleanup = runtime.SetErrorFlag(query, 1u);
    if (node == end) return query_cleanup;

    runtime.SetCursor(runtime.ReadWord(node + 28u));
    auto& current = runtime.CurrentName(manager);
    const auto size = std::strlen(name);
    if (size > 0xfffffffdu) throw std::length_error("cursor name");
    const auto length = static_cast<std::uint32_t>(size);
    const auto reference = current.data == 0u
                               ? 0u : runtime.ReadByte(current.data - 1u);
    const bool shared = current.data != 0u &&
                        reference != 0u && reference != 0xffu;
    if (length == 0u) {
        if (shared) {
            runtime.WriteByte(current.data - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
            return runtime.SetErrorFlag(current, 0u);
        }
        return runtime.SetErrorFlag(current, 1u);
    }

    if (shared) {
        runtime.Reallocate(current, length);
    } else if (current.capacity > 0x1fu ||
               current.capacity < length) {
        (void)runtime.SetErrorFlag(current, 1u);
        runtime.Reallocate(current, length);
    }
    runtime.CopyCString(current, name, length);
    return static_cast<std::uint8_t>(current.data);
}

}  // namespace pal4
