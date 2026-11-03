#pragma once

#include "pal4/role_static_elements.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

enum class RoleSelectorEvent {
    Clicked,
    KeyDown,
};

class IRoleSelectorWindowRuntime : public IRoleStaticElementsRuntime {
public:
    virtual void LoadLayout(std::uint32_t object,
                            const char* layout) = 0;
    virtual void ConstructNavigation(
        std::uint32_t navigation) = 0;
    virtual void DestructNavigation(
        std::uint32_t navigation) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void ConstructVector2(std::uint32_t vector) = 0;
    virtual void InitializeCloseNameBuffer() = 0;
    [[nodiscard]] virtual std::string ReadLayoutPrefix(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t object, const std::string& name) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 4>
        CopyPrefixArguments(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 2>
        GetPosition(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Subscribe(std::uint32_t window,
                           RoleSelectorEvent event,
                           std::uint32_t callback) = 0;
    virtual void ReleaseSubscriptionCallback(
        std::uint32_t callback) = 0;
    virtual void SetupMouseHandlers(std::uint32_t navigation,
                                    std::uint32_t button) = 0;
    virtual void LayoutButtons(std::uint32_t object) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window,
                                bool enabled) = 0;
    virtual void CleanupCloseNameBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class IRoleSelectorLifecycleRuntime {
public:
    virtual ~IRoleSelectorLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyCallbackResult() = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t object) = 0;
};

class IRoleSelectorVisibilityRuntime : public IRoleSelectorLifecycleRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::uint32_t name, std::uint32_t visible,
        std::uint32_t option) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CloneCallback(
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t LayoutButtons(
        std::uint32_t object) = 0;
    virtual void SetModal(std::uint32_t root, bool enabled) = 0;
    virtual void SetParent(std::uint32_t object,
                           std::uint32_t visible,
                           std::uint32_t parent) = 0;
};

class RoleSelectorWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x8442ecu;
    static constexpr std::uint32_t kNavigationVtable = 0x8442c4u;
    static constexpr std::uint32_t kCallbackVtable = 0x8442bcu;

    // IDA 0x4b6360. The incoming name argument is not read by this body.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t unused_name,
        IRoleSelectorWindowRuntime& runtime);
    // IDA 0x4b6670.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object,
        IRoleSelectorLifecycleRuntime& runtime);
    // IDA 0x4b66e0.
    [[nodiscard]] static std::uint32_t ShowOrHide(
        std::uint32_t visible, std::uint32_t callback_source,
        std::uint32_t context,
        IRoleSelectorVisibilityRuntime& runtime);
    // IDA 0x4b67d0.
    static void SetModalState(
        std::uint32_t object, std::uint32_t visible,
        std::uint32_t parent,
        IRoleSelectorVisibilityRuntime& runtime);
    // IDA 0x4b6810. Its EAX result is incidental when the slot is empty.
    [[nodiscard]] static std::uint32_t CleanupCallback(
        std::uint32_t object,
        IRoleSelectorLifecycleRuntime& runtime);
};

}  // namespace pal4
