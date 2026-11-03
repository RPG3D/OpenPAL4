#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/combat_actor_status.h"
#include "pal4/script_skill_context.h"
#include "pal4/native_animation_event_queue_runtime.h"
#include "pal4/effect_handle.h"
#include <bit>
#include <limits>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
std::uint32_t& Word(std::uint32_t a,std::uint32_t offset=0u) {
    return *reinterpret_cast<std::uint32_t*>(a+offset);
}
std::string Text(std::uint32_t a) {
    return Word(a+4u) ? std::string(reinterpret_cast<char*>(Word(a+4u)),Word(a+8u)) : std::string{};
}
template<class F> void Reject(F&& f) {
    bool rejected=false;
    try { f(); } catch(const std::logic_error&) { rejected=true; }
    Check(rejected,"unsupported or invalid operation must reject");
}

// Protocol fixture: inactive groups must short-circuit Release before tree,
// Dialog, pool or notification access; it does not implement those services.
class StoppedGroup final : public pal4::IEffectHandleReleaseRuntime {
public:
    static constexpr std::uint32_t group=0x5000u;
    bool active{};unsigned checks{};
    [[noreturn]] static void Unexpected(){throw std::runtime_error("inactive effect group reached live payload operations");}
    std::uint32_t InitializeSystem()override{return 1u;}
    bool Validate(std::uint32_t system,std::uint32_t owner)override{Check(system==1u && owner==group,"retired effect group identity");++checks;return active;}
    std::uint32_t FindElement(std::uint32_t,const pal4::EffectSystemHandleWords&)override{Unexpected();}
    std::uint32_t ReadWord(std::uint32_t)override{Unexpected();}
    std::uint32_t RegisterSystem()override{return 1u;}
    bool ContainsOwner(std::uint32_t system,std::uint32_t owner)override{return Validate(system,owner);}
    std::string ReadNameAttribute(std::uint32_t)override{Unexpected();}
    void WriteWord(std::uint32_t,std::uint32_t)override{Unexpected();}
    void ResetDialogRuntimeState(std::uint32_t)override{Unexpected();}
    void DispatchOwnerRelease(std::uint32_t,pal4::EffectSystemHandleWords,std::uint32_t)override{Unexpected();}
    void ReturnEffect(std::uint32_t,std::uint32_t,std::uint32_t)override{Unexpected();}
    void EraseEntry(std::uint32_t,std::uint32_t)override{Unexpected();}
    void ProcessEffectList(std::uint32_t)override{Unexpected();}
};

