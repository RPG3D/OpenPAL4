#include "pal4/game_data_ai_config_object.h"

#include <initializer_list>

namespace pal4 {
namespace {

void ZeroTriplet(std::uint32_t object, std::uint32_t offset,
                 IGameDataAiConfigObjectRuntime& runtime) {
    runtime.WriteWord(object + offset, 0);
    runtime.WriteWord(object + offset + 4, 0);
    runtime.WriteWord(object + offset + 8, 0);
}

std::uint32_t CreateEffectObject(
    std::uint32_t name, std::uint32_t vtable,
    IGameDataAiConfigObjectRuntime& runtime) {
    const std::uint32_t object = runtime.Allocate(0x8c);
    if (object == 0) return 0;
    runtime.EffectConstruct(object, name);
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        runtime.WriteByte(object + offset, static_cast<std::uint8_t>(name));
        ZeroTriplet(object, offset + 4, runtime);
    }
    runtime.WriteWord(object, vtable);
    return object;
}

std::uint32_t DestroyEffectObject(
    std::uint32_t object, std::uint32_t vtable,
    IGameDataAiConfigObjectRuntime& runtime) {
    runtime.WriteWord(object, vtable);
    runtime.ClearEffectData(object);
    runtime.WriteWord(object + 100, runtime.ReadWord(object + 96));
    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    ZeroTriplet(object, 128, runtime);
    const std::uint32_t first = runtime.ReadWord(object + 112);
    const std::uint32_t last = runtime.ReadWord(object + 116);
    for (std::uint32_t element = first; element != last; element += 60)
        runtime.DestroyElement(element, 0);
    runtime.Free(first);
    ZeroTriplet(object, 112, runtime);
    runtime.Free(runtime.ReadWord(object + 96));
    ZeroTriplet(object, 96, runtime);
    return runtime.EffectDestruct(object);
}

std::uint32_t ResetEffectObject(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    runtime.ClearEffectData(object);
    const std::uint32_t previous_end = runtime.ReadWord(object + 100);
    runtime.WriteWord(object + 100, runtime.ReadWord(object + 96));
    return previous_end;
}

}  // namespace

std::uint32_t GameDataAiConfigObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x8444f4, runtime);
}

std::uint32_t GameDataAiConfigObject::Destroy(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return DestroyEffectObject(object, 0x8444f4, runtime);
}

std::uint32_t GameDataAiConfigObject::ResetEffects(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return ResetEffectObject(object, runtime);
}

std::uint32_t GameDataAiPropertyObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x844514, runtime);
}

std::uint32_t GameDataAiPropertyObject::Destroy(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return DestroyEffectObject(object, 0x844514, runtime);
}

std::uint32_t GameDataAiPropertyObject::ResetEffects(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return ResetEffectObject(object, runtime);
}

std::uint8_t GameDataUpgradeEffectObject::InitializeItemId(
    std::uint32_t object, IGameDataUpgradeEffectNameRuntime& runtime) {
    const bool allocated = runtime.PrepareTemporaryName(
        static_cast<std::uint8_t>(object), "ITEM_ID");
    runtime.CreateEffect(object, allocated ? "ITEM_ID" : "", 0);
    if (allocated) runtime.ReleaseTemporaryName();
    return 1;
}

std::uint32_t GameDataUpgradeEffectObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x844534, runtime);
}

std::uint32_t GameDataUpgradeEffectObject::Destroy(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return DestroyEffectObject(object, 0x844534, runtime);
}

std::uint32_t GameDataUpgradeEffectObject::ResetEffects(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return ResetEffectObject(object, runtime);
}

std::uint32_t GameDataDropEffectObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x844554, runtime);
}

std::uint32_t GameDataDropEffectObject::Destroy(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return DestroyEffectObject(object, 0x844554, runtime);
}

std::uint32_t GameDataDropEffectObject::ResetEffects(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return ResetEffectObject(object, runtime);
}

std::uint32_t GameDataEquipRelationEffectObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x84458c, runtime);
}

std::uint32_t GameDataEquipRelationEffectObject::Destroy(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return DestroyEffectObject(object, 0x84458c, runtime);
}

std::uint32_t GameDataEquipRelationEffectObject::ResetEffects(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    return ResetEffectObject(object, runtime);
}

std::uint32_t GameDataEquipEffectObject::Create(
    std::uint32_t name, IGameDataEquipEffectObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x8445ac, runtime);
}

