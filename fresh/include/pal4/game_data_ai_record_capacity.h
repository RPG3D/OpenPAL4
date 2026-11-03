#pragma once

#include "pal4/game_data_ai_record_sorter.h"

#include <cstdint>

namespace pal4 {

class IGameDataAiRecordCapacityRuntime {
public:
    virtual ~IGameDataAiRecordCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual GameDataAiRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataAiRecord& record) = 0;
    [[nodiscard]] virtual GameDataAiKeyEntry ReadKey(
        std::uint32_t address) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
};

class GameDataAiRecordCapacity final {
public:
    // IDA 0x4cc9f0: grow record and key arrays to the requested capacity.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::uint32_t requested,
        IGameDataAiRecordCapacityRuntime& runtime);
};

class IGameDataAiPropertyCapacityRuntime {
public:
    virtual ~IGameDataAiPropertyCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual GameDataAiPropertyRecord ReadRecord(
        std::uint32_t address) = 0;
    virtual void WriteRecord(std::uint32_t address,
                             const GameDataAiPropertyRecord& record) = 0;
    [[nodiscard]] virtual GameDataAiKeyEntry ReadKey(
        std::uint32_t address) = 0;
    virtual void WriteKey(std::uint32_t address,
                          const GameDataAiKeyEntry& entry) = 0;
};

class GameDataAiPropertyCapacity final {
public:
    // IDA 0x4cd420: grow 68-byte record and 8-byte key arrays.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::uint32_t requested,
        IGameDataAiPropertyCapacityRuntime& runtime);
};

}  // namespace pal4
