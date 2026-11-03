#pragma once

#include "pal4/game_data_parameter_record.h"

#include <cstdint>

namespace pal4 {

class IGameDataParameterIndexRuntime
    : public IGameDataParameterRecordDestroyRuntime {
public:
    virtual ~IGameDataParameterIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class GameDataParameterIndex final {
public:
    // IDA 0x4e3200: compact adjacent duplicate records and rebuild index.
    static void Normalize(std::uint32_t table,
                          IGameDataParameterIndexRuntime& runtime);
    // IDA 0x4e3440: destroy 328-byte records over a half-open range.
    static void DestroyRange(std::uint32_t begin, std::uint32_t end,
                             IGameDataParameterRecordDestroyRuntime& runtime);
};

}  // namespace pal4
