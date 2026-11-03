#include "pal4/native_collision_event_router.h"
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

namespace {
struct Owner final : pal4::INativeCollisionEventCallbacks {
    std::uint32_t identity;
    std::vector<std::array<std::uint32_t,3>> calls;
    explicit Owner(std::uint32_t id):identity(id){}
    void Enter(std::uint32_t id,std::uint32_t previous)override{
        if(id!=identity)throw std::invalid_argument("foreign callback generation");
        calls.push_back({id,previous,0u});
    }
    void Leave(std::uint32_t id)override{
        if(id!=identity)throw std::invalid_argument("foreign callback generation");
        calls.push_back({id,0u,1u});
    }
};
template<class F>void Reject(F f){bool rejected{};try{f();}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("expected rejection");}
void Check(bool ok){if(!ok)throw std::runtime_error("collision router check");}
}
int main(){
    pal4::NativeCollisionEventRouter router;Owner evf(0x1000u),obstacle(0x2000u),other(0x2000u);
    router.BindFallback(&evf);router.Register(obstacle.identity,obstacle);
    router.Enter(evf.identity,7u);router.Enter(obstacle.identity,9u);router.Leave(obstacle.identity);
    Check(evf.calls.size()==1u && evf.calls[0][1]==7u && obstacle.calls.size()==2u &&
        obstacle.calls[0][1]==9u && obstacle.calls[1][2]==1u);
    Reject([&]{router.Register(obstacle.identity,other);});
    Reject([&]{router.Unregister(obstacle.identity,other);});
    router.Unregister(obstacle.identity,obstacle);Check(router.Count()==0u);
    Reject([&]{router.Enter(obstacle.identity,0u);}); // Retired helper cannot reach its former owner.
    router.Register(obstacle.identity,other);router.Enter(other.identity,3u);
    Check(obstacle.calls.size()==2u && other.calls.size()==1u); // Explicit replacement generation.
    router.Unregister(other.identity,other);router.BindFallback(nullptr);
    Reject([&]{router.Enter(evf.identity,0u);});Reject([&]{router.Leave(other.identity);});
    Reject([&]{router.BindFallback(&router);});Reject([&]{router.Register(0u,evf);});
    std::cout<<"native_collision_event_router_tests passed\n";
}
