#include "pal4/game_data_shop_goods_object.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t GameDataShopGoodsObject::Create(
    std::uint32_t name, IGameDataShopGoodsObjectRuntime& runtime) {
    const auto object = runtime.Allocate(kBytes);
    if (!object) return 0;
    runtime.ConstructBase(object, name);
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        runtime.WriteByte(object + offset, static_cast<std::uint8_t>(name));
        for (std::uint32_t word = 4; word <= 12; word += 4)
            runtime.WriteWord(object + offset + word, 0);
    }
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t GameDataShopGoodsObject::Destroy(
    std::uint32_t object, IGameDataShopGoodsObjectRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.ClearEffectData(object);
    const auto primary_begin = runtime.ReadWord(object + 96);
    const auto primary_end = runtime.ReadWord(object + 100);
    for (auto record = primary_begin; record != primary_end; record += 36)
        runtime.DestroyRecordText(record + 20);
    runtime.WriteWord(object + 100, primary_begin);

    runtime.ProcessElements(object + 108);
    runtime.Free(runtime.ReadWord(object + 128));
    for (std::uint32_t offset = 128; offset <= 136; offset += 4)
        runtime.WriteWord(object + offset, 0);

    const auto secondary_begin = runtime.ReadWord(object + 112);
    const auto secondary_end = runtime.ReadWord(object + 116);
    for (auto record = secondary_begin; record != secondary_end; record += 60)
        runtime.DestroySecondaryRecord(record);
    runtime.Free(secondary_begin);
    for (std::uint32_t offset = 112; offset <= 120; offset += 4)
        runtime.WriteWord(object + offset, 0);

    runtime.Free(primary_begin);
    for (std::uint32_t offset = 96; offset <= 104; offset += 4)
        runtime.WriteWord(object + offset, 0);
    return runtime.DestroyBase(object);
}

}  // namespace pal4
