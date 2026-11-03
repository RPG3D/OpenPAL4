#include "pal4/comparison_scene_motion_trace.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {
void Check(bool condition,const char* message){
    if(!condition){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}
}
std::string Export(const pal4::ComparisonSceneMotionTrace& trace){
    std::ostringstream out;trace.Write(out);return out.str();
}
unsigned Capacity(const std::string& text){
    const auto at=text.find("\"capacity\":");
    Check(at!=std::string::npos,"export includes the actual bounded capacity");
    return static_cast<unsigned>(std::stoul(text.substr(at+11u)));
}
void DisabledAndOwnedSamples(){
    pal4::ComparisonSceneMotionSample before{},after{};
    before.position[0]=12345u;after.position[0]=67890u;
    pal4::ComparisonSceneMotionTrace disabled(false);
    disabled.RecordStep(1u,0.017f,before,after);
    Check(Export(disabled).find("\"attempted\":0,\"dropped\":0,\"records\":[]")!=std::string::npos,
          "disabled observer does not record or count frames");
    pal4::ComparisonSceneMotionTrace enabled(true);
    enabled.RecordStep(0u,0.017f,before,after);
    Check(Export(enabled).find("\"attempted\":0")!=std::string::npos,"idle input is ignored");
    enabled.RecordStep(1u,0.017f,before,after);
    before.position[0]=1u;after.position[0]=2u;
    const auto first=Export(enabled);
    Check(first.find("\"position_bits\":[12345,0,0]")!=std::string::npos &&
          first.find("\"position_bits\":[67890,0,0]")!=std::string::npos,
          "observer owns the copied bit patterns after caller storage changes");
    Check(first==Export(enabled),"export does not mutate or consume the trace");
}
void ActualLongRouteBoundary(){
    pal4::ComparisonSceneMotionTrace trace(true);
    pal4::ComparisonSceneMotionSample sample{};
    // live106 needs4109 actual input records by the seventh encounter.
    for(unsigned i=0;i<4109u;++i){sample.position[0]=i;trace.RecordStep(2u,0.017f,sample,sample);}
    const auto text=Export(trace);
    Check(text.find("\"attempted\":4109,\"dropped\":0")!=std::string::npos,
          "the actual live106 route fits without the old4096-record truncation");
    Check(text.find("\"position_bits\":[4108,0,0]")!=std::string::npos,
          "the last required route record is present");
}
void OverflowRetainsPrefix(){
    pal4::ComparisonSceneMotionTrace trace(true);
    const auto capacity=Capacity(Export(trace));
    Check(capacity>=4109u && capacity<=65536u,"diagnostic memory remains explicitly bounded");
    pal4::ComparisonSceneMotionSample sample{};
    for(unsigned i=0;i<capacity+2u;++i){sample.direction[0]=i;trace.RecordStep(4u,0.017f,sample,sample);}
    const auto text=Export(trace);
    Check(text.find("\"attempted\":"+std::to_string(capacity+2u)+",\"dropped\":2")!=std::string::npos,
          "overflow reports both dropped records");
    Check(text.find("\"direction_bits\":["+std::to_string(capacity-1u)+",0,0]")!=std::string::npos &&
          text.find("\"direction_bits\":["+std::to_string(capacity)+",0,0]")==std::string::npos,
          "overflow preserves the original chronological prefix");
}
}
int main(){DisabledAndOwnedSamples();ActualLongRouteBoundary();OverflowRetainsPrefix();
    std::cout<<"PASS: motion observer disabled/copy/export/long-route/bounded-overflow\n";}
