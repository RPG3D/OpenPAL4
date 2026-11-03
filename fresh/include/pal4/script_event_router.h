#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemEventRouterTreeRuntime;

class IScriptEventRouterRuntime {
public:
    virtual ~IScriptEventRouterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateObject(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateNode(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
    virtual void GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t identifier,
        std::uint32_t create_entry, std::uint32_t destroy_entry) = 0;
    virtual void ResetHeadCursor(std::uint32_t router) = 0;
    virtual void ReleaseOwnedCallback(std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t FindNode(
        std::uint32_t router, std::uint32_t key) = 0;
    virtual void InvokeCallback(std::uint32_t handler,
                                std::uint32_t payload) = 0;
};

class ScriptEventRouter final {
public:
    static constexpr std::uint32_t kSingletonPointer = 0x8fa048u;
    static constexpr std::uint32_t kSingletonGuard = 0x8fa04cu;
    static constexpr std::uint32_t kNilPointer = 0x8f609cu;
    static constexpr std::uint32_t kInstanceCount = 0x8fa050u;
    static constexpr std::uint32_t kCreateEntry = 0x563000u;
    static constexpr std::uint32_t kDestroyEntry = 0x5630f0u;

    // IDA 0x562fc0/0x563000/0x5630f0.
    [[nodiscard]] static std::uint32_t GetInstance(
        IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t router, IScriptEventRouterRuntime& runtime);
    // IDA 0x563110/0x5631d0.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t router, IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessEvent(
        std::uint32_t router, std::uint32_t key,
        std::uint32_t payload, std::uint32_t unused,
        IScriptEventRouterRuntime& runtime);
    // IDA 0x574c30. Registers an owner/callback for an event key and mode.
    [[nodiscard]] static std::uint32_t RegisterHandler(
        std::uint32_t router, std::uint32_t mode,
        std::uint32_t key, std::uint32_t owner,
        std::uint32_t callback, ISystemEventRouterTreeRuntime& runtime);
};

class ScriptEventRouterTree final {
public:
    // IDA 0x563240/0x563670/0x5636c0/0x5636d0/0x563720.
    [[nodiscard]] static std::uint32_t EraseToOutput(
        std::uint32_t router, std::uint32_t output,
        std::uint32_t node, IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseNode(
        std::uint32_t router, std::uint32_t node,
        IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseRecursive(
        std::uint32_t root, IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetEvent(std::uint32_t entry);
    [[nodiscard]] static std::uint32_t NextEvent(
        std::uint32_t iterator_slot,
        IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextNode(
        std::uint32_t node, IScriptEventRouterRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveEvent(
        std::uint32_t list, std::uint32_t output,
        std::uint32_t entry, IScriptEventRouterRuntime& runtime);
};

}  // namespace pal4
