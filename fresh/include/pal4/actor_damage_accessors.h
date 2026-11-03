#pragma once

#include <cstdint>

namespace pal4 {

class IActorDamageConstructionRuntime {
public:
    virtual ~IActorDamageConstructionRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class IActorDamageSlotRuntime : public IActorDamageConstructionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address)=0;
};

class IActorDamageAccessorRuntime : public IActorDamageSlotRuntime {
public:
    ~IActorDamageAccessorRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t CallIntGet(
        std::uint32_t actor, std::uint32_t captured_vtable,
        std::uint32_t virtual_offset, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t CallIntSet(
        std::uint32_t actor, std::uint32_t captured_vtable,
        std::uint32_t virtual_offset, std::uint32_t index,
        std::int32_t value) = 0;
    [[nodiscard]] virtual double CallFloatGet(
        std::uint32_t actor, std::uint32_t captured_vtable,
        std::uint32_t virtual_offset, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t CallFloatSet(
        std::uint32_t actor, std::uint32_t captured_vtable,
        std::uint32_t virtual_offset, std::uint32_t index,
        std::uint32_t value_bits) = 0;
};

class ActorDamageAccessors final {
public:
    // IDA 0x56cdd0-0x56ce20.
    [[nodiscard]] static std::uint32_t ConstructCountCallback(
        std::uint32_t object, IActorDamageConstructionRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetPropertyCount();
    [[nodiscard]] static std::uint32_t GetActorCount672(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetActorCount668(
        std::uint32_t actor, IActorDamageSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetActorCount672(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageConstructionRuntime& runtime);

    // IDA 0x56d150-0x56d4f0: direct state getters.
    [[nodiscard]] static std::uint32_t GetCountState632(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState636(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState640(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState644(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState648(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState652(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState656(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState660(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCountState664(
        std::uint32_t actor, IActorDamageAccessorRuntime& runtime);

    // IDA 0x56d7a0/0x56d870/0x56d940: direct float additions.
    [[nodiscard]] static double AddDirectFloat484(
        std::uint32_t actor, std::int32_t index, float delta,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static double AddDirectFloat508(
        std::uint32_t actor, std::int32_t index, float delta,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static double AddDirectFloat536(
        std::uint32_t actor, std::int32_t index, float delta,
        IActorDamageAccessorRuntime& runtime);

    // IDA 0x56bf80.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IActorDamageConstructionRuntime& runtime);

    // IDA 0x56bfc0-0x56c2b0, in function-entry order.
    [[nodiscard]] static std::uint32_t GetWord344(
        std::uint32_t actor, std::uint32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddWord48_64(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static double GetFloat376(
        std::uint32_t actor, std::int32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddFloat44_60(
        std::uint32_t actor, std::uint32_t index,
        float delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetWord464(
        std::uint32_t actor, std::uint32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddWord28_32(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetWord392(
        std::uint32_t actor, std::uint32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddWord88_92(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static double GetFloat416(
        std::uint32_t actor, std::int32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddFloat100_104(
        std::uint32_t actor, std::uint32_t index,
        float delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static double GetFloat440(
        std::uint32_t actor, std::int32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddFloat112_116(
        std::uint32_t actor, std::uint32_t index,
        float delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetWord544(
        std::uint32_t actor, std::uint32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddWord52_68(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static double GetFloat584(
        std::uint32_t actor, std::int32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddFloat40_56(
        std::uint32_t actor, std::uint32_t index,
        float delta, IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static double GetFloat604(
        std::uint32_t actor, std::int32_t index,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t AddFloat124_128(
        std::uint32_t actor, std::uint32_t index,
        float delta, IActorDamageAccessorRuntime& runtime);

    // IDA 0x56c2e0/0x56c300.
    [[nodiscard]] static std::uint32_t SetConditional672(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageAccessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMirrored332_668(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageSlotRuntime& runtime);
};

}  // namespace pal4
