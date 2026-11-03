#pragma once
#include "pal4/native_animated_actor_asset.h"
#include <rwcore.h>

namespace pal4::test {
// Observe real SDK allocations while loading the asset. Forward every callback
// to the creating allocator; never change allocation size, address or timing.
class ActorLoadOrderProbe final {
public:
    explicit ActorLoadOrderProbe(const NativeAnimatedActorAsset& actor):actor_(actor) {
        table_=RwOsGetMemoryInterface();prior_=*table_;active_=this;
        *table_={Allocate,Free,Reallocate,AllocateZero};
    }
    ~ActorLoadOrderProbe(){*table_=prior_;active_=nullptr;}
    ActorLoadOrderProbe(const ActorLoadOrderProbe&)=delete;
    [[nodiscard]] bool Passed()const{return observed_>0 && !early_shadow_;}
private:
    static void Inspect() {
        ++active_->observed_;
        active_->early_shadow_|=active_->actor_.Shadow()!=nullptr && active_->actor_.ActionCount()==0;
    }
    static void* Allocate(std::size_t size,RwUInt32 hint) {
        Inspect();return active_->prior_.rwmalloc(size,hint);
    }
    static void Free(void* memory){active_->prior_.rwfree(memory);}
    static void* Reallocate(void* memory,std::size_t size,RwUInt32 hint) {
        Inspect();return active_->prior_.rwrealloc(memory,size,hint);
    }
    static void* AllocateZero(std::size_t count,std::size_t size,RwUInt32 hint) {
        Inspect();return active_->prior_.rwcalloc(count,size,hint);
    }
    inline static ActorLoadOrderProbe* active_{};
    const NativeAnimatedActorAsset& actor_;
    RwMemoryFunctions* table_{};
    RwMemoryFunctions prior_{};
    unsigned observed_{};
    bool early_shadow_{};
};
}
