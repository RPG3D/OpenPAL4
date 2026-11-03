#pragma once

#include <cstdint>

namespace pal4 {

class ICombatLogBufferStorageRuntime {
public:
    virtual ~ICombatLogBufferStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

enum class CombatLogTextSource {
    Output,
    Input,
    Empty,
};

class ICombatLogTextRuntime {
public:
    virtual ~ICombatLogTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void AssignText(std::uint32_t result, std::uint32_t source,
                            std::uint32_t length,
                            CombatLogTextSource kind) = 0;
};

class ICombatLogBaseRuntime {
public:
    virtual ~ICombatLogBaseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeLocale() = 0;
};

class CombatLogBuffer final {
public:
    // IDA 0x57e9d0-0x57eda0.
    [[nodiscard]] static std::int32_t Add(
        std::uint32_t buffer, std::int32_t character,
        ICombatLogBufferStorageRuntime& runtime);
    [[nodiscard]] static std::int32_t Remove(
        std::uint32_t buffer, std::int32_t character,
        ICombatLogBufferStorageRuntime& runtime);
    [[nodiscard]] static std::int32_t Peek(
        std::uint32_t buffer, ICombatLogBufferStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t SeekRelative(
        std::uint32_t buffer, std::uint32_t result,
        std::int32_t offset, std::uint32_t origin, std::uint8_t mode,
        ICombatLogBufferStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t SeekPosition(
        std::uint32_t buffer, std::uint32_t result,
        std::int32_t first_offset, std::int32_t second_offset,
        std::uint8_t mode, ICombatLogBufferStorageRuntime& runtime);
    // IDA 0x57eff0/0x57f190/0x57f2a0.
    [[nodiscard]] static std::uint32_t GetString(
        std::uint32_t buffer, std::uint32_t result,
        ICombatLogTextRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFromText(
        std::uint32_t buffer, std::uint32_t text,
        ICombatLogBufferStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupStorage(
        std::uint32_t buffer, ICombatLogBufferStorageRuntime& runtime);
    // IDA 0x57f320/0x57f3a0/0x57f480.
    [[nodiscard]] static std::uint32_t ConstructBase(
        std::uint32_t buffer, ICombatLogBaseRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeStorage(
        std::uint32_t buffer, std::uint32_t source, std::int32_t length,
        std::uint32_t flags, ICombatLogBufferStorageRuntime& runtime);
    [[nodiscard]] static std::uint8_t ConvertFlags(std::uint8_t flags);
};

}  // namespace pal4
