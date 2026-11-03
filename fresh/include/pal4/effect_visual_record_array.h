#pragma once

#include <cstdint>

namespace pal4 {

class IEffectVisualRecordArrayRuntime {
public:
    virtual ~IEffectVisualRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Copy36(std::uint32_t destination,
                        std::uint32_t source) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class EffectVisualRecordArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 36u;

    // IDA 0x642540: insert repeated raw 36-byte effect-visual records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IEffectVisualRecordArrayRuntime& runtime);
};

}  // namespace pal4
