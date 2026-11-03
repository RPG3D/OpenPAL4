#include "pal4/portable/effect_sequence.h"
#include "pal4/portable/effect_dialog.h"
#include "pal4/portable/package.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <source_location>
#include <sstream>
#include <stdexcept>
using namespace pal4;using namespace pal4::portable;
namespace {
void Check(bool v,std::source_location at=std::source_location::current()){if(!v)throw std::runtime_error("effect sequence contract at "+std::to_string(at.line()));}
template<class F> void Reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}Check(rejected);}
struct Random final:ParticleRandom {std::mt19937 engine;unsigned Time() override{return 1234;}void Seed(unsigned n) override{engine.seed(n);}int Next15() override{return static_cast<int>(engine() & 32767);}};
BinaryXmlNode Task(std::string name,int mode,float wait,float duration){BinaryXmlNode n;n.name=name;n.attributes={{"Attr_Class_Name",3,name},{"Attr_Task_Wait_Mode",1,std::int32_t(mode)},{"Attr_Task_Wait_Time",2,wait},{"Attr_Task_Expect_Time",2,duration}};return n;}
EffectPlan Fixture(){EffectPlan p;p.object_ids={"particle","cylinder"};BinaryXmlNode particle;particle.name="RtkPrtStdAtomicObject";particle.attributes={{"Attr_Class_Name",3,particle.name},{"Attr_Prt_Texture",3,std::string("test.png")},{"Attr_Prt_MaxPrt",1,std::int32_t(8)},{"Attr_Prt_PrtEmit",1,std::int32_t(2)},{"Attr_Prt_PrtEmit_Bias",1,std::int32_t(0)},{"Attr_Prt_PrtLife",2,.5f},{"Attr_Prt_PrtLife_Bias",2,0.f},{"Attr_Prt_ColorGradual",1,std::int32_t(0)}};p.templates.emplace("particle",particle);
    BinaryXmlNode cylinder;cylinder.name="CylinderObject";cylinder.attributes={{"Attr_Class_Name",3,cylinder.name},{"Attr_Cylinder_Texture",3,std::string("test.png")},{"Attr_Cylinder_BorderNumb",1,std::int32_t(8)},{"Attr_Cylinder_Asterialize",1,std::int32_t(0)},{"Attr_Cylinder_Angel",2,360.f}};p.templates.emplace("cylinder",cylinder);
    auto decrease=Task("DecressTask",2,0,.4f);decrease.attributes.insert(decrease.attributes.end(),{{"Attr_Task_Decress_Decress",1,std::int32_t(1)},{"Attr_Task_Decress_Time",2,1.f}});
    p.task_ids={0,0,0,1,1};p.task_parameters={Task("ShowobjTask",2,.1f,0),decrease,Task("HideobjTask",1,0,0),Task("ShowobjTask",2,0,0),Task("HideobjTask",1,0,0)};
    BinaryXmlNode unused;unused.name="UnsupportedObject";p.templates.emplace("unused",unused);return p;
}
std::vector<std::uint8_t> Png(){const std::string hex="89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c4890000000d49444154789c63f8ffffff7f0009fb03fd2a86e38a0000000049454e44ae426082";std::vector<std::uint8_t> bytes;for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));return bytes;}
}
int main(int argc,char** argv){try{
    if(argc==4 && std::string_view(argv[1])=="h076"){
        std::ifstream stream(argv[2],std::ios::binary);PackageReader package(stream);
        const auto read=[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{return package.ReadIfPresent(member);};
        EffectDocuments documents(read);EffectTextures textures(read);Random random;ParticleCounts pc;EffectObjectCounts cc;
        std::ofstream report(argv[3]);
        {EffectDialog dialog(ReadEffectPlan(*documents.Load("H_076")),textures,random,pc,cc);
            Check(dialog.Prepare() && dialog.BeginWaiting() && dialog.QueuedObjects()==3);
            Check(dialog.Sequence().ObjectCount()==4 && dialog.Sequence().IsDummy(3) && pc.instances==3 && cc.cylinders==0);
            for(unsigned i=0;i<60;++i){dialog.AdvanceTasks(1.f/60);dialog.AdvanceObjects(1.f/60);}
            for(unsigned i=0;i<3;++i){const auto* particle=dialog.Sequence().Particle(i);Check(particle && particle->Visible() && !particle->Snapshot().empty());}
            dialog.Sequence().WriteReport(report);
            Check(!dialog.RenderAllowed(3,{0,0,0}));dialog.Reset();Check(dialog.QueuedObjects()==0);
        }
        Check(bool(report) && pc.instances==0 && cc.cylinders==0);std::cout<<"original H_076: three smoke emitters, retail dummy skipped, cleanup passed\n";return 0;
    }
    if(argc==3 || argc==4){std::vector<float> deltas(180,1.f/60);if(argc==4){deltas.clear();std::ifstream times(argv[3]);float dt;while(times>>dt){Check(std::isfinite(dt)&&dt>=0&&deltas.size()<4096);deltas.push_back(dt);}Check(!deltas.empty());}
        std::ifstream stream(argv[1],std::ios::binary);PackageReader package(stream);auto read=[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(member))return {};return package.Read(member);};EffectDocuments documents(read);EffectTextures textures(read);Random random;ParticleCounts pc;EffectObjectCounts cc;std::ofstream out(argv[2]);
        {EffectSequence sequence(*documents.Load("H_077"),textures,random,pc,cc);Check(sequence.ObjectCount()==3 && sequence.ObjectName(0)=="P_01" && sequence.ObjectName(1)=="lz" && sequence.ObjectName(2)=="yzt3");out<<"{\"frames\":[";
            bool saw_particle=false,saw_delayed=false,saw_cylinder=false;
            for(std::size_t i=0;i<deltas.size();++i){sequence.Advance(deltas[i]);saw_particle|=!sequence.Particle(1)->Snapshot().empty();saw_delayed|=sequence.Particle(0)->Visible() && !sequence.Particle(0)->Snapshot().empty();saw_cylinder|=sequence.Cylinder(2)->Visible();if(i)out<<',';sequence.WriteReport(out);}
            Check(sequence.TasksFinished() && saw_particle && saw_delayed && saw_cylinder && !sequence.Particle(0)->Visible() && !sequence.Particle(1)->Visible() && !sequence.Cylinder(2)->Visible());out<<"],\"final\":";sequence.WriteReport(out);
            sequence.Restart();Check(!sequence.TasksFinished() && pc.created==2 && cc.created==1);sequence.Advance(.1f);sequence.Stop();Check(sequence.TasksFinished() && !sequence.Particle(1)->Visible());out<<",\"cached_restart\":true}";}
        Check(pc.instances==0 && cc.cylinders==0 && bool(out));
        Random dialog_random;ParticleCounts dialog_pc;EffectObjectCounts dialog_cc;std::ofstream lifecycle(std::string(argv[2])+".dialog.json");
        auto gob_plan=ReadEffectPlan(*documents.Load("H_077"));gob_plan.distance_clip=false; // Jeffect001原GobEffect参数覆盖EFT默认值。
        {EffectDialog dialog(gob_plan,textures,dialog_random,dialog_pc,dialog_cc);
            Check(dialog.Prepare() && !dialog.Waiting() && dialog.QueuedObjects()==3 && dialog.BeginWaiting());
            lifecycle<<"{\"frames\":[";
            for(std::size_t i=0;i<deltas.size();++i){dialog.AdvanceTasks(deltas[i]);dialog.AdvanceObjects(deltas[i]);if(i)lifecycle<<',';dialog.Sequence().WriteReport(lifecycle);}
            Check(!dialog.HasTasks() && !dialog.Waiting() && dialog.QueuedObjects()==0);
            Check(dialog_pc.created==2 && dialog_cc.created==1);lifecycle<<"],\"natural_reset\":true";
            Check(dialog.Prepare() && dialog.BeginWaiting());dialog.AdvanceTasks(.1f);dialog.AdvanceObjects(.1);
            auto parent=IdentityEffectMatrix();parent[12]=100;dialog.SetTransform({1,2,3},{0,90,0},{1,1,1});dialog.SetParentTransform(parent);
            Check(dialog.WorldTransform()[12]==101);const auto duration=dialog.Stop(false);
            Check(duration==2 && !dialog.HasTasks() && dialog.QueuedObjects()==3 && dialog.Sequence().Particle(1)->Visible());
            lifecycle<<",\"stop_duration\":"<<duration<<",\"stopped\":";dialog.Sequence().WriteReport(lifecycle);
            for(unsigned i=0;i<150;++i)dialog.AdvanceObjects(1./60);
            for(const auto& p:dialog.Sequence().Particle(1)->Snapshot())Check(p.color[3]==0);
            dialog.Reset();Check(dialog.QueuedObjects()==0);Check(dialog_pc.created==2 && dialog_cc.created==1);
            lifecycle<<",\"cached_resume\":true,\"timed_drain\":true,\"sdk_equivalence\":false}";
        }
        Check(dialog_pc.instances==0 && dialog_cc.cylinders==0 && bool(lifecycle));
        std::cout<<"original H_077 three objects/ten tasks plus Dialog prepare/wait/natural reset/cached resume/stop/drain/release passed\n";return 0;
    }
    EffectTextures textures([](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(name.ends_with(".dds"))return {};return Png();});Random random;ParticleCounts pc;EffectObjectCounts cc;
    {EffectSequence sequence(Fixture(),textures,random,pc,cc);Check(sequence.ObjectCount()==2 && !sequence.Particle(0)->Visible());sequence.Advance(.05f);Check(!sequence.Particle(0)->Visible() && sequence.Particle(0)->Snapshot().empty() && sequence.Cylinder(1)->Visible());sequence.Advance(.05f);Check(sequence.Particle(0)->Visible());
        for(unsigned i=0;i<30 && !sequence.TasksFinished();++i)sequence.Advance(.05f);Check(sequence.TasksFinished() && !sequence.Particle(0)->Visible() && !sequence.Cylinder(1)->Visible());sequence.Restart();Check(pc.created==1 && cc.created==1);sequence.Advance(.1f);sequence.Stop();Check(!sequence.Particle(0)->Visible());}
    Check(pc.instances==0 && cc.cylinders==0);
    {EffectSequence deferred(Fixture(),textures,random,pc,cc,false);Check(!deferred.HasTasks());
        deferred.ReleaseObject(1);Reject([&]{deferred.BeginTasks(false);});Reject([&]{deferred.ResetObject(1);});
        deferred.AdvanceObjects(.1);deferred.EnsureObject(1);deferred.BeginTasks(false);
        Reject([&]{deferred.ReleaseObject(0);});deferred.DestroyTasks();deferred.ReleaseObject(0);deferred.AdvanceObjects(.1);}
    Check(pc.instances==0 && cc.cylinders==0);
    {
        auto p=Fixture();p.camera_start=1;p.object_ids.push_back("editor");
        BinaryXmlNode dummy;dummy.name="ClumpObject";dummy.attributes={{"Attr_Class_Name",3,dummy.name},{"Attr_Dumy",1,std::int32_t(1)}};
        p.templates.emplace("editor",dummy);p.task_ids.push_back(2);
        // Retail skips even an otherwise unsupported task on an editor dummy.
        p.task_parameters.push_back(Task("EditorOnlyUnsupportedTask",2,0,-1));
        EffectDialog dialog(p,textures,random,pc,cc);
        Check(dialog.Sequence().IsDummy(2) && dialog.Sequence().HasObject(2));
        Check(dialog.Prepare() && dialog.QueuedObjects()==2 && dialog.BeginWaiting());
        dialog.AdvanceTasks(.2f);dialog.AdvanceObjects(.2f);
        Check(dialog.Sequence().Particle(0)->Visible());
        dialog.Reset();Check(dialog.QueuedObjects()==0 && !dialog.HasTasks());
        p.templates["editor"].attributes.back().value=std::int32_t(0);
        Reject([&]{EffectSequence unsupported(p,textures,random,pc,cc);});
    }
    Check(pc.instances==0 && cc.cylinders==0);
    auto dialog_plan=Fixture();dialog_plan.camera_start=1;
    {const auto particles_created=pc.created,cylinders_created=cc.created;
        EffectDialog dialog(dialog_plan,textures,random,pc,cc);Check(!dialog.HasTasks() && dialog.QueuedObjects()==0);
        Reject([&]{dialog.SetParentTransform(IdentityEffectMatrix());});
        Check(dialog.Prepare() && dialog.HasTasks() && !dialog.Waiting() && dialog.QueuedObjects()==2);
        dialog.AdvanceTasks(1);Check(!dialog.Sequence().Cylinder(1)->Visible());
        dialog.SetTransform({2,3,4},{0,0,0},{2,3,4});auto parent=IdentityEffectMatrix();parent[12]=10;parent[13]=20;parent[14]=30;
        dialog.SetParentTransform(parent);Check(dialog.WorldTransform()[12]==12 && dialog.WorldTransform()[13]==23);
        Check(dialog.Sequence().Cylinder(1)->Transform()[12]==12);
        Check(dialog.BeginWaiting() && dialog.Waiting() && !dialog.BeginWaiting());
        dialog.AdvanceTasks(.1f);dialog.AdvanceObjects(.1);Check(dialog.Sequence().Particle(0)->Visible());
        for(unsigned i=0;i<30 && dialog.HasTasks();++i){dialog.AdvanceTasks(.05f);dialog.AdvanceObjects(.05);}
        Check(!dialog.HasTasks() && !dialog.Waiting() && dialog.QueuedObjects()==0);
        Check(!dialog.Sequence().Particle(0)->Visible() && !dialog.Sequence().Cylinder(1)->Visible());
        Check(dialog.Prepare() && dialog.BeginWaiting());Check(pc.created==particles_created+1 && cc.created==cylinders_created+1);
        dialog.AdvanceTasks(.1f);dialog.AdvanceObjects(.1);const auto live=dialog.Sequence().Particle(0)->Snapshot().size();Check(live>0);
        const auto duration=dialog.Stop(false);Check(duration>=.5 && !dialog.HasTasks() && dialog.QueuedObjects()==2 && dialog.Sequence().Particle(0)->Visible());
        Check(dialog.Sequence().Particle(0)->Snapshot().size()==live);dialog.AdvanceObjects(.01);Check(dialog.Sequence().Particle(0)->Snapshot().size()<=live);
        dialog.AdvanceObjects(2);for(const auto& p:dialog.Sequence().Particle(0)->Snapshot())Check(p.color[3]==0);dialog.Reset();Check(dialog.QueuedObjects()==0);
        Check(dialog.Prepare() && dialog.BeginWaiting());dialog.SetTransform({2,3,4},{0,0,0},{1,1,1});dialog.SetParentTransform(parent);
        (void)dialog.Stop(true);Check(dialog.WorldTransform()[12]==2);dialog.Reset();
        Reject([&]{dialog.AdvanceTasks(-1);});Reject([&]{dialog.AdvanceObjects(-1);});
        auto invalid=IdentityEffectMatrix();invalid[3]=1;Reject([&]{dialog.SetParentTransform(invalid);});
    }
    Check(pc.instances==0 && cc.cylinders==0);
    auto clipped=dialog_plan;clipped.distance_clip=true;{EffectDialog valid(clipped,textures,random,pc,cc);Check(valid.Prepare());const auto& m=valid.Sequence().Particle(0)->Transform();const Point3 center{m[12],m[13],m[14]};Check(valid.RenderAllowed(0,center,0));Check(valid.RenderAllowed(0,{center[0]+1200,center[1],center[2]}));Check(!valid.RenderAllowed(0,{center[0]+1201,center[1],center[2]}));Reject([&]{valid.RenderAllowed(0,center,-1);});}
    auto probability=dialog_plan;probability.camera_start=.5f;Reject([&]{EffectDialog invalid(probability,textures,random,pc,cc);});
    auto bad_tasks=dialog_plan;bad_tasks.task_ids[0]=99;{EffectDialog invalid(bad_tasks,textures,random,pc,cc);Reject([&]{(void)invalid.Prepare();});}
    Check(pc.instances==0 && cc.cylinders==0);
    auto missing=Fixture();missing.task_ids[0]=99;Reject([&]{EffectSequence invalid(missing,textures,random,pc,cc);});Check(pc.instances==0 && cc.cylinders==0);
    auto referenced=Fixture();referenced.object_ids.push_back("unused");Reject([&]{EffectSequence invalid(referenced,textures,random,pc,cc);});Check(pc.instances==0 && cc.cylinders==0);
    std::cout<<"cross-object task ordering, referenced-only creation, Decress, restart and rollback passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
