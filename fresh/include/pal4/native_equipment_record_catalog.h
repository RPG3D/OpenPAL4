#pragma once

#include "pal4/native_game_database.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pal4 {

// IDA 0x4d25a0 registers the 400-byte Equip image consumed by inventory.
class NativeEquipmentRecordCatalog final {
public:
    static constexpr std::uint32_t kRecordBytes = 400u;
    static constexpr std::uint32_t kTextBegin = 152u;
    static constexpr std::uint32_t kTextCount = 13u;

    explicit NativeEquipmentRecordCatalog(const NativeGameDatabase& database);

    [[nodiscard]] std::uint32_t Address(std::uint32_t id) const noexcept;
    [[nodiscard]] std::uint32_t Type(std::uint32_t id) const noexcept;
    [[nodiscard]] std::uint32_t ClassForType(std::uint32_t type) const noexcept;

private:
    struct Image {
        std::array<std::uint32_t, kRecordBytes / 4u> words{};
        std::array<std::unique_ptr<std::uint8_t[]>, kTextCount> text{};
    };

    std::vector<Image> images_;
    std::unordered_map<std::uint32_t, std::size_t> by_id_;
    std::array<std::uint32_t, 7u> classes_{};
};

}  // namespace pal4
