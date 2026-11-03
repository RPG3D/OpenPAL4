#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataValueRecordRuntime {
public:
    virtual ~IGameDataValueRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class GameDataValueRecord final {
public:
    // IDA 0x4cbdc0: zero thirteen DWORDs beginning at +16.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t object, IGameDataValueRecordRuntime& runtime);
    // IDA 0x4cbe00: index gates and two x87 float-sum limits.
    [[nodiscard]] static bool CheckValues(
        std::uint32_t object, IGameDataValueRecordRuntime& runtime);
};

}  // namespace pal4
