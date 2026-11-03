#include "pal4/actor_manager.h"
#include <bit>

namespace pal4 {
namespace {

std::uint32_t CleanupOwnedArray(const std::uint32_t block,
                                const std::uint32_t begin_offset,
                                IActorManagerLifecycleRuntime& runtime) {
    const auto begin = runtime.ReadWord(block + begin_offset);
    const auto end = runtime.ReadWord(block + begin_offset + 4u);
    for (auto entry = begin; entry != end; entry += 8u) {
        if (runtime.ReadByte(entry)) {
            const auto object = runtime.ReadWord(entry + 4u);
            if (object) runtime.DestroyOwned(object, 1u);
        }
    }
    const auto result = runtime.Free(begin);
    runtime.WriteWord(block + begin_offset, 0);
    runtime.WriteWord(block + begin_offset + 4u, 0);
    runtime.WriteWord(block + begin_offset + 8u, 0);
    return result;
}

template <typename Callback>
std::uint32_t VisitActors(const std::uint32_t manager, Callback&& callback,
                          IActorManagerUpdateRuntime& runtime) {
    for (auto slot = runtime.ReadWord(manager + 36u);
         slot != runtime.ReadWord(manager + 40u); slot += 4u)
        callback(runtime.ReadWord(slot));
    for (auto slot = runtime.ReadWord(manager + 4u);
         slot != runtime.ReadWord(manager + 8u); slot += 4u)
        callback(runtime.ReadWord(slot));
    return runtime.ReadWord(manager + 8u);
}

}  // namespace

std::uint32_t ActorManagerSingleton::Get(
    ActorManagerSingletonState& state,
    IActorManagerLifecycleRuntime& runtime) {
    if (state.guard & 1u) return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.RegisterSingleton(
        runtime.SingletonManager(), "cbActorManager", 1029u,
        0x5297b0u, 0x529810u);
    return state.instance;
}

std::uint32_t ActorManagerSingleton::Create(
    IActorManagerLifecycleRuntime& runtime) {
    const auto manager = runtime.Allocate(ActorManagerLifecycle::kSize);
    if (!manager) return 0;
    const auto initialized = ActorManagerLifecycle::Initialize(manager, runtime);
    if (!initialized) (void)runtime.Free(manager);
    return initialized;
}

std::uint32_t ActorManagerSingleton::Destroy(
    const std::uint32_t manager, IActorManagerLifecycleRuntime& runtime) {
    if (!manager) return 0;
    (void)ActorManagerLifecycle::Cleanup(manager, runtime);
    return runtime.Free(manager);
}

std::uint32_t ActorManagerLifecycle::Initialize(
    const std::uint32_t manager, IActorManagerLifecycleRuntime& runtime) {
    runtime.InitializePrimary(manager);
    runtime.InitializeSecondary(manager + 32u);
    runtime.InitializeTertiary(manager + 64u);
    // IDA copies an uninitialized stack byte to +96.
    runtime.WriteByte(manager + 96u, 0);
    const auto head = runtime.Allocate(16u);
    if (!head) {
        (void)CleanupOwnedArrays(manager + 64u, runtime);
        runtime.CleanupSecondary(manager + 32u);
        (void)runtime.CleanupPrimary(manager);
        return 0;
    }
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 4u, head);
    runtime.WriteWord(manager + 100u, head);
    runtime.WriteWord(manager + 104u, 0);
    return manager;
}

std::uint32_t ActorManagerLifecycle::CleanupOwnedArrays(
    const std::uint32_t block, IActorManagerLifecycleRuntime& runtime) {
    (void)CleanupOwnedArray(block, 20u, runtime);
    return CleanupOwnedArray(block, 4u, runtime);
}

std::uint32_t ActorManagerLifecycle::CleanupOwnedList(
    const std::uint32_t block, IActorManagerLifecycleRuntime& runtime) {
    return CleanupOwnedArray(block, 4u, runtime);
}

void ActorManagerLifecycle::ClearCombatRosters(IActorManagerRosterServices& runtime) {
    runtime.ClearPlayers();
    runtime.ClearMonsters();
    runtime.ResetTaskFlags();
}

