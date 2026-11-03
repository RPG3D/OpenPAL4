#include "pal4/select_dialog.h"

#include <cstdint>
#include <initializer_list>

namespace pal4 {

std::uint32_t SelectDialog::Destruct(
    const std::uint32_t object,
    ISelectDialogLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 40u, kNavigationVtable);
    runtime.CleanupCallbacks(object);
    runtime.Free(runtime.ReadWord(object + 104u));
    for (const auto offset : {104u, 108u, 112u})
        runtime.WriteWord(object + offset, 0u);
    runtime.Free(runtime.ReadWord(object + 88u));
    for (const auto offset : {88u, 92u, 96u})
        runtime.WriteWord(object + offset, 0u);
    const auto begin = runtime.ReadWord(object + 72u);
    const auto end = runtime.ReadWord(object + 76u);
    for (auto record = begin; record != end; record += 16u)
        runtime.SetErrorFlag(record, 1u);
    runtime.Free(begin);
    for (const auto offset : {72u, 76u, 80u})
        runtime.WriteWord(object + offset, 0u);
    runtime.Free(runtime.ReadWord(object + 48u));
    for (const auto offset : {48u, 52u, 56u})
        runtime.WriteWord(object + offset, 0u);
    return runtime.DestructBase(object);
}

}  // namespace pal4
