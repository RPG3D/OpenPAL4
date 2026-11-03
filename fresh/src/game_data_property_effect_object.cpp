#include "pal4/game_data_property_effect_object.h"

#include <initializer_list>

namespace pal4 {
namespace {

constexpr std::uint32_t kVtable = 0x84462c;
constexpr std::uint32_t kRecordBytes = 168;

void ZeroTriplet(std::uint32_t object, std::uint32_t offset,
                 IGameDataAiConfigObjectRuntime& runtime) {
    for (std::uint32_t step : {0u, 4u, 8u})
        runtime.WriteWord(object + offset + step, 0);
}

void DestroyRecordTexts(std::uint32_t record,
                        IGameDataPropertyEffectRuntime& runtime) {
    for (std::uint32_t i = 7; i > 0; --i)
        runtime.DestroyTextRecord(record + 16 + (i - 1) * 16);
}

}  // namespace

std::uint32_t GameDataPropertyEffectObject::Create(
    std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime) {
    const auto object = runtime.Allocate(140);
    if (!object) return 0;
    runtime.EffectConstruct(object, name);
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        runtime.WriteByte(object + offset, static_cast<std::uint8_t>(name));
        ZeroTriplet(object, offset + 4, runtime);
    }
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t GameDataPropertyEffectObject::Destroy(
    std::uint32_t object, IGameDataPropertyEffectRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto original_end = runtime.ReadWord(object + 100);
    for (auto record = records; record != original_end; record += kRecordBytes)
        DestroyRecordTexts(record, runtime);
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
    for (auto record = records; record != new_end; record += kRecordBytes)
        DestroyRecordTexts(record, runtime);
    runtime.Free(records);
    ZeroTriplet(object, 96, runtime);
    return runtime.EffectDestruct(object);
}

std::uint32_t GameDataPropertyRecordStorage::Clear(
    std::uint32_t array, IGameDataPropertyEffectRuntime& runtime) {
    const auto records = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    DestroyRange(records, end, runtime);
    runtime.Free(records);
    ZeroTriplet(array, 4, runtime);
    return 0;
}

void GameDataPropertyEffectObject::ResetEffects(
    std::uint32_t object, IGameDataPropertyEffectRuntime& runtime) {
    runtime.ClearEffectData(object);
    const auto records = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    GameDataPropertyRecordStorage::DestroyRange(records, end, runtime);
    runtime.WriteWord(object + 100, records);
}

void GameDataPropertyRecordStorage::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IGameDataPropertyEffectRuntime& runtime) {
    for (auto record = begin; record != end; record += kRecordBytes)
        DestroyRecord(record, runtime);
}

void GameDataPropertyRecordStorage::DestroyRecord(
    std::uint32_t record, IGameDataPropertyEffectRuntime& runtime) {
    DestroyRecordTexts(record, runtime);
}

}  // namespace pal4
