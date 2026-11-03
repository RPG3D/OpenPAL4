#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

class ILevelUpActionRuntime {
public:
    virtual ~ILevelUpActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetActorState(std::uint32_t actor,
                               std::uint32_t state,
                               std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text, const std::array<std::uint32_t, 3>& timing) = 0;
    virtual std::uint8_t AssignLevelUpText(std::uint32_t child);
    virtual void FreeText(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t header, std::uint32_t length,
        std::uint8_t flag) = 0;
    virtual void CopyText(std::uint32_t header,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
    virtual void DetachText(std::uint32_t header) = 0;
    virtual void MoveTextBytes(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t length) = 0;
    virtual void SetTextSize(std::uint32_t header,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t DetachTextAndReturn(
        std::uint32_t header) = 0;
};

class LevelUpActionTask final {
public:
    static constexpr std::uint32_t kSize = 48u;
    static constexpr std::uint32_t kVtable = 0x845cd4u;

    // IDA 0x5664a0: set state 7 and schedule the level-up text child.
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, ILevelUpActionRuntime& runtime);

    // IDA 0x568c10/0x568c30: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);


};

}  // namespace pal4
