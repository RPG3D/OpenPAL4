#pragma once

#include <cstdint>

namespace pal4 {

class IGameArenaStringBufferRuntime {
public:
    virtual ~IGameArenaStringBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    // Writes the destination low address byte at +0 and clears +4/+8/+0xc.
    virtual void InitializeBuffer(std::uint32_t destination) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(std::uint32_t source) = 0;
    virtual void SetErrorFlag(std::uint32_t destination,
                              std::uint8_t value) = 0;
    virtual void Reallocate(std::uint32_t destination,
                            std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
};

class GameArenaStringBuffer final {
public:
    // IDA 0x5d7950, copies the arena string pointer at +8.
    [[nodiscard]] static std::uint32_t First(
        std::uint32_t destination, IGameArenaStringBufferRuntime& runtime);
    // IDA 0x5d7a20, copies the arena string pointer at +0xe0.
    [[nodiscard]] static std::uint32_t Second(
        std::uint32_t destination, IGameArenaStringBufferRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t destination, std::uint32_t arena_offset,
        IGameArenaStringBufferRuntime& runtime);
};

}  // namespace pal4
