#pragma once

#include <cstdint>

namespace pal4 {

struct CeguiTextRecordArrayState {
    std::uint32_t unused{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity{};
};
static_assert(sizeof(CeguiTextRecordArrayState) == 16u);

class ICeguiTextRecordRuntime {
public:
    virtual ~ICeguiTextRecordRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextAddress() = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t record, std::uint32_t size,
        std::uint8_t mode) = 0;
    virtual void DetachAndReset(std::uint32_t record) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t size) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t size) = 0;
    [[noreturn]] virtual void ThrowRange() = 0;
};

class ICeguiTextRecordArrayRuntime : public ICeguiTextRecordRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateRecords(
        std::uint32_t bytes) = 0;
    virtual void FreeRecords(std::uint32_t address) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void EraseText(std::uint32_t,
                           std::uint32_t,
                           std::uint32_t) {}
};

class CeguiTextRecord final {
public:
    static constexpr std::uint32_t kBytes = 16u;
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;

    // 0x419750.
    static void Copy(std::uint32_t destination, std::uint32_t source,
                     ICeguiTextRecordRuntime& runtime);
};

class CeguiTextRecordArray final {
public:
    // 0x4194b0.
    static void Insert(CeguiTextRecordArrayState& state,
                       std::uint32_t position, std::uint32_t count,
                       std::uint32_t value,
                       ICeguiTextRecordArrayRuntime& runtime);
    // 0x435330: move [source, end) to destination and destroy the tail.
    [[nodiscard]] static std::uint32_t EraseRange(
        CeguiTextRecordArrayState& state,
        std::uint32_t destination, std::uint32_t source,
        ICeguiTextRecordArrayRuntime& runtime);
};

}  // namespace pal4
