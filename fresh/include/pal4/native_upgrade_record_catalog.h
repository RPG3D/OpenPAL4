#pragma once

#include "pal4/native_game_database.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pal4 {

// Owns the contiguous 160-byte UpgradeData records read by IDA 0x57DB70.
class NativeUpgradeRecordCatalog final {
public:
    using Words = std::array<std::uint32_t, 40u>;

    explicit NativeUpgradeRecordCatalog(const NativeGameDatabase& database);

    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] std::size_t Count() const noexcept { return records_.size(); }
    [[nodiscard]] const Words& Record(std::size_t index) const {
        return records_.at(index);
    }

private:
    std::vector<Words> records_;
};

}  // namespace pal4
