#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataRecordAccessRuntime {
public:
    virtual ~IGameDataRecordAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindEffectEntry(
        std::uint32_t effect_collection, std::uint32_t key) = 0;
};

class GameDataRecordAccess final {
public:
    // IDA 0x4d5210: return the fourth object DWORD.
    [[nodiscard]] static std::uint32_t MemberAt16(
        std::uint32_t object, IGameDataRecordAccessRuntime& runtime);
    // IDA 0x4d5220: find an Effect entry through member +12.
    [[nodiscard]] static std::uint32_t FindEffect(
        std::uint32_t object, std::uint32_t key,
        IGameDataRecordAccessRuntime& runtime);
    // IDA 0x4d5230: return the first matching 12-byte record's second word.
    [[nodiscard]] static std::uint32_t FirstMatchingRecordValue(
        std::uint32_t object, std::uint32_t key,
        IGameDataRecordAccessRuntime& runtime);
    // IDA 0x4d5310: signed lower-bound key search into 400-byte records.
    [[nodiscard]] static std::uint32_t FindRecord(
        std::uint32_t object, std::int32_t key,
        IGameDataRecordAccessRuntime& runtime);
    // IDA 0x4d5380: report whether FindRecord returns a nonzero address.
    [[nodiscard]] static bool HasRecord(
        std::uint32_t object, std::int32_t key,
        IGameDataRecordAccessRuntime& runtime);
};

}  // namespace pal4
