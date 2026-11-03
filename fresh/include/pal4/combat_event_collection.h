#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventCollectionRuntime {
public:
    virtual ~ICombatEventCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class CombatEventCollection final {
public:
    // IDA 0x57f4a0. Each record is 20 bytes; the header stores begin/end/capacity_end.
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t collection, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        ICombatEventCollectionRuntime& runtime);
};

}  // namespace pal4
