#pragma once

#include <cstdint>

namespace pal4 {

class ICylinderVertexPairArrayRuntime {
public:
    virtual ~ICylinderVertexPairArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Copy40(std::uint32_t destination,
                        std::uint32_t source) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class CylinderVertexPairArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 40u;

    // IDA 0x6448d0: insert repeated raw cylinder vertex pairs.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        ICylinderVertexPairArrayRuntime& runtime);
};

}  // namespace pal4
