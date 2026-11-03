#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorManagerSingletonState {
    std::uint8_t guard = 0;
    std::uint32_t instance = 0;
};

class IActorManagerMemory {
public:
    virtual ~IActorManagerMemory() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class IActorManagerLifecycleRuntime : public IActorManagerMemory {
public:
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
    virtual void InitializePrimary(std::uint32_t block) = 0;
    virtual void InitializeSecondary(std::uint32_t block) = 0;
    virtual void InitializeTertiary(std::uint32_t block) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPrimary(std::uint32_t block) = 0;
    virtual void CleanupSecondary(std::uint32_t block) = 0;
    virtual void EraseTaskNode(std::uint32_t block, std::uint32_t node) = 0;
    virtual void DestroyOwned(std::uint32_t object, std::uint32_t flags) = 0;
    virtual void ProcessEvent41(std::uint32_t collection) = 0;
    virtual void CleanupData5(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ResetTaskFlags(
        std::uint32_t queue) = 0;
};

// The529BD0 sequence also applies to native host collections, without
// exposing legacy VC6 vector/list layouts to the native implementation.
class IActorManagerRosterServices {
public:
    virtual ~IActorManagerRosterServices() = default;
    virtual void ClearPlayers() = 0;
    virtual void ClearMonsters() = 0;
    virtual void ResetTaskFlags() = 0;
};

class ActorManagerLifecycle final {
public:
    static constexpr std::uint32_t kSize = 108u;
    static void ClearCombatRosters(IActorManagerRosterServices&);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t manager, IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t manager, IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupOwnedArrays(
        std::uint32_t block, IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupOwnedList(
        std::uint32_t block, IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyData(
        std::uint32_t manager, IActorManagerLifecycleRuntime& runtime);
};

class ActorManagerSingleton final {
public:
    [[nodiscard]] static std::uint32_t Get(
        ActorManagerSingletonState& state,
        IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IActorManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t manager, IActorManagerLifecycleRuntime& runtime);
};

class IActorManagerUpdateRuntime {
public:
    virtual ~IActorManagerUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ProcessEvent40(std::uint32_t collection,
                                std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateTaskQueue(
        std::uint32_t queue, std::uint32_t argument) = 0;
    virtual void SetActorProperty(std::uint32_t actor,
                                  std::uint32_t argument) = 0;
    virtual void UpdateComponent(std::uint32_t component,
                                 std::uint32_t argument) = 0;
    virtual void UpdateRenderableCollection(
        std::uint32_t collection, std::uint32_t argument) = 0;
};

// Native collections retain their own storage; no VC6 list image is required
// to preserve the original 529A90 Player -> Monster -> shared-task order.
class IActorManagerDataServices {
public:
    virtual ~IActorManagerDataServices() = default;
    virtual void UpdatePlayers(float elapsed) = 0;
    virtual void UpdateMonsters(float elapsed) = 0;
    virtual void UpdateTasks(float elapsed) = 0;
};

//529B70 visits the first Player roster then the first Monster roster, including
//offstage actors. Native owners keep host storage and dispatch their real queues.
class IActorManagerRenderableServices {
public:
    virtual ~IActorManagerRenderableServices()=default;
    virtual void DispatchPlayers(std::uint32_t context)=0;
    virtual void DispatchMonsters(std::uint32_t context)=0;
};

class ActorManagerUpdate final {
public:
    static void UpdateData(float elapsed, IActorManagerDataServices&);
    static void UpdateRenderables(std::uint32_t context,IActorManagerRenderableServices&);
    [[nodiscard]] static std::uint32_t UpdateData(
        std::uint32_t manager, std::uint32_t argument,
        IActorManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessActors(
        std::uint32_t manager, std::uint32_t argument,
        IActorManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateComponents(
        std::uint32_t manager, std::uint32_t argument,
        IActorManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateRenderables(
        std::uint32_t manager, std::uint32_t argument,
        IActorManagerUpdateRuntime& runtime);
};

}  // namespace pal4
