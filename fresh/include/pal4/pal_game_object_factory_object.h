#pragma once

#include <cstdint>

namespace pal4 {

class IPalGameObjectFactoryObjectRuntime {
public:
    virtual ~IPalGameObjectFactoryObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IPalGameObjectFactoryControlRuntime {
public:
    virtual ~IPalGameObjectFactoryControlRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void InvokeActorControlProperty(
        std::uint32_t object, std::uint32_t virtual_offset,
        std::uint32_t value) = 0;
};

// Eight-byte factory handles registered for game-object types 4 and 0.
class PalGameObjectFactoryObject final {
public:
    [[nodiscard]] static std::uint32_t ReturnZero(
        std::uint32_t argument) noexcept;
    [[nodiscard]] static std::uint32_t ReturnOne(
        std::uint32_t argument) noexcept;
    [[nodiscard]] static std::uint32_t CreateClimb(
        IPalGameObjectFactoryObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructNormal(
        std::uint32_t object, IPalGameObjectFactoryObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateNormal(
        IPalGameObjectFactoryObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMovementFlagOne(
        std::uint32_t argument,
        IPalGameObjectFactoryControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMovementFlagZero(
        IPalGameObjectFactoryControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateSound(
        IPalGameObjectFactoryObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetActorPropertyZero(
        std::uint32_t argument,
        IPalGameObjectFactoryControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetActorPropertyOne(
        IPalGameObjectFactoryControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateLocalFloat(
        IPalGameObjectFactoryObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateLocalFloor(
        IPalGameObjectFactoryObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateMachine(
        IPalGameObjectFactoryObjectRuntime& runtime);
    static void DestructBase(std::uint32_t object,
                             IPalGameObjectFactoryObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        IPalGameObjectFactoryObjectRuntime& runtime);
};

}  // namespace pal4
