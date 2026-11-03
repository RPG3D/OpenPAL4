#pragma once

#include <cstdint>

namespace pal4 {

class IEndAttackChildRuntime {
public:
    virtual ~IEndAttackChildRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class EndAttackBaseChildTask final {
public:
    static constexpr std::uint32_t kSize = 68u;
    static constexpr std::uint32_t kVtable = 0x845a40u;

    // IDA 0x565890: factory behind EndAttackBaseFactory 0x845984.
    [[nodiscard]] static std::uint32_t Create(
        IEndAttackChildRuntime& runtime);
};

}  // namespace pal4
