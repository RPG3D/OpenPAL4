#pragma once

#include "pal4/system_event_router_tree.h"
#include "pal4/system_manager_update.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class ISystemManagerHandlerRuntime : public ISystemEventRouterTreeRuntime {
public:
    virtual ~ISystemManagerHandlerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    virtual void Unregister(std::uint32_t router, std::uint32_t key,
                            std::uint32_t handle);
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t router, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback) {
        return SystemEventRouterTree::Register(
            router, mode, key, owner, callback, 0x8f609cu, *this);
    }
};

class SystemManagerHandlers final {
public:
    struct Binding {
        std::uint32_t key_global, mode, callback;
        std::string_view name;
    };
    [[nodiscard]] static std::span<const Binding> Bindings();
    // IDA 0x5279b0 and 0x527b70; the 13 (handle, key) pairs start at +4.
    static void Initialize(std::uint32_t manager,
                           ISystemManagerHandlerRuntime& runtime);
    static std::uint32_t Cleanup(std::uint32_t manager,
                                 ISystemManagerHandlerRuntime& runtime);
};

class ISystemManagerLifecycleRuntime : public ISystemManagerHandlerRuntime {
public:
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void TouchSkillSystem() = 0;
};

class SystemManagerLifecycle final {
public:
    // IDA 0x527c00, 0x527c70, 0x527ca0, 0x5287e0 and 0x528800.
    static std::uint32_t Construct(std::uint32_t manager,
                                   ISystemManagerLifecycleRuntime& runtime);
    static std::uint32_t Destruct(std::uint32_t manager,
                                  ISystemManagerLifecycleRuntime& runtime);
    static std::uint32_t UpdateState(
        std::uint32_t manager, ISystemManagerLifecycleRuntime& runtime);
    static void Deactivate(std::uint32_t manager,
                           ISystemManagerLifecycleRuntime& runtime);
    // IDA 0x5287c0, vtable844908 slot0.
    static void Activate(std::uint32_t manager,
                         ISystemManagerLifecycleRuntime& runtime);
    static void CleanupMemoryBlock(
        std::uint32_t block, ISystemManagerLifecycleRuntime& runtime);
};

class ISystemManagerCallbackRuntime : public ISystemManagerArrayRuntime {
public:
    virtual ~ISystemManagerCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InternalObject(
        std::uint32_t event_object) = 0;
    virtual void PrepareObject(std::uint32_t object) = 0;
    virtual void AppendObject(std::uint32_t manager, std::uint32_t object) = 0;
    virtual void TouchSkillSystem() = 0;
    virtual void Update(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeManagerSlot0(
        std::uint32_t manager) = 0;
};

class SystemManagerCallbacks final {
public:
    // IDA 0x527ff0, 0x528080 and 0x5280c0.
    static void AddObject(std::uint32_t manager, std::uint32_t event_object,
                          ISystemManagerCallbackRuntime& runtime);
    static void ProcessObject(std::uint32_t manager,
                              std::uint32_t event_object,
                              ISystemManagerCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t InvokeManager(
        std::uint32_t manager, ISystemManagerCallbackRuntime& runtime);
};

}  // namespace pal4
