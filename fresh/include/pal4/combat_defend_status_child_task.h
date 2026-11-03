#pragma once

#include <cstdint>

namespace pal4 {

class IDefendStatusChildRuntime {
public:
    virtual ~IDefendStatusChildRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class DefendStatusChildTask final {
public:
    static constexpr std::uint32_t kSize = 64u;
    static constexpr std::uint32_t kVtable = 0x845cf4u;

    // IDA 0x568db0: allocate and construct the Defend status child.
    [[nodiscard]] static std::uint32_t Create(
        IDefendStatusChildRuntime& runtime);
};

}  // namespace pal4
