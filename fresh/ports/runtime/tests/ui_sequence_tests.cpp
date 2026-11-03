#include "pal4/portable/ui_sequence.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("UI sequence contract failed");}
template<class F>void Reject(F action){bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}Check(rejected);}
std::string Xml(unsigned loops){return "<SequenceImage Name=\"probe_sequence\" LoopCount=\""+std::to_string(loops)+"\" FrameCount=\"2\"><ImageFrame Index=\"0\" Duration=\"0.2\" Imageset=\"probe_set\" Image=\"frame_a\"/><ImageFrame Index=\"1\" Duration=\"0.2\" Imageset=\"probe_set\" Image=\"frame_b\"/></SequenceImage>";}
}
int main(){try{
    const float steps[]{0,.1f,.1f,.01f,.2f,.4f,.01f,1,.31f,.1f,.1f,.1f,.7f,.19f,.01f};
    const unsigned expected[]{0,0,1,1,0,0,0,0,0,1,1,0,1,0,0};
    UiSequencePlayback infinite(ParseUiSequence(Xml(0)));
    for(unsigned i=0;i<15;++i){Check(infinite.Advance(steps[i])==expected[i] && !infinite.Finished());if(i==7)Check(std::abs(infinite.Elapsed()-.62f)<1e-6f);}
    for(unsigned loops:{1u,2u}){
        UiSequencePlayback finite(ParseUiSequence(Xml(loops)));
        for(unsigned i=0;i<15;++i){const auto frame=finite.Advance(steps[i]);Check(frame==(loops==1 && i>=4?1u:loops==2 && i>=5?0u:expected[i]));Check(finite.Finished()==(i>=(loops==1?4u:5u)));}
    }
    const auto before=infinite.Elapsed();Reject([&]{infinite.Advance(-1);});Reject([&]{infinite.Advance(std::numeric_limits<float>::infinity());});Check(infinite.Elapsed()==before);
    for(const auto& bad:{std::string("<x/>"),std::string("<SequenceImage Name=\"a\" LoopCount=\"0\" FrameCount=\"1\"/>"),std::string("<SequenceImage Name=\"a\" LoopCount=\"-1\" FrameCount=\"1\"/>")})Reject([&]{ParseUiSequence(bad);});
    auto bad=Xml(0);bad.replace(bad.find("Index=\"1\""),9,"Index=\"0\"");Reject([&]{ParseUiSequence(bad);});
    bad=Xml(0);bad.replace(bad.find("Duration=\"0.2\""),14,"Duration=\"nan\"");Reject([&]{ParseUiSequence(bad);});
    auto definition=ParseUiSequence(Xml(0));definition.frames[0].duration=0;Reject([&]{UiSequencePlayback invalid(definition);});
    std::cout<<"UI sequence PC infinite/finite frame traces, long-step carry, completion hold, XML validation and invalid time passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
