#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pal4 {

struct GameDataMissionPointerArray {
    std::uint32_t begin_address = 0;
    std::size_t capacity = 0;
    std::vector<std::uint32_t> records;
};

class IGameDataMissionSelectionRuntime {
public:
    virtual ~IGameDataMissionSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePointerStorage(
        std::size_t capacity) = 0;
    virtual void FreePointerStorage(std::uint32_t address) = 0;
};

class GameDataMissionSelection final {
public:
    // IDA 0x4db280: collect all 68-byte records and sort pointers by
    // signed member +8, descending.
    [[nodiscard]] static std::uint32_t CollectAndSort(
        std::uint32_t manager, GameDataMissionPointerArray& output,
        IGameDataMissionSelectionRuntime& runtime);
};

}  // namespace pal4