std::uint32_t GameDataEquipEffectObject::Destroy(
    std::uint32_t object, IGameDataEquipEffectObjectRuntime& runtime) {
    runtime.WriteWord(object, 0x8445ac);
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    GameDataEquipRecordLifecycle::DestroyRange(records, end, runtime);
    runtime.WriteWord(object + 100, records);

    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    ZeroTriplet(object, 128, runtime);
    const auto fields = runtime.ReadWord(object + 112);
    const auto field_end = runtime.ReadWord(object + 116);
    for (std::uint32_t field = fields; field != field_end; field += 60)
        runtime.DestroyElement(field, 0);
    runtime.Free(fields);
    ZeroTriplet(object, 112, runtime);
    runtime.Free(records);
    ZeroTriplet(object, 96, runtime);
    return runtime.EffectDestruct(object);
}

void GameDataEquipEffectObject::ResetEffects(
    std::uint32_t object, IGameDataEquipEffectObjectRuntime& runtime) {
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    GameDataEquipRecordLifecycle::DestroyRange(records, end, runtime);
    runtime.WriteWord(object + 100, records);
}

std::uint32_t GameDataEquipRecordStorage::Clear(
    std::uint32_t array, IGameDataEquipEffectObjectRuntime& runtime) {
    const auto records = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    GameDataEquipRecordLifecycle::DestroyRange(records, end, runtime);
    runtime.Free(records);
    ZeroTriplet(array, 4, runtime);
    return 0;
}

std::uint32_t GameDataMissionEffectObject::Create(
    std::uint32_t name, IGameDataMissionEffectRuntime& runtime) {
    return CreateEffectObject(name, 0x8445cc, runtime);
}

std::uint32_t GameDataMonsterEffectObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x8445ec, runtime);
}

std::uint32_t GameDataMonsterEffectObject::Destroy(
    std::uint32_t object, IGameDataMonsterEffectRuntime& runtime) {
    runtime.WriteWord(object, 0x8445ec);
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto original_end = runtime.ReadWord(object + 100);
    for (auto record = records; record != original_end; record += 596)
        GameDataMonsterRecord::DestroySubrecord(record + 336, runtime);
    runtime.WriteWord(object + 100, records);

    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    ZeroTriplet(object, 128, runtime);
    const auto fields = runtime.ReadWord(object + 112);
    const auto field_end = runtime.ReadWord(object + 116);
    for (auto field = fields; field != field_end; field += 60)
        runtime.DestroyElement(field, 0);
    runtime.Free(fields);
    ZeroTriplet(object, 112, runtime);

    const auto new_end = runtime.ReadWord(object + 100);
    for (auto record = records; record != new_end; record += 596)
        GameDataMonsterRecord::DestroySubrecord(record + 336, runtime);
    runtime.Free(records);
    ZeroTriplet(object, 96, runtime);
    return runtime.EffectDestruct(object);
}

void GameDataMonsterEffectObject::ResetEffects(
    std::uint32_t object, IGameDataMonsterEffectRuntime& runtime) {
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto record = records; record != end; record += 596)
        GameDataMonsterRecord::DestroyFullRecord(record, runtime);
    runtime.WriteWord(object + 100, records);
}

std::uint32_t GameDataMonsterRecordStorage::Clear(
    std::uint32_t array, IGameDataMonsterEffectRuntime& runtime) {
    const auto records = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    for (auto record = records; record != end; record += 596)
        GameDataMonsterRecord::DestroyFullRecord(record, runtime);
    runtime.Free(records);
    ZeroTriplet(array, 4, runtime);
    return 0;
}

namespace {

void DestroyParameterTexts(std::uint32_t record,
                           IGameDataParameterEffectRuntime& runtime) {
    GameDataParameterRecordLifecycle::Destroy(record, runtime);
}

}  // namespace

std::uint32_t GameDataParameterEffectObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    return CreateEffectObject(name, 0x84460c, runtime);
}

