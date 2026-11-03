#include "pal4/game_data_upgrade_table_lifecycle.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t GameDataUpgradeTableLifecycle::Construct(
    std::uint32_t name, IGameDataEffectTableRuntime& runtime) {
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

std::uint32_t GameDataUpgradeTableLifecycle::Destroy(
    std::uint32_t object, IGameDataEffectTableRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.ClearEffectData(object);
    const auto record_begin = runtime.ReadWord(object + 96);
    runtime.WriteWord(object + 100, record_begin);
    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    for (const std::uint32_t offset : {128u, 132u, 136u})
        runtime.WriteWord(object + offset, 0);
    const auto aux_begin = runtime.ReadWord(object + 112);
    const auto aux_end = runtime.ReadWord(object + 116);
    for (auto record = aux_begin; record != aux_end; record += 60u)
        runtime.DestroyAuxRecord(record);
    runtime.Free(aux_begin);
    for (const std::uint32_t offset : {112u, 116u, 120u})
        runtime.WriteWord(object + offset, 0);
    runtime.Free(record_begin);
    for (const std::uint32_t offset : {96u, 100u, 104u})
        runtime.WriteWord(object + offset, 0);
    return runtime.DestructEffect(object);
}

std::uint32_t GameDataUpgradeTableLifecycle::ClearLoadedRecords(
    std::uint32_t object, IGameDataEffectTableRuntime& runtime) {
    runtime.ClearEffectData(object);
    const auto old_end = runtime.ReadWord(object + 100);
    runtime.WriteWord(object + 100, runtime.ReadWord(object + 96));
    return old_end;
}

}  // namespace pal4
