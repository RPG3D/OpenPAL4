#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>

namespace pal4 {
namespace {

std::uint8_t ReleaseReference(
    const StringBufferState& text,
    IUiFrameManagerLoadCursorRuntime& runtime) {
    if (text.data == 0u) return 0u;
    const auto header = text.data - 1u;
    const auto reference = runtime.ReadByte(header);
    if (reference == 0u || reference == 0xffu)
        return static_cast<std::uint8_t>(runtime.Free(header));
    runtime.WriteByte(header,
                      static_cast<std::uint8_t>(reference - 1u));
    return static_cast<std::uint8_t>(header);
}

}  // namespace

std::uint8_t UiFrameManagerSingleton::LoadCursor(
    const std::uint32_t manager, const char* name,
    IUiFrameManagerLoadCursorRuntime& runtime) {
    StringBufferState query{};
    (void)runtime.SetErrorFlag(query, 0u);
    runtime.AssignCString(query, name);
    const auto end = runtime.TreeEnd(manager);
    const auto found = runtime.Find(manager, query);
    const auto query_cleanup = runtime.SetErrorFlag(query, 1u);
    if (found != end) return query_cleanup;

    runtime.LogMissingCursor(
        "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp",
        0x82u,
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiCursorManager loadCursor %s OK\n\n\n",
        name);

    StringBufferState path{};
    (void)runtime.SetErrorFlag(path, 0u);
    runtime.AssignCString(path, "gamedata\\cursor\\");
    runtime.AppendCString(path, name);
    const auto image = runtime.LoadImage(path, 2u, 32u, 32u, 0x10u);

    StringBufferState source_name{};
    (void)runtime.SetErrorFlag(source_name, 0u);
    runtime.AssignCString(source_name, name);
    UiCursorRecord cursor{};
    (void)runtime.SetErrorFlag(cursor.name, 0u);
    runtime.AssignString(cursor.name, source_name);
    cursor.image = image;
    runtime.InsertOrFind(manager, cursor);
    (void)runtime.SetErrorFlag(cursor.name, 1u);
    (void)ReleaseReference(source_name, runtime);
    return ReleaseReference(path, runtime);
}

}  // namespace pal4
