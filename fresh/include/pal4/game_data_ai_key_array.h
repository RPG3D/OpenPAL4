#pragma once

#include "pal4/game_data_ai_record_sorter.h"

#include <cstdint>

namespace pal4 {

class IGameDataAiKeyArrayRuntime {
public:
    virtual ~IGameDataAiKeyArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual GameDataAiKeyEntry ReadKey(
        std::uint32_t address) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataAiKeyArray final {
public:
    // IDA 0x4cd650: insert an 8-byte key/index pair at a specified position.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        const GameDataAiKeyEntry& entry, IGameDataAiKeyArrayRuntime& runtime);
    // IDA 0x4ce7d0: copy a half-open range of 8-byte key/index pairs.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IGameDataAiKeyArrayRuntime& runtime);
};

}  // namespace pal4
