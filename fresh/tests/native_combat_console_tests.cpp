#include "pal4/native_combat_console.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_animation_event_queue_runtime.h"
#include "pal4/combat_console_event_subscriptions.h"
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <functional>

namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
class Activity final : public pal4::IAnimationEventQueueActivityRuntime {
public:
    pal4::NativeBaseObjectMemory& memory;
    unsigned advanced{}, deleted{};
    std::function<void()> on_delete;
    explicit Activity(pal4::NativeBaseObjectMemory& m) : memory(m) {}
    void DeleteAnimation(std::uint32_t a) override { if(on_delete)on_delete(); (void)memory.Free(a); ++deleted; }
    void AdvanceAnimation(std::uint32_t, float delta) override { Check(delta==0.25f,"animation delta"); ++advanced; }
    void CallAnimationEvent(std::uint32_t, std::uint32_t) override { throw std::runtime_error("unexpected animation dispatch"); }
    std::uint32_t AnimationFinished(std::uint32_t) override { return 1u; }
};
}
int main() {
    try {
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeScriptEventRouter router;
        Activity activity(memory);
        const auto unrelated = router.Subscribe(router.Key("ae_attack_missed"), 1u, [](std::uint32_t) {});
        {
            pal4::NativeCombatConsole console(memory,router);
            Check(!console.Address(), "cold console has no address");
            const auto address = console.Construct();
            Check(address && memory.OutstandingAllocations()==1u &&
                  memory.ReadDword(address+8u)==0u && memory.ReadDword(address+60u)==0x3e4ccccdu,
                  "original252-byte console uses shared queue construction");
            bool rejected{};
            try { (void)console.Construct(); } catch (const std::logic_error&) { rejected=true; }
            Check(rejected, "duplicate console construction rejected");
            for (unsigned cycle=0;cycle<3;++cycle) {
                console.Subscribe();
                Check(console.SubscriptionCount()==23u && router.SubscriptionCount()==24u,
                      "repeat subscription replaces exactly23 console handlers");
            }
            rejected=false;
            try { (void)router.Dispatch("ae_attack_missed",71u); }
            catch (const std::logic_error&) { rejected=true; }
            Check(rejected && console.SubscriptionCount()==23u, "missing UI callback fails explicitly without losing subscriptions");
            std::vector<std::array<std::uint32_t,3>> calls;
            console.BindDispatch([&](std::uint32_t object,std::uint32_t entry,std::uint32_t payload) {
                calls.push_back({object,entry,payload});
            });
            Check(router.Dispatch("ae_physical_attack",71u)==1u && calls.empty(),
                  "original empty callback does not invent UI behavior");
            Check(router.Dispatch("ae_attack_missed",71u)==2u &&
                  calls==std::vector<std::array<std::uint32_t,3>>{{address,0x540020u,71u}},
                  "real named subscription forwards original owner/callback/payload");
            calls.clear();
            Check(router.Dispatch("ae_immunity_skill",72u)==1u && calls[0][1]==0x540270u,
                  "mode3 immunity event callback is routed");
            unsigned expected{};calls.clear();
            for (const auto& binding:pal4::CombatConsoleEventSubscriptions::Bindings()) {
                (void)router.Dispatch(binding.name,75u);
                if(binding.callback!=0x57b8d0u) ++expected;
            }
            Check(expected==17u && calls.size()==17u, "all nonempty original console bindings dispatch");
            console.BindAnimationActivity(&activity);
            const auto animation=memory.Allocate(64u);
            memory.WriteDword(animation+48u,0u);
            memory.WriteDword(animation+52u,std::bit_cast<std::uint32_t>(-1.0f));
            console.AddAnimation(animation,true,false);
            rejected=false;
            try { console.BindAnimationActivity(nullptr); } catch (const std::logic_error&) { rejected=true; }
            Check(rejected,"live animation owner cannot be replaced");
            console.Update(0.25f);
            Check(activity.advanced==1u && activity.deleted==1u && memory.OutstandingAllocations()==1u,
                  "shared native animation queue advances, finishes and releases owned animation");
            const auto immediate=memory.Allocate(64u),delayed=memory.Allocate(64u),borrowed=memory.Allocate(64u);
            console.AddAnimation(immediate,true,false);
            console.AddAnimation(delayed,true,true);
            console.AddAnimation(borrowed,false,false);
            unsigned guarded{};
            activity.on_delete=[&]{
                Check(console.SubscriptionCount()==23u,"subscriptions survive animation deletion");
                for(unsigned operation=0;operation<2;++operation){
                    bool nested_rejected{};
                    try {if(operation)console.Destroy();else console.Reset();}
                    catch(const std::logic_error&){nested_rejected=true;}
                    Check(nested_rejected,"nested reset/destruction rejected before mutation");++guarded;
                }
            };
            console.Reset();console.Reset();
            activity.on_delete={};
            Check(guarded==4u && activity.deleted==3u && memory.OutstandingAllocations()==2u,
                  "reset clears immediate/delayed owned animations and retains borrowed storage");
            (void)memory.Free(borrowed);
            Check(console.SubscriptionCount()==23u && router.SubscriptionCount()==24u && router.Contains(unrelated),
                  "console reset retains all subscriptions and unrelated event owner");
            console.Unsubscribe();
            Check(!console.SubscriptionCount() && router.SubscriptionCount()==1u,"explicit unsubscribe is separate from reset");
            console.Subscribe();console.Destroy();console.Destroy();
            Check(!console.Address() && memory.OutstandingAllocations()==0u && router.SubscriptionCount()==1u,
                  "repeated destruction releases queue and subscriptions exactly once");
            (void)console.Construct();console.Subscribe();
            const auto final_animation=memory.Allocate(64u);
            console.AddAnimation(final_animation,true,false);
        }
        Check(memory.OutstandingAllocations()==0u && activity.deleted==4u &&
                  router.SubscriptionCount()==1u && router.Contains(unrelated),
              "reconstruction and implicit destruction retire borrowed callbacks before storage");
        router.Clear();
        std::cout << "Native Combat console: shared construction, all23 subscriptions, explicit UI boundary, "
                     "real queue activity, replacement/reset and zero final allocations verified\n";
        return 0;
    } catch(const std::exception& error) { std::cerr << error.what() << '\n';return 1; }
}
