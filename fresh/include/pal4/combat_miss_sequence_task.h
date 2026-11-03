#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

struct MissMessageChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class MissMessageChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845d14u;

    // IDA 0x569320: initialize the first Miss child factory slot.
    [[nodiscard]] static MissMessageChildFactorySlot* Initialize(
        MissMessageChildFactorySlot* slot);
};

class IMissStartRuntime {
public:
    virtual ~IMissStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ActorVirtual512(std::uint32_t source,
                                 std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    [[nodiscard]] virtual std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t start,
                            std::uint32_t count) = 0;
};

class MissSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845c64u;

    // IDA 0x5684f0/0x568510: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);

    // IDA 0x569250: dispatch the miss and enqueue two child tasks.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IMissStartRuntime& runtime);
};

class QuickMissSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845c80u;

    // IDA 0x5685b0/0x5685d0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
};

}  // namespace pal4
