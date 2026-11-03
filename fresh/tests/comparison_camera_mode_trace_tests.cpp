#include "pal4/comparison_camera_mode_trace.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
int main(){
    try {
        pal4::ComparisonCameraModeTrace trace;
        pal4::ComparisonFrameGate::Observation outer{};
        outer.completed=7801u;outer.callbacks=7802u;outer.depth=1u;
        trace.Record(5u,false,outer);
        Check(!trace.Enabled() && !trace.Count(),"mode observer defaults to disabled");
        trace.Arm(101u,false);trace.Record(5u,false,outer);
        Check(!trace.Count(),"disabled arm does not collect");
        trace.Arm(102u,true);
        trace.Record(5u,false,outer);
        outer.depth=2u;trace.Record(5u,true,outer);
        Check(trace.Count()==2u && trace.At(0).sequence==1u && trace.At(1).sequence==2u &&
              trace.At(0).mode==5u && !trace.At(0).after && trace.At(1).after &&
              trace.At(0).outer_completed==7801u && trace.At(1).outer_callbacks==7802u &&
              trace.At(0).outer_depth==1u && trace.At(1).outer_depth==2u,
              "whole-handler boundaries retain supplied integer outer-frame observations");
        for(unsigned i=2u;i<64u;++i)trace.Record(i,false,outer);
        const auto first=trace.At(0),last=trace.At(63);
        trace.Record(999u,true,outer);trace.Record(1000u,false,outer);
        Check(trace.Count()==64u && trace.Dropped()==2u && trace.At(0).sequence==first.sequence &&
              trace.At(0).mode==first.mode && trace.At(63).mode==last.mode,
              "bounded capacity reports drops and never overwrites collected mode edges");
        std::ostringstream out;trace.WriteJSON(out);
        Check(out.str().find("\"request\":102")!=std::string::npos &&
              out.str().find("\"dropped\":2")!=std::string::npos &&
              out.str().find("\"after\":true")!=std::string::npos &&
              out.str().find("\"outer_completed\":7801")!=std::string::npos,
              "mode trace serialization retains request, completion and bound evidence");
        trace.Arm(103u,true);
        Check(!trace.Count() && !trace.Dropped(),"rearm clears bounded observations and drops");
        trace.Record(0u,false,outer);Check(trace.At(0).sequence==1u,"rearm resets sequence");
        trace.Arm(0u,true);Check(!trace.Enabled() && !trace.Count(),"zero request cannot enable observation");
        std::cout<<"camera mode POD trace contracts pass; real process gate untouched\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
