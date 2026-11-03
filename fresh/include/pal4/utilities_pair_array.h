#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IUtilitiesPairArrayRuntime {
public:
    virtual ~IUtilitiesPairArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class UtilitiesPairArray final {
public:
    // IDA 0x6602b0/0x6602e0: plain copies/fills of 8-byte records.
    [[nodiscard]] static std::uint32_t CopyRangePlain(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IUtilitiesPairArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t FillPlain(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source, IUtilitiesPairArrayRuntime& runtime);
    // IDA 0x50c4e0: compact adjacent equal 8-byte pairs.
    [[nodiscard]] static std::uint32_t SelectUnique(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const std::function<bool(std::uint32_t, std::uint32_t)>& equals,
        IUtilitiesPairArrayRuntime& runtime);

    // IDA 0x506e60: insert an 8-byte integer pair without element cleanup.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IUtilitiesPairArrayRuntime& runtime);
};

}  // namespace pal4
