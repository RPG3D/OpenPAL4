#include "pal4/portable/monster_behavior_tasks.h"
#include <array>
#include "pal4/monster_behavior_factory_registration.h"
#include "pal4/portable/scene_data.h"
#include "pal4/portable/package.h"
#include "pal4/portable/monster_object_members.h"
#include "../src/monster_behavior_scheduler.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("monster behavior task contract failed");}
template<class F>void Reject(F call){bool rejected=false;try{call();}catch(const std::exception&){rejected=true;}Check(rejected);}
struct Stats {unsigned entered{},updated{},released{},destroyed{},alive{},forced_inactive{},completion_queries{},double_release{};std::vector<int> kinds;};
// 仅合同辅助对象：不驱动原怪物、网格、动画或剧情。
struct Factory final:IMonsterBehaviorActionFactory {
    Stats stats;std::function<void()> on_enter;bool supported{true},throw_create{},throw_enter{},throw_update{},initially_active{true},activate_class_two{true};
    struct Action final:IMonsterBehaviorAction {
        Factory& factory;int kind;bool active,released{};unsigned updates{};
        Action(Factory& f,int k):factory(f),kind(k),active(f.initially_active){++f.stats.alive;}
        ~Action() override {--factory.stats.alive;++factory.stats.destroyed;}
        void Enter() override {if(factory.on_enter)factory.on_enter();if(factory.throw_enter)throw std::runtime_error("contract entry failure");++factory.stats.entered;if(kind==2 && factory.activate_class_two)active=true;}
        bool Active() const override{return active;}
        void SetActive(bool value) override{active=value;if(!value)++factory.stats.forced_inactive;}
        void Update(float) override {if(factory.throw_update)throw std::runtime_error("contract update failure");++updates;++factory.stats.updated;}
        bool Complete() const override {++factory.stats.completion_queries;return updates>=2;}
        void Release() noexcept override {if(released)++factory.stats.double_release;released=true;active=false;++factory.stats.released;}
    };
    bool Supports(std::int32_t kind) const override {return supported && kind>=0 && kind<4;}
    std::unique_ptr<IMonsterBehaviorAction> Create(std::int32_t kind) override {
        if(throw_create)throw std::runtime_error("contract construction failure");
        stats.kinds.push_back(kind);return std::make_unique<Action>(*this,kind);
    }
};
MonsterBehaviorDefinition Definition(int kind=1,float wait=.5f,float duration=1.f,int mode=0){
    MonsterBehaviorDefinition d;d.fuzzy=MonsterBehaviorGroup{{{kind,100,1,wait,duration}},mode};return d;
}
void ObjectConstruction(){
    // 实际成员组合；辅助动作仍为本文件的显式合同工厂，不代表场景移动。
    Factory factory;ObjectMemory storage;pal4::MonsterBehaviorManagerState state{};pal4::BinaryXmlNode memory;
    MonsterObjectMembers members(state,0x10000,storage,factory,Definition(1,0,.25f),memory,[]{return 0u;});
    Check(state.words[0]==0x842c98 && state.words[0x170/4]==0x842c8c && state.words[0x1ec/4]==0x842c80);
    Check(state.words[0x6cc/4]==0x10000 && state.words[0x6dc/4]==0x10000 && state.words[0x6d4/4]==0x849384);
    Check(state.words[0x654/4]==0 && state.words[0x6e0/4]==0 && state.words[0x6f8/4]==0x3f800000 && state.words[0x6fc/4]==0x3f800000);
    Check(storage.AllocationCount()==4 && memory.name=="behAIAtomicMgr-memory" && state.words[0x668/4]==members.Scheduler().MemoryToken());
    Check(state.words[0x688/4+1]!=0 && state.words[0x688/4+2]==0 && (state.words[0x688/4+3]&0xff)==1);
    Check(state.words[0x6a0/4]==0 && state.words[0x6a8/4]==4); // 不伪造原STL节点。
    const std::uint32_t expected[]{0x429c70,0x429a30,0x42a130,0x42a360};
    for(int id=0;id<4;++id)Check(members.Tasks().FactoryAddress(id)==expected[id]);
    const auto before=state.words;const auto count=storage.AllocationCount();state.words[0x170/4+2]=123;const auto invalid=state.words;
    Reject([&]{members.Close();});Check(state.words==invalid && storage.AllocationCount()==count && !members.Closed());state.words=before;
    members.PrepareBehavior();Reject([&]{members.PrepareBehavior();});Check(storage.AllocationCount()>4);
    Check(members.Tasks().Start(Definition(1,0,.25f)));members.Tasks().Advance(0);members.Tasks().Advance(.125f);
    Check(factory.stats.entered==1 && factory.stats.updated==1);
    members.Close();members.Close();Check(members.Closed() && storage.AllocationCount()==0 && factory.stats.alive==0 && factory.stats.released==1);
    Check(!state.words[0x668/4] && !state.words[0x688/4+1] && !state.words[0x6a8/4]);Reject([&]{members.Tasks();});Reject([&]{members.PrepareBehavior();});
    for(unsigned budget:{16u,32u,64u,100u}){
        ObjectMemory limited({0x10000000,0x70000000,budget,100});pal4::MonsterBehaviorManagerState image{};pal4::BinaryXmlNode remembered;remembered.name="saved-memory";
        Reject([&]{MonsterObjectMembers fail(image,0x20000,limited,factory,Definition(),remembered,[]{return 0u;});});
        Check(limited.AllocationCount()==0 && remembered.name=="saved-memory" && image.words==pal4::MonsterBehaviorManagerState{}.words);
    }
    ObjectMemory limited({0x10000000,0x70000000,150,100});pal4::MonsterBehaviorManagerState image{};pal4::BinaryXmlNode remembered;
    MonsterObjectMembers unprepared(image,0x20000,limited,factory,Definition(),remembered,[]{return 0u;});
    Reject([&]{unprepared.PrepareBehavior();});Check(unprepared.Scheduler().Closed() && limited.AllocationCount()==3);
    unprepared.Close();Check(limited.AllocationCount()==0);
}
void Timing(){
    Factory f;MonsterBehaviorTasks tasks(f,[]{throw std::logic_error("unexpected RNG");return 0u;});
    Check(tasks.Start(Definition()) && !tasks.Ready() && tasks.TaskCount()==1);
    tasks.Advance(.25f);Check(f.stats.entered==0);
    tasks.Advance(.25f);Check(f.stats.entered==1 && f.stats.updated==0);
    tasks.Advance(.5f);Check(f.stats.updated==1 && f.stats.released==0);
    tasks.Advance(.5f);Check(f.stats.updated==2 && f.stats.released==1 && !tasks.Ready() && tasks.TaskCount()==1);
    tasks.Advance(0);Check(tasks.Ready() && tasks.TaskCount()==0 && tasks.HelperCount()==0 && f.stats.alive==0);
    Check(f.stats.double_release==0 && f.stats.completion_queries==0);
    auto chain=Definition(1,.5f,.5f);chain.fuzzy->choices.push_back({0,1,1,0,.5f});
    Check(tasks.Start(chain));tasks.Advance(.5f);Check(f.stats.entered==2);
    tasks.Advance(.5f);Check(f.stats.entered==2); // previous完成当帧，严格大于0的间隔尚未满足。
    tasks.Advance(0);Check(f.stats.entered==2);
    tasks.Advance(.125f);Check(f.stats.entered==3);
    tasks.Advance(.5f);tasks.Advance(0);Check(tasks.Ready() && f.stats.released==3);
    Reject([&]{tasks.Advance(-1);});
}
void CompletionAndCancellation(){
    for(int kind=0;kind<4;++kind){
        Factory f;MonsterBehaviorTasks tasks(f,[]{return 0u;});
        Check(tasks.Start(Definition(kind,0,kind==2?1000.f:.25f)));tasks.Advance(0);
        Check(f.stats.entered==1 && f.stats.forced_inactive==(kind==2?1u:0u));
        tasks.Advance(.125f);Check(f.stats.released==0);
        tasks.Advance(.125f);Check(f.stats.released==1);tasks.Advance(0);
        Check(tasks.Ready() && f.stats.alive==0 && f.stats.destroyed==1);
        Check(f.stats.completion_queries==(kind==2?2u:0u));
    }
    Factory waiting;{MonsterBehaviorTasks tasks(waiting,[]{return 0u;});tasks.Start(Definition());tasks.Cancel();Check(tasks.Ready() && tasks.HelperCount()==0);}Check(waiting.stats.entered==0);
    Factory running;{MonsterBehaviorTasks tasks(running,[]{return 0u;});tasks.Start(Definition(1,0,-1));tasks.Advance(0);tasks.Advance(100);Check(!tasks.Ready());}
    Check(running.stats.released==1 && running.stats.alive==0);
    Factory inactive;inactive.initially_active=false;
    {MonsterBehaviorTasks tasks(inactive,[]{return 0u;});tasks.Start(Definition(0,0,-1));tasks.Advance(0);tasks.Advance(1);tasks.Cancel();}
    Check(inactive.stats.updated==0 && inactive.stats.released==0 && inactive.stats.alive==0);
}
void SelectionAndFailures(){
    Factory f;unsigned random_calls{};unsigned sample=20000;MonsterBehaviorTasks tasks(f,[&]{++random_calls;return sample;});
    auto d=Definition(0,0,.25f,1);d.fuzzy->choices[0].proportion=25;d.fuzzy->choices.push_back({3,75,1,0,.25f});
    Check(tasks.Start(d));tasks.Advance(0);Check(random_calls==1 && f.stats.kinds.back()==3);tasks.Cancel();
    d.fuzzy->mode=2;sample=0;Check(tasks.Start(d));tasks.Advance(0);Check(random_calls==2 && f.stats.kinds.back()==0);tasks.Cancel();
    Check(tasks.Start(Definition(3,0,.25f,2)));tasks.Advance(0);Check(random_calls==2);tasks.Cancel();
    sample=32767;auto miss=Definition(0,0,.25f,1);miss.fuzzy->choices[0].proportion=1;
    Check(!tasks.Start(miss) && tasks.Ready() && tasks.TaskCount()==0 && random_calls==3);
    auto empty=Definition(0,0,0,2);empty.fuzzy->choices.clear();Reject([&]{tasks.Start(empty);});Check(tasks.TaskCount()==0 && tasks.HelperCount()==0);
    Check(tasks.Start(Definition(-1)) && tasks.TaskCount()==0 && tasks.Ready());
    auto partial=Definition();partial.fuzzy->mode.reset();Reject([&]{tasks.Start(partial);});
    f.supported=false;Reject([&]{tasks.Start(Definition());});Check(tasks.Ready() && tasks.TaskCount()==0);f.supported=true;
    for(int failure=0;failure<3;++failure){
        f.throw_create=failure==0;f.throw_enter=failure==1;f.throw_update=failure==2;
        Check(tasks.Start(Definition(1,0,1)));
        if(failure<2)Reject([&]{tasks.Advance(0);});
        else{tasks.Advance(0);Reject([&]{tasks.Advance(.125f);});}
        Check(tasks.Ready() && tasks.TaskCount()==0 && tasks.HelperCount()==0 && f.stats.alive==0);
        Reject([&]{tasks.Advance(0);});
    }
    f.throw_create=f.throw_enter=f.throw_update=false;
    Check(tasks.Start(Definition(1,0,0)));tasks.Advance(0);tasks.Advance(0);tasks.Advance(0);Check(tasks.Ready());
    Check(f.stats.double_release==0);
    f.on_enter=[&]{tasks.Cancel();};Check(tasks.Start(Definition(1,0,1)));
    Reject([&]{tasks.Advance(0);});Check(tasks.TaskCount()==0 && tasks.HelperCount()==0 && f.stats.alive==0);
    f.on_enter={};std::ostringstream report;tasks.WriteReport(report);
    Check(report.str().find("\"temporary_blocks\":0")!=std::string::npos);
}
void FactoryRegistration(){
    const std::array<std::uint32_t,4> expected{0x429c70,0x429a30,0x42a130,0x42a360};
    struct Sink final:pal4::IMonsterBehaviorFactoryRegistrationRuntime {
        std::vector<std::array<std::uint32_t,3>> records;unsigned fail_at=99;
        void InsertBehavior(std::uint32_t tree,std::uint32_t kind,std::uint32_t function) override{records.push_back({tree,kind,function});if(kind==fail_at)throw std::runtime_error("registration failure");}
    } sink;
    pal4::RegisterMonsterBehaviorFactories(0x12340,sink);Check(sink.records.size()==4);
    for(unsigned i=0;i<4;++i)Check(sink.records[i]==std::array<std::uint32_t,3>{0x12340,i,expected[i]});
    Sink failed;failed.fail_at=2;Reject([&]{pal4::RegisterMonsterBehaviorFactories(0x12340,failed);});Check(failed.records.size()==3 && failed.records.back()[1]==2);
    Factory f;MonsterBehaviorTasks tasks(f,[]{return 0u;});Check(tasks.RegisteredFactoryCount()==4);
    for(unsigned i=0;i<4;++i)Check(tasks.FactoryAddress(static_cast<std::int32_t>(i))==expected[i]);
    Check(!tasks.FactoryAddress(-1) && !tasks.FactoryAddress(4) && !tasks.FactoryAddress(0x7fffffff));
    auto unknown=Definition();unknown.fuzzy->choices[0].class_id=4;Reject([&]{(void)tasks.Start(unknown);});Check(tasks.Ready() && tasks.TaskCount()==0 && f.stats.alive==0);
}

