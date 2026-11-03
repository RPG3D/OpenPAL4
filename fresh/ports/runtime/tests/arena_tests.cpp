#include "pal4/portable/arena.h"
#include "pal4/portable/arena_services.h"
#include "pal4/portable/arena_source.h"
#include "pal4/portable/bsp_scene.h"
#include "pal4/portable/dff_collision.h"
#include "pal4/portable/scene_objects.h"
#include "pal4/portable/scene_npcs.h"
#include "pal4/portable/atomic_distance.h"
#include "pal4/portable/scene_events.h"
#include "pal4/portable/package.h"
#include "pal4/portable/original_animation.h"
#include "pal4/portable/actor_playback.h"
#include "pal4/portable/original_player_data.h"
#include "pal4/portable/player_action.h"
#include "pal4/portable/monster_player_view.h"
#include "pal4/portable/gbk.h"
#include <fstream>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("arena contract failed");}
void CompareEventGeometry(const ObjectCollisionGeometry& a,const ObjectCollisionGeometry& b){
    Check(a.atomics.size()==b.atomics.size());for(std::size_t i=0;i<a.atomics.size();++i){const auto& x=a.atomics[i];const auto& y=b.atomics[i];Check(x.frame==y.frame && x.flags==y.flags && x.triangles.size()==y.triangles.size());
        for(std::size_t j=0;j<x.triangles.size();++j){const auto& p=x.triangles[j];const auto& q=y.triangles[j];Check(p.atomic==q.atomic && p.material==q.material && p.triangle==q.triangle);for(unsigned v=0;v<3;++v)for(unsigned k=0;k<3;++k)Check(std::abs(p.points[v][k]-q.points[v][k])<0.001f);}}
}
void Word(std::string& b,unsigned x){for(unsigned n=0;n<32;n+=8)b+=static_cast<char>(x>>n);}
void Text(std::string& b,const std::string& s){Word(b,static_cast<unsigned>(s.size()+1));b+=s;b+='\0';}
std::string Fixture(){std::string b="P4AD";for(unsigned x:{1,3,1,0})Word(b,x);for(const auto* s:{"Q01","N01","cabin"})Text(b,s);Word(b,1);Text(b,"MC001.cam");return b;}
struct Host final:IScriptContextServices {unsigned calls{};ScriptResponse Invoke(const ScriptRequest&)override{++calls;return {};}};
struct OriginalScripts final:ISceneEventScripts,IScriptContextServices {
    std::filesystem::path game_data;ScriptGlobals globals{};ScriptStack stack{globals};std::string loaded,prepared;unsigned pushes{};
    explicit OriginalScripts(const char* path):game_data(path){}
    ScriptResponse Invoke(const ScriptRequest&) override{throw std::logic_error("offline event probe does not execute gameplay");}
    bool Contains(std::string_view world,std::string_view declaration) override{return stack.ContainsActive(world,declaration);}
    void LoadWorld(std::string_view world) override {
        if(loaded==world)return;
        std::ifstream input(game_data/"script.cpk",std::ios::binary);PackageReader package(input);
        stack.LoadCsb(world,package.Read(std::string(world)+".csb"),DecodeGbk);loaded=world;
    }
    std::int32_t Push(std::string_view world,std::string_view declaration) override {
        const auto id=stack.Push(world,declaration,*this);if(id){++pushes;prepared=declaration;}return id?0:-6;
    }
};

}
int main(int argc,char** argv){
    try{
        if(argc==4 && std::string_view(argv[1])=="player-view-original"){
            ArenaSource source(argv[2]);const auto data=ReadOriginalPlayerData(ReadOriginalDatabasePackage(argv[2]));PlayerRoster roster;
            roster.Prepare(data.roles,data.upgrades,[&](std::string_view model){return source.ReadOriginalActorResource("gamedata\\PALActor\\"+std::string(model)+"\\",model,{},true);});
            ActorController controller;controller.Initialize(1280,720,[&]{return source.ReadOriginalActorResource("gamedata\\PALObject\\JDumy\\","JDumy");});
            MonsterPlayerView view(roster,controller);std::ofstream out(argv[3]);out<<"{\"players\":[";
            constexpr float yaw[]{0,90,180,-90};constexpr std::array<std::array<float,3>,4> forward{{{0,0,1},{1,0,0},{0,0,-1},{-1,0,0}}};
            for(unsigned i=0;i<4;++i){roster.EnterScene("Q01","N01",i);roster.SetPosition(i,{10.f+i,20.f,30.f});roster.SetRotation(i,{0,yaw[i],0});controller.SelectActor(i,1);
                const auto value=view.ControlledActor();const auto target=view.Target(value.address);Check(value.active && target.active && target.position==value.position);
                const auto direction=view.TargetForward(value.address);for(unsigned a=0;a<3;++a)Check(std::abs(direction[a]-forward[i][a])<0.00001f);
                if(i)out<<',';out<<"{\"token\":"<<value.address<<",\"half_height_bits\":"<<std::bit_cast<std::uint32_t>(value.half_height)<<",\"center_bits\":[";
                for(unsigned a=0;a<3;++a){if(a)out<<',';out<<std::bit_cast<std::uint32_t>(value.collision_position[a]);}out<<"],\"forward_bits\":[";for(unsigned a=0;a<3;++a){if(a)out<<',';out<<std::bit_cast<std::uint32_t>(direction[a]);}out<<"]}";}
            bool rejected=false;try{(void)view.TargetForward(0);}catch(const std::invalid_argument&){rejected=true;}Check(rejected);
            out<<"]}";Check(bool(out));return 0;
        }
        if(argc==3 && std::string_view(argv[1])=="archive-lifetime-original"){
            ArenaSource source(argv[2]);const std::string directory="gamedata\\PALActor\\101\\";
            auto catalog=source.OpenOriginalAnimations(directory,"101");Check(catalog.store && !catalog.names.empty());
            const auto world=source.ReadObjectMember("gamedata\\PALWorld\\M01\\2\\","2.bsp");Check(world && !world->empty());
            const auto scene=source.ReadSceneMember("Q01\\N01\\N01_floor.dff");Check(scene && !scene->empty());
            Check(source.ReleaseUnusedArchives()==2 && source.ReleaseUnusedArchives()==0);
            auto first=catalog.store->Get(catalog.names.front());Check(first && !first->tracks.empty());first.reset();
            const auto reopened=source.ReadObjectMember("gamedata\\PALWorld\\M01\\2\\","2.bsp");Check(reopened && *world==*reopened);
            const auto reopened_scene=source.ReadSceneMember("Q01\\N01\\N01_floor.dff");Check(reopened_scene && *scene==*reopened_scene);
            catalog.store.reset();Check(source.ReleaseUnusedArchives()==3 && source.ReleaseUnusedArchives()==0);
            Check(source.ReadObjectMember(directory,"101.dff").has_value());Check(source.ReleaseUnusedArchives()==1);
            std::cout<<"original archive retention, release and reopen passed\n";return 0;
        }
        if(argc==3 && std::string_view(argv[1])=="lazy-player-actions"){
            ArenaSource source(argv[2]);const auto data=ReadOriginalPlayerData(ReadOriginalDatabasePackage(argv[2]));PlayerRoster roster;
            roster.Prepare(data.roles,data.upgrades,[&](std::string_view model){return source.ReadOriginalActorResource("gamedata\\PALActor\\"+std::string(model)+"\\",model,{},true);});
            roster.EnterScene("Q01","N01");std::ifstream input(std::filesystem::path(argv[2])/"palactor.cpk",std::ios::binary);PackageReader package(input);
            const auto read=[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);};
            PlayerActions actions(roster,read);
            for(unsigned i=0;i<4;++i){
                const auto& slot=roster.Slots()[i];const auto model=slot.role.Text("RA_MODEL");
                Check(slot.actor->clip_loader && actions.SelectTaskAction(i,"C01"));
                auto eager=source.ReadOriginalActorResource("gamedata\\PALActor\\"+std::string(model)+"\\",model);
                ActorPlayback reference(eager,std::string(model),read);Check(reference.Select("C01"));reference.SetRestart(true);
                actions.Advance(.1f);reference.Advance(.1f);std::vector<Vertex> actual,expected;actions.Sample(i,actual);reference.Sample(expected);
                Check(actual.size()==expected.size() && !actual.empty());for(std::size_t n=0;n<actual.size();++n)for(unsigned k=0;k<3;++k)Check(actual[n].position[k]==expected[n].position[k]);
                Check(actions.SelectTaskAction(i,"C02",.2f) && reference.SelectBlended("C02",.2f));
                actions.Advance(.1f);reference.Advance(.1f);actions.Sample(i,actual);reference.Sample(expected);
                for(std::size_t n=0;n<actual.size();++n)for(unsigned k=0;k<3;++k)Check(actual[n].position[k]==expected[n].position[k]);
                std::cout<<"lazy player action model="<<model<<" vertices="<<actual.size()<<" blend=passed\n";
            }
            return 0;
        }
        if((argc==3 || argc==4) && std::string_view(argv[1])=="lazy-actor-original"){
            const std::string model=argc==4?argv[3]:"101";const auto directory="gamedata\\PALActor\\"+model+"\\";
            OriginalAnimationCatalog catalog;
            {
                ArenaSource source(argv[2]);catalog=source.OpenOriginalAnimations(directory,model);Check(catalog.store && !catalog.names.empty());
                auto first=catalog.store->Get(catalog.names.front());auto shared=catalog.store->Get(catalog.names.front());Check(first==shared);
                const auto count_reads=[&]{std::ostringstream report;source.WriteReport(report);const auto text=report.str();
                    Check(text.find("palactor.cpk")!=std::string::npos);std::size_t count=0,pos=0;while((pos=text.find(".anm",pos))!=std::string::npos){++count;pos+=4;}return count;};
                Check(count_reads()==1);first.reset();shared.reset();auto again=catalog.store->Get(catalog.names.front());Check(count_reads()==2);
            }
            Check(catalog.store && !catalog.names.empty());
            auto first=catalog.store->Get(catalog.names.front());auto same=catalog.store->Get(catalog.names.front());Check(first==same);
            const auto duration=first->duration;std::weak_ptr<const AnimationClip> released=first;first.reset();same.reset();Check(released.expired());
            auto again=catalog.store->Get(catalog.names.front());Check(again->duration==duration && !again->tracks.empty());
            ArenaSource source(argv[2]);auto lazy=source.ReadOriginalActorResource(directory,model,{},true);
            std::ostringstream initial_report;source.WriteReport(initial_report);const auto initial=initial_report.str();
            const auto dff=model+".dff";const auto dff_pos=initial.find(dff);
            Check(dff_pos!=std::string::npos && initial.find(dff,dff_pos+dff.size())==std::string::npos && initial.find(".anm")==std::string::npos);
            auto eager=source.ReadOriginalActorResource(directory,model);Check(lazy.clip_loader && lazy.clips.size()==eager.clips.size());
            for(std::size_t i=0;i<lazy.clips.size();++i)Check(lazy.clips[i].name==eager.clips[i].name && lazy.clips[i].tracks.empty());
            ActorPlayback playback(lazy,model,[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
            Check(playback.Select(lazy.clips.front().name));std::vector<Vertex> actual,reference;playback.Sample(actual);SampleActorVertices(eager,0,0,reference,AnimationTimeMode::clamp);
            Check(actual.size()==reference.size());for(std::size_t i=0;i<actual.size();++i)for(unsigned axis=0;axis<3;++axis)Check(actual[i].position[axis]==reference[i].position[axis]);
            ActorClipLease lease;lease.Sample(lazy,0,.5f,actual);SampleActorVertices(eager,0,.5f,reference);
            Check(actual.size()==reference.size());for(std::size_t i=0;i<actual.size();++i)for(unsigned axis=0;axis<3;++axis)Check(actual[i].position[axis]==reference[i].position[axis]);lease.Reset();
            std::cout<<"original lazy actor model="<<model<<" clips="<<catalog.names.size()<<" first="<<catalog.names.front()<<" reload=passed\n";return 0;
        }
        if(argc==3 && std::string_view(argv[1])=="m01-2-original-world"){
            ArenaSource source(argv[2]);const auto scene=source.ReadOriginalWorld("M01","2");
            std::size_t nonfinite=0;for(const auto& vertex:scene.vertices)if(!std::isfinite(vertex.uv0[0]) || !std::isfinite(vertex.uv0[1]))++nonfinite;
            Check(nonfinite>0);std::cout<<"M01/2 original BSP vertices="<<scene.vertices.size()<<" nonfinite_uv0_references="<<nonfinite<<'\n';return 0;
        }
        if(argc==3 && std::string_view(argv[1])=="m01-3-original-world"){
            ArenaSource source(argv[2]);const auto scene=source.ReadOriginalWorld("M01","3");
            std::size_t unlit=0;for(const auto& draw:scene.draws)if(draw.lightmap<0)++unlit;
            Check(unlit>0 && !scene.vertices.empty());
            const auto bsp=source.ReadObjectMember("gamedata\\PALWorld\\M01\\3\\","3.bsp");
            const auto config=source.ReadObjectMember("gamedata\\PALWorld\\M01\\3\\","3_ltMap.cfg");Check(bsp && config);
            const auto texture=[](std::string_view){return Texture{1,1,{255,255,255,255},false};};
            bool corrupt_rejected=false;try{(void)ReadBspScene(*bsp,*config,texture,[](std::string_view)->std::optional<Texture>{throw std::runtime_error("invalid lightmap");});}catch(const std::runtime_error& error){corrupt_rejected=std::string_view(error.what())=="invalid lightmap";}Check(corrupt_rejected);
            std::cout<<"M01/3 original BSP vertices="<<scene.vertices.size()<<" draws="<<scene.draws.size()<<" no_lightmap_draws="<<unlit<<'\n';return 0;
        }
        if(argc==6 && (std::string_view(argv[1])=="chapter-original-only" || std::string_view(argv[1])=="chapter-lazy-only")){
            const bool lazy=std::string_view(argv[1])=="chapter-lazy-only";
            ArenaSource source(argv[2]);const auto database=ReadOriginalDatabasePackage(argv[2]);
            const auto arena=LoadArenaResources(argv[4],argv[5],[&](auto name){return source.Read(name);},
                [&]{return source.ReadOriginalCollision(argv[4],argv[5]);},true,
                [&](const SceneGob& gob){auto bytes=source.ReadObjectMember(gob.resource_directory,gob.model+".dff");if(!bytes)throw std::runtime_error("original object collision missing");return ReadDffObjectCollision(*bytes);},
                [&]{return source.ReadOriginalDescription(argv[4],argv[5],database.scenes);},
                [&](auto kind){return source.ReadOriginalEnvironment(argv[4],argv[5],kind);},
                [&]{return source.ReadOriginalWorld(argv[4],argv[5]);},
                [&](const SceneNpc& npc){return source.ReadOriginalActorResource("gamedata\\PALActor\\"+npc.resource+"\\",npc.resource,{},lazy);},
                [&](const SceneGob& gob){return source.ReadOriginalActorResource(gob.resource_directory,gob.model,gob.variant,lazy);});
            if(lazy && !arena.catalog.npcs.empty()){
                std::ifstream input(std::filesystem::path(argv[2])/"palactor.cpk",std::ios::binary);PackageReader package(input);
                SceneNpcs npcs(arena,[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);});
                Check(npcs.Playback(0) && !npcs.Playback(0)->Action().empty());
                const auto& npc=arena.catalog.npcs[0];ArenaSource reference_source(argv[2]);
                auto reference_actor=reference_source.ReadOriginalActorResource("gamedata\\PALActor\\"+npc.resource+"\\",npc.resource);
                ActorPlayback reference(reference_actor,npc.resource,[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);});
                Check(reference.Select(npcs.Playback(0)->Action()));npcs.Advance(.1f);reference.Advance(.1f);
                std::vector<Vertex> pose,expected;npcs.Sample(0,pose);reference.Sample(expected);Check(!pose.empty() && pose.size()==expected.size());
                for(std::size_t i=0;i<pose.size();++i)for(unsigned axis=0;axis<3;++axis)Check(pose[i].position[axis]==expected[i].position[axis]);
            }
            std::ofstream output(argv[3]);if(!output)throw std::runtime_error("cannot write arena report");WriteArenaReport(output,arena);
            std::ofstream report(std::string(argv[3])+".source.json");if(!report)throw std::runtime_error("cannot write source report");source.WriteReport(report);return 0;
        }
        if(argc==7 && (std::string_view(argv[1])=="full-environment-original" || std::string_view(argv[1])=="environment-original" || std::string_view(argv[1])=="source-original" || std::string_view(argv[1])=="source" || std::string_view(argv[1])=="objects" || std::string_view(argv[1])=="collision" || std::string_view(argv[1])=="events" || std::string_view(argv[1])=="gob-interaction" || std::string_view(argv[1])=="event-interaction")){
            ArenaSource source(argv[2],argv[3]);
            const auto full=std::string_view(argv[1])=="full-environment-original";
            const auto original=full || std::string_view(argv[1])=="source-original" || std::string_view(argv[1])=="environment-original";
            const auto environment=full || std::string_view(argv[1])=="environment-original";
            OriginalDatabaseCatalogs database;if(original)database=ReadOriginalDatabasePackage(argv[2]);
            auto a=LoadArenaResources(argv[5],argv[6],[&](auto name){if(original && (name.ends_with("collision.p4c") || name.ends_with(".p4g") || (environment && name.find("/environment/")!=name.npos) || (full && (name.ends_with("arena.p4d") || name.ends_with("world.p4s") || name.ends_with(".p4a")))))throw std::runtime_error("converted cache was requested");return source.Read(name);},
                original?std::function<CollisionMesh()>([&]{return source.ReadOriginalCollision(argv[5],argv[6]);}):std::function<CollisionMesh()>{},original,
                original?std::function<ObjectCollisionGeometry(const SceneGob&)>([&](const SceneGob& gob){auto bytes=source.ReadObjectMember(gob.resource_directory,gob.model+".dff");Check(bytes.has_value());return ReadDffObjectCollision(*bytes);}):std::function<ObjectCollisionGeometry(const SceneGob&)>{},
                full?std::function<ArenaDescription()>([&]{return source.ReadOriginalDescription(argv[5],argv[6],database.scenes);}):std::function<ArenaDescription()>{},
                environment?std::function<Scene(std::string_view)>([&](auto kind){return source.ReadOriginalEnvironment(argv[5],argv[6],kind);}):std::function<Scene(std::string_view)>{},
                full?std::function<Scene()>([&]{return source.ReadOriginalWorld(argv[5],argv[6]);}):std::function<Scene()>{},
                full?std::function<Actor(const SceneNpc&)>([&](const auto& npc){return source.ReadOriginalActorResource("gamedata\\PALActor\\"+npc.resource+"\\",npc.resource);}):std::function<Actor(const SceneNpc&)>{},
                full?std::function<Actor(const SceneGob&)>([&](const auto& gob){return source.ReadOriginalActorResource(gob.resource_directory,gob.model,gob.variant);}):std::function<Actor(const SceneGob&)>{});
            if(full){for(const auto& entry:a.actors){const auto bytes=source.Read("arena/"+std::string(argv[5])+"/"+argv[6]+"/entities/"+entry.name+".p4a");std::istringstream in(std::string(bytes.begin(),bytes.end()));const auto old=ReadActor(in);const auto& actual=entry.actor;Check(actual.mesh.vertices.size()==old.mesh.vertices.size() && actual.frames.size()==old.frames.size() && actual.palettes.size()==old.palettes.size());
                for(const auto& y:old.clips){const auto it=std::find_if(actual.clips.begin(),actual.clips.end(),[&](const auto& x){return x.name==y.name;});Check(it!=actual.clips.end() && it->duration==y.duration && it->tracks.size()==y.tracks.size());for(std::size_t t=0;t<it->tracks.size();++t){Check(it->tracks[t].frame==y.tracks[t].frame && it->tracks[t].keys.size()==y.tracks[t].keys.size());for(std::size_t k=0;k<it->tracks[t].keys.size();++k){const auto& p=it->tracks[t].keys[k];const auto& q=y.tracks[t].keys[k];Check(p.time==q.time && p.rotation==q.rotation && p.translation==q.translation);}}}}
            }
            if(environment){std::ofstream diagnostic(std::string(argv[4])+".environment-sources.json");source.WriteReport(diagnostic);}
            if(original){const auto direct=source.ReadOriginalDescription(argv[5],argv[6],database.scenes);Check(a.description.id==direct.id && a.description.type==direct.type && a.description.earthball==direct.earthball && a.description.title==direct.title && a.description.camera_files==direct.camera_files);for(const auto& layer:a.description.environment_files)Check(std::find(direct.environment_files.begin(),direct.environment_files.end(),layer)!=direct.environment_files.end());}
            if(original)for(std::size_t i=0;i<a.event_collisions.size();++i)if(a.event_collisions[i]){const auto bytes=source.Read("arena/"+std::string(argv[5])+"/"+argv[6]+"/events/event_"+std::to_string(i)+".p4g");std::istringstream in(std::string(bytes.begin(),bytes.end()));CompareEventGeometry(*a.event_collisions[i],ReadObjectCollision(in));}
            if(original)for(const auto& actor:a.actors)if(actor.collision){const auto bytes=source.Read("arena/"+std::string(argv[5])+"/"+argv[6]+"/entities/"+actor.name+".p4g");std::istringstream in(std::string(bytes.begin(),bytes.end()));CompareEventGeometry(*actor.collision,ReadObjectCollision(in));}
            if(full){const auto bytes=source.Read("arena/"+std::string(argv[5])+"/"+argv[6]+"/world.p4s");std::istringstream in(std::string(bytes.begin(),bytes.end()));const auto expected=ReadScene(in);Check(a.world.vertices.size()==expected.vertices.size() && a.world.draws.size()==expected.draws.size() && a.world.textures.size()==expected.textures.size() && a.world.saturation==expected.saturation && a.world.brightness==expected.brightness);for(std::size_t i=0;i<a.world.vertices.size();++i){const auto& x=a.world.vertices[i];const auto& y=expected.vertices[i];for(unsigned j=0;j<3;++j)Check(x.position[j]==y.position[j]);for(unsigned j=0;j<2;++j)Check(x.uv0[j]==y.uv0[j] && x.uv1[j]==y.uv1[j]);for(unsigned j=0;j<4;++j)Check(std::abs(x.color[j]-y.color[j])<0.00001f);}for(std::size_t i=0;i<a.world.draws.size();++i){const auto& x=a.world.draws[i];const auto& y=expected.draws[i];Check(x.first==y.first && x.count==y.count && x.diffuse==y.diffuse && x.lightmap==y.lightmap);}}
            if(environment)for(const auto& layer:a.environment){if(full && std::string_view(argv[5])=="Q01" && std::string_view(argv[6])=="N01" && layer.kind=="clipNA"){Check(layer.mesh.vertices.size()==168 && layer.mesh.draws.size()==7 && layer.mesh.textures.size()==7);continue;}const auto bytes=source.Read("arena/"+std::string(argv[5])+"/"+argv[6]+"/environment/"+layer.kind+".p4s");std::istringstream in(std::string(bytes.begin(),bytes.end()));const auto expected=ReadScene(in);if(layer.mesh.vertices.size()!=expected.vertices.size() || layer.mesh.draws.size()!=expected.draws.size() || layer.mesh.textures.size()!=expected.textures.size())throw std::runtime_error("environment counts differ: "+layer.kind);for(std::size_t i=0;i<expected.textures.size();++i){const auto& x=layer.mesh.textures[i];const auto& y=expected.textures[i];if(x.width!=y.width || x.height!=y.height || x.rgba.size()!=y.rgba.size())throw std::runtime_error("environment texture dimensions differ: "+layer.kind+" index="+std::to_string(i)+" actual="+std::to_string(x.width)+"x"+std::to_string(x.height)+" expected="+std::to_string(y.width)+"x"+std::to_string(y.height));for(std::size_t j=0;j<x.rgba.size();++j)if(std::abs(int(x.rgba[j])-int(y.rgba[j]))>(j%4==3?0:1))throw std::runtime_error("environment texture pixel differs: "+layer.kind+" index="+std::to_string(i)+" channel="+std::to_string(j));}}
            std::ofstream output(argv[4]);
            if(std::string_view(argv[1])=="event-interaction"){
                OriginalScripts scripts(argv[2]);SceneCollision collision;
                SceneEvents events(a,collision,scripts);events.ActivateAll();
                std::size_t index=a.catalog.events.size();
                for(std::size_t i=0;i<a.catalog.events.size();++i){const auto& e=a.catalog.events[i];
                    if(e.effect_type==5 && !e.script.empty() && !e.properties.empty()){index=i;break;}}
                Check(index<a.catalog.events.size());
                for(std::size_t i=0;i<events.States().size();++i)if(i!=index)Check(events.SetVisible(events.States()[i].name,false));
                const auto& event=a.catalog.events[index];const auto& property=event.properties[0];
                Point3 position{std::bit_cast<float>(property[0]),std::bit_cast<float>(property[1]),std::bit_cast<float>(property[2])};
                Check(events.FindInteraction(position,1,1)==index && events.States()[index].movement_shapes==0);
                Check(events.TriggerInteraction(index) && scripts.pushes==1 && scripts.stack.State()==ScriptState::prepared);
                Check(!events.TriggerInteraction(index) && scripts.pushes==1);
                Check(events.SetVisible(event.name,false) && !events.FindInteraction(position,1,1) && !events.TriggerInteraction(index));
                output<<"{\"isolated_fixture\":true,\"selected\":"<<std::quoted(event.name)<<",\"original_csb_loaded\":"<<std::quoted(scripts.loaded)<<",\"prepared_function\":"<<std::quoted(scripts.prepared)<<",\"pushes\":"<<scripts.pushes<<",\"game_calls\":0}";
            }else if(std::string_view(argv[1])=="gob-interaction"){
                OriginalScripts scripts(argv[2]);SceneMinimap minimap(argv[5],argv[6],[&](auto member){return source.ReadSceneMember(member);});
                SceneObjects objects(a,minimap,[&](auto directory,auto member){return source.ReadObjectMember(directory,member);});
                std::size_t index=a.catalog.gobs.size();
                for(std::size_t i=0;i<a.catalog.gobs.size();++i)if(a.catalog.gobs[i].type==0 && !a.catalog.gobs[i].script.empty()){index=i;break;}
                Check(index<a.catalog.gobs.size());const auto& gob=a.catalog.gobs[index];
                Check(objects.SetVisible(gob.name,true) && objects.SetResearch(gob.name,true)); // 独立夹具，不执行场景初始化或修改实际应用。
                const auto selected=objects.FindInteraction(gob.position,100);Check(selected==index);
                const auto before=objects.States()[index].uses;Check(objects.Interact(index,scripts));
                Check(scripts.pushes==1 && scripts.stack.State()==ScriptState::prepared && scripts.prepared=="void "+gob.script+"()");
                const auto after=objects.States()[index].uses;Check(static_cast<std::int32_t>(before)<=0 || after==before-1);
                output<<"{\"fixture_enables_object\":true,\"selected\":"<<std::quoted(gob.name)<<",\"original_csb_loaded\":"<<std::quoted(scripts.loaded)<<",\"prepared_function\":"<<std::quoted(scripts.prepared)<<",\"uses_before\":"<<before<<",\"uses_after\":"<<after<<",\"game_calls\":0,\"objects\":";objects.WriteReport(output);output<<'}';
            }else if(std::string_view(argv[1])=="events"){

                OriginalScripts scripts(argv[2]);
                SceneEvents* active=nullptr;SceneCollision collision([&](auto resource,bool entering,auto previous){Check(active!=nullptr);active->OnCollision(resource,entering,previous);});
                SceneEvents events(a,collision,scripts);active=&events;events.ActivateAll();std::size_t total{},selected=a.catalog.events.size();
                for(std::size_t i=0;i<events.States().size();++i){total+=events.States()[i].movement_shapes;if(selected==a.catalog.events.size() && events.States()[i].type==1 && events.States()[i].movement_shapes)selected=i;}
                Check(selected<a.catalog.events.size());
                for(std::size_t i=0;i<events.States().size();++i)if(i!=selected)Check(events.SetVisible(events.States()[i].name,false));
                const auto& triangle=a.event_collisions[selected]->atomics.front().triangles.front();Point3 center{};
                for(auto point:triangle.points)for(unsigned axis=0;axis<3;++axis)center[axis]+=point[axis]/3;
                const auto resolve=[&]{std::array<float,4> upper{center[0],center[1],center[2],.5f},lower=upper;Point3 delta{.001f,0,0};collision.Resolve(upper,lower,delta);};
                resolve();Check(scripts.pushes==1 && scripts.stack.State()==ScriptState::prepared && scripts.stack.Depth()==1);
                resolve();Check(scripts.pushes==1);Check(events.SetVisible(events.States()[selected].name,false));resolve();Check(scripts.pushes==1);
                output<<"{\"registered_shapes\":"<<total<<",\"isolated_trigger\":true,\"original_csb_loaded\":"<<std::quoted(scripts.loaded)<<",\"prepared_function\":"<<std::quoted(scripts.prepared)<<",\"pushes\":"<<scripts.pushes<<",\"game_calls\":0,\"events\":";events.WriteReport(output);output<<'}';
            }else if(std::string_view(argv[1])=="collision"){

                SceneMinimap minimap(argv[5],argv[6],[&](auto member){return source.ReadSceneMember(member);});
                DistanceOptimization engine;std::vector<std::unique_ptr<AtomicDistanceInstance>> distances;SceneCollision collision;
                SceneObjects objects(a,minimap,[&](auto directory,auto member){return source.ReadObjectMember(directory,member);});
                distances.resize(objects.States().size());
                for(std::size_t i=0;i<objects.States().size();++i)if(objects.States()[i].actor)distances[i]=std::make_unique<AtomicDistanceInstance>(a.actors[*objects.States()[i].actor].actor,engine);
                unsigned eligibility_reads{};objects.BindMovement(collision,[&](std::size_t object,std::size_t atomic){++eligibility_reads;Check(bool(distances.at(object)));return distances[object]->CollisionAllowed(atomic);});
                const auto count=[&]{std::size_t total{};for(const auto& state:objects.States())total+=state.movement_shapes;return total;};
                const auto before=count();Check(before>0);std::string selected;
                for(const auto& state:objects.States())if(state.movement_shapes){selected=state.name;break;}
                Check(objects.SetVisible(selected,false));const auto hidden=count();Check(hidden<before);
                Check(objects.SetVisible(selected,true) && count()==before);objects.Advance(.1f);
                std::vector<Vertex> ignored;std::vector<Matrix> frames;
                for(std::size_t i=0;i<objects.States().size();++i)if(objects.States()[i].actor){
                    const auto& state=objects.States()[i];objects.Sample(i,ignored,&frames);auto model=InstanceMatrix(state.position,state.rotation);
                    for(unsigned c=0;c<3;++c)for(unsigned r=0;r<3;++r)model[c*4+r]*=state.scale;
                    (void)distances[i]->Evaluate(frames,model,{0,0,0});
                }
                std::array<float,4> upper{-1000000,1000000,-1000000,1},lower{-1000000,999999,-1000000,1};Point3 delta{};collision.Resolve(upper,lower,delta);Check(eligibility_reads>0);
                output<<"{\"before\":"<<before<<",\"after_hide\":"<<hidden<<",\"after_show\":"<<count()<<",\"eligibility_reads\":"<<eligibility_reads<<",\"objects\":";objects.WriteReport(output);output<<",\"collision\":";collision.WriteReport(output);output<<'}';
            }else if(std::string_view(argv[1])=="objects"){
                SceneMinimap minimap(argv[5],argv[6],[&](auto member){return source.ReadSceneMember(member);});
                SceneObjects objects(a,minimap,[&](auto directory,auto member){return source.ReadObjectMember(directory,member);});
                Check(objects.SetVisible("entrance0",false));Check(objects.SetVisible("entrance2",true));Check(objects.SetVisible("entrance2",false));
                objects.WriteReport(output);
            }else WriteArenaReport(output,a);
            std::ofstream report(std::string(argv[4])+".source.json");source.WriteReport(report);return 0;
        }
        {
            ArenaSource source("unused","unused");
            for(auto bad:{"../arena/Q01/Q01/a.p4a","arena/Q01/../a.p4s","arena//Q01/a.p4d","arena/Q01/Q01/a.exe"}){
                bool rejected=false;try{source.Read(bad);}catch(const std::invalid_argument&){rejected=true;}Check(rejected);
            }
        }
        {
            const auto cache=std::filesystem::temp_directory_path()/("pal4-arena-source-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            std::filesystem::create_directories(cache/"arena/Q01/N01");
            unsigned fallback_calls{};ArenaSource source("unused",cache,[&](auto){++fallback_calls;return std::vector<std::uint8_t>{1};});
            Check(source.Read("arena/Q01/N01/arena.p4d")==std::vector<std::uint8_t>{1} && fallback_calls==1);
            const auto file=cache/"arena/Q01/N01/arena.p4d";
            {std::ofstream output(file,std::ios::binary);output.put(2);}
            Check(source.Read("arena/Q01/N01/arena.p4d")==std::vector<std::uint8_t>{2} && fallback_calls==1);
            ArenaSource original_only("unused");bool original_rejected=false;try{original_only.Read("arena/Q01/N01/arena.p4d");}catch(const std::runtime_error&){original_rejected=true;}Check(original_rejected && fallback_calls==1);
            {std::ofstream output(file,std::ios::binary|std::ios::trunc);}
            bool rejected=false;try{source.Read("arena/Q01/N01/arena.p4d");}catch(const std::runtime_error&){rejected=true;}
            Check(rejected && fallback_calls==1); // Corrupt user data must not silently load development assets.
            std::filesystem::remove_all(cache);
        }
        if(argc==3 || argc==5){
            const auto a=LoadArenaResources(argc==5?argv[3]:"Q01",argc==5?argv[4]:"N01",[&](std::string_view name){
                std::ifstream input(std::filesystem::path(argv[1])/std::string(name),std::ios::binary);
                if(!input)throw std::runtime_error("missing arena resource "+std::string(name));
                return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>());
            });
            std::ofstream output(argv[2]);WriteArenaReport(output,a);return 0;
        }
        const auto bytes=Fixture();std::istringstream input(bytes);const auto d=ReadArenaDescription(input);Check(d.world=="Q01" && d.scene=="N01" && d.type==1 && d.camera_files.size()==1);
        for(std::size_t i=0;i<bytes.size();++i){bool failed=false;try{std::istringstream short_input(bytes.substr(0,i));(void)ReadArenaDescription(short_input);}catch(const std::exception&){failed=true;}Check(failed);}
        {
            auto b=Fixture();b[4]=2;Word(b,2);Text(b,"clip");Text(b,"sky");
            std::istringstream v2(b);const auto d2=ReadArenaDescription(v2);Check(d2.environment_files==std::vector<std::string>({"clip","sky"}));
            for(std::size_t n=Fixture().size();n<b.size();++n){bool failed=false;try{std::istringstream cut(b.substr(0,n));ReadArenaDescription(cut);}catch(const std::exception&){failed=true;}Check(failed);}
            for(auto invalid:{"../sky","water","clip"}){auto bad=Fixture();bad[4]=2;Word(bad,2);Text(bad,"clip");Text(bad,invalid);bool failed=false;try{std::istringstream in(bad);ReadArenaDescription(in);}catch(const std::exception&){failed=true;}Check(failed);}
        }
        {auto bytes3=Fixture();bytes3[4]=3;Word(bytes3,0);Word(bytes3,1);std::istringstream in(bytes3);Check(ReadArenaDescription(in).object_collision_cache);
            auto both=Fixture();both[4]=3;Word(both,0);Word(both,3);std::istringstream all(both);const auto capabilities=ReadArenaDescription(all);Check(capabilities.object_collision_cache && capabilities.event_collision_cache);
         bytes3.back()=static_cast<char>(0x80);bool rejected=false;try{std::istringstream bad(bytes3);ReadArenaDescription(bad);}catch(const std::runtime_error&){rejected=true;}Check(rejected);}
        {ArenaResources arena;arena.monsters_verified_empty=true;ValidateArenaGameplaySupport(arena);
            arena.catalog.monsters.emplace_back();bool rejected=false;try{ValidateArenaGameplaySupport(arena);}catch(const std::runtime_error&){rejected=true;}Check(rejected);
            arena.catalog.monsters.clear();arena.catalog.paths.emplace_back();rejected=false;try{ValidateArenaGameplaySupport(arena);}catch(const std::runtime_error&){rejected=true;}Check(rejected);
            arena.catalog.paths.clear();arena.monsters_verified_empty=false;rejected=false;try{ValidateArenaGameplaySupport(arena);}catch(const std::runtime_error&){rejected=true;}Check(rejected);}
        {
            unsigned calls=0;ArenaSource source({}, {},[&](std::string_view){++calls;return std::vector<std::uint8_t>{1,2,3};});
            for(unsigned i=0;i<4100;++i){const auto member="arena/Q01/N01/report_"+std::to_string(i)+".p4s";Check(source.Read(member)==std::vector<std::uint8_t>({1,2,3}));}
            std::ostringstream report;source.WriteReport(report);const auto value=report.str();
            Check(calls==4100 && value.find("\"dropped_reads\":4")!=std::string::npos && value.find("report_0.p4s")==std::string::npos && value.find("report_4.p4s")!=std::string::npos && value.find("report_4099.p4s")!=std::string::npos);
            std::size_t count=0,position=0;while((position=value.find("\"member\":",position))!=std::string::npos){++count;position+=9;}Check(count==4096);
        }
        Host host;ArenaServices service(host);const ScriptCall load{65,10100,{std::string("q01"),std::string("n01"),std::string(""),false}};
        const ScriptToken token{7,4};Check(service.Invoke({token,load}).suspend && service.Waiting());
        Check(!service.Finish(token));const auto command=service.TakeCommand();Check(command && command->world=="Q01" && command->scene=="N01");
        Check(!service.TakeCommand() && !service.Finish({7,3}));Check(service.Finish(token) && !service.Finish(token));
        auto other_bool=load;other_bool.arguments[3]=true;Check(service.Invoke({{8,1},other_bool}).suspend);
        Check(!service.Invoke({{8,2},load}).suspend);service.Cancel();Check(!service.Waiting() && !service.TakeCommand());
        auto bad=load;bad.arguments[0]=std::string("../Q01");bool failed=false;try{service.Invoke({token,bad});}catch(const std::invalid_argument&){failed=true;}Check(failed && !service.Waiting());
        const ScriptCall other{103,0,{false}};service.Invoke({token,other});Check(host.calls==1);
        std::cout<<"arena loading checks passed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
