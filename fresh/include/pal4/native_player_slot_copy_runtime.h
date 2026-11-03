#pragma once

#include "pal4/player_manager_slot_data_copier.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pal4 {

// Owns copied Role strings while their x86 player-slot views remain in use.
// The source Role image marks its strings as non-shareable, so the rewritten
// slot copier delegates allocation here instead of using VC6 COW storage.
class NativePlayerSlotCopyRuntime final :
    public IPlayerManagerSlotCopyRuntime {
public:
    NativePlayerSlotCopyRuntime() = default;
    NativePlayerSlotCopyRuntime(const NativePlayerSlotCopyRuntime&) = delete;
    NativePlayerSlotCopyRuntime& operator=(
        const NativePlayerSlotCopyRuntime&) = delete;
    NativePlayerSlotCopyRuntime(NativePlayerSlotCopyRuntime&&) = delete;
    NativePlayerSlotCopyRuntime& operator=(
        NativePlayerSlotCopyRuntime&&) = delete;

    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] std::uint32_t EmptyTextAddress() override;
    void EraseText(std::uint32_t record, std::uint32_t start,
                   std::uint32_t count) override;
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override;
    [[nodiscard]] bool AllocateText(std::uint32_t record,
                                     std::uint32_t length,
                                     std::uint8_t mode) override;
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t count) override;

    void ReleaseText(std::uint32_t record);
    void ValidateText(std::uint32_t record) const;
    void SetTextSize(std::uint32_t record, std::uint32_t length);
    // Archive input shares the session's existing Role text owner.
    void AssignText(std::uint32_t record, std::string_view value);
    [[nodiscard]] std::size_t OwnedTextCount() const noexcept {
        return text_.size();
    }

private:
    void ClearText(std::uint32_t record);

    std::unordered_map<std::uint32_t, std::string> text_;
};

}  // namespace pal4
