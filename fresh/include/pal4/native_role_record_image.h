#pragma once

#include "pal4/native_game_database.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace pal4 {

// Owns the 320-byte x86 Role record consumed by PlayerManagerSlot_CopyData.
// The text prefix forces the native slot adapter to copy instead of retaining
// a VC6 COW reference. The record address remains valid until this image moves
// or is destroyed.
class NativeRoleRecordImage final {
public:
    static constexpr std::size_t kWordCount = 80u;
    using Words = std::array<std::uint32_t, kWordCount>;

    NativeRoleRecordImage() = default;
    NativeRoleRecordImage(NativeRoleRecordImage&&) = default;
    NativeRoleRecordImage& operator=(NativeRoleRecordImage&&) = default;
    NativeRoleRecordImage(const NativeRoleRecordImage&) = delete;
    NativeRoleRecordImage& operator=(const NativeRoleRecordImage&) = delete;

    [[nodiscard]] static std::optional<NativeRoleRecordImage> FromDatabase(
        const NativeGameDatabase& database, std::uint32_t player_index);
    [[nodiscard]] const Words& Data() const noexcept { return words_; }
    [[nodiscard]] std::uint32_t Address() const noexcept;

private:
    [[nodiscard]] bool SetText(std::uint32_t offset, std::string_view value);

    Words words_{};
    std::array<std::unique_ptr<std::uint8_t[]>, 9u> text_{};
};

// Both original Role lookups (0x441250 and 0x43ec80) read the same four rows.
// Keep their x86 images alive until player loading and postprocessing finish.
class NativeRoleRecordCatalog final {
public:
    explicit NativeRoleRecordCatalog(const NativeGameDatabase& database);

    [[nodiscard]] std::uint32_t Address(std::uint32_t player_index) const
        noexcept;
    [[nodiscard]] bool Owns(std::uint32_t record) const noexcept;

private:
    std::array<NativeRoleRecordImage, 4u> roles_{};
};

}  // namespace pal4
