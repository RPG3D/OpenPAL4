#pragma once

#include "pal4/game_data_role_database.h"
#include "pal4/game_data_role_table.h"

#include <vector>

namespace pal4 {

class IGameDataRoleTableIndexRuntime {
public:
    virtual ~IGameDataRoleTableIndexRuntime() = default;
    virtual void AssignRecord(GameDataRoleTableRecord& destination,
                              const GameDataRoleTableRecord& source) = 0;
    virtual void DestroyRecord(GameDataRoleTableRecord& record) = 0;
};

class GameDataRoleTableIndex final {
public:
    // IDA 0x4ec0d0: compare the Role key at +228.
    [[nodiscard]] static bool EqualKey(
        const GameDataRoleTableRecord& left,
        const GameDataRoleTableRecord& right);
    // IDA 0x4ea830: compact adjacent duplicate Role IDs and rebuild index.
    static void Rebuild(std::vector<GameDataRoleTableRecord>& records,
                        std::vector<GameDataRoleIndexEntry>& index,
                        IGameDataRoleTableIndexRuntime& runtime);
};

}  // namespace pal4
