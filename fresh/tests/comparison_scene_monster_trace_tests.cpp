#include "pal4/comparison_scene_monster_trace.h"
#include <iostream>
#include <sstream>
#include <string>
namespace {
void Check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
std::string Export(const pal4::ComparisonSceneMonsterTrace& trace){std::ostringstream out;trace.Write(out);return out.str();}
struct Actor{
    std::array<std::uint32_t,0x740u/4u> words{};
    explicit Actor(const char* name){words[1]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(name));words[0x1f0u/4u]=1u;}
    void* Data(){return words.data();}
};
void DisabledAndSelection(){
    pal4::ComparisonSceneMonsterTrace disabled(false);
    auto ignored=disabled.Begin(reinterpret_cast<void*>(1u),0.017f,0u);disabled.End(ignored,0u);
    Check(Export(disabled).find("\"count\":0")!=std::string::npos,"disabled observer does not read the actor");
    pal4::ComparisonSceneMonsterTrace trace(true,"m02-01-06;m02-01-07",8u);Actor other("m02-01-05");
    auto ticket=trace.Begin(other.Data(),0.017f,0u);Check(!ticket.active,"unselected actor ignored");
    for(const auto* names:{"","m02-01-06;","m02-01-06;m02-01-06","m02\n06"}){
        bool rejected=false;try{pal4::ComparisonSceneMonsterTrace invalid(true,names);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"invalid or duplicate selectors rejected before frames");}
}
void SamplesIdleAndExport(){
    Actor actor("m02-01-06"),target("player");actor.words[0x6e4u/4u]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(target.Data()));
    actor.words[0xacu/4u]=123u;target.words[0xacu/4u]=456u;
    const auto source=actor.words;pal4::ComparisonSceneMonsterTrace trace(true,"m02-01-06",8u);
    auto first=trace.Begin(actor.Data(),0.017f,7u);trace.End(first,99u);
    Check(actor.words==source,"recording never writes actor image");
    {pal4::ComparisonSceneMonsterScope scope(trace,actor.Data(),0.017f,7u);scope.Complete(98u);}
    actor.words[0x6e0u/4u]=1u;
    auto moving=trace.Begin(actor.Data(),0.017f,7u);actor.words[0xacu/4u]=124u;target.words[0xacu/4u]=457u;trace.End(moving,97u);
    auto next=trace.Begin(actor.Data(),0.017f,7u);trace.End(next,96u);
    const auto text=Export(trace);
    Check(text==Export(trace),"repeated export does not consume frames or mutate counters");
    Check(text.find("\"count\":3,\"dropped\":0,\"aborted\":0,\"nested\":0,\"pending\":0,\"idle_skipped\":1")!=std::string::npos,
          "initial idle retained, repeated static idle counted, all pursuit frames retained");
    Check(text.find("\"target_position_bits\":[456,0,0]")!=std::string::npos && text.find("\"target_position_bits\":[457,0,0]")!=std::string::npos,
          "target movement at the real update boundary copied independently");
    Check(text.find("\"frames\":4")!=std::string::npos && text.find("\"mode\":7,\"result\":99")!=std::string::npos,
          "skipped idle still contributes actual frames, original mode/result retained");
}
void FailureAndOverflow(){
    Actor actor("m02-01-06");actor.words[0x6e0u/4u]=1u;
    pal4::ComparisonSceneMonsterTrace trace(true,"m02-01-06",2u);
    {pal4::ComparisonSceneMonsterScope scope(trace,actor.Data(),0.017f,0u);}
    for(unsigned i=0;i<3u;++i){auto t=trace.Begin(actor.Data(),0.017f,0u);trace.End(t,i+1u);trace.End(t,100u);}
    const auto text=Export(trace);
    Check(text.find("\"count\":2,\"dropped\":2,\"aborted\":1,\"nested\":0,\"pending\":0")!=std::string::npos,
          "uncompleted scope records failure, actual overflow retained, finishing twice ignored");
    pal4::ComparisonSceneMonsterTrace nested(true,"m02-01-06",2u);
    auto a=nested.Begin(actor.Data(),0.017f,0u),b=nested.Begin(actor.Data(),0.017f,0u);nested.End(b,1u);nested.End(a,2u);
    Check(Export(nested).find("\"nested\":1,\"pending\":0")!=std::string::npos,"nested updates explicitly invalidate simple sequential comparison");
}
void IdleStateChanges(){
    Actor actor("m02-01-06");pal4::ComparisonSceneMonsterTrace trace(true,"m02-01-06",8u);
    auto first=trace.Begin(actor.Data(),0.017f,0u);trace.End(first,0u);
    auto change=trace.Begin(actor.Data(),0.017f,0u);actor.words[0x5f4u/4u]=2u;trace.End(change,0u);
    Check(Export(trace).find("\"count\":2,\"dropped\":0")!=std::string::npos,
          "idle state change must be retained even when position and pursuit stay unchanged");
}
void LongPatrolPrefix(){
    // live111 lost observations before the first battle: three selected
    // monsters change while patrolling, even when pursuit is zero.
    Actor actor("m02-02-04");pal4::ComparisonSceneMonsterTrace trace(true,"m02-02-04");
    for(unsigned i=0;i<20000u;++i){
        auto ticket=trace.Begin(actor.Data(),0.017f,0x12340000u);
        actor.words[0xacu/4u]=i+1u;trace.End(ticket,i);
    }
    const auto text=Export(trace);
    Check(text.find("\"count\":20000,\"dropped\":0")!=std::string::npos,
          "default observer retains patrol prefix beyond actual live111 overflow");
    Check(text.find("\"sequence\":19999")!=std::string::npos && text==Export(trace),
          "long prefix retains chronological tail and repeated export is non-consuming");
    for(unsigned capacity:{0u,262145u}){
        bool rejected=false;try{pal4::ComparisonSceneMonsterTrace invalid(true,"m02-02-04",capacity);}
        catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"observer capacity remains explicitly bounded");
    }
}
void RealCallBoundary(){
    unsigned calls{};pal4::ComparisonSceneMonsterTrace disabled(false);
    Check(pal4::ObserveComparisonSceneMonsterUpdate(disabled,reinterpret_cast<void*>(1u),0.017f,0u,[&]{++calls;return 0x89abcdefu;})==0x89abcdefu && calls==1u,
          "disabled boundary calls its original once, preserving result and avoiding all actor reads");
    Actor actor("m02-01-06");pal4::ComparisonSceneMonsterTrace trace(true,"m02-01-06",8u);
    const auto result=pal4::ObserveComparisonSceneMonsterUpdate(trace,actor.Data(),0.017f,0u,[&]{++calls;actor.words[0xacu/4u]=12u;return 123u;});
    Check(result==123u && calls==2u,"enabled boundary executes the actual call exactly once");
    bool propagated=false;try{(void)pal4::ObserveComparisonSceneMonsterUpdate(trace,actor.Data(),0.017f,0u,[&]() -> std::uint32_t {++calls;throw std::runtime_error("original failure");});}
    catch(const std::runtime_error& e){propagated=std::string(e.what())=="original failure";}
    Check(propagated && calls==3u && Export(trace).find("\"aborted\":1,\"nested\":0,\"pending\":0")!=std::string::npos,
          "original exception is propagated once and incomplete observation cannot appear successful");
}
}
int main(){DisabledAndSelection();SamplesIdleAndExport();FailureAndOverflow();IdleStateChanges();LongPatrolPrefix();RealCallBoundary();std::cout<<"PASS: selected scene Monster boundary copies/counters/export/failure/overflow\n";}
