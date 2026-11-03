#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerOwnedSlotRuntime {
public:
    virtual ~IActorManagerOwnedSlotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void DeleteActor(std::uint32_t actor, std::uint32_t flags) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerOwnedSlot final {
public:
    static void Release(std::uint32_t slot,
                        IActorManagerOwnedSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t MoveConstructNullable(
        std::uint32_t destination, std::uint32_t source,
        IActorManagerOwnedSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t MoveConstruct(
        std::uint32_t destination, std::uint32_t source,
        IActorManagerOwnedSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t slot, std::uint8_t flags,
        IActorManagerOwnedSlotRuntime& runtime);
    // IDA 0x555f70.
    [[nodiscard]] static std::uint32_t DestroyEventPayload(
        std::uint32_t slot, std::uint8_t flags,
        IActorManagerOwnedSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t MoveAssign(
        std::uint32_t destination, std::uint32_t source,
        IActorManagerOwnedSlotRuntime& runtime);
};

}  // namespace pal4
