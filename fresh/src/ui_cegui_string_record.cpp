#include "pal4/ui_cegui_string_record.h"

namespace pal4 {

void UiCeguiStringRecord::Free(
    const std::uint32_t record,
    IUiCeguiStringRecordRuntime& runtime) {
    const auto text = runtime.ReadWord(record + 156u);
    if (text != 0u) {
        const auto count = runtime.ReadByte(text - 1u);
        if (count == 0u || count == 0xffu)
            runtime.FreeMemory(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(record + 156u, 0u);
    runtime.WriteWord(record + 160u, 0u);
    runtime.WriteWord(record + 164u, 0u);
    runtime.DestroyString(record);
}

}  // namespace pal4
