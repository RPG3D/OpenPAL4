#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

struct DefendStatusTaskFactorySlot {
    std::uint32_t vtable = 0u;
};

class DefendStatusTaskFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845cf0u;
    // IDA 0x568d70: initialize the one-word child factory slot.
    [[nodiscard]] static DefendStatusTaskFactorySlot* Initialize(
        DefendStatusTaskFactorySlot* slot);
};

class IDefendSequenceRuntime {
public:
    virtual ~IDefendSequenceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void NotifyActor(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    [[nodiscard]] virtual std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
};

class DefendSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 48u;
    static constexpr std::uint32_t kVtable = 0x845b4cu;

    // IDA 0x568cf0: actor476 then timed Defend status child.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IDefendSequenceRuntime& runtime);

    // IDA 0x567500/0x567520: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
};

}  // namespace pal4
