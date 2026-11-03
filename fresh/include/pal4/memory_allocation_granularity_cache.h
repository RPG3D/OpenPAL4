#pragma once

#include <cstdint>

namespace pal4 {

class ISystemAllocationGranularity {
public:
    virtual ~ISystemAllocationGranularity() = default;
    [[nodiscard]] virtual std::uint32_t GetSystemAllocationGranularity() = 0;
};

class MemoryAllocationGranularityCache final {
public:
    [[nodiscard]] std::uint32_t Get(ISystemAllocationGranularity& platform);
    [[nodiscard]] std::uint32_t CachedValue() const { return granularity_; }

private:
    std::uint32_t granularity_ = 0u;
};

}  // namespace pal4
