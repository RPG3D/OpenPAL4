#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataSharedTextRuntime {
public:
    virtual ~IGameDataSharedTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(std::uint32_t address) = 0;
};

class GameDataSharedTextRecord final {
public:
    // IDA 0x4c8800: release text at +12 and restore the base vtable.
    [[nodiscard]] static std::uint32_t ReleaseFirst(
        std::uint32_t object, IGameDataSharedTextRuntime& runtime);
    // IDA 0x4c89a0: same ownership rule for text at +20.
    [[nodiscard]] static std::uint32_t ReleaseSecond(
        std::uint32_t object, IGameDataSharedTextRuntime& runtime);
};

}  // namespace pal4
