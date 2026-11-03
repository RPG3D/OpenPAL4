#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class IActorManagerMainEventRuntime {
public:
    virtual ~IActorManagerMainEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t router, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindEventGroup(
        std::uint32_t router, std::uint32_t key) = 0;
    virtual void ReleaseOwnedSlot(std::uint32_t slot) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class ActorManagerMainEventHandlers final {
public:
    struct Binding {std::uint32_t key_global;std::uint32_t callback;std::string_view name;};
    [[nodiscard]] static std::span<const Binding> Bindings();
    static constexpr std::uint32_t kFirstSlot = 0x130u;
    static constexpr std::uint32_t kBindingCount = 10u;

    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t manager, IActorManagerMainEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t manager, IActorManagerMainEventRuntime& runtime);
};

class IActorManagerLifecycleRuntime : public IActorManagerMainEventRuntime {
public:
    virtual void InitializeGameObject(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupGameObject(
        std::uint32_t manager) = 0;
    virtual void ConstructNameNode(std::uint32_t node) = 0;
    virtual void ResetNameNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ResetEffectHandle(
        std::uint32_t& first, std::uint32_t& second) = 0;
};

class ActorManagerLifecycle final {
public:
    static constexpr std::uint32_t kVtable = 0x8452c8u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t manager, IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t manager, IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetEffectArgumentPair(
        std::uint32_t first, std::uint32_t second, std::uint32_t third,
        IActorManagerLifecycleRuntime& runtime);
};

}  // namespace pal4
