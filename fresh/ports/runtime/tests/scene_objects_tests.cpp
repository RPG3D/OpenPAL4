#include "pal4/portable/scene_objects.h"
#include "pal4/portable/scene_interaction.h"
#include "pal4/portable/animation_table.h"
#include "pal4/portable/atomic_distance.h"
#include "pal4/portable/scene_events.h"
#include "pal4/portable/game_services.h"
#include <bit>
#include <source_location>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value,const std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("scene object contract failed at line "+std::to_string(at.line()));}
std::vector<std::uint8_t> Bytes(std::string_view text){return {text.begin(),text.end()};}
ArenaResources Fixture(int plays=0,bool hold=false,bool default_play=true) {
    ArenaResources arena;arena.description.world="Q01";arena.description.scene="N01";
    SceneGob gob;gob.name="entry";gob.type=6;gob.model="TEST";gob.resource_directory="gamedata\\PALObject\\TEST\\";gob.flags[0]=1;
    pal4::BinaryXmlNode root;root.name="PAL4-GameObject-Parameters";
    root.attributes.push_back({"PAL4-GameObject-object-scale",2,2.f});
    root.attributes.push_back({"PAL4-GameObject-object-research-num",2,3.9f});
    root.attributes.push_back({"PAL4-GameObject-object-camera-collide",1,std::int32_t{1}});
    pal4::BinaryXmlNode action;action.name="PAL4_GameObject-action";
    action.attributes={{"PAL4_GameObject-action-default-play",1,std::int32_t(default_play)},
        {"PAL4_GameObject-action-play-times",1,std::int32_t(plays)},
        {"PAL4_GameObject-action-holding-end",1,std::int32_t(hold)}};
    root.children.push_back(action);const auto xml=pal4::BinaryXmlConfig::Serialize(root);Check(xml && gob.config.Parse(*xml));arena.catalog.gobs.push_back(std::move(gob));
    ArenaActor model;model.name="entry";model.gob_type=6;
    const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};model.actor.frames.push_back({-1,identity});model.actor.palettes.push_back({0,identity});
    model.actor.mesh.vertices.resize(1);model.actor.mesh.vertices[0].position[0]=1;model.actor.influences.push_back({{0,0,0,0},{1,0,0,0}});
    model.actor.clips.push_back({"A",2,{{0,{{0,{0,0,0,1},{10,0,0}},{2,{0,0,0,1},{12,0,0}}}}}});
    model.actor.clips.push_back({"Z",2,{{0,{{0,{0,0,0,1},{0,0,0}},{2,{0,0,0,1},{2,0,0}}}}}});
    arena.actors.push_back(std::move(model));return arena;
}
std::optional<std::vector<std::uint8_t>> Read(std::string_view directory,std::string_view member) {
    Check(directory=="gamedata\\PALObject\\TEST\\");if(member=="TEST.tab")return Bytes("// preserve original order\n{first$Z.anm}\n{second$A.anm}\n");return {};
}
ArenaResources CollisionFixture(unsigned mode=1){
    auto arena=Fixture(-1);arena.catalog.gobs[0].flags[1]=mode;
    auto& actor=arena.actors[0].actor;actor.atomic_spheres={{0,{0,0,0,1}},{0,{100,0,0,1}}};actor.mesh.draws.resize(1);
    auto& bindings=actor.render_bindings.emplace();bindings.materials={{255,255,255,255}};bindings.geometry_materials={{0},{0}};bindings.atomics={{0,1},{1,4}};bindings.draws={{1,0}};
    auto& geometry=arena.actors[0].collision.emplace();
    for(unsigned i=0;i<2;++i){const float x=i?100.f:0.f;ObjectCollisionAtomic atomic;atomic.frame=0;atomic.flags=i?4:1;
        atomic.triangles={{{Point3{x,-50,-50},Point3{x,50,-50},Point3{x,-50,50}},i,0,0},{{Point3{x,50,-50},Point3{x,50,50},Point3{x,-50,50}},i,0,1}};geometry.atomics.push_back(std::move(atomic));}
    return arena;
}
SceneMinimap Minimap(){return SceneMinimap("Q01","N01",[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});}
void InteractionController() {
    auto arena=CollisionFixture();auto& gob=arena.catalog.gobs[0];gob.type=0;gob.flags[2]=1;
    gob.flags[3]=std::bit_cast<unsigned>(20.f);gob.script="TakeBow";arena.actors[0].gob_type=0;
    SceneNpc npc;npc.name="NPC";npc.resource="TEST";npc.script="Talk";npc.flags={1,1,std::bit_cast<unsigned>(20.f)};
    arena.catalog.npcs.push_back(npc);auto model=arena.actors[0];model.name="NPC";model.gob_type.reset();
    arena.actors.push_back(std::move(model));
    arena.collision.floor.push_back({{Point3{0,0,10},Point3{-10,0,-10},Point3{10,0,-10}},0,0,0});
    arena.description.event_collision_cache=true;SceneEvent event;event.name="machine";event.effect_type=5;event.script="Rest";event.clump={1};
    event.properties.push_back({});arena.catalog.events.push_back(event);arena.event_collisions.emplace_back(ObjectCollisionGeometry{});
    struct Scripts:ISceneEventScripts {
        std::vector<std::string> entries;
        bool Contains(std::string_view,std::string_view) override{return false;}
        void LoadWorld(std::string_view world) override{Check(world=="Q01");}
        std::int32_t Push(std::string_view world,std::string_view entry) override{Check(world=="Q01");entries.emplace_back(entry);return 0;}
    } scripts;
    struct Hints:pal4::IActorControlGameEventRuntime {
        unsigned windows{},shown{},sounds{};
        std::uint32_t CreateHintWindow() override{++windows;return 1;}
        std::optional<std::string> TriggerText(std::uint32_t key) override{Check(key==0x109);return "original hint";}
        void InitializeHintWindow(std::uint32_t window,std::string_view text,float x,float y,float opacity,std::uint32_t zero,std::uint32_t one) override{
            Check(window==1 && text=="original hint" && x==.5f && y==.15f && opacity==1 && zero==0 && one==1);++shown;
        }
        void PlayHintSound(std::string_view path,std::uint32_t repeat) override{Check(path==pal4::PalActorControlGameEvent::kSoundPath && repeat==1);++sounds;}
    } hints;
    auto map=Minimap();SceneObjects objects(arena,map,Read);
    SceneNpcs npcs(arena,[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
    SceneCollision collision;SceneEvents events(arena,collision,scripts);events.ActivateAll();
    SceneInteraction control(arena,npcs,objects,events,scripts,hints,[](auto,auto){return true;});
    auto step=[&](bool pressed,bool blocked=false,unsigned actor=0){return control.Advance(pressed,blocked,{0,0,0},1,1,actor);};
    Check(step(true) && scripts.entries==std::vector<std::string>{"void Talk()"});
    Check(step(true) && scripts.entries.size()==1);Check(step(false));
    Check(npcs.SetVisible("NPC",false));
    Check(step(true) && scripts.entries.back()=="void TakeBow()" && scripts.entries.size()==2);
    Check(step(false));
    Check(!step(true,true) && scripts.entries.size()==2);
    Check(!step(true) && scripts.entries.size()==2); // 模态关闭时仍按住，不产生新按下沿。
    (void)step(false);Check(objects.SetVisible("entry",false));
    const auto machine_handled=step(true);
    Check(machine_handled && scripts.entries.back()=="void Rest()" && scripts.entries.size()==3);
    Check(step(true) && scripts.entries.size()==3);Check(step(false));
    control.Reset();Check(!step(false));
    std::ostringstream report;control.WriteReport(report);Check(report.str().find("\"targets\":[0,0,0]")!=std::string::npos);
    // 错误角色只能显示原提示，不能绕过门控执行普通物件。
    pal4::BinaryXmlNode root;root.name="PAL4-GameObject-Parameters";
    root.attributes={{"PAL4-GameObject-object-validfor",1,std::int32_t{1}}};
    const auto config=pal4::BinaryXmlConfig::Serialize(root);Check(config && arena.catalog.gobs[0].config.Parse(*config));
    SceneObjects restricted(arena,map,Read);SceneInteraction gate(arena,npcs,restricted,events,scripts,hints,[](auto,auto){return true;});
    Check(gate.Advance(true,false,{0,0,0},1,1,0) && hints.shown==1 && hints.sounds==1 && scripts.entries.size()==3);
    (void)gate.Advance(false,false,{0,0,0},1,1,0);
    Check(gate.Advance(true,false,{0,0,0},1,1,1) && hints.shown==1 && scripts.entries.size()==4 && scripts.entries.back()=="void TakeBow()");
}

void ObjectConfigurationReset() {
    auto arena=CollisionFixture();auto& gob=arena.catalog.gobs[0];
    gob.type=0;gob.flags[2]=1;gob.flags[3]=std::bit_cast<unsigned>(20.f);
    gob.script="TakeBow";arena.actors[0].gob_type=0;
    auto minimap=Minimap();SceneObjects objects(arena,minimap,Read);
    struct Scripts final:ISceneEventScripts {
        bool Contains(std::string_view,std::string_view) override{return false;}
        void LoadWorld(std::string_view) override{}
        std::int32_t Push(std::string_view,std::string_view) override{return 0;}
    } scripts;
    Check(objects.States()[0].uses==3);
    Check(objects.Interact(0,scripts) && objects.States()[0].uses==2);
    Check(objects.Interact(0,scripts) && objects.States()[0].uses==1);
    Check(objects.Interact(0,scripts) && objects.States()[0].uses==0);
    Check(objects.ResetConfiguration("entry") && objects.States()[0].uses==3);
    Check(!objects.ResetConfiguration("missing"));

    auto get_item_arena=CollisionFixture();
    get_item_arena.catalog.gobs[0].type=3;
    get_item_arena.actors[0].gob_type=3;
    auto get_item_map=Minimap();SceneObjects get_item(get_item_arena,get_item_map,Read);
    Check(get_item.ResetConfiguration("entry"));

    auto action_arena=Fixture();auto action_map=Minimap();
    SceneObjects action(action_arena,action_map,Read);
    bool unsupported=false;
    try{(void)action.ResetConfiguration("entry");}
    catch(const std::logic_error&){unsupported=true;}
    Check(unsupported);

    std::ostringstream trace;GameServices game(trace);
    game.reset_object=[&](std::string_view name){(void)objects.ResetConfiguration(name);};
    ScriptGlobals globals{};ScriptStack stack(globals);
    stack.Compile("gob_reset","void Reset(){giGOBReset(\"entry\");}");
    Check(objects.Interact(0,scripts) && objects.States()[0].uses==2);
    Check(stack.Push("gob_reset","void Reset()",game).has_value());
    Check(stack.Tick(0)==ScriptState::finished && objects.States()[0].uses==3);
    ScriptStack missing_stack(globals);
    missing_stack.Compile("gob_reset_missing","void Reset(){giGOBReset(\"missing\");}");
    Check(missing_stack.Push("gob_reset_missing","void Reset()",game).has_value());
    Check(missing_stack.Tick(0)==ScriptState::finished);
}

}
int main(){try{InteractionController();ObjectConfigurationReset();
    const auto table=ReadAnimationTable(Bytes("// comment\n{first$Z.anm}\n{se/*ignored*/cond$A.anm}\n"));
    Check(table.size()==2 && table[0].first=="first" && table[0].second=="Z.anm" && table[1].first=="second");
    auto arena=Fixture();auto map=Minimap();SceneObjects objects(arena,map,Read);
    Check(objects.States().size()==1 && objects.States()[0].scale==2 && objects.States()[0].uses==3 && objects.Drawable(0));
    Check(objects.IsVisible("entry") && !objects.IsVisible("missing"));
    Check(objects.States()[0].movement_registered && objects.States()[0].camera_registered);
    Check(objects.Playback(0)->Action()=="Z" && objects.Playback(0)->Time()==0);objects.Advance(1);Check(objects.Playback(0)->Time()==0);
    Check(!objects.SetVisible("missing",true));Check(objects.SetVisible("entry",false) && !objects.IsVisible("entry"));
    Check(!objects.Drawable(0) && !objects.States()[0].world_attached && !objects.States()[0].movement_registered && !objects.States()[0].camera_registered);
    Check(objects.SetVisible("entry",true) && objects.IsVisible("entry") && objects.Drawable(0) && objects.States()[0].camera_registered);
    std::vector<Vertex> pose;objects.Sample(0,pose);Check(pose[0].position[0]==1);
    {auto end_arena=Fixture(0,true);auto end_map=Minimap();SceneObjects end(end_arena,end_map,Read);Check(end.Playback(0)->Time()==2);end.Sample(0,pose);Check(pose[0].position[0]==3);}
    {auto finite_arena=Fixture(2);auto finite_map=Minimap();SceneObjects finite(finite_arena,finite_map,Read);
        finite.Advance(2);Check(finite.Playback(0)->Completed());finite.Advance(2);finite.Advance(.5f);Check(finite.Playback(0)->Time()==0);
        std::ostringstream output;finite.WriteReport(output);Check(output.str().find("\"paused\":true")!=std::string::npos);}
    {auto infinite_arena=Fixture(-1);auto infinite_map=Minimap();SceneObjects infinite(infinite_arena,infinite_map,Read);infinite.Advance(.5f);Check(infinite.Playback(0)->Time()==.5f);
        Check(infinite.SetVisible("entry",true));Check(infinite.Playback(0)->Time()==.5f);}
    {auto paused_arena=Fixture(-1,false,false);auto paused_map=Minimap();SceneObjects paused(paused_arena,paused_map,Read);paused.Advance(.5f);Check(paused.Playback(0)->Time()==0);}
    {
        auto source=CollisionFixture();auto minimap=Minimap();DistanceOptimization engine;AtomicDistanceInstance distance(source.actors[0].actor,engine);
        SceneCollision collision;SceneObjects bound(source,minimap,Read);bool eligible=true;
        bound.BindMovement(collision,[&](std::size_t object,std::size_t atomic){Check(object==0);return eligible && distance.CollisionAllowed(atomic);});
        Check(bound.States()[0].movement_backend_bound && bound.States()[0].movement_shapes==2);
        Check(source.actors[0].actor.mesh.draws.size()==1); // 不可见原子的完整碰撞仍已登记。
        std::array<float,4> upper{-2,2,0,1},lower{-2,0,0,1};Point3 delta{1.5f,0,0};collision.Resolve(upper,lower,delta,0x10000);
        Check(std::abs(delta[0]-1)<.001f); // 玩家owner不能与第一个GOB的owner混淆。
        eligible=false;delta={1.5f,0,0};collision.Resolve(upper,lower,delta);Check(delta[0]==1.5f);eligible=true;
        Check(bound.SetVisible("entry",false));Check(bound.States()[0].movement_shapes==0);
        delta={1.5f,0,0};collision.Resolve(upper,lower,delta);Check(delta[0]==1.5f);
        Check(bound.SetVisible("entry",true));Check(bound.States()[0].movement_shapes==2);
        bound.Advance(1);delta={1.5f,0,0};collision.Resolve(upper,lower,delta);Check(delta[0]==1.5f); // 动画使碰撞面从x0移到x2。
        upper[0]=lower[0]=0;delta={1.5f,0,0};collision.Resolve(upper,lower,delta);Check(std::abs(delta[0]-1)<.001f);
        bound.UnbindMovement();Check(!bound.States()[0].movement_backend_bound && bound.States()[0].movement_registered && bound.States()[0].movement_shapes==0);
        delta={1.5f,0,0};collision.Resolve(upper,lower,delta);Check(delta[0]==1.5f);
    }
    {
        auto source=CollisionFixture(0);auto minimap=Minimap();SceneCollision collision;SceneObjects detection(source,minimap,Read);
        detection.BindMovement(collision,[](auto,auto){return true;});std::array<float,4> upper{-2,2,0,1},lower{-2,0,0,1};Point3 delta{1.5f,0,0};collision.Resolve(upper,lower,delta);Check(delta[0]==1.5f);
    }
    {
        auto source=Fixture();auto minimap=Minimap();SceneCollision collision;SceneObjects old_cache(source,minimap,Read);bool rejected=false;
        try{old_cache.BindMovement(collision,[](auto,auto){return true;});}catch(const std::logic_error&){rejected=true;}Check(rejected && !old_cache.States()[0].movement_backend_bound);
    }
    {
        auto normal=CollisionFixture();auto& source=normal.catalog.gobs[0];source.type=0;source.flags[2]=1;source.flags[3]=std::bit_cast<unsigned>(20.f);source.script="TakeBow";normal.actors[0].gob_type=0;
        auto minimap=Minimap();SceneObjects items(normal,minimap,Read);
        {bool allowed=false;SceneCollision collision;SceneObjects filtered(normal,minimap,Read);
         filtered.BindMovement(collision,[&](auto,auto){return allowed;});Check(!filtered.FindInteraction({0,0,0},1));
         allowed=true;Check(filtered.FindInteraction({0,0,0},1)==0);}

        struct Scripts:ISceneEventScripts {
            bool active{};unsigned loads{},pushes{};std::string declaration;
            bool Contains(std::string_view world,std::string_view entry) override{Check(world=="Q01" && entry=="void TakeBow()");return active;}
            void LoadWorld(std::string_view world) override{Check(world=="Q01");++loads;}
            std::int32_t Push(std::string_view world,std::string_view entry) override{Check(world=="Q01");declaration=entry;active=true;++pushes;return 0;}
        } scripts;
        Check(items.SetResearch("entry",false) && !items.FindInteraction({0,0,0},1));
        Check(items.SetResearch("entry",true) && !items.SetResearch("missing",true));
        {ScriptGlobals globals{};std::ostringstream trace;GameServices services(trace);services.object_research=[&](auto name,bool enabled){Check(items.SetResearch(name,enabled));};
         ScriptStack stack(globals);stack.Compile("research","void Enable(){giGameObjectSetResearch(\"entry\",false);}");
         Check(stack.Push("research","void Enable()",services).has_value() && stack.Tick(0)==ScriptState::finished && !items.InteractionEnabled(0));
         Check(items.SetResearch("entry",true));}

        Check(items.FindInteraction({0,0,0},1)==0 && !items.FindInteraction({100,0,0},1));
        Check(!items.InteractionInRange(0,{0,1,0},1) && items.InteractionInRange(0,{0,.9f,0},1));
        Check(items.Interact(0,scripts) && scripts.pushes==1 && items.States()[0].uses==2);
        Check(items.Interact(0,scripts) && scripts.pushes==1 && items.States()[0].uses==2); // 活动缓存不重复消耗。
        scripts.active=false;Check(items.Interact(0,scripts) && items.States()[0].uses==1);
        scripts.active=false;Check(items.Interact(0,scripts) && items.States()[0].uses==0 && items.States()[0].disappearance_requested);
        Check(!items.Interact(0,scripts) && !items.FindInteraction({0,0,0},1));
        normal.catalog.gobs[0].name="z";normal.actors[0].name="z";
        normal.catalog.gobs.push_back(normal.catalog.gobs[0]);normal.catalog.gobs.back().name="a";
        normal.actors.push_back(normal.actors[0]);normal.actors.back().name="a";
        SceneObjects ties(normal,minimap,Read);Check(ties.FindInteraction({0,0,0},1)==1);
        Check(ties.SetVisible("a",false) && ties.FindInteraction({0,0,0},1)==0);
    }
    bool failed=false;try{objects.Advance(std::numeric_limits<float>::quiet_NaN());}catch(const std::logic_error&){failed=true;}Check(failed);
    failed=false;try{SceneObjects missing(arena,map,[](auto,auto)->std::optional<std::vector<std::uint8_t>>{return {};});}catch(const std::logic_error&){failed=true;}Check(failed);
    std::cout<<"scene object visibility, original tab order and action contracts passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
