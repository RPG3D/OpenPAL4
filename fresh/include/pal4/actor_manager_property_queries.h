#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPropertyQueryRuntime {
public:
    virtual ~IActorManagerPropertyQueryRuntime() = default;
    [[nodiscard]] virtual std::int32_t ActorStateSlot20(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t start,
                            std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void Copy(std::uint32_t destination,
                      std::uint32_t source,
                      std::uint32_t bytes) = 0;
};

class ActorManagerPropertyQueries final {
public:
    static constexpr std::uint32_t kFixedDescriptor = 0x844a14u;
    static constexpr std::uint32_t kDescriptorTable = 0x844a08u;
    static constexpr std::uint32_t kDefaultDescriptor = 0x8f6140u;
    static constexpr std::uint32_t kEmptyText = 0x841964u;
    static constexpr std::uint32_t kEmptyErrorString = 0x8dc780u;

    [[nodiscard]] static std::uint32_t FixedDescriptor();
    [[nodiscard]] static std::uint32_t StateDescriptor(
        std::uint32_t actor, IActorManagerPropertyQueryRuntime& runtime);
    [[nodiscard]] static std::uint32_t StateDescriptor(std::int32_t slot);
    [[nodiscard]] static std::uint32_t NameOrEmpty(
        std::uint32_t actor, IActorManagerPropertyQueryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyTextVariant(
        std::uint32_t actor, std::uint32_t destination,
        std::int32_t selector, IActorManagerPropertyQueryRuntime& runtime);
};

class ActorManagerPlayerPropertyQueries final {
public:
    static constexpr std::uint32_t kDescriptorTable = 0x8449d8u;
    static constexpr std::uint32_t kDefaultDescriptor = 0x8f94a0u;
    static constexpr std::uint32_t kFixedDescriptor = 0x8449e4u;

    [[nodiscard]] static std::uint32_t StateDescriptor(
        std::uint32_t actor, IActorManagerPropertyQueryRuntime& runtime);
    [[nodiscard]] static std::uint32_t StateDescriptor(std::int32_t slot);
    [[nodiscard]] static std::uint32_t FixedDescriptor();
    [[nodiscard]] static std::uint32_t CopyTextVariant(
        std::uint32_t actor, std::uint32_t destination,
        std::int32_t selector, IActorManagerPropertyQueryRuntime& runtime);
};

}  // namespace pal4
