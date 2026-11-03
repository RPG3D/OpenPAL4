#pragma once

#include <cstdint>

namespace pal4 {

class ReaderWriterLockState final {
public:
    [[nodiscard]] ReaderWriterLockState& Initialize() {
        first_ = 0u;
        second_ = 0u;
        return *this;
    }
    void Set(std::uint32_t first, std::uint32_t second) {
        first_ = first;
        second_ = second;
    }
    [[nodiscard]] std::uint32_t First() const { return first_; }
    [[nodiscard]] std::uint32_t Second() const { return second_; }

private:
    std::uint32_t first_ = 0u;
    std::uint32_t second_ = 0u;
};

}  // namespace pal4
