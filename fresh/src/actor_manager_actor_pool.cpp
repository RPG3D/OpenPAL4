#include "pal4/actor_manager_actor_pool.h"
#include <new>

namespace pal4 {
namespace {
// Retains the existing image-boundary algorithms; no native VC6 vector added.
class ImagePool final : public IActorManagerModelPoolRuntime {
public:
    std::uint32_t pool;
    IActorManagerActorPoolRuntime& runtime;
    ImagePool(std::uint32_t p,IActorManagerActorPoolRuntime& r):pool(p),runtime(r){}
    bool CombatInactive() override { return runtime.CombatInactive(); }
    std::size_t CachedCount() override {
        const auto begin=runtime.ReadWord(pool+4u);
        return begin ? (runtime.ReadWord(pool+8u)-begin)/8u : 0u;
    }
    std::string CachedName(std::size_t index) override {
        return runtime.ActorName(runtime.ReadWord(runtime.ReadWord(pool+4u)+
            static_cast<std::uint32_t>(index)*8u+4u));
    }
    std::uint32_t ExtractCached(std::size_t index) override {
        const auto slot=runtime.ReadWord(pool+4u)+static_cast<std::uint32_t>(index)*8u;
        const auto actor=runtime.ReadWord(slot+4u);
        runtime.WriteByte(slot,0u);
        EraseSlot(slot);
        return actor;
    }
    void EraseSlot(std::uint32_t slot) {
        const auto end=runtime.ReadWord(pool+8u);
        for (auto source=slot+8u;source!=end;source+=8u)
            (void)ActorManagerOwnedSlot::MoveAssign(source-8u,source,runtime);
        (void)ActorManagerOwnedSlot::Destroy(end-8u,0u,runtime);
        runtime.WriteWord(pool+8u,end-8u);
    }
    std::uint32_t CreateModel(std::string_view name) override {
        const auto storage=runtime.Allocate(0x610u);
        if (!storage) throw std::bad_alloc();
        const auto actor=runtime.ConstructActor(storage);
        if (runtime.InitializeActor(actor,name,name)) return actor;
        runtime.DeleteActor(actor,1u);
        return 0u;
    }
    void DeactivateModel(std::uint32_t actor,std::uint32_t context) override {
        runtime.DeactivateActor(actor,context);
    }
    void EvictOldest() override { EraseSlot(runtime.ReadWord(pool+4u)); }
    void AppendModel(std::uint32_t actor) override {
        const auto temporary=runtime.CreateTemporarySlot(actor);
        try {
            (void)ActorManagerOwnedSlotArray::InsertCopies(
                pool,runtime.ReadWord(pool+8u),1u,temporary,runtime);
        } catch (...) {
            (void)ActorManagerOwnedSlot::Destroy(temporary,0u,runtime);
            throw;
        }
        (void)ActorManagerOwnedSlot::Destroy(temporary,0u,runtime);
    }
};
}
std::uint32_t ActorManagerActorPool::TakeOrCreate(
    std::uint32_t pool,std::string_view name,IActorManagerActorPoolRuntime& runtime) {
    ImagePool adapter(pool,runtime);
    return TakeOrCreate(name,adapter);
}
void ActorManagerActorPool::Return(std::uint32_t pool,std::uint32_t limit,
    std::uint32_t actor,std::uint32_t context,IActorManagerActorPoolRuntime& runtime) {
    ImagePool adapter(pool,runtime);
    Return(limit,actor,context,adapter);
}
} // namespace pal4
