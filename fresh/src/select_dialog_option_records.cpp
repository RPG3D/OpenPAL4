#include "pal4/select_dialog_option_records.h"

#include <cstdint>

namespace pal4 {

void SelectDialogOptionRecords::Cleanup(
    const std::uint32_t begin, const std::uint32_t end,
    ISelectDialogRecordRuntime& runtime) {
    for (auto record = begin; record != end; record += 16u) {
        const auto pointer = runtime.ReadWord(record + 4u);
        if (pointer != 0u) {
            const auto reference = runtime.ReadByte(pointer - 1u);
            if (reference == 0u || reference == 0xffu) {
                runtime.Free(pointer - 1u);
            } else {
                runtime.WriteByte(pointer - 1u,
                                  static_cast<std::uint8_t>(
                                      reference - 1u));
            }
        }
        runtime.WriteWord(record + 4u, 0u);
        runtime.WriteWord(record + 8u, 0u);
        runtime.WriteWord(record + 12u, 0u);
    }
}

std::uint32_t SelectDialogOptionRecords::CopyRange(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t destination,
    ISelectDialogRecordRuntime& runtime) {
    auto target = destination;
    for (auto record = begin; record != end; record += 16u) {
        runtime.CopyRecord(target, record);
        target += 16u;
    }
    return target;
}

void SelectDialogOptionRecords::CopyRepeated(
    const std::uint32_t destination, const std::uint32_t count,
    const std::uint32_t source,
    ISelectDialogRecordRuntime& runtime) {
    auto target = destination;
    for (auto remaining = count; remaining != 0u; --remaining) {
        runtime.CopyRecord(target, source);
        target += 16u;
    }
}

}  // namespace pal4
