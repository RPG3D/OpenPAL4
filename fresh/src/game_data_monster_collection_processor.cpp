#include "pal4/game_data_monster_collection_processor.h"

namespace pal4 {

std::uint8_t GameDataMonsterCollectionProcessor::Process(
    const std::array<std::uint32_t, 5>& keys,
    std::uint32_t collection,
    IGameDataMonsterCollectionRuntime& runtime) {
    std::uint8_t result = 0;
    for (const auto key : keys) {
        const auto database = runtime.GetDatabase();
        if (runtime.StopSound(database, key)) {
            const auto token = runtime.PlaySound(runtime.GetDatabase(), key);
            const auto record = runtime.ResolveSound(token);
            result = static_cast<std::uint8_t>(record);
            if (record) {
                result = runtime.FadeIn(record);
                if (result)
                    result = runtime.DispatchSound(
                        collection, runtime.CollectionEnd(collection),
                        runtime.ReadRecordValue(record));
            }
            continue;
        }
        result = runtime.StopMusic(runtime.GetDatabase(), key);
        if (!result) continue;
        const auto token = runtime.PlayMusic(runtime.GetDatabase(), key);
        const auto record = runtime.ResolveMusic(token);
        result = static_cast<std::uint8_t>(record);
        if (!record) continue;
        result = runtime.FadeIn(record);
        if (result)
            result = static_cast<std::uint8_t>(
                runtime.InsertMusicAtFront(
                    collection, runtime.ReadRecordValue(record)));
    }
    return result;
}

}  // namespace pal4
