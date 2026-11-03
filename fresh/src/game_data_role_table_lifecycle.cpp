#include "pal4/game_data_role_table_lifecycle.h"

#include <initializer_list>

namespace pal4 {
namespace {

void ForEachRecord(std::uint32_t begin, std::uint32_t end,
                   std::uint32_t stride, auto&& action) {
    for (auto address = begin; address != end; address += stride)
        action(address);
}

}  // namespace

std::uint32_t GameDataRoleTableLifecycle::Construct(
    std::uint32_t name, IGameDataRoleTableRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (!object) return 0;
    runtime.ConstructEffect(object, name);
    for (const std::uint32_t group : {92u, 108u, 124u}) {
        runtime.WriteByte(object + group,
                          static_cast<std::uint8_t>(name));
        for (const std::uint32_t offset : {4u, 8u, 12u})
            runtime.WriteWord(object + group + offset, 0);
    }
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t GameDataRoleTableLifecycle::Destroy(
    std::uint32_t object, IGameDataRoleTableRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.ClearEffectData(object);
    const auto record_begin = runtime.ReadWord(object + 96);
    const auto record_end = runtime.ReadWord(object + 100);
    ForEachRecord(record_begin, record_end, 320u, [&](std::uint32_t record) {
        runtime.CleanupTextRecords(record + 144);
    });
    runtime.WriteWord(object + 100, record_begin);
    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    for (const std::uint32_t offset : {128u, 132u, 136u})
        runtime.WriteWord(object + offset, 0);
    const auto aux_begin = runtime.ReadWord(object + 112);
    const auto aux_end = runtime.ReadWord(object + 116);
    ForEachRecord(aux_begin, aux_end, 60u, [&](std::uint32_t record) {
        runtime.DestroyAuxRecord(record);
    });
    runtime.Free(aux_begin);
    for (const std::uint32_t offset : {112u, 116u, 120u})
        runtime.WriteWord(object + offset, 0);
    runtime.Free(record_begin);
    for (const std::uint32_t offset : {96u, 100u, 104u})
        runtime.WriteWord(object + offset, 0);
    return runtime.DestructEffect(object);
}

std::uint32_t GameDataRoleTableLifecycle::DestroyRecordArray(
    std::uint32_t array, IGameDataRoleTableRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    DestroyRecordRange(begin, end, runtime);
    runtime.Free(begin);
    for (const std::uint32_t offset : {4u, 8u, 12u})
        runtime.WriteWord(array + offset, 0);
    return 0;
}

void GameDataRoleTableLifecycle::ClearLoadedRecords(
    std::uint32_t object, IGameDataRoleTableRuntime& runtime) {
    runtime.ClearEffectData(object);
    const auto begin = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    DestroyRecordRange(begin, end, runtime);
    runtime.WriteWord(object + 100, begin);
}

void GameDataRoleTableLifecycle::DestroyRecordRange(
    std::uint32_t begin, std::uint32_t end,
    IGameDataRoleTableRuntime& runtime) {
    ForEachRecord(begin, end, 320u, [&](std::uint32_t record) {
        runtime.DestroyRecord(record);
    });
}

}  // namespace pal4
