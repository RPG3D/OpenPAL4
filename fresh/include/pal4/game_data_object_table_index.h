#pragma once

#include "pal4/game_data_object_table.h"

#include <cstdint>
#include <vector>

namespace pal4 {

struct GameDataObjectIndexEntry {
    std::int32_t key;
    std::uint32_t record_index;
};

class IGameDataObjectTableIndexRuntime {
public:
    virtual ~IGameDataObjectTableIndexRuntime() = default;
    virtual void AssignRecord(GameDataObjectRecord& destination,
                              const GameDataObjectRecord& source) = 0;
    virtual void DestroyRecord(GameDataObjectRecord& record) = 0;
};

class GameDataObjectTableIndex final {
public:
    // IDA 0x4ef930: compact adjacent equal-hash records and rebuild index.
    static void Rebuild(
        std::vector<GameDataObjectRecord>& records,
        std::vector<GameDataObjectIndexEntry>& index,
        IGameDataObjectTableIndexRuntime& runtime);
};

}  // namespace pal4
