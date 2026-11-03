#include "native_combat_actor_exit_probe.h"
#include "pal4/native_combat_actor_exit_events.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_effect_scene_session.h"
#include <rwcore.h>
#include <rpworld.h>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
namespace pal4::test {
void VerifyNativeCombatActorExit(const std::filesystem::path& root,NativePlayerManagerSession& scene,
    const NativeGameDatabase& db,INativeSkillSystemServices& system,NativeEffectSceneSession& effects){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,"exit fixture expected scoped mutation rejection");};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    NativeMonsterRecordCatalog records(db);NativeSkillRecordCatalog catalog(db);NativePlayerCollectionCatalog passive(db);
    NativeCombatSystemImage image;NativeScriptEventRouter router;
    struct World{RpWorld* p{};~World(){if(p)RpWorldDestroy(p);}} world;
    RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
    world.p=RpWorldCreate(&bounds);check(world.p!=nullptr,"exit fixture owns actual SDK world");
    image.world.base.words[4]=reinterpret_cast<std::uint32_t>(world.p);image.world.base.words[12]=1u;
    std::array<std::uint32_t,100> effect_manager{};effect_manager[96]=effect_manager[98]=0xffffffffu;
    effects.BindCombatEffects(reinterpret_cast<std::uint32_t>(effect_manager.data()),reinterpret_cast<std::uint32_t>(&image),router);
    struct Effects{NativeEffectSceneSession& e;~Effects(){try{e.ClearCombatEffects();e.UnbindCombatEffects();}catch(...){}}} combat_effects{effects};
    NativeCombatPlayerManager players(scene,router,system);NativeCombatSkillActorRegistry registry(router,catalog,system);
    std::array<std::uint32_t,128> game{};
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){auto owner=std::make_unique<NativeActorInstanceOwner>();
        owner->Construct(reinterpret_cast<std::uint32_t>(game.data()));owner->Load(root,name);return owner;});
    NativeCombatMonsterManager monsters(pool,records,catalog,router,system,registry);
    const auto a=monsters.CreateMonster(2579,6u),b=monsters.CreateMonster(2580,6u),reserve=monsters.CreateMonster(2580,6u);
    monsters.InitializeMonster(a,0u,passive,effects);monsters.InitializeMonster(b,1u,passive,effects);
    monsters.Skills(reserve).SetFacetState(0u,false);
    NativeCombatRosterTargets targets(reinterpret_cast<std::uint32_t>(&image),players,monsters,effects,router);
    std::unique_ptr<NativeCombatActorExitEvents> exits;
    std::vector<std::uint32_t> observed_non_exit,outcomes,tokens;
    // Isolated composition: only the exit consumer is executed. Other registered
    // UI/coordinator callbacks are recorded, not accepted as battle verification.
    NativeCombatEventHandlers handlers(router,image.ActorHandlersSlot(),image.ExtendedHandlersSlot(),
        [&](std::uint32_t owner,std::uint32_t entry,std::uint32_t payload){
            if(entry==0x57b710u){check(exits!=nullptr,"actual Fled consumer is installed");exits->HandleFled(owner,payload);}
            else observed_non_exit.push_back(entry);
        });
    exits=std::make_unique<NativeCombatActorExitEvents>(image,handlers,registry,players,monsters,targets,effects,passive,router,
        [](std::uint32_t){throw std::logic_error("Fled fixture must not record Monster death progress");});
    const auto close=[&]{for(auto t:tokens)(void)router.Unsubscribe(t);tokens.clear();exits->Close();handlers.Close();
        targets.Close();monsters.ClearActors(effects);monsters.Close();players.Close();pool.Clear();
        effects.ClearCombatEffects();effects.UnbindCombatEffects();};
    try{
        const std::array event{a+716u};const auto payload=reinterpret_cast<std::uint32_t>(event.data());
        reject([&]{exits->HandleFled(handlers.AllAddress(),payload);});
        reject([&]{(void)monsters.RetireActorDuringExit(a,effects);});
        reject([&]{monsters.InitializeReplacementDuringExit(a,reserve,0u,passive,effects);});
        const auto cursor=monsters.CreateStageIterator();
        reject([&]{monsters.Skills(a).Fled();});monsters.DeleteStageIterator(cursor);
        check(monsters.StageCount()==2u && monsters.ModelOwner(a).Slot()==0u && !monsters.Skills(a).ExitDispatchActive(),
            "live stage cursor prevents actual532 retirement and releases source dispatch permit");
        bool fail_leave=true;unsigned leave_guards{},join_guards{};
        tokens.push_back(router.Subscribe(router.Key("combat_over"),1u,[&](std::uint32_t p){outcomes.push_back(word(p));}));
        tokens.push_back(router.Subscribe(router.Key("me_leave_stage"),1u,[&](std::uint32_t){
            reject([&]{exits->Close();});reject([&]{monsters.Close();});
            reject([&]{(void)monsters.CreateMonster(2579,0u);});++leave_guards;
            if(fail_leave)throw std::runtime_error("intentional Fled leave observer failure");
        }));
        tokens.push_back(router.Subscribe(router.Key("me_join_stage"),1u,[&](std::uint32_t p){
            check(word(p)==reserve+716u,"replacement joins actual reserve actor");
            reject([&]{monsters.InitializeReplacementDuringExit(a,reserve,0u,passive,effects);});++join_guards;
        }));
        bool failed{};try{monsters.Skills(a).Fled();}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional Fled leave observer failure";}
        check(failed && !monsters.ActiveIterators() && !monsters.Skills(a).ExitDispatchActive() && monsters.ModelOwner(a).Slot()==0u,
            "leave observer failure unwinds first-roster pins and exact source permit");
        fail_leave=false;monsters.Skills(a).Fled();
        check(monsters.ModelOwner(a).Slot()==6u && monsters.ModelOwner(reserve).Slot()==0u && word(reserve+828u)==1u &&
            monsters.StageCount()==2u && outcomes.empty() && join_guards==1u,
            "actual532 selects reserve through shared57B420, retires source and initializes original slot");
        monsters.Skills(b).Fled();
        check(monsters.ModelOwner(b).Slot()==6u && monsters.StageCount()==1u && outcomes.empty(),
            "non-last Fled without reserve retires source and continues battle");
        monsters.Skills(reserve).Fled();
        check(monsters.ModelOwner(reserve).Slot()==6u && !monsters.StageCount() && outcomes==std::vector<std::uint32_t>{1u} &&
            monsters.Count()==3u && registry.Count()==3u && !monsters.ActiveIterators() && !targets.ActiveIterators() && leave_guards>=3u,
            "last Monster Fled emits original outcome1 and retains all actor owners until synchronous532 unwinds");
        close();check(!registry.Count() && !monsters.Count() && !pool.CachedCount(),"exit fixture retires actual resource/registry owners");
        std::cout<<"Native Monster exit probe: actual532/57B710, original2579/2580 resources, reserve replacement, non-last/last, outcome1, stage-cursor rejection, leave failure/retry and retained owners pass; UI/coordinator observations="<<observed_non_exit.size()<<" (not full battle)\n";
    }catch(...){try{close();}catch(...){}throw;}
}
}