std::uint32_t ActorManagerLifecycle::DestroyData(
    const std::uint32_t manager, IActorManagerLifecycleRuntime& runtime) {
    class Services final : public IActorManagerRosterServices {
        std::uint32_t manager_;
        IActorManagerLifecycleRuntime& runtime_;
    public:
        std::uint32_t result{};
        Services(std::uint32_t m,IActorManagerLifecycleRuntime& r):manager_(m),runtime_(r){}
        void ClearPlayers() override { runtime_.ProcessEvent41(manager_+32u); }
        void ClearMonsters() override { runtime_.CleanupData5(manager_); }
        void ResetTaskFlags() override { result=runtime_.ResetTaskFlags(manager_+96u); }
    } services(manager,runtime);
    ClearCombatRosters(services);
    return services.result;
}

std::uint32_t ActorManagerLifecycle::Cleanup(
    const std::uint32_t manager, IActorManagerLifecycleRuntime& runtime) {
    if (!manager) return 0;
    const auto head = runtime.ReadWord(manager + 100u);
    if (head) {
        for (auto node = runtime.ReadWord(head); node != head;) {
            const auto next = runtime.ReadWord(node);
            runtime.EraseTaskNode(manager + 96u, node);
            node = next;
        }
        (void)runtime.Free(head);
    }
    runtime.WriteWord(manager + 100u, 0);
    runtime.WriteWord(manager + 104u, 0);
    (void)CleanupOwnedArrays(manager + 64u, runtime);
    runtime.CleanupSecondary(manager + 32u);
    return runtime.CleanupPrimary(manager);
}

std::uint32_t ActorManagerUpdate::UpdateData(
    const std::uint32_t manager, const std::uint32_t argument,
    IActorManagerUpdateRuntime& runtime) {
    class Services final : public IActorManagerDataServices {
        std::uint32_t manager_;
        IActorManagerUpdateRuntime& runtime_;
    public:
        std::uint32_t result{};
        Services(std::uint32_t m,IActorManagerUpdateRuntime& r):manager_(m),runtime_(r){}
        void UpdatePlayers(float dt) override {runtime_.ProcessEvent40(manager_+32u,std::bit_cast<std::uint32_t>(dt));}
        void UpdateMonsters(float dt) override {runtime_.ProcessEvent40(manager_,std::bit_cast<std::uint32_t>(dt));}
        void UpdateTasks(float dt) override {result=runtime_.UpdateTaskQueue(manager_+96u,std::bit_cast<std::uint32_t>(dt));}
    } services(manager,runtime);
    UpdateData(std::bit_cast<float>(argument),services);
    return services.result;
}

void ActorManagerUpdate::UpdateData(float elapsed,IActorManagerDataServices& services) {
    services.UpdatePlayers(elapsed);
    services.UpdateMonsters(elapsed);
    services.UpdateTasks(elapsed);
}

std::uint32_t ActorManagerUpdate::ProcessActors(
    const std::uint32_t manager, const std::uint32_t argument,
    IActorManagerUpdateRuntime& runtime) {
    return VisitActors(manager, [&](const std::uint32_t actor) {
        runtime.SetActorProperty(actor, argument);
    }, runtime);
}

std::uint32_t ActorManagerUpdate::UpdateComponents(
    const std::uint32_t manager, const std::uint32_t argument,
    IActorManagerUpdateRuntime& runtime) {
    return VisitActors(manager, [&](const std::uint32_t actor) {
        const auto component = runtime.ReadWord(actor + 808u);
        if (component) runtime.UpdateComponent(component, argument);
    }, runtime);
}

void ActorManagerUpdate::UpdateRenderables(std::uint32_t context,IActorManagerRenderableServices& services){
    services.DispatchPlayers(context);services.DispatchMonsters(context);
}
std::uint32_t ActorManagerUpdate::UpdateRenderables(
    const std::uint32_t manager,const std::uint32_t argument,IActorManagerUpdateRuntime& runtime){
    class Services final : public IActorManagerRenderableServices {
        std::uint32_t manager;IActorManagerUpdateRuntime& runtime;
        void Dispatch(std::uint32_t begin,std::uint32_t end,std::uint32_t context){
            for(auto slot=runtime.ReadWord(manager+begin);slot!=runtime.ReadWord(manager+end);slot+=4u)
                runtime.UpdateRenderableCollection(runtime.ReadWord(slot)+728u,context);
        }
    public:
        Services(std::uint32_t m,IActorManagerUpdateRuntime& r):manager(m),runtime(r){}
        void DispatchPlayers(std::uint32_t c)override{Dispatch(36u,40u,c);}
        void DispatchMonsters(std::uint32_t c)override{Dispatch(4u,8u,c);}
    } services(manager,runtime);
    UpdateRenderables(argument,services);return runtime.ReadWord(manager+8u);
}

}  // namespace pal4
