#include "pal4/portable/scene_npcs.h"
#include "pal4/portable/npc_action.h"
#include "pal4/portable/npc_walk.h"
#include <cmath>
#include <bit>
#include "pal4/portable/arena_source.h"
#include "pal4/portable/package.h"
#include "pal4/portable/collision_surface.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
ArenaResources Fixture(){
    ArenaResources arena;SceneNpc npc;npc.name="NPC";npc.resource="TEST";npc.position={0,4,0};npc.flags[0]=1;arena.catalog.npcs.push_back(std::move(npc));
    ArenaActor model;model.name="NPC";model.animation="C01";
    const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};model.actor.frames.push_back({-1,identity});model.actor.palettes.push_back({0,identity});
    model.actor.mesh.vertices.resize(3);model.actor.mesh.vertices[0].position[0]=1;model.actor.influences.resize(3,{{0,0,0,0},{1,0,0,0}});
    model.actor.atomic_spheres.push_back({0,{0,0,0,1}});
    model.actor.clips.push_back({"C01",2,{{0,{{0,{0,0,0,1},{0,0,0}},{2,{0,0,0,1},{2,0,0}}}}}});
    auto action=model.actor.clips.front();action.name="J01";action.duration=.5f;action.tracks[0].keys.back().time=.5f;model.actor.clips.push_back(std::move(action));
    auto run=model.actor.clips.front();run.name="Z02";model.actor.clips.push_back(std::move(run));
    arena.actors.push_back(std::move(model));
    arena.collision.floor.push_back({{Point3{0,0,10},Point3{-10,0,-10},Point3{10,0,-10}},0,0,0});return arena;
}
void InteractionSelection() {
    auto arena=Fixture();
    auto& first=arena.catalog.npcs[0];
    first.name="Z_NPC";first.script="func2002";first.flags[1]=1;
    first.flags[2]=std::bit_cast<std::uint32_t>(5.f);
    arena.actors[0].name=first.name;
    auto& bindings=arena.actors[0].actor.render_bindings.emplace();
    bindings.atomics={{0,1}};
    auto second=first;second.name="A_NPC";
    arena.catalog.npcs.push_back(second);
    auto model=arena.actors[0];model.name=second.name;
    arena.actors.push_back(std::move(model));
    SceneNpcs npcs(arena,[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
    Check(npcs.InteractionScript(0)=="func2002" && npcs.InteractionRadius(0)==5.f,
          "NPC interaction record fields lost");
    Check(npcs.FindInteraction({0,0,0})==1,"equal distance must preserve name order");
    Check(npcs.FindInteraction({0,0,0},[](auto i,auto){return i==0;})==0,
          "original Atomic eligibility was ignored");
    Check(!npcs.FindInteraction({0,0,0},[](auto,auto){return false;}),
          "ineligible models remained selectable");
    npcs.SetVisible("A_NPC",false);
    Check(!npcs.InteractionEnabled(1) && npcs.FindInteraction({0,0,0})==0,
          "hidden NPC remained selectable");
    arena.catalog.npcs[0].flags[1]=0;
    Check(!npcs.InteractionEnabled(0) && !npcs.FindInteraction({0,0,0}),
          "record interaction gate ignored");
    arena.catalog.npcs[0].flags[1]=1;
    npcs.SetMotionPosition(0,{3,0,4});
    Check(npcs.FindInteraction({0,0,0})==0,"inclusive 3D radius rejected");
    npcs.SetMotionPosition(0,{0,6,0});
    Check(!npcs.FindInteraction({0,0,0}),"vertical distance was ignored");
}

}
int main(int argc,char** argv){try{
    if(argc==11 && std::string_view(argv[1])=="--replay-npc-walk"){
        ArenaSource source(argv[2],argv[3]);const auto arena=LoadArenaResources("Q01","Q01",[&](auto member){return source.Read(member);});
        std::ifstream input(std::filesystem::path(argv[2])/"palactor.cpk",std::ios::binary);PackageReader package(input);
        SceneNpcs npcs(arena,[&](auto member)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(member))return {};return package.Read(member);});
        const std::string name=argv[4];Check(npcs.SetVisible(name,true),"original replay NPC is missing");
        NpcWalks walk(npcs,arena.collision,arena.world);Check(!walk.Start({1,1},name,{std::stof(argv[5]),std::stof(argv[6]),std::stof(argv[7])},std::string_view(argv[8])=="1",false).suspend,"nonblocking replay suspended");
        std::ifstream times(argv[9]);std::ofstream report(argv[10]);Check(bool(times) && bool(report),"replay files unavailable");
        float delta{};unsigned visible{};
        while(times>>delta>>visible){Check(visible<=1,"invalid replay visibility");const auto i=std::find_if(npcs.States().begin(),npcs.States().end(),[&](const auto& n){return n.name==name;});Check(i!=npcs.States().end(),"replay NPC lost");
            if(bool(i->active)!=(visible!=0))Check(npcs.SetVisible(name,visible!=0),"replay visibility lost");
            npcs.Advance(delta);Check(walk.Advance(delta).empty(),"nonblocking replay emitted a script completion");walk.WriteReport(report);report<<'\n';
        }
        Check(times.eof() && bool(report),"replay input/output failure");return 0;
    }
    if(argc>=5 && argc<=8){
        const std::string scene=argc>=7?argv[6]:"Q01";
        ArenaSource source(argv[1],argv[2]);const auto arena=LoadArenaResources("Q01",scene,[&](auto name){return source.Read(name);});
        std::ifstream input(std::filesystem::path(argv[1])/"palactor.cpk",std::ios::binary);PackageReader package(input);
        SceneNpcs npcs(arena,[&](auto member)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(member))return {};return package.Read(member);});
        if(scene=="Q01"){
            Check(npcs.States().size()==4,"original Q01 NPC count");for(std::size_t i=0;i<4;++i)Check(!npcs.Drawable(i),"original hidden NPC drawn");
            Check(npcs.SetVisible("MN001",true) && npcs.Drawable(0),"original show callback failed");
        }else{Check(scene=="N01" && npcs.States().size()==1 && npcs.Drawable(0),"original N01 NPC initial entry failed");}
        if(argc==8){Check(npcs.HasAction(0,argv[7]),"original NPC requested action missing");Check(npcs.Playback(0)->Select(argv[7]),"original NPC selection failed");npcs.Playback(0)->SetRestart(false);}
        npcs.Advance(argc>=6?std::stof(argv[5]):.25f);
        std::vector<Vertex> pose;npcs.Sample(0,pose);std::ofstream vertices(argv[4]);vertices<<pose.size()<<'\n';
        double sum{};for(const auto& v:pose)sum+=v.position[0]+2.0*v.position[1]+3.0*v.position[2];vertices.precision(17);vertices<<sum<<'\n';
        std::ofstream report(argv[3]);npcs.WriteReport(report);return 0;
    }
    InteractionSelection();
    auto arena=Fixture();CollisionSurface surface(arena.collision.floor);
    const auto face=surface.Select({0,5,0});Check(face==1 && surface.Project(face,{0,5,0})==Point3{0,0,0},"original select/project failed");
    Check(surface.Select({30,0,0})==1,"nearest edge face not found");const auto clamp=surface.Project(1,{30,5,0});Check(clamp[0]<30 && clamp[1]==0,"outside point not clamped");
    auto upper=arena.collision.floor.front();for(auto& p:upper.points)p[1]=20;arena.collision.floor.push_back(upper);CollisionSurface stacked(arena.collision.floor);
    Check(stacked.Select({0,18,0})==2 && stacked.Select({0,2,0})==1,"stacked floor height selection failed");
    SceneNpcs npcs(arena,[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});Check(npcs.Drawable(0) && npcs.States()[0].collision_bound,"initial NPC entry failed");
    std::vector<Vertex> before,after;npcs.Sample(0,before);npcs.Advance(.5f);npcs.Sample(0,after);Check(after[0].position[0]>before[0].position[0],"NPC animation did not advance");
    Check(!npcs.SetVisible("missing",true),"missing NPC matched");Check(npcs.SetVisible("NPC",false),"hide failed");const auto hidden=npcs.States()[0];
    Check(!npcs.Drawable(0) && hidden.active==0 && !hidden.world_attached && !hidden.movement_registered && !hidden.camera_registered && !hidden.collision_bound,"hide left registration");
    npcs.Advance(.5f);std::vector<Vertex> frozen;npcs.Sample(0,frozen);Check(frozen[0].position[0]==after[0].position[0],"hidden NPC advanced");
    Check(npcs.SetVisible("NPC",true) && npcs.Drawable(0) && npcs.States()[0].position==Point3{0,4,0},"show changed source position");
    Check(npcs.SetVisible("NPC",true) && npcs.Drawable(0),"repeated show failed");
    std::ostringstream report;npcs.WriteReport(report);Check(report.str().find("\"enters\":2,\"leaves\":3")!=std::string::npos,"show did not leave before enter");
    bool failed=false;try{npcs.SetVisible(std::string("NPC\0x",5),true);}catch(const std::logic_error&){failed=true;}Check(failed,"NUL NPC name accepted");
    CollisionSurface empty({});Check(empty.Select({0,0,0})==0,"empty collision surface selected a face");
    {
        NpcActions tasks(npcs);auto* playback=npcs.Playback(0);Check(playback!=nullptr,"NPC playback owner missing");
        Check(!tasks.Start({1,1},"missing","J01",1,true).suspend,"missing NPC suspended script");
        Check(tasks.Start({1,2},"NPC","J01",1,true).suspend && playback->Action()=="J01","NPC blocking action not selected");
        tasks.Advance(.1f);Check(playback->Time()==0,"NPC task advanced animation twice");
        npcs.Advance(.2f);Check(tasks.Advance(.2f).empty() && playback->Action()=="J01","NPC action completed too early");
        std::vector<Vertex> moving;npcs.Sample(0,moving);Check(moving[0].position[0]>1,"NPC action not used for geometry");
        npcs.SetVisible("NPC",false);const float hiddenTime=playback->Time();npcs.Advance(.2f);Check(tasks.Advance(.2f).empty() && playback->Time()==hiddenTime,"hidden NPC action advanced");npcs.SetVisible("NPC",true);
        std::vector<NpcActionCompletion> done;for(unsigned i=0;i<20 && done.empty();++i){npcs.Advance(.05f);done=tasks.Advance(.05f);}
        Check(done.size()==1 && done[0].token==ScriptToken{1,2} && done[0].error.empty() && playback->Action()=="C01","NPC completion or idle restore wrong");
        Check(tasks.Advance(.1f).empty(),"NPC completed twice");
        Check(!tasks.Start({1,3},"NPC","J01",1,false).suspend,"nonblocking NPC action suspended");
        for(unsigned i=0;i<20;++i){npcs.Advance(.05f);Check(tasks.Advance(.05f).empty(),"nonblocking NPC emitted script completion");}Check(playback->Action()=="C01","nonblocking NPC action never closed");
        tasks.Start({1,4},"NPC","J01",1,true);tasks.Start({1,5},"NPC","J01",1,true);auto replaced=tasks.Advance(0);
        Check(replaced.size()==1 && replaced[0].token==ScriptToken{1,4} && !replaced[0].error.empty(),"replaced NPC wait was lost");
        bool missingAction=false;try{tasks.Start({1,6},"NPC","missing",1,true);}catch(const std::logic_error&){missingAction=true;}Check(missingAction && playback->Action()=="J01","missing NPC clip silently accepted");
    }
    {
        auto isolated=Fixture();SceneNpcs owners(isolated,[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
        NpcActions tasks(owners);auto* playback=owners.Playback(0);
        Check(!tasks.End({9,1},"missing").suspend,"missing NPC end suspended");
        tasks.Start({9,2},"NPC","J01",1,false);owners.Advance(.2f);tasks.Advance(.2f);
        playback->SetPause(true);Check(tasks.End({9,3},"NPC").suspend,"NPC end did not register wait");
        Check(tasks.Advance(.1f).empty() && playback->Time()==.2f,"NPC end advanced animation or completed early");
        owners.Advance(.1f);Check(playback->Time()>.2f && tasks.Advance(.1f).empty(),"NPC end failed to release hold or completed early");
        std::vector<NpcActionCompletion> done;
        for(unsigned i=0;i<20 && done.empty();++i){owners.Advance(.05f);done=tasks.Advance(.05f);}
        Check(done.size()==1 && done[0].token==ScriptToken{9,3} && done[0].error.empty() && playback->Action()=="C01","NPC end did not complete and restore idle");
        Check(tasks.Advance(0).empty(),"NPC end completed twice");
        Check(tasks.End({9,4},"NPC").suspend,"idle-cycle end did not wait");
        Check(tasks.Advance(0).empty(),"idle-cycle end completed before boundary");
        done.clear();for(unsigned i=0;i<60 && done.empty();++i){owners.Advance(.05f);done=tasks.Advance(.05f);}
        Check(done.size()==1 && done[0].token==ScriptToken{9,4} && playback->Action()=="C01" && playback->Time()==0,"idle-cycle end did not observe wrap and restore idle");
        std::ostringstream result;tasks.WriteReport(result);
        Check(result.str().find("\"end_observed_complete\":true")!=std::string::npos,"NPC end did not observe actual completion");
    }
    {
        auto isolated=Fixture();isolated.catalog.npcs[0].position[1]=0;
        SceneNpcs owners(isolated,[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
        {
            NpcWalks walk(owners,isolated.collision,isolated.world);const auto start=owners.States()[0].position;
            Check(!walk.Start({10,1},"missing",{4,0,0},true,true).suspend,"missing NPC walk suspended");
            Check(walk.Start({10,2},"NPC",{4,0,0},true,true).suspend && owners.Playback(0)->Action()=="Z02","NPC run did not use fallback animation");
            Check(owners.States()[0].position==start,"NPC moved during route setup");
            const auto first_step=walk.Advance(.01f);
            Check(first_step.empty() && owners.States()[0].position==start,"NPC start node was not consumed separately");
            Check(walk.Advance(.01f).empty() && owners.States()[0].position[0]>start[0] && owners.Playback(0)->Time()==0,"NPC run did not move or advanced animation twice");
            std::vector<NpcWalkCompletion> done;for(unsigned i=0;i<30 && done.empty();++i)done=walk.Advance(.05f);
            Check(done.size()==1 && done[0].token==ScriptToken{10,2} && done[0].error.empty() && std::abs(owners.States()[0].position[0]-4)<.001f && owners.Playback(0)->Action()=="C01","NPC run completion/idle/target failed");
            Check(walk.Advance(.1f).empty(),"NPC run completed twice");
            Check(!walk.Start({10,3},"NPC",{-4,0,0},false,false).suspend,"nonblocking NPC walk suspended");
            owners.SetVisible("NPC",false);const auto hidden=owners.States()[0].position;Check(walk.Advance(1).empty() && owners.States()[0].position==hidden,"hidden NPC movement advanced");owners.SetVisible("NPC",true);
            Check(walk.End({10,4},"NPC").suspend,"NPC end movement failed to wait");done.clear();for(unsigned i=0;i<50 && done.empty();++i)done=walk.Advance(.05f);
            Check(done.size()==1 && done[0].token==ScriptToken{10,4} && done[0].error.empty(),"NPC end movement completion failed");
            walk.Start({10,5},"NPC",{4,0,0},true,true);walk.Start({10,6},"NPC",{0,0,3},true,true);done=walk.Advance(0);
            Check(done.size()==1 && done[0].token==ScriptToken{10,5} && !done[0].error.empty(),"replaced NPC movement wait lost");
            bool duplicate=false;try{NpcWalks other(owners,isolated.collision,isolated.world);}catch(const std::logic_error&){duplicate=true;}Check(duplicate,"NPC acquired two movement owners");
        }
        std::ostringstream report;owners.WriteReport(report);Check(report.str().find("\"motion_bound\":false")!=std::string::npos,"NPC movement owner was not detached");
    }
    std::cout<<"NPC scene enter/leave, animation, collision projection and hidden-state contracts passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