void ContainerImages(){
    Factory f;MonsterBehaviorTasks tasks(f,[]{return 0u;});std::array<std::uint32_t,4> image{0xaabbccff,0,0,0x112233ff};const auto original=image;
    Reject([&]{(void)tasks.BindContainerImage(image,1);});Check(image==original);
    Check(tasks.Start(Definition(1,0,.25f)));const auto binding=tasks.BindContainerImage(image,0x80000);
    Check(tasks.OwnsContainerImage(binding) && image[0]==0xaabbcc00 && image[1] && image[2]==1 && image[3]==0x11223300);
    Reject([&]{(void)tasks.BindContainerImage(image,0x80000);});Reject([&]{tasks.UnbindContainerImage(binding+1);});
    tasks.Advance(0);tasks.Advance(.25f);Check(image[2]==1 && (image[3]&0xff)==0);tasks.Advance(0);Check(image[2]==0 && image[3]==0x11223301);
    const auto valid=image;image[2]=99;Reject([&]{tasks.Advance(0);});Reject([&]{(void)tasks.Ready();});image=valid;tasks.Cancel();Check(image[2]==0);
    Check(tasks.Start(Definition(0,0,.25f)) && image[2]==1);tasks.UnbindContainerImage(binding);const auto detached=image;
    tasks.Advance(0);Check(image==detached && !tasks.OwnsContainerImage(binding));
    std::array<std::uint32_t,4> other{};const auto next=tasks.BindContainerImage(other,0x90000);Check(next!=binding && other[2]==1);
    tasks.Cancel();Check(other[2]==0 && other[3]==1);tasks.UnbindContainerImage(next);Check(f.stats.alive==0 && !f.stats.double_release);
    Factory bad;bad.throw_create=true;MonsterBehaviorTasks failed(bad,[]{return 0u;});std::array<std::uint32_t,4> failed_image{};const auto failed_binding=failed.BindContainerImage(failed_image,0xa0000);
    Check(failed.Start(Definition(1,0,.25f)));Reject([&]{failed.Advance(0);});Check(failed_image[2]==0 && failed_image[3]==1 && bad.stats.alive==0);failed.UnbindContainerImage(failed_binding);
}

}
int main(int argc,char** argv){try{
    if(argc==6 && (std::string_view(argv[1])=="source" || std::string_view(argv[1])=="construction-source")){
        const bool construction=std::string_view(argv[1])=="construction-source";
        std::ifstream file(argv[2],std::ios::binary);PackageReader package(file);
        const auto bytes=package.Read(std::string(argv[3])+"\\"+argv[4]+"\\mstInfo.mst");
        std::istringstream stream(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
        const auto records=ReadSceneMonsters(stream);std::ofstream out(argv[5]);
        out<<"{\"contract_helpers_only\":true,\"gameplay_executed\":false,\"records\":[";bool first=true;
        std::uint32_t ordinal{};for(const auto& record:records){
            const auto d=ReadMonsterBehaviorDefinition(record.behavior);if(!construction && !d.fuzzy)continue;
            Factory f;ObjectMemory storage;pal4::MonsterBehaviorManagerState image{};pal4::BinaryXmlNode memory;
            std::unique_ptr<MonsterObjectMembers> members;std::unique_ptr<MonsterBehaviorTasks> standalone;
            if(construction){members=std::make_unique<MonsterObjectMembers>(image,0x10000u+(ordinal++)*0x10000u,storage,f,d,memory,[]{return 0u;});members->PrepareBehavior();}
            else standalone=std::make_unique<MonsterBehaviorTasks>(f,[]{return 0u;});
            auto& tasks=construction?members->Tasks():*standalone;Check(tasks.Start(d)==d.fuzzy.has_value());unsigned frames{};
            while(!tasks.Ready() && frames<10000){tasks.Advance(1.f/60);++frames;}
            Check(tasks.Ready() && tasks.TaskCount()==0 && tasks.HelperCount()==0 && f.stats.alive==0);
            if(!first)out<<',';first=false;
            out<<"{\"name\":"<<std::quoted(record.name)<<",\"frames\":"<<frames<<",\"entered\":"<<f.stats.entered
                <<",\"updated\":"<<f.stats.updated<<",\"released\":"<<f.stats.released<<",\"queue\":";tasks.WriteReport(out);
            if(construction){
                out<<",\"fuzzy_behavior\":"<<(d.fuzzy?"true":"false")<<",\"constructed_words\":[";for(std::size_t i=0;i<image.words.size();++i){if(i)out<<',';out<<image.words[i];}out<<']';
                members->Close();Check(storage.AllocationCount()==0);out<<",\"remaining_allocations\":0";
            }
            out<<'}';
        }
        out<<"]}";Check(bool(out));return 0;
    }
    ObjectConstruction();FactoryRegistration();ContainerImages();Timing();CompletionAndCancellation();SelectionAndFailures();
    std::cout<<"original behavior selection, task timing, completion, cancellation and helper failure contracts passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
