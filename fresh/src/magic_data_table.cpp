#include "pal4/magic_data_table.h"

#include <initializer_list>

namespace pal4 {
namespace {

void ClearTriplet(std::uint32_t address,
                  IMagicDataTableRuntime& runtime) {
    for (std::uint32_t offset = 0; offset < 12; offset += 4)
        runtime.WriteWord(address + offset, 0);
}

void DestroyMagicRange(std::uint32_t begin, std::uint32_t end,
                       IMagicDataTableRuntime& runtime) {
    for (auto record = begin; record != end;
         record += MagicDataTable::kRecordBytes)
        static_cast<void>(runtime.DestroyMagicRecord(record));
}

}  // namespace

std::uint32_t MagicDataTable::Create(
    std::uint32_t name, IMagicDataTableRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectBytes);
    if (!object) return 0;
    runtime.ConstructBase(object, name);
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        runtime.WriteByte(object + offset, static_cast<std::uint8_t>(name));
        ClearTriplet(object + offset + 4, runtime);
    }
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t MagicDataTable::Destroy(
    std::uint32_t object, IMagicDataTableRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    static_cast<void>(runtime.ClearEffectData(object));
    const auto record_begin = runtime.ReadWord(object + 96);
    const auto record_end = runtime.ReadWord(object + 100);
    DestroyMagicRange(record_begin, record_end, runtime);
    runtime.WriteWord(object + 100, record_begin);

    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    ClearTriplet(object + 128, runtime);

    const auto property_begin = runtime.ReadWord(object + 112);
    const auto property_end = runtime.ReadWord(object + 116);
    for (auto record = property_begin; record != property_end;
         record += kPropertyRecordBytes)
        runtime.DestroyPropertyRecord(record);
    runtime.Free(property_begin);
    ClearTriplet(object + 112, runtime);

    DestroyMagicRange(runtime.ReadWord(object + 96),
                      runtime.ReadWord(object + 100), runtime);
    runtime.Free(runtime.ReadWord(object + 96));
    ClearTriplet(object + 96, runtime);
    return runtime.DestroyBase(object);
}

std::uint32_t MagicDataTable::ReleaseRecordArray(
    std::uint32_t array, IMagicDataTableRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    DestroyMagicRange(begin, end, runtime);
    runtime.Free(begin);
    ClearTriplet(array + 4, runtime);
    return 0;
}

std::uint32_t MagicDataTable::ClearRows(
    std::uint32_t object, IMagicDataTableRuntime& runtime) {
    auto result = runtime.ClearEffectData(object);
    const auto begin = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto record = begin; record != end; record += kRecordBytes)
        result = runtime.DestroyMagicRecord(record);
    runtime.WriteWord(object + 100, begin);
    return result;
}

}  // namespace pal4
