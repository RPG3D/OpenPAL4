#include "pal4/native_combat_timed_events.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
void Check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F> void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed,"expected rejection");}
}
int main(){try{
    std::array<std::uint32_t,130> combat{};
    const auto address=reinterpret_cast<std::uint32_t>(combat.data());
    auto& slot=combat[103];
    using Events=pal4::NativeCombatTimedEvents;
    Reject([&]{Events q(address,combat[102],[](auto,auto){});});
    Reject([&]{Events q(address,slot,{});});
    Check(!slot,"invalid construction leaves slot empty");
    std::vector<std::uint32_t> methods;
    Events* active{};bool throw_callback=false;
    Events q(address,slot,[&](auto owner,auto method){
        Check(owner==address,"callback owner preserved");
        Reject([&]{active->Close();});Reject([&]{active->Update(0.1f);});
        Reject([&]{active->ReleaseCallbacks();});
        if(throw_callback)throw std::runtime_error("callback failure");
        methods.push_back(method);
    });active=&q;
    Check(slot==q.Address() && !q.TimerCount() && !q.EventCount(),"dual lists published");
    Reject([&]{Events duplicate(address,slot,[](auto,auto){});});
    Reject([&]{q.Schedule("bad",-1.0f,address,1u);});
    Reject([&]{q.Repeat("bad",std::numeric_limits<float>::infinity(),address,1u);});
    Reject([&]{q.Update(std::numeric_limits<float>::quiet_NaN());});
    q.Schedule("close_system",1.0f,address,1u);q.Repeat("turn",1.0f,address,2u);
    q.Update(1.0f);Check(methods.empty(),"original strict threshold retained");
    q.Update(0.01f);Check(methods==std::vector<std::uint32_t>{1u,2u},"events precede timers");
    q.Update(0.01f);Check(!q.EventCount(),"one shot deferred unlink");
    Check(q.CancelTimer("turn")==1u,"timer name cancellation");q.Update(0.01f);
    Check(!q.TimerCount(),"canceled timer retired");
    q.Schedule("failure",0.0f,address,3u);throw_callback=true;
    Reject([&]{q.Update(0.01f);});throw_callback=false;
    q.Update(0.01f);Check(methods.back()==3u,"callback failure leaves queue retryable");
    q.Update(0.01f);
    const auto empty_allocations=q.OutstandingAllocations();
    q.Schedule("pending",100.0f,address,4u);q.Repeat("pending",10.0f,address,5u);
    const auto before_release=q.OutstandingAllocations(),calls=methods.size();
    const auto original_address=q.Address();
    const auto old_slot=slot;slot=123u;Reject([&]{q.ReleaseCallbacks();});slot=old_slot;
    Check(q.OutstandingAllocations()==before_release && q.EventCount()==1u && q.TimerCount()==1u,
          "foreign publication rejects collection reset before ownership changes");
    q.ReleaseCallbacks();q.ReleaseCallbacks();
    Check(slot==original_address && q.Address()==original_address && q.EventCount()==1u && q.TimerCount()==1u &&
          q.OutstandingAllocations()+2u==before_release && methods.size()==calls,
          "Start reset deletes two owned callbacks, preserving nodes/counts and stable member412");
    q.Update(0.0f);Check(q.EventCount()==1u && q.TimerCount()==1u,"zero frame retains canceled nodes");
    q.Schedule("new_battle",0.0f,address,6u);
    q.Update(0.01f);
    Check(methods.size()==calls+1u && methods.back()==6u && q.EventCount()==1u && !q.TimerCount(),
          "new callback runs while canceled old battle nodes unlink without dispatch");
    q.Update(0.01f);
    Check(!q.EventCount() && q.OutstandingAllocations()==empty_allocations,
          "post-reset frame returns to original queue allocations");
    q.Schedule("pending_close",100.0f,address,7u);q.Repeat("pending_close",10.0f,address,8u);
    const auto published=slot;slot=123u;Reject([&]{q.Close();});slot=published;
    q.Close();q.Close();
    Check(!slot && !q.OutstandingAllocations() && !q.EventCount() && !q.TimerCount(),"all queued ownership retired");
    Reject([&]{q.Update(1.0f);});
    Reject([&]{q.ReleaseCallbacks();});
    {Events restart(address,slot,[](auto,auto){});restart.Schedule("pending",99.0f,address,1u);}
    Check(!slot,"destructor clears publication");
    std::cout<<"PASS: Combat member412 dual queues, Start callback release/deferred unlink, stable publication, ordering, retry, guards and retirement\n";
    return EXIT_SUCCESS;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}}
