#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class PackageNameCrc final {
public:
    // IDA 0x793400: called when the package manager is initialized.
    void InitializeTable() noexcept;
    // IDA 0x793440: caller lowercases a file name before this call.
    [[nodiscard]] std::uint32_t Compute(const std::uint8_t* name) const noexcept;
    [[nodiscard]] bool Initialized() const noexcept { return initialized_; }
    [[nodiscard]] std::uint32_t TableEntry(std::uint8_t index) const noexcept {
        return table_[index];
    }

private:
    std::array<std::uint32_t, 256> table_{};
    bool initialized_ = false;
};

}  // namespace pal4
