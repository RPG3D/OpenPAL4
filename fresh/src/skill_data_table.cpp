#include "pal4/skill_data_table.h"

#include <initializer_list>

namespace pal4 {
namespace {

void DestroySkillRange(std::uint32_t begin, std::uint32_t end,
                       ISkillDataTableRuntime& runtime) {
    for (auto record = begin; record != end;
         record += SkillDataTable::kSkillRecordBytes)
        static_cast<void>(runtime.DestroySkillRecord(record));
}

void ClearTriplet(std::uint32_t address, ISkillDataTableRuntime& runtime) {
    for (std::uint32_t offset = 0; offset < 12; offset += 4)
        runtime.WriteWord(address + offset, 0);
}

}  // namespace

std::uint32_t SkillDataTable::Create(
    std::uint32_t name, ISkillDataTableRuntime& runtime) {
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

std::uint32_t SkillDataTable::Destroy(
    std::uint32_t object, ISkillDataTableRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    static_cast<void>(runtime.ClearEffectData(object));
    const auto skill_begin = runtime.ReadWord(object + 96);
    const auto skill_end = runtime.ReadWord(object + 100);
    DestroySkillRange(skill_begin, skill_end, runtime);
    runtime.WriteWord(object + 100, skill_begin);

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

    const auto remaining_end = runtime.ReadWord(object + 100);
    DestroySkillRange(runtime.ReadWord(object + 96), remaining_end, runtime);
    runtime.Free(runtime.ReadWord(object + 96));
    ClearTriplet(object + 96, runtime);
    return runtime.DestroyBase(object);
}

std::uint32_t SkillDataTable::ReleaseRecordArray(
    std::uint32_t array, ISkillDataTableRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    DestroySkillRange(begin, end, runtime);
    runtime.Free(begin);
    ClearTriplet(array + 4, runtime);
    return 0;
}

std::uint32_t SkillDataTable::ClearRows(
    std::uint32_t object, ISkillDataTableRuntime& runtime) {
    auto result = runtime.ClearEffectData(object);
    const auto begin = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto record = begin; record != end;
         record += kSkillRecordBytes)
        result = runtime.DestroySkillRecord(record);
    runtime.WriteWord(object + 100, begin);
    return result;
}

}  // namespace pal4
