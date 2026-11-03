#pragma once
#include "pal4/actor_manager_combat_actor.h"
#include "pal4/native_player_slot_copy_runtime.h"

namespace pal4 {

// Native memory boundary for shared 531170. Text is always copied into the
// existing host std::string owner; no legacy COW reference may escape here.
class NativeCombatActorCopyRuntime final : public IActorManagerCombatCopyRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void EraseText(std::uint32_t record, std::uint32_t start, std::uint32_t count) override;
    void SetErrorFlag(std::uint32_t record, std::uint8_t value) override;
    bool AllocateText(std::uint32_t record, std::uint32_t length, std::uint8_t mode) override;
    void SetTextSize(std::uint32_t record, std::uint32_t length) override;
    void CopyBytes(std::uint32_t destination, std::uint32_t source, std::uint32_t count) override;
    void AssignText(std::uint32_t record,std::string_view value);
    void ReleaseActorTexts(std::uint32_t actor);
    void ReleaseText(std::uint32_t record);
    std::size_t OwnedTextCount() const noexcept { return texts_.OwnedTextCount(); }
private:
    NativePlayerSlotCopyRuntime texts_;
};
} // namespace pal4
