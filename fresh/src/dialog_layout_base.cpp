#include "pal4/dialog_layout_base.h"

#include <cstdint>
#include <string>

namespace pal4 {

namespace {

std::string CStringPrefix(const std::string& value) {
    return value.substr(0u, value.find('\0'));
}

void AssignIfAllocated(const std::uint32_t record,
                       const std::string& value,
                       IDialogLayoutRuntime& runtime) {
    if (runtime.AllocateBuffer(
            record, static_cast<std::uint32_t>(value.size()), true)) {
        runtime.CopyBytes(record, value);
        runtime.SetBufferSize(record,
                              static_cast<std::uint32_t>(value.size()));
    }
}

}  // namespace

std::uint32_t DialogLayoutBase::LoadLayout(
    const std::uint32_t object, const std::string& layout,
    const std::string* prefix, const std::string* name_override,
    IDialogLayoutRuntime& runtime) {
    const auto byte = runtime.IndeterminateBufferByte();
    const auto layout_name = CStringPrefix(layout);
    unsigned stage = 0u;
    try {
        const auto name_record = object + 4u;
        runtime.WriteByte(name_record, byte);
        runtime.SetErrorFlag(name_record, 0u);
        stage = 1u;
        AssignIfAllocated(name_record, layout_name, runtime);

        const auto prefix_record = object + 20u;
        runtime.WriteByte(prefix_record, byte);
        runtime.SetErrorFlag(prefix_record, 0u);
        stage = 2u;
        runtime.WriteWord(object, kVtable);
        runtime.ProcessPkScripts();

        const auto path =
            std::string("gamedata\\ui\\layouts\\") +
            layout_name + ".xml";
        if (prefix != nullptr) {
            AssignIfAllocated(prefix_record,
                              CStringPrefix(*prefix), runtime);
        } else {
            AssignIfAllocated(prefix_record, layout_name, runtime);
            runtime.AppendBuffer(prefix_record, "/");
        }
        const auto utf8_prefix = runtime.ConvertToUtf8(
            runtime.ReadBuffer(prefix_record));
        const auto root = runtime.LoadWindowLayout(
            path, utf8_prefix, "");
        runtime.WriteWord(object + 36u, root);
        if (name_override != nullptr)
            AssignIfAllocated(name_record,
                              CStringPrefix(*name_override), runtime);
        runtime.CleanupPkScripts();
        return object;
    } catch (...) {
        if (stage >= 2u) runtime.CleanupBuffer(object + 20u);
        if (stage >= 1u) runtime.CleanupBuffer(object + 4u);
        throw;
    }
}

}  // namespace pal4
