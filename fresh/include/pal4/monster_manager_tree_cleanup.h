#pragma once

#include <cstdint>

namespace pal4 {

class IMonsterManagerTreeCleanupRuntime {
public:
    virtual ~IMonsterManagerTreeCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) const = 0;
    virtual void ReleaseTextRecord(std::uint32_t record) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

// IDA 0x438a90. Frees a subtree and returns the current shared nil address.
class MonsterManagerTreeCleanup final {
public:
    [[nodiscard]] static std::uint32_t DestroySubtree(
        std::uint32_t root, IMonsterManagerTreeCleanupRuntime& runtime);
};

}  // namespace pal4
