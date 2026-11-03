#include "pal4/portable/effect_playback.h"
#include "pal4/portable/package.h"
#include "pal4/portable/gob_effect_instance.h"
#include "pal4/portable/scene_objects.h"
#include "pal4/portable/scene_collision.h"
#include <fstream>
#include <iostream>
#include <random>
#include <source_location>
#include <sstream>
#include <stdexcept>
using namespace pal4;using namespace pal4::portable;
namespace {
void Check(bool v,std::source_location at=std::source_location::current()){if(!v)throw std::runtime_error("effect playback contract at "+std::to_string(at.line()));}
template<class F> void Reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}Check(rejected);}
struct Random final:ParticleRandom {std::mt19937 engine;unsigned Time() override{return 1234;}void Seed(unsigned n) override{engine.seed(n);}int Next15() override{return static_cast<int>(engine()&32767);}};
EffectPlan Fixture(){EffectPlan plan;plan.camera_start=1;plan.object_ids={"cylinder"};BinaryXmlNode cylinder;cylinder.name="CylinderObject";cylinder.attributes={{"Attr_Class_Name",3,cylinder.name},{"Attr_Cylinder_Texture",3,std::string("test.png")},{"Attr_Cylinder_BorderNumb",1,std::int32_t(8)},{"Attr_Cylinder_Asterialize",1,std::int32_t(0)}};plan.templates.emplace("cylinder",cylinder);plan.task_ids={0,0};for(const auto* name:{"ShowobjTask","HideobjTask"}){BinaryXmlNode task;task.name=name;task.attributes={{"Attr_Class_Name",3,task.name},{"Attr_Task_Wait_Mode",1,std::int32_t(1)},{"Attr_Task_Wait_Time",2,0.f},{"Attr_Task_Expect_Time",2,.1f}};plan.task_parameters.push_back(task);}return plan;}
std::vector<std::uint8_t> Png(){const std::string hex="89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c4890000000d49444154789c63f8ffffff7f0009fb03fd2a86e38a0000000049454e44ae426082";std::vector<std::uint8_t> bytes;for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));return bytes;}
void DrainNatural(EffectPlayback& playback,EffectPlayback::Handle h){for(unsigned i=0;i<180 && playback.Dialog(h)->HasTasks();++i)playback.Advance(1.f/60);Check(playback.Contains(h) && !playback.Dialog(h)->HasTasks());playback.Advance(0);Check(!playback.Contains(h));}
}
int main(int argc,char** argv){try{
    if(argc==4){std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);auto read=[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);};EffectDocuments documents(read);EffectTextures textures(read);Random random;ParticleCounts pc;EffectObjectCounts cc;
        EffectPlayback playback([&](std::string_view name){return ReadEffectPlan(*documents.Load(name));},textures,random,pc,cc);auto parent=IdentityEffectMatrix();parent[12]=20;parent[13]=10;GobEffectInstance gob({"H_077",1,0,0},parent,playback);
        std::ifstream steps(argv[3]);std::ofstream out(argv[2]);out<<"{\"frames\":[";std::string action;float dt{};unsigned frames=0;
        while(steps>>action>>dt){Check(frames<4096 && std::isfinite(dt) && dt>=0);if(action=="show")gob.SetVisible(true);else if(action=="hide")gob.SetVisible(false);else Check(action=="none");playback.Advance(dt);if(frames++)out<<',';playback.WriteReport(out);}
        Check(frames>0 && steps.eof());out<<"]}";Check(bool(out));std::cout<<"original H_077 actual-delta and GOB-command replay passed\n";return 0;
    }
    if(argc==3){std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);auto read=[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);};EffectDocuments documents(read);EffectTextures textures(read);Random random;ParticleCounts pc;EffectObjectCounts cc;std::ofstream out(argv[2]);
        {EffectPlayback playback([&](std::string_view name){return ReadEffectPlan(*documents.Load(name));},textures,random,pc,cc);
            auto h=playback.Create("H_077",1,false);Check(playback.Contains(h) && playback.Dialog(h)->Prepared());auto parent=IdentityEffectMatrix();parent[12]=100;parent[13]=10;playback.Attach(h,parent);Check(playback.BeginWaiting(h));
            out<<"{\"natural_frames\":[";for(unsigned i=0;i<180;++i){playback.Advance(1.f/60);if(i)out<<',';playback.WriteReport(out);}Check(playback.ActiveCount()==0 && playback.CachedCount()==1 && playback.PendingCount()==0);
            Check(pc.created==2 && cc.created==1);out<<"],\"natural\":";playback.WriteReport(out);
            h=playback.Create("H_077",1,false);playback.Attach(h,parent);Check(playback.BeginWaiting(h));playback.Advance(.2f);const auto old=h;const auto matrix=playback.Dialog(h)->WorldTransform();playback.Stop(h);
            Check(h.system==0 && h.id==0xffffffffu && !playback.Contains(old) && playback.PendingCount()==1);playback.Visit([&](unsigned,const EffectDialog& d){Check(d.WorldTransform()==matrix);});
            out<<",\"stopped\":";playback.WriteReport(out);out<<",\"pending_frames\":[";
            for(unsigned i=0;i<150;++i){playback.Advance(1.f/60);if(i)out<<',';playback.WriteReport(out);}Check(playback.PendingCount()==0 && playback.CachedCount()==1 && pc.created==2 && cc.created==1);
            out<<"],\"drained\":";playback.WriteReport(out);
            h=playback.Create("H_077",2,false);Check(playback.BeginWaiting(h));for(unsigned i=0;i<360 && playback.Contains(h);++i)playback.Advance(1.f/60);Check(!playback.Contains(h) && playback.CachedCount()==1);
            out<<",\"two_repeats\":";playback.WriteReport(out);playback.Clear();Check(pc.instances==0 && cc.cylinders==0);out<<",\"released_all\":true,\"original_sdk_equivalence\":false}";
        }
        Check(bool(out) && pc.instances==0 && cc.cylinders==0);std::cout<<"original H_077 handle creation/natural retirement/world-preserving stop/pending drain/cache/repeats/release passed\n";return 0;
    }
    EffectTextures textures([](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(name.ends_with(".dds"))return {};return Png();});Random random;ParticleCounts pc;EffectObjectCounts cc;
    {EffectPlayback playback([](std::string_view name){Check(name=="TEST");return Fixture();},textures,random,pc,cc);
        auto h=playback.Create("test",1,false);const auto stale=h;Check(playback.Contains(h) && playback.Dialog(h)->Prepared() && cc.created==1);playback.Advance(2);Check(playback.Dialog(h)->Prepared() && !playback.Dialog(h)->Sequence().Cylinder(0)->Visible());
        playback.SetTransform(h,{16,209,-70},{0,0,0},{1.2f,1.2f,1.2f});
        const auto menu_transform=playback.Dialog(h)->WorldTransform();Check(menu_transform[12]==16 && menu_transform[13]==209 && menu_transform[14]==-70 && menu_transform[0]==1.2f);
        playback.SetTransform(h,{0,1,1},{0,0,0},{1,1,1});
        auto parent=IdentityEffectMatrix();parent[12]=10;parent[13]=20;playback.Attach(h,parent);Check(playback.BeginWaiting(h));playback.Advance(.05f);Check(playback.Dialog(h)->Sequence().Cylinder(0)->Visible());const auto matrix=playback.Dialog(h)->WorldTransform();playback.Stop(h);
        Check(h.system==0 && h.id==0xffffffffu && playback.PendingCount()==1 && playback.ActiveCount()==0);playback.Visit([&](unsigned,const EffectDialog& d){Check(d.WorldTransform()==matrix && d.Sequence().Cylinder(0)->Visible());});
        playback.Advance(.8f);Check(playback.PendingCount()==1 && playback.CachedCount()==0);playback.Advance(0);Check(playback.PendingCount()==0 && playback.CachedCount()==1);
        h=playback.Create("test",1,false);Check(h.id!=stale.id && !playback.Contains(stale) && cc.created==1);Reject([&]{playback.Attach(stale,parent);});Reject([&]{playback.SetTransform(stale,{0,0,0},{0,0,0},{1,1,1});});Check(playback.BeginWaiting(h));DrainNatural(playback,h);Check(playback.CachedCount()==1);
        h=playback.Create("test",2,false);Check(playback.BeginWaiting(h));for(unsigned i=0;i<180 && playback.Contains(h);++i)playback.Advance(1.f/60);Check(!playback.Contains(h));
        h=playback.Create("test",-1,false);Check(playback.BeginWaiting(h));for(unsigned i=0;i<180;++i)playback.Advance(1.f/60);Check(playback.Contains(h));playback.Release(h);Check(h.system==0 && playback.ActiveCount()==0);
        auto first=playback.Create("test",1,false);auto second=playback.Create("test",1,false);Check(playback.ActiveCount()==2 && cc.created==2);playback.Release(first);Check(playback.Contains(second));playback.Release(second);
        EffectPlayback foreign([](auto){return Fixture();},textures,random,pc,cc);auto other=foreign.Create("TEST",1,false);auto copy=other;playback.Release(copy);Check(copy.id==other.id && foreign.Contains(other));foreign.Clear();Check(!foreign.Contains(other));
        auto clipped=playback.Create("test",1,true);Check(playback.Contains(clipped));playback.Release(clipped);Check(playback.ActiveCount()==0);Reject([&]{playback.Create("../test",1,false);});Reject([&]{playback.Advance(-1);});
        auto zero=playback.Create("test",0,false);Check(playback.BeginWaiting(zero));playback.Advance(0);Check(!playback.Contains(zero));
        playback.Clear();Check(pc.instances==0 && cc.cylinders==0);
        {GobEffectInstance gob({"test",1,0,0},parent,playback);gob.SetVisible(false);Check(playback.ActiveCount()==0);
            gob.SetVisible(true);const auto first_handle=gob.Handle();Check(gob.Visible() && playback.Contains(first_handle));
            gob.SetVisible(true);Check(playback.ActiveCount()==1 && !playback.Contains(first_handle) && gob.Handle().id!=first_handle.id);
            playback.Advance(.05f);gob.SetVisible(false);Check(!gob.Visible() && gob.Handle().system==0 && playback.PendingCount()==1);
            gob.SetVisible(false);Check(playback.PendingCount()==1);gob.SetVisible(true);Check(playback.ActiveCount()==1 && playback.PendingCount()==1);
            playback.Clear();Check(!playback.Contains(gob.Handle()));}
        Check(pc.instances==0 && cc.cylinders==0);
        {ArenaResources arena;arena.description.world="Q01";arena.description.scene="Q01";SceneGob gob;gob.name="effect";gob.type=8;gob.position={5,6,7};gob.flags[0]=1;
            BinaryXmlNode root;root.name="PAL4-GameObject-Parameters";BinaryXmlNode config;config.name="PAL4_GameObject-effect";config.attributes={{"PAL4_GameObject-effect-name",3,std::string("test")},{"PAL4_GameObject-effect-times",1,std::int32_t(1)},{"PAL4_GameObject-effect-distance-clip",1,std::int32_t(0)}};root.children.push_back(config);const auto bytes=BinaryXmlConfig::Serialize(root);Check(bytes && gob.config.Parse(*bytes));arena.catalog.gobs.push_back(std::move(gob));
            SceneMinimap map("Q01","Q01",[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});SceneCollision collision;SceneObjects objects(arena,map,[](auto,auto)->std::optional<std::vector<std::uint8_t>>{return {};},&playback);
            objects.BindMovement(collision,[](auto,auto){return true;});
            objects.ActivateInitialEffects();Check(playback.ActiveCount()==1);
            unsigned token{};playback.Visit([&](unsigned t,const EffectDialog&){token=t;});
            objects.ActivateInitialEffects();playback.Visit([&](unsigned t,const EffectDialog&){Check(t==token);});
            std::ostringstream initial;objects.WriteReport(initial);Check(initial.str().find("\"initial_effect_pending\":false")!=std::string::npos);
            Check(objects.SetVisible("effect",true) && playback.ActiveCount()==1 && objects.States()[0].world_attached && !objects.Drawable(0));
            Check(objects.States()[0].movement_registered && objects.States()[0].movement_shapes==0);
            playback.Visit([&](unsigned,const EffectDialog& d){Check(d.Waiting() && d.WorldTransform()[12]==5 && d.WorldTransform()[13]==7);});
            playback.Advance(.05f);Check(objects.SetVisible("effect",false) && playback.PendingCount()==2 && !objects.States()[0].world_attached && !objects.States()[0].movement_registered);playback.Clear();
        }
        Check(pc.instances==0 && cc.cylinders==0);
    }
    Check(pc.instances==0 && cc.cylinders==0);std::cout<<"original effect active/pending timing, cache, repeat, world transform, stale/foreign handles and cleanup passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
