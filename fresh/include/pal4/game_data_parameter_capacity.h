#pragma once

#include "pal4/game_data_parameter_record.h"

#include <cstdint>

namespace pal4 {

class IGameDataParameterCapacityRuntime
    : public IGameDataParameterRecordDestroyRuntime {
public:
    virtual ~IGameDataParameterCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
};

class GameDataParameterCapacity final {
public:
    // IDA 0x4e3470: reserve 328-byte records and 8-byte key/index pairs.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IGameDataParameterCapacityRuntime& runtime);
};

}  // namespace pal4