// Deliberate service spies: verify the real native adapter's dispatch arguments
// and lifetime, without pretending these implement the game's property events.
class StatsServices final : public pal4::ICombatActorStatusRuntime,
                            public pal4::IScriptSkillContextRuntime {
public:
    pal4::NativeCombatActorOwner& owner;
    std::vector<std::uint32_t> calls;
    bool fail_skill{}, fail_health{}, check_reentry{};
    std::uint32_t status{99u};
    explicit StatsServices(pal4::NativeCombatActorOwner& actor) : owner(actor) {}
    std::uint32_t ReadWord(std::uint32_t a) override { return Word(a); }
    std::int32_t Query(std::uint32_t,std::uint32_t) override {
        throw std::logic_error("unexpected scalar query");
    }
    std::int32_t QueryIndexed(std::uint32_t a,std::uint32_t slot,std::uint32_t index) override {
        Check(a==owner.Address(),"health receiver is original actor");
        Check((slot==0x30u && index==0u)||(slot==0x34u && index==7u),"original health query slots");
        return static_cast<std::int32_t>(Word(a,slot==0x30u ? 344u : 572u));
    }
    std::int32_t Set(std::uint32_t a,std::uint32_t slot,std::int32_t value) override {
        Check(a==owner.Address() && slot==0xacu,"health setter must remain actor virtual AC");
        if (check_reentry) {
            Reject([&]{(void)owner.RebuildStats(*this,*this);});
            Reject([&]{owner.Destroy();});
        }
        if (fail_health) throw std::logic_error("injected missing health property service");
        status=static_cast<std::uint32_t>(value);
        return 100+value;
    }
    std::uint32_t InvokeVirtual(std::uint32_t a,std::uint32_t slot,
                                std::span<const std::uint32_t> args) override {
        Check(a==owner.Address(),"skill event receiver");
        Check(slot==280u||slot==308u||slot==284u||slot==312u,"skill virtual slot");
        if(slot==280u || slot==284u)
            Check(args.size()==4u && args[1]==1u && args[3]==a,"skill actor mode and target");
        else Check(args.size()==3u && args[2]==1u,"skill target arguments");
        calls.push_back(slot);
        return 0u;
    }
    std::uint32_t SkillSystemInstance() override { return 0x1234u; }
    void ProcessSkillChain(std::uint32_t system,std::uint32_t,
                           std::uint32_t a,std::uint32_t context,std::uint32_t target) override {
        Check(system==0x1234u && a==owner.Address() && target==a && context==a,"skill chain context");
        calls.push_back(999u);
        if(fail_skill) throw std::logic_error("injected unbound skill effect");
        Word(a,344u)=100u;
        Word(a,572u)=999u; // shared531450 must restore old HP after skills.
    }
};
void Stats(pal4::NativeCombatActorOwner& owner) {
    const auto a=owner.Address();
    std::array<std::uint32_t,292u> saved;
    std::memcpy(saved.data(),reinterpret_cast<void*>(a),sizeof(saved));
    StatsServices service(owner);
    const auto allocations=owner.OwnedAllocations();
    const auto texts=owner.OwnedTexts();
    const auto original_damage=Word(a,800u);
    Word(a,800u)=0x1234u;
    Reject([&]{(void)owner.RebuildStats(service,service);});
    Check(Word(a,800u)==0x1234u && owner.OwnedAllocations()==allocations,
          "foreign damage rejects before dereference or mutation");
    Word(a,800u)=original_damage;
    Word(a,8u)=100u; Word(a,16u)=30u; // BASIC HP/MP maxima, copied into +340.
    for (const auto [hp,state] : std::array<std::pair<std::uint32_t,std::uint32_t>,7>{{
        {150u,0u},{100u,0u},{40u,1u},{20u,2u},{1u,2u},{0u,3u},{0xffffffffu,3u}}}) {
        Word(a,572u)=hp; Word(a,576u)=150u; Word(a,580u)=99u;
        Word(a,344u)=999u;
        const auto previous=Word(a,800u);
        Check(owner.RebuildStats(service,service)==static_cast<int>(100u+state),"health virtual return propagated");
        Check(service.status==state,"shared HP status thresholds");
        Check(Word(a,572u)==(hp==0xffffffffu ? 0u : (hp>100u?100u:hp)),"signed HP clamp");
        Check(Word(a,576u)==100u && Word(a,580u)==30u,"validated stamina/MP clamp");
        Check(Word(a,344u)==100u && Word(a,800u)!=previous && Word(Word(a,800u))==0x845e70u,
              "BASIC refresh and real damage replacement");
        Check(owner.OwnedAllocations()==allocations && owner.OwnedTexts()==texts,"stable replacement ownership");
    }
    Word(a,576u)=0xffffffffu; Word(a,580u)=0xffffffffu;
    (void)owner.RebuildStats(service,service);
    Check(!Word(a,576u)&&!Word(a,580u),"negative stamina/MP clamp");
    std::array<std::uint32_t,3u> record{1u,0u,1u};
    const auto pointer=[](const auto& value){return reinterpret_cast<std::uint32_t>(value.data());};
    std::array<std::uint32_t,2u> fixed{pointer(record),a};
    std::array<std::uint32_t,3u> timed{pointer(record),a,123u};
    Word(a,696u)=pointer(fixed); Word(a,700u)=pointer(fixed)+sizeof(fixed);
    Word(a,680u)=pointer(timed); Word(a,684u)=pointer(timed)+sizeof(timed);
    Word(a,8u)=200u; Word(a,572u)=150u;
    (void)owner.RebuildStats(service,service);
    Check(service.calls==std::vector<std::uint32_t>{280,308,999,284,312,280,308,999,284,312},
          "both actual skill list layouts share original dispatch order");
    Check(Word(a,572u)==100u,"saved HP restored and clamped after skill changes maximum");
    service.fail_skill=true;
    Reject([&]{(void)owner.RebuildStats(service,service);});
    Check(owner.OwnedAllocations()==allocations,"skill failure retains valid damage ownership");
    Word(a,696u)=Word(a,700u)=Word(a,680u)=Word(a,684u)=0u;
    service.fail_skill=false; service.fail_health=true;
    Reject([&]{(void)owner.RebuildStats(service,service);});
    service.fail_health=false; service.check_reentry=true;
    (void)owner.RebuildStats(service,service);
    // Restore original BASIC/identity fields but keep the current owned damage.
    const auto damage=Word(a,800u);
    std::memcpy(reinterpret_cast<void*>(a),saved.data(),sizeof(saved));
    Word(a,800u)=damage;
    Check(owner.OwnedAllocations()==allocations,"repeated failures and retry retain five allocations");
}
// Protocol fixture only: no CEGUI rendering or game damage creation is claimed.
void Animations(pal4::NativeCombatActorOwner& owner) {
    class Activity final : public pal4::IAnimationEventQueueActivityRuntime {
    public:
        pal4::NativeCombatActorOwner& owner;
        unsigned advances{},deletes{},events{};
        bool finished{},fail{};
        explicit Activity(pal4::NativeCombatActorOwner& a):owner(a){}
        void Guards() {
            Reject([&]{owner.Destroy();});
            Reject([&]{owner.UnbindAnimationActivities();});
            Reject([&]{owner.ClearAnimations();});
            Reject([&]{owner.UpdateAnimations(0.0f);});
            Reject([&]{owner.DispatchAnimations(0u);});
            Reject([&]{owner.AddAnimation(1u,false);});
        }
        void DeleteAnimation(std::uint32_t) override { Guards();++deletes; }
        void AdvanceAnimation(std::uint32_t,float) override {
            Guards();if(fail)throw std::logic_error("fixture advance failure");++advances;
        }
        void CallAnimationEvent(std::uint32_t,std::uint32_t arg) override {
            Guards();Check(arg==77u,"original queue event argument");++events;
        }
        std::uint32_t AnimationFinished(std::uint32_t) override { return finished ? 1u:0u; }
    } activity(owner);
    const auto allocations=owner.OwnedAllocations();
    owner.UpdateAnimations(0.1f);owner.DispatchAnimations(77u);owner.ClearAnimations();
    Reject([&]{owner.UpdateAnimations(-0.1f);});
    Reject([&]{owner.UpdateAnimations(std::numeric_limits<float>::quiet_NaN());});
    Reject([&]{owner.AddAnimation(1u,false);});
    owner.BindAnimationActivities(activity);
    Reject([&]{owner.BindAnimationActivities(activity);});
    Reject([&]{owner.Destroy();});
    Reject([&]{owner.AddAnimation(0u,true);});
    std::array<std::uint32_t,18> first{},second{};
    const auto a=reinterpret_cast<std::uint32_t>(first.data());
    const auto b=reinterpret_cast<std::uint32_t>(second.data());
    Word(a,52u)=std::bit_cast<std::uint32_t>(-1.0f);
    owner.AddAnimation(a,true);
    owner.AddAnimation(b,false,true);
    Check(owner.AnimationCount()==2u && owner.OwnedAllocations()==allocations+2u,"queue uses actor allocator and real head");
    Check(std::bit_cast<float>(Word(b,52u))==0.2f,"shared stagger reads actual previous elapsed");
    Reject([&]{owner.UnbindAnimationActivities();});
    owner.DispatchAnimations(77u);Check(activity.events==2u,"queue dispatches owned and borrowed entries");
    owner.UpdateAnimations(0.2f);
    Check(activity.advances==1u && Word(b,52u)==0u && Word(b,48u)==0u,"delay hitting zero never advances this frame");
    owner.UpdateAnimations(0.1f);
    Check(activity.advances==3u && Word(b,52u)==0u && std::bit_cast<float>(Word(b,48u))==0.1f,"zero delay advances on the next frame after reaching zero");
    activity.fail=true;Reject([&]{owner.UpdateAnimations(0.1f);});activity.fail=false;
    owner.UpdateAnimations(0.1f);Check(activity.advances==5u,"advance failure releases reentry guards");
    activity.finished=true;owner.UpdateAnimations(0.1f);
    Check(!owner.AnimationCount() && activity.deletes==1u && owner.OwnedAllocations()==allocations,"finished owned entry deleted; borrowed retained; nodes freed");
    owner.AddAnimation(a,true);owner.ClearAnimations();
    Check(activity.deletes==2u && owner.OwnedAllocations()==allocations,"clear dispatches actual owned deletion");
    owner.UnbindAnimationActivities();owner.UpdateAnimations(0.1f);
}
}
int main(int argc,char** argv) {
    try {
        Check(argc==2,"original root required");
        std::array<pal4::NativeCombatActorOwner,3> actors;
        std::array<std::array<std::string,11>,3> texts;
        std::array<std::array<std::uint32_t,84>,3> properties;
        constexpr std::array ids{2579,2580,2580};
        {
            pal4::NativeGameDatabase db;
            Check(db.Load(argv[1]),"load original database");
            pal4::NativeMonsterRecordCatalog catalog(db);
            Reject([&]{ actors[0].Construct(catalog,-1); });
            Check(!actors[0].Address()&&!actors[0].OwnedAllocations(),"missing record leaves empty owner");
            for(std::size_t i=0; i<actors.size(); ++i) {
                const auto source=catalog.Find(ids[i]);
                const auto a=actors[i].Construct(catalog,ids[i]);
                Reject([&]{actors[i].Construct(catalog,ids[i]);});
                Check(Word(a)==0x844d08u && Word(a+716u)==0x844cf0u && Word(a+724u)==0x844ce8u,
                      "original derived vtable evidence");
                Check(Word(a+804u)==2u && Word(a+808u)==0u,"monster mode and detached child");
                Check(Word(a+960u)==static_cast<std::uint32_t>(ids[i]),"original monster identity");
                const auto damage=Word(a+800u), action=Word(a+1156u);
                Check(damage && Word(damage)==0x845e70u && Word(damage+8u)==0u,"actual damage object");
                Check(action && Word(action)==0x845898u && Word(action+4u)==a,"action retains owning actor");
                Check(actors[i].OwnedAllocations()==5u,"damage, queue head, nil, tree head, action ownership");
                const auto head=Word(a+840u);
                Check(Word(head)==head && Word(head+8u)==head && Word(a+848u)==0u,"actual empty effect tree");
                for(const auto off:{852u,860u,868u,876u,884u})
                    Check(Word(a+off)==0xffffffffu && Word(a+off+4u)==0u,"original effect handles");
                std::memcpy(properties[i].data(),reinterpret_cast<const void*>(source),336u);
                Check(std::memcmp(reinterpret_cast<void*>(a+4u),properties[i].data(),336u)==0 &&
                      std::memcmp(reinterpret_cast<void*>(a+340u),properties[i].data(),336u)==0,"both BASIC snapshots");
                for(std::size_t j=0;j<11u;++j) {
                    texts[i][j]=Text(source+408u+16u*j);
                    Check(Text(a+968u+16u*j)==texts[i][j],"all original texts copied");
                    if(!texts[i][j].empty()) Check(Word(a+972u+16u*j)!=Word(source+412u+16u*j),"source not borrowed");
                }
            }
            Check(Word(actors[1].Address()+972u)!=Word(actors[2].Address()+972u),"duplicate monster IDs have independent texts");
            for(auto& actor:actors) { Stats(actor);Animations(actor); }
            for(int repeat=0;repeat<8;++repeat) {
                pal4::NativeCombatActorOwner owner;
                owner.Construct(catalog,2580); owner.Destroy(); owner.Destroy();
                Check(!owner.OwnedAllocations()&&!owner.OwnedTexts(),"repeated explicit cleanup");
                owner.Construct(catalog,2579); owner.Destroy();
            }
        }
        // Both source owners are gone. Native actors retain independent data.
        for(std::size_t i=0;i<actors.size();++i) {
            const auto a=actors[i].Address();
            Check(std::memcmp(reinterpret_cast<void*>(a+4u),properties[i].data(),336u)==0,"properties survive source teardown");
            for(std::size_t j=0;j<11u;++j) Check(Text(a+968u+16u*j)==texts[i][j],"texts survive source teardown");
            const auto allocations=actors[i].OwnedAllocations();
            Word(a+808u)=0x1234u;
            Reject([&]{actors[i].Destroy();});
            Check(actors[i].Address()==a && actors[i].OwnedAllocations()==allocations && Word(a)==0x844d08u,
                  "populated unsupported child rejected before destruction");
            Word(a+808u)=0u;
            Word(a+856u)=0x1234u;
            Reject([&]{actors[i].Destroy();});
            Word(a+856u)=0u;
            if(i==0u){
                StoppedGroup effects;
                Word(a+868u)=42u;Word(a+872u)=StoppedGroup::group;
                effects.active=true;Reject([&]{actors[i].DestroyAfterEffectShutdown(StoppedGroup::group,effects);});
                Check(actors[i].Address()==a && Word(a)==0x844d08u && actors[i].OwnedAllocations()==allocations,"registered effect group rejects before destructor mutation");
                effects.active=false;Word(a+872u)=StoppedGroup::group+4u;
                Reject([&]{actors[i].DestroyAfterEffectShutdown(StoppedGroup::group,effects);});
                Word(a+872u)=StoppedGroup::group;Reject([&]{actors[i].Destroy();});
                Check(Word(a)==0x844d08u && actors[i].OwnedAllocations()==allocations,"foreign group rejects and failed shutdown binding does not escape into default destruction");
                actors[i].DestroyAfterEffectShutdown(StoppedGroup::group,effects);
                Check(effects.checks>=4u,"shared original destructor calls inactive-group Release without payload access");
            }else actors[i].Destroy();
            actors[i].Destroy();
            Check(!actors[i].Address()&&!actors[i].OwnedAllocations()&&!actors[i].OwnedTexts(),"all ownership retired");
        }
        std::cout << "Original cave actors 2579/2580/2580: construction, native stats replacement/clamps and borrowed skill/status dispatch verified; production Combat remains unbound\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
