#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using GameDataEquipRecord = std::array<std::uint32_t, 100>;

class IGameDataEquipRecordRuntime {
public:
    virtual ~IGameDataEquipRecordRuntime() = default;
    virtual void DestroyTextRecord(std::uint32_t text) = 0;
};

class GameDataEquipRecordLifecycle final {
public:
    // IDA 0x4d6cb0: EH array destructor for 13 text fields at +152.
    static void Destroy(std::uint32_t record,
                        IGameDataEquipRecordRuntime& runtime);
    // IDA 0x4d6af0: destroy each 400-byte record in a half-open range.
    static void DestroyRange(std::uint32_t first, std::uint32_t last,
                             IGameDataEquipRecordRuntime& runtime);
};

}  // namespace pal4
