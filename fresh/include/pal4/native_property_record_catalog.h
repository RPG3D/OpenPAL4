#pragma once

#include "pal4/native_game_database.h"
#include <array>
#include <memory>
#include <unordered_map>

namespace pal4 {

// Owns the complete 168-byte Property images referenced by inventory items.
// Field offsets/names come from the shared IDA-backed property registry.
class NativePropertyRecordCatalog final {
public:
    static constexpr std::uint32_t kRecordBytes = 168u;
    explicit NativePropertyRecordCatalog(const NativeGameDatabase& database);
    [[nodiscard]] std::uint32_t Address(std::uint32_t id) const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept { return images_.size(); }
    // Original table order, used by5715F0 rank-reward pool enumeration.
    [[nodiscard]] std::uint32_t At(std::size_t index) const {
        return reinterpret_cast<std::uint32_t>(images_.at(index).words.data());
    }
private:
    struct Image {
        std::array<std::uint32_t, 42u> words{};
        std::array<std::unique_ptr<std::uint8_t[]>, 7u> text{};
    };
    std::vector<Image> images_;
    std::unordered_map<std::uint32_t, std::size_t> by_id_;
};
}  // namespace pal4
