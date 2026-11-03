#pragma once

#include "pal4/game_data_monster_record.h"

#include <cstdint>

namespace pal4 {

class IGameDataMonsterCapacityRuntime
    : public IGameDataMonsterRecordDestroyRuntime {
public:
    virtual ~IGameDataMonsterCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
};

class GameDataMonsterCapacity final {
public:
    // IDA 0x4df670: reserve 596-byte records and 8-byte key/index pairs.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IGameDataMonsterCapacityRuntime& runtime);
};

}  // namespace pal4
