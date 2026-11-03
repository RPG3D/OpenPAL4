#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

struct WinActionChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class WinActionSecondChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845a80u;
    // IDA 0x566480.
    [[nodiscard]] static WinActionChildFactorySlot* Initialize(
        WinActionChildFactorySlot* slot);
};

class WinActionThirdChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845a84u;
    // IDA 0x566490.
    [[nodiscard]] static WinActionChildFactorySlot* Initialize(
        WinActionChildFactorySlot* slot);
};

class IWinActionRuntime {
public:
    virtual ~IWinActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    // Original actor virtual568, distinct from virtual476 StartAction.
    virtual void RestoreRequestedModel(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> ActorPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text, const std::array<std::uint32_t, 3>& timing) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    // Native hosts can assign through their owned action-string boundary.
    virtual std::uint8_t AssignVictoryText(std::uint32_t child,std::uint32_t original_header);
    virtual void SetTextErrorFlag(std::uint32_t header,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t header,
                                            std::uint32_t length,
                                            std::uint8_t flag) = 0;
    virtual void CopyText(std::uint32_t header, std::uint32_t source,
                          std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t EraseText(
        std::uint32_t header, std::uint32_t start,
        std::uint32_t count) = 0;
};

class WinActionTask final {
public:
    static constexpr std::uint32_t kSize = 48u;
    static constexpr std::uint32_t kVtable = 0x845cb8u;

    // IDA 0x5661e0: schedule three children and choose victory text.
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, IWinActionRuntime& runtime);

    // IDA 0x5689d0/0x5689f0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
};

}  // namespace pal4
