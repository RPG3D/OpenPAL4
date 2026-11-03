#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerDataTransferRuntime {
public:
    virtual ~IActorManagerPlayerDataTransferRuntime() = default;
    // The game assigns source2584 to indexed name2 through a temporary text.
    // Native owners override this boundary with host strings, avoiding VC6 COW.
    virtual std::uint32_t FinishCopyFromSource(std::uint32_t actor,std::uint32_t source);
    // Native outcome owners assign into the persistent player's host strings.
    virtual void AssignFinalText(std::uint32_t destination, std::uint32_t source);
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source, std::uint32_t count) = 0;
    virtual void CopyProperty2(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void CopyProperty(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void CopyArray(std::uint32_t destination,
                           std::uint32_t source) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void EraseText(std::uint32_t record, std::uint32_t start,
                           std::uint32_t count) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t length) = 0;
    virtual void ActorSlot16(std::uint32_t actor,
                             std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot20(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot24(
        std::uint32_t actor, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot52(
        std::uint32_t actor, std::uint32_t mode) = 0;
    virtual void ActorSlot68(std::uint32_t actor,
                             std::uint32_t mode,
                             std::uint32_t value) = 0;
    virtual void ActorSlot276(std::uint32_t actor) = 0;
    virtual void CleanupObjectLists(std::uint32_t actor) = 0;
    virtual void ProcessCollections(std::uint32_t player) = 0;
    virtual void ResetObjectProperties(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryTextRecord() = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t value) = 0;
    virtual void SetProperty278(std::uint32_t actor,
                                std::uint32_t mode,
                                std::uint32_t text_record) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
};

class ActorManagerPlayerDataTransfer final {
public:
    // IDA 0x538660: copy the player's saved data into a combat actor.
    [[nodiscard]] static std::uint32_t CopyFromSource(
        std::uint32_t actor, std::uint32_t source,
        IActorManagerPlayerDataTransferRuntime& runtime);
    // IDA 0x5388d0: write a combat actor's current data back to the player.
    [[nodiscard]] static std::uint32_t CopyToPlayer(
        std::uint32_t actor, std::uint32_t player,
        IActorManagerPlayerDataTransferRuntime& runtime);
};

}  // namespace pal4
