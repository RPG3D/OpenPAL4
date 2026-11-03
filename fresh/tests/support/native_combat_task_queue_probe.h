#pragma once
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_task_domain.h"
#include "pal4/native_combat_start_request.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include <functional>
#include <array>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
// Explicit protocol fixture, not an attack implementation. Opaque task IDs
// prove the queue calls owners instead of dereferencing assumed death layouts.
inline void VerifyNativeCombatTaskQueueDomains(){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    struct Task {std::uint32_t identity{},next{};bool ready{},enabled{true},processed{},prepared{};};
    struct Domain final : INativeCombatTaskDomain {
        std::map<std::uint32_t,Task> tasks;
        std::vector<std::string>& trace;
        std::function<void()> on_prepare;
        std::function<void()> on_wait;
        std::function<void(std::uint32_t)> on_update;
        std::function<void(std::uint32_t)> on_cancel;
        explicit Domain(std::vector<std::string>& t):trace(t){}
        bool Owns(std::uint32_t t) const noexcept override{return tasks.contains(t);}
        bool Ready(std::uint32_t t) const override{return tasks.at(t).ready;}
        bool Enabled(std::uint32_t t) const override{return tasks.at(t).enabled;}
        void SetEnabled(std::uint32_t t,bool b) override{tasks.at(t).enabled=b;}
        std::uint32_t Identity(std::uint32_t t) const override{return tasks.at(t).identity;}
        bool Processed(std::uint32_t t) const override{return tasks.at(t).processed;}
        void Record(const char* op,std::uint32_t t){trace.push_back(std::string(op)+std::to_string(t));}
        void Prepare(std::uint32_t t) override{if(on_prepare)on_prepare();tasks.at(t).prepared=true;Record("prepare",t);}
        void BeginWaiting(std::uint32_t t) override{
            if(on_wait)on_wait();
            if(!tasks.at(t).prepared)throw std::logic_error("fixture task not prepared");Record("wait",t);
        }
        void Update(std::uint32_t t,float) override{if(on_update)on_update(t);tasks.at(t).ready=true;Record("update",t);}
        void Stop(std::uint32_t t) override{(void)tasks.at(t);Record("stop",t);}
        std::uint32_t ResultEvent(std::uint32_t t) override{Record("result",t);return tasks.at(t).next;}
        void Cancel(std::uint32_t t) override{if(on_cancel)on_cancel(t);if(!tasks.erase(t))throw std::logic_error("fixture foreign task");Record("cancel",t);}
    };
    RandomNumberGenerator random;NativePlayerCombatEvents events;NativeSkillSystemOwner skills(random,events);
    (void)skills.Construct("H:/PAL4/objs");
    std::vector<std::string> trace;Domain first(trace),second(trace);
    {
        NativeCombatTaskQueue empty(skills);
        check(!empty.Count() && !empty.DomainCount(),"Start queue exists before any actor domain");
        empty.ResetForStart();empty.Update(0.01f);
        NativeCombatSystemImage image;
        auto* bytes=reinterpret_cast<std::uint8_t*>(&image);
        auto word=[&](unsigned offset)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(bytes+offset);};
        bytes[501]=0xabu;bytes[502]=0u;bytes[517]=1u;bytes[518]=9u;bytes[519]=8u;
        word(388)=123u;word(392)=456u;word(504)=3u;word(508)=4u;word(512)=789u;
        empty.BindActions(first);first.tasks.emplace(0x99000u,Task{99u});empty.Register(0x99000u);
        first.on_cancel=[&](std::uint32_t task){
            check(!first.tasks.at(task).enabled,"Start disables tasks before cancelling owners");
            check(word(508)==4u && word(512)==789u && bytes[519]==8u,
                  "Start retires task owners before mutating per-request fields");
            throw std::logic_error("intentional Start task cancellation failure");
        };
        bool failed=false;
        try{InitializeNativeCombatStartRequest(image,empty,skills,0u);}catch(const std::logic_error&){failed=true;}
        check(failed && empty.Count()==1u && word(504)==3u && bytes[501]==0xabu,
              "failed task cleanup retains task ownership and prior request fields");
        first.on_cancel=[&](std::uint32_t task){
            check(!first.tasks.at(task).enabled && word(508)==4u && word(512)==789u,
                  "Start retry finishes task retirement before fields");
        };
        InitializeNativeCombatStartRequest(image,empty,skills,0x101u);first.on_cancel={};
        check(!empty.Count() && empty.DomainCount()==1u && first.tasks.empty(),"Start drains tasks without retiring domain services");
        check(word(388)==0u && word(392)==0u && word(504)==1u && word(508)==7u && word(512)==0u &&
              bytes[501]==0u && bytes[502]==0u && bytes[516]==1u && bytes[517]==1u && bytes[518]==0u && bytes[519]==0xabu,
              "Start field commit saves old flag, clears nonpreserved flag, truncates mode and preserves cold byte");
        bytes[501]=0x66u;bytes[502]=1u;word(512)=123u;
        InitializeNativeCombatStartRequest(image,empty,skills,0u);
        check(bytes[501]==0x66u && bytes[502]==1u && bytes[519]==0x66u && bytes[516]==0u && word(512)==0u,
              "repeat Start respects preserve flag and resets prior elapsed time");
        empty.DetachActions();trace.clear();
        std::cout<<"Native combat Start request: pre-actor queue, task cancellation failure/retry, flag preservation and repeat reset pass\n";
    }
    unsigned notifications{};
    NativeCombatTaskQueue queue(first,skills,[&](std::uint32_t actor){check(actor==7u,"incoming identity notification");++notifications;});
    queue.BindActions(second);check(queue.DomainCount()==2u,"two different task domains bind");
    const auto reject=[&](auto&& f,const char* why){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,why);};
    reject([&]{queue.BindActions(first);},"duplicate task domain rejected");
    Domain third(trace);
    const std::array<INativeCombatTaskDomain*,2> existing_batch{&third,&first},duplicate_batch{&third,&third},null_batch{&third,nullptr};
    reject([&]{queue.BindActions(existing_batch);},"batch with existing owner rejected before publication");
    reject([&]{queue.BindActions(duplicate_batch);},"duplicate within batch rejected");
    reject([&]{queue.BindActions(null_batch);},"null owner in batch rejected");
    check(queue.DomainCount()==2u && !queue.UsesActions(third),"invalid batches preserve all prior domain bindings");
    check(!queue.Register(0u) && !queue.Count(),"null registration preserves queue");
    first.tasks.emplace(0x10000u,Task{7u,0u,false,true,true});
    second.tasks.emplace(0x20000u,Task{7u,0x10080u});
    first.tasks.emplace(0x10080u,Task{8u});
    check(queue.Register(0x10000u)==0x10000u,"processed task registration");
    reject([&]{queue.Register(0x10000u);},"duplicate task registration rejected");
    reject([&]{queue.Register(0x90000u);},"foreign task registration rejected");
    reject([&]{queue.DetachActions();},"domain lifetime protected while queue live");
    check(queue.Register(0x20000u)==0x20000u && notifications==1u && !first.tasks.at(0x10000u).enabled,
        "cross-domain replacement notifies identity and disables old task");
    trace.clear();queue.Update(0.01f);
    const std::vector<std::string> expected{"update131072","cancel65536","stop131072","result131072",
        "cancel131072","prepare65664","wait65664"};
    check(trace==expected && queue.Count()==1u && first.tasks.at(0x10080u).prepared,
        "domain-specific stop/result and cross-domain continuation preserve original order");
    queue.Update(0.01f);check(!queue.Count() && first.tasks.empty() && second.tasks.empty(),"continuation drains all owners");
    first.tasks.emplace(0x10600u,Task{14u});second.tasks.emplace(0x20600u,Task{15u});
    queue.Register(0x10600u);trace.clear();
    first.on_update=[&](std::uint32_t t){
        check(t==0x10600u && queue.IsDispatching(),"reaction registration originates in actual child update");
        reject([&]{queue.Update(.01f);},"child cannot recursively update queue");
        reject([&]{queue.Clear();},"child cannot delete borrowed queue nodes");
        check(queue.Register(0x20600u)==0x20600u,"child synchronously registers independently owned reaction");
    };
    second.on_prepare=[&]{reject([&]{queue.Register(0u);},"reaction Prepare cannot recursively register");};
    queue.Update(.01f);first.on_update={};second.on_prepare={};
    const std::vector<std::string> reaction_expected{"prepare132608","wait132608","update67072","update132608",
        "stop67072","result67072","cancel67072","stop132608","result132608","cancel132608"};
    check(trace==reaction_expected && !queue.Count() && first.tasks.empty() && second.tasks.empty(),
        "synchronous reaction is updated in same live-list pass and both domains drain in original order");
    first.tasks.emplace(0x10100u,Task{9u});
    first.on_prepare=[&]{reject([&]{queue.Clear();},"task callbacks cannot reenter queue");};
    queue.Register(0x10100u);first.on_prepare={};
    queue.ResetFlags();check(!first.tasks.at(0x10100u).enabled,"ResetFlags dispatches to task owner");
    queue.Clear();check(first.tasks.empty(),"cancelled task cleanup uses owner");
    first.tasks.emplace(0x10200u,Task{10u});
    first.on_prepare=[] {throw std::logic_error("intentional task preparation failure");};
    reject([&]{queue.Register(0x10200u);},"prepare failure propagates");
    check(!queue.Count() && first.Owns(0x10200u),"failed registration leaves task with caller owner");
    first.on_prepare={};first.Cancel(0x10200u);
    // ResultEvent transfers a cross-domain successor to Update itself, unlike
    // public Register whose failing task remains with its caller.
    for(bool wait_failure:{false,true}){
        first.tasks.emplace(0x10500u,Task{12u,0x20500u});
        second.tasks.emplace(0x20500u,Task{13u});queue.Register(0x10500u);
        auto fail=[] {throw std::logic_error("intentional successor preparation/wait failure");};
        if(wait_failure)second.on_wait=fail;else second.on_prepare=fail;
        trace.clear();reject([&]{queue.Update(.01f);},"successor failure propagates");
        check(!queue.Count() && !queue.PendingSuccessorCount() && first.tasks.empty() && second.tasks.empty(),
              "failed successor registration retires predecessor and unpublished successor");
        check(trace.front()=="update66816" && trace.back()=="cancel132352",
              "failure cleanup preserves original predecessor-before-successor order");
        second.on_wait={};second.on_prepare={};
        reject([&]{queue.Update(.01f);},"failed update requires explicit retirement before reuse");
        queue.Clear();
    }
    first.tasks.emplace(0x10500u,Task{12u,0x20500u});
    second.tasks.emplace(0x20500u,Task{13u});queue.Register(0x10500u);
    second.on_prepare=[] {throw std::logic_error("intentional successor preparation failure");};
    second.on_cancel=[](auto){throw std::logic_error("intentional successor cancellation failure");};
    reject([&]{queue.Update(.01f);},"failed successor cancellation propagates");
    check(!queue.Count() && queue.PendingSuccessorCount()==1u && second.Owns(0x20500u),
          "throwing cancellation retains unpublished successor ownership");
    reject([&]{queue.Update(.01f);},"pending cancellation blocks update retry");
    reject([&]{queue.Register(0u);},"pending cancellation blocks registration");
    reject([&]{queue.DetachActions();},"pending successor protects domain lifetime");
    reject([&]{queue.BindActions(third);},"pending successor blocks new domain binding");
    reject([&]{queue.Clear();},"repeated cancellation failure retains lease");
    check(queue.PendingSuccessorCount()==1u && second.Owns(0x20500u),"failed Clear remains retryable");
    second.on_cancel={};second.on_prepare={};queue.Clear();
    check(!queue.PendingSuccessorCount() && second.tasks.empty(),"Clear retries pending successor cancellation");
    // Result is already transferred when predecessor cancellation throws. Do
    // not resume its ResultEvent; retire both owners through Clear instead.
    first.tasks.emplace(0x10500u,Task{12u,0x20500u});
    second.tasks.emplace(0x20500u,Task{13u});queue.Register(0x10500u);
    first.on_cancel=[](auto){throw std::logic_error("intentional predecessor cancellation failure");};
    reject([&]{queue.Update(.01f);},"predecessor cancellation failure propagates");
    check(queue.Count()==1u && !queue.PendingSuccessorCount() && second.tasks.empty(),
          "predecessor cancellation failure still retires unpublished successor");
    reject([&]{queue.Update(.01f);},"transferred predecessor result cannot be requested again");
    first.on_cancel={};queue.Clear();
    check(first.tasks.empty(),"predecessor cleanup remains retryable");
    std::cout<<"Native combat successor ownership: cross-domain prepare/wait failure, cancellation failure/retry and domain lifetime pass\n";
    queue.DetachActions();check(!queue.DomainCount(),"empty queue detaches domain borrowers");
    const std::array<INativeCombatTaskDomain*,2> fresh_batch{&first,&second};
    queue.BindActions(fresh_batch);
    check(queue.DomainCount()==2u && queue.UsesActions(first) && queue.UsesActions(second),"valid batch publishes all owners");
    queue.DetachActions();
    // Missing actor480 is an explicit failure, not a death-derived no-op.
    NativeCombatTaskQueue unbound(first,skills);first.tasks.emplace(0x10300u,Task{11u,0u,false,true,true});
    first.tasks.emplace(0x10400u,Task{11u});unbound.Register(0x10300u);
    reject([&]{unbound.Register(0x10400u);},"missing actor480 service rejected");
    check(unbound.Count()==1u && first.tasks.at(0x10300u).enabled,"unbound identity callback preserves old task flag");
    unbound.Clear();first.Cancel(0x10400u);unbound.DetachActions();skills.Destroy();
    std::cout<<"Native combat task domains: cross-domain replacement/result, processed identity callback, opaque handles, reentry/failure and cleanup pass\n";
}
}
