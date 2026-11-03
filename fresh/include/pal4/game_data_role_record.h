#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataRoleRecordRuntime {
public:
    virtual ~IGameDataRoleRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void InitializeText(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class GameDataRoleRecord final {
public:
    // IDA 0x4e7dc0: nine text slots, six zero scalars, and type four.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t record, IGameDataRoleRecordRuntime& runtime);
    // IDA 0x4e80c0: normalize role ID and validate range/category.
    [[nodiscard]] static bool NormalizeAndValidate(
        std::uint32_t record, IGameDataRoleRecordRuntime& runtime);
    // IDA 0x4e84e0: check the second role ID against 1001..1004.
    [[nodiscard]] static bool HasValidLinkedRoleId(
        std::uint32_t record, IGameDataRoleRecordRuntime& runtime);
};

}  // namespace pal4
