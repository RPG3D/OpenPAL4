#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct FatalChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class FatalChildFactories final {
public:
    // IDA 0x56b350/0x56b360/0x56b370/0x56b380/0x56b560.
    [[nodiscard]] static FatalChildFactorySlot* InitializeEvent(
        FatalChildFactorySlot* slot);
    [[nodiscard]] static FatalChildFactorySlot* InitializeFirstFinal(
        FatalChildFactorySlot* slot);
    [[nodiscard]] static FatalChildFactorySlot* InitializeFinish(
        FatalChildFactorySlot* slot);
    [[nodiscard]] static FatalChildFactorySlot* InitializeEnd(
        FatalChildFactorySlot* slot);
    [[nodiscard]] static FatalChildFactorySlot* InitializeMessage(
        FatalChildFactorySlot* slot);
};

class IFatalSequenceRuntime {
public:
    virtual ~IFatalSequenceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetActorState(std::uint32_t actor,
                               std::uint32_t state,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    [[nodiscard]] virtual std::uint32_t EventDataByIndex(
        std::uint32_t index) = 0;
    virtual void AssignText(std::uint32_t header,
                            std::uint32_t source) = 0;
    virtual void SetMessage(std::uint32_t header,
                            std::uint32_t source) = 0;
};

class IFinalHitCompletionRuntime {
public:
    virtual ~IFinalHitCompletionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ActorVirtual172(std::uint32_t actor,
                                 std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorVirtual544(
        std::uint32_t actor, std::uint32_t target) = 0;
};

}  // namespace pal4
