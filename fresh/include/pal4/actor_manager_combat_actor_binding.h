#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

// Game attachment sequence independent of the manager's legacy container image.
class IActorModelAttachmentRuntime {
public:
    virtual ~IActorModelAttachmentRuntime()=default;
    virtual std::uint32_t Model(std::uint32_t actor)=0;
    virtual void StoreModel(std::uint32_t actor,std::uint32_t child)=0;
    virtual void DetachCurrent(std::uint32_t actor)=0;
    virtual std::uint32_t AcquireModel(std::string_view name)=0;
    virtual void InitializeFrame(std::uint32_t child)=0;
    virtual void DisableCamera(std::uint32_t child)=0;
    virtual std::uint32_t BindParent(std::uint32_t child,std::uint32_t actor)=0;
    virtual void ReturnModel(std::uint32_t child,std::uint32_t limit)=0;
};

class IActorManagerCombatValueRuntime {
public:
    virtual ~IActorManagerCombatValueRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};
class IActorManagerCombatWordRuntime : public IActorManagerCombatValueRuntime {
public:
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};
class IActorManagerCombatBindingRuntime : public IActorManagerCombatWordRuntime {
public:
    [[nodiscard]] virtual bool ProcessActionTask(std::uint32_t task) = 0;
    virtual void ActorSlot588(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ManagerSingleton() = 0;
    [[nodiscard]] virtual std::uint32_t CreateChild(
        std::uint32_t collection, std::uint32_t list,
        std::string_view name) = 0;
    virtual void GetAnimationFrame(std::uint32_t child) = 0;
    virtual void RenderCamera(std::uint32_t child,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeChild(
        std::uint32_t child, std::uint32_t actor) = 0;
    virtual void ReleaseChild(std::uint32_t collection,
                              std::uint32_t list,
                              std::uint32_t limit,
                              std::uint32_t child) = 0;
};

class ActorManagerCombatActorBinding final {
public:
    [[nodiscard]] static std::uint32_t AttachChild(std::uint32_t actor,
        std::string_view name,IActorModelAttachmentRuntime& runtime);
    static void DetachChild(std::uint32_t actor,IActorModelAttachmentRuntime& runtime);
    [[nodiscard]] static bool IsActionValid(
        std::uint32_t actor, IActorManagerCombatBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t Value924(
        std::uint32_t actor, std::uint32_t unused,
        IActorManagerCombatValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t AttachChild(
        std::uint32_t actor, std::string_view name,
        IActorManagerCombatBindingRuntime& runtime);
    static void DetachChild(
        std::uint32_t actor, IActorManagerCombatBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetValue1164(
        std::uint32_t actor, std::uint32_t value,
        IActorManagerCombatWordRuntime& runtime);
};

}  // namespace pal4