std::uint32_t GameDataParameterEffectObject::Destroy(
    std::uint32_t object, IGameDataParameterEffectRuntime& runtime) {
    runtime.WriteWord(object, 0x84460c);
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto original_end = runtime.ReadWord(object + 100);
    for (auto record = records; record != original_end; record += 328)
        DestroyParameterTexts(record, runtime);
    runtime.WriteWord(object + 100, records);

    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    ZeroTriplet(object, 128, runtime);
    const auto fields = runtime.ReadWord(object + 112);
    const auto field_end = runtime.ReadWord(object + 116);
    for (auto field = fields; field != field_end; field += 60)
        runtime.DestroyElement(field, 0);
    runtime.Free(fields);
    ZeroTriplet(object, 112, runtime);

    const auto new_end = runtime.ReadWord(object + 100);
    for (auto record = records; record != new_end; record += 328)
        DestroyParameterTexts(record, runtime);
    runtime.Free(records);
    ZeroTriplet(object, 96, runtime);
    return runtime.EffectDestruct(object);
}

std::uint32_t GameDataParameterRecordStorage::Clear(
    std::uint32_t array, IGameDataParameterEffectRuntime& runtime) {
    const auto records = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    for (auto record = records; record != end; record += 328)
        DestroyParameterTexts(record, runtime);
    runtime.Free(records);
    ZeroTriplet(array, 4, runtime);
    return 0;
}

void GameDataParameterEffectObject::ResetEffects(
    std::uint32_t object, IGameDataParameterEffectRuntime& runtime) {
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto record = records; record != end; record += 328)
        DestroyParameterTexts(record, runtime);
    runtime.WriteWord(object + 100, records);
}

std::uint32_t GameDataMissionEffectObject::Destroy(
    std::uint32_t object, IGameDataMissionEffectRuntime& runtime) {
    runtime.WriteWord(object, 0x8445cc);
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto original_end = runtime.ReadWord(object + 100);
    for (auto record = records; record != original_end; record += 68) {
        runtime.DestroyTextRecord(record + 56);
        runtime.DestroyTextRecord(record + 40);
        runtime.DestroyTextRecord(record + 24);
    }
    runtime.WriteWord(object + 100, records);

    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    ZeroTriplet(object, 128, runtime);
    const auto fields = runtime.ReadWord(object + 112);
    const auto field_end = runtime.ReadWord(object + 116);
    for (auto field = fields; field != field_end; field += 60)
        runtime.DestroyElement(field, 0);
    runtime.Free(fields);
    ZeroTriplet(object, 112, runtime);

    const auto new_end = runtime.ReadWord(object + 100);
    for (auto record = records; record != new_end; record += 68)
        (void)runtime.DestroyMissionRecord(record);
    runtime.Free(records);
    ZeroTriplet(object, 96, runtime);
    return runtime.EffectDestruct(object);
}

std::uint32_t GameDataMissionRecordStorage::Clear(
    std::uint32_t array, IGameDataMissionEffectRuntime& runtime) {
    const auto records = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    for (auto record = records; record != end; record += 68)
        (void)runtime.DestroyMissionRecord(record);
    runtime.Free(records);
    ZeroTriplet(array, 4, runtime);
    return 0;
}

std::uint32_t GameDataMissionEffectObject::ResetEffects(
    std::uint32_t object, IGameDataMissionEffectRuntime& runtime) {
    auto result = runtime.ClearEffectDataForReset(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto record = records; record != end; record += 68)
        result = runtime.DestroyMissionRecord(record);
    runtime.WriteWord(object + 100, records);
    return result;
}

std::uint32_t GameDataAiConfigStorage::ClearRecords(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    const std::uint32_t first = runtime.ReadWord(object + 4);
    const std::uint32_t last = runtime.ReadWord(object + 8);
    for (std::uint32_t element = first; element != last; element += 60)
        runtime.ResetRecordPointers(element);
    runtime.Free(first);
    ZeroTriplet(object, 4, runtime);
    return 0;
}

std::uint32_t GameDataAiConfigStorage::ClearFlags(
    std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime) {
    runtime.ProcessElements(object);
    runtime.Free(runtime.ReadWord(object + 20));
    ZeroTriplet(object, 20, runtime);
    const std::uint32_t first = runtime.ReadWord(object + 4);
    const std::uint32_t last = runtime.ReadWord(object + 8);
    for (std::uint32_t element = first; element != last; element += 60) {
        runtime.SetErrorFlag(element + 36, 1);
        runtime.SetErrorFlag(element + 20, 1);
        runtime.SetErrorFlag(element, 1);
    }
    runtime.Free(first);
    ZeroTriplet(object, 4, runtime);
    return 0;
}

}  // namespace pal4
