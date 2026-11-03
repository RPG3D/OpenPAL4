#pragma once

#include <cstdint>

namespace pal4 {

class ISystemPageInfo {
public:
    virtual ~ISystemPageInfo() = default;
    [[nodiscard]] virtual std::uint32_t GetSystemPageSize() = 0;
};

class MemoryPageSizeCache final {
public:
    [[nodiscard]] std::uint32_t Get(ISystemPageInfo& platform);
    [[nodiscard]] std::uint32_t CachedValue() const { return page_size_; }

private:
    std::uint32_t page_size_ = 0u;
};

}  // namespace pal4
