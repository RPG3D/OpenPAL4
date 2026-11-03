#pragma once

#include <cstdint>

namespace pal4 {

class IJigsawWindowStorageRuntime {
public:
    virtual ~IJigsawWindowStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    virtual void TouchCell(std::uint32_t record,
                           std::uint32_t cell) = 0;
    virtual void TouchRange(std::uint32_t begin,
                            std::uint32_t end) = 0;
    virtual void TouchNoop() = 0;
    virtual void FreeRaw(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FreeZero(
        std::uint32_t address) = 0;
    virtual void FreeSized(std::uint32_t address,
                           std::int32_t words) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
};

class JigsawWindowStorage final {
public:
    // IDA 0x49fbd0: puzzle subobject at frame +0x2c.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t puzzle, IJigsawWindowStorageRuntime& runtime);
    // IDA 0x49fcc0: containing jigsaw frame.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t frame, IJigsawWindowStorageRuntime& runtime);
    // IDA 0x49fdb0: only the nested 16-byte-record vector.
    [[nodiscard]] static std::uint32_t ClearGroups(
        std::uint32_t vector, IJigsawWindowStorageRuntime& runtime);
};

}  // namespace pal4
