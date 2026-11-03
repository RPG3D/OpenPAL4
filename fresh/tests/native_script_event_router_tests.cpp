#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_final_hit_events.h"
#include "pal4/final_hit_task.h"
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct FinalHitRuntime final : pal4::IFinalHitTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t ReadWord(std::uint32_t a) override { return words.at(a); }
    void WriteWord(std::uint32_t a,std::uint32_t v) override { words[a]=v; }
    std::uint8_t ReadByte(std::uint32_t a) override { return bytes[a]; }
    void WriteByte(std::uint32_t a,std::uint8_t v) override { bytes[a]=v; }
    std::string_view ReadText(std::uint32_t) override { throw std::logic_error("unused"); }
    std::uint32_t CombatSystem() override { throw std::logic_error("unused"); }
    std::uint32_t CombatCamera(std::uint32_t) override { throw std::logic_error("unused"); }
    pal4::DeadEffectHandle CreateEffect(std::uint32_t,std::string_view,std::uint32_t,std::uint32_t,std::uint32_t) override { throw std::logic_error("unused"); }
    void ResetTemporaryEffect(pal4::DeadEffectHandle&) override { throw std::logic_error("unused"); }
    void ResetOwnedEffect(std::uint32_t) override { throw std::logic_error("unused"); }
    std::uint32_t CreateText(std::string_view) override { throw std::logic_error("unused"); }
    void ReleaseText(std::uint32_t) override { throw std::logic_error("unused"); }
    void SetProperties(std::uint32_t,std::uint32_t,std::uint32_t) override { throw std::logic_error("unused"); }
    std::uint32_t ChildPosition(std::uint32_t) override { throw std::logic_error("unused"); }
    void UpdatePosition(std::uint32_t,std::uint32_t,std::uint8_t) override { throw std::logic_error("unused"); }
    void FinalizeEffect(std::uint32_t) override { throw std::logic_error("unused"); }
    bool NeedsUpdate(std::uint32_t) override { throw std::logic_error("unused"); }
    void Free(std::uint32_t) override { throw std::logic_error("unused"); }
    std::uint32_t DestroyBase(std::uint32_t) override { throw std::logic_error("unused"); }
};
void FinalHitEvents(pal4::NativeScriptEventRouter& router) {
    constexpr std::uint32_t task=0x1000u, event=0x2000u;
    FinalHitRuntime runtime;
    runtime.words={{task+80u,42u},{task+84u,99u},{event,42u},{event+4u,98u}};
    {
        pal4::NativeCombatFinalHitEvents subscription(router,runtime,task);
        subscription.Subscribe(); subscription.Subscribe();
        Check(subscription.Active() && router.SubscriptionCount()==1u,"FinalHit replaces old subscription");
        (void)router.Dispatch("effect_end",event);
        Check(runtime.bytes[task+88u]==0u,"different effect handle does not complete task");
        runtime.words[event+4u]=99u;
        (void)router.Dispatch("effect_end",event);
        Check(runtime.bytes[task+88u]==1u,"matching effect handle completes task");
        subscription.Clear(); subscription.Clear();
        Check(!subscription.Active() && !router.SubscriptionCount(),"FinalHit clear is idempotent");
        subscription.Subscribe();
    }
    Check(!router.SubscriptionCount(),"FinalHit destructor retires callback");
}
}
int main() {
    try {
        pal4::NativeScriptEventRouter router;
        const auto key = router.Key("combat_prepare");
        Check(router.Key("combat_prepare") == key && router.Key("Combat_prepare") != key,
              "exact case-sensitive interned event names");
        std::vector<int> calls;
        const auto callback = [&](int value) {
            return [&, value](std::uint32_t payload) {
                Check(payload == 712u, "native payload forwarded unchanged"); calls.push_back(value);
            };
        };
        const auto older = router.Subscribe(key, 2u, callback(20));
        const auto newer = router.Subscribe(key, 2u, callback(21));
        const auto last = router.Subscribe(key, 3u, callback(30));
        const auto first = router.Subscribe(key, 1u, callback(10));
        Check(router.Dispatch("combat_prepare", 712u) == 4u &&
                  calls == std::vector<int>{10,21,20,30},
              "original ascending modes and newest-first equal modes");
        calls.clear(); router.Pause(true);
        Check(!router.Dispatch(key, 712u) && calls.empty(), "paused dispatch gate");
        router.Pause(false);
        Check(!router.Dispatch("missing", 712u), "missing event dispatch");
        Check(router.Unsubscribe(newer) && !router.Unsubscribe(newer) && !router.Contains(newer),
              "targeted removal is idempotent");
        Check(router.Dispatch(key,712u) == 3u && calls == std::vector<int>{10,20,30},
              "removal preserves unrelated callbacks");
        router.Clear(); calls.clear();
        Check(!router.SubscriptionCount() && !router.Contains(first) && !router.Contains(last),
              "clear retires all active subscriptions");
        std::uint32_t self{}, victim{}; bool added{};
        self = router.Subscribe(key, 1u, [&](std::uint32_t) {
            calls.push_back(1);
            Check(router.Unsubscribe(self) && router.Unsubscribe(victim), "safe live removal");
            if (!added) {
                added = true;
                (void)router.Subscribe(key, 1u, [&](std::uint32_t) { calls.push_back(11); });
                (void)router.Subscribe(key, 3u, [&](std::uint32_t) { calls.push_back(3); });
            }
        });
        victim = router.Subscribe(key, 2u, [&](std::uint32_t) { calls.push_back(2); });
        Check(self != older && router.Dispatch(key,0u) == 2u && calls == std::vector<int>{1,3},
              "live traversal sees later-mode additions and skips earlier equal-mode insertions/removals");
        calls.clear();
        Check(router.Dispatch(key,0u) == 2u && calls == std::vector<int>{11,3},
              "deferred node cleanup preserves new callbacks");
        router.Clear(); calls.clear();
        const auto other = router.Key("combat_reset");
        (void)router.Subscribe(other, 2u, [&](std::uint32_t) { calls.push_back(2); });
        (void)router.Subscribe(key, 1u, [&](std::uint32_t) {
            calls.push_back(1); Check(router.Dispatch(other,0u) == 1u, "nested dispatch");
        });
        Check(router.Dispatch(key,0u) == 1u && calls == std::vector<int>{1,2}, "nested event ordering");
        router.Clear();
        std::uint32_t failure{};
        failure = router.Subscribe(key, 1u, [&](std::uint32_t) {
            (void)router.Unsubscribe(failure); throw std::runtime_error("intentional callback failure");
        });
        bool failed{};
        try { (void)router.Dispatch(key,0u); } catch (const std::runtime_error&) { failed=true; }
        Check(failed && !router.SubscriptionCount(), "exception unwinds active dispatch and removed callbacks");
        auto owner = std::make_shared<int>(1);
        std::weak_ptr<int> weak = owner;
        const auto owned = router.Subscribe(key,1u,[owner](std::uint32_t) {});
        owner.reset(); Check(!weak.expired(), "router retains callback captures");
        Check(router.Unsubscribe(owned) && weak.expired(), "unsubscription releases callback ownership");
        (void)router.Subscribe(key,1u,[&](std::uint32_t) { router.Clear(); });
        (void)router.Subscribe(key,2u,[&](std::uint32_t) { throw std::runtime_error("cleared callback ran"); });
        Check(!router.Dispatch(key,0u) && !router.SubscriptionCount(), "clear during live dispatch");
        FinalHitEvents(router);
        std::cout << "Native event router: original mode order, pause, payload, targeted removal, live mutation, "
                     "nested calls, exception unwind and callback ownership verified\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
