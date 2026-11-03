#include "pal4/game_data_shop_record_storage.h"

namespace pal4 {

std::uint32_t GameDataShopRecordStorage::ClearAndFree(
    std::uint32_t array, IGameDataShopRecordStorageRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    for (auto record = begin; record != end; record += kRecordBytes)
        static_cast<void>(runtime.DestroyRecord(record));
    runtime.Free(begin);
    runtime.WriteWord(array + 4, 0);
    runtime.WriteWord(array + 8, 0);
    runtime.WriteWord(array + 12, 0);
    return 0;
}

std::uint32_t GameDataShopRecordStorage::ClearObjectRecords(
    std::uint32_t object, IGameDataShopRecordStorageRuntime& runtime) {
    auto result = runtime.ClearEffectData(object);
    const auto begin = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto record = begin; record != end; record += kRecordBytes)
        result = runtime.DestroyRecord(record);
    runtime.WriteWord(object + 100, begin);
    return result;
}

std::uint32_t GameDataShopRecordStorage::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t incoming_eax,
    IGameDataShopRecordStorageRuntime& runtime) {
    auto result = incoming_eax;
    for (auto record = begin; record != end; record += kRecordBytes)
        result = runtime.DestroyRecord(record);
    return result;
}

}  // namespace pal4
