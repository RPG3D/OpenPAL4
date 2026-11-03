#include "pal4/comparison_camera_collision_trace.h"
#include "pal4/camera_data.h"
#include <bit>
#include <cassert>
#include <sstream>

int main(){
    using namespace pal4;
    {
        ComparisonClumpQueryTrace off;
        assert(!off.Contains(reinterpret_cast<const float*>(1)));
        assert(off.Begin(reinterpret_cast<const float*>(1),0,0,0)==ComparisonClumpQueryTrace::kNone);
        ComparisonClumpQueryTrace queries(true,{850,900,-100,200,-600,-450});
        const std::array<float,6> vertical{865,30,-520,865,-20,-520};
        const std::array<float,4> hit{.5f,865,5,-520};
        auto oblique=vertical;oblique[3]+=1.0f;
        assert(queries.Begin(oblique.data(),123,127,8064)==ComparisonClumpQueryTrace::kNone);
        auto outside=vertical;outside[0]=outside[3]=901;
        assert(queries.Begin(outside.data(),123,127,8064)==ComparisonClumpQueryTrace::kNone);
        for(unsigned i=0;i<ComparisonClumpQueryTrace::kCapacity;++i){
            const auto ticket=queries.Begin(vertical.data(),123,127,8064);assert(ticket==i);
            queries.End(ticket,i==0?0:1,hit.data(),456,7,127,8064);
            queries.End(ticket,99,nullptr,999,99,0,0); // Never overwrite a completed query.
        }
        assert(queries.Begin(vertical.data(),123,127,8064)==ComparisonClumpQueryTrace::kNone);
        std::ostringstream out;queries.Write(out);const auto text=out.str();
        assert(text.find("\"count\":256,\"dropped\":1")!=std::string::npos);
        assert(text.find("\"result\":99")==std::string::npos);
        assert(text.find("\"hit\":[0,0,0,0]")!=std::string::npos);
        assert(text.find("\"hit\":[1056964608,1146634240,1084227584,3288465408]")!=std::string::npos);
    }
    ComparisonCameraCollisionTrace disabled(false);
    assert(!disabled.Begin(reinterpret_cast<const void*>(1)).active);
    CameraDataState camera;
    camera.At(0x74)=std::bit_cast<std::uint32_t>(910.0f);
    camera.At(0x78)=std::bit_cast<std::uint32_t>(72.0f);
    camera.At(0x7c)=std::bit_cast<std::uint32_t>(-165.0f);
    {
        const auto before=camera.words;
        ComparisonCameraCollisionTrace startup(true,{900,920,65,80,-180,-150},126u);
        auto first=startup.Begin(&camera);
        assert(first.active && startup.Count()==0u);
        startup.End(first,1u);
        assert(startup.Arm(126u) && startup.Count()==1u);
        assert(camera.words==before);
        ComparisonCameraCollisionTrace off(false,{},127u);
        assert(!off.Begin(reinterpret_cast<const void*>(1)).active && off.Count()==0u);
    }
    ComparisonCameraCollisionTrace trace(true,{900,920,65,80,-180,-150});
    assert(!trace.Begin(&camera).active && !trace.Arm(0));
    assert(trace.Arm(123));
    auto outside=camera;outside.At(0x74)=std::bit_cast<std::uint32_t>(1000.0f);
    assert(!trace.Begin(&outside).active);
    const auto initial=camera.words;
    auto& gate=GetComparisonFrameGate();
    const auto previous_gate=gate;
    gate=ComparisonFrameGate(true,true);
    assert(gate.RequestCheckpointArm(900));
    assert(gate.BeginFrame());assert(gate.EndFrame());
    assert(gate.Grant(901,ComparisonCameraCollisionTrace::kFrames));
    const std::array<float,6> ray{910,72,-165,974,84,-84};
    const std::array<float,4> hit{.5f,942,78,-124.5f};
    for(unsigned frame=0;frame<ComparisonCameraCollisionTrace::kFrames;++frame){
        assert(gate.BeginFrame());gate.NoteCallback();
        const auto gate_before=gate.Observe();
        {
        ComparisonCameraAdvanceScope advance(trace,&camera,.017f);
        auto ticket=trace.Begin(&camera);assert(ticket.active);
        assert(trace.Arm(123)); // duplicate delivery acknowledges without clearing the producer
        assert(!trace.Arm(124)); // cannot reset a producer-owned capture
        const auto fp=ComparisonCameraCollisionTrace::FloatingPointState();
        const unsigned queries=frame==0?ComparisonCameraCollisionTrace::kQueries+1:1;
        for(unsigned q=0;q<queries;++q){
            auto t=trace.BeginQuery(q%2,ray.data(),1234);
            trace.EndQuery(t,1,hit.data(),4567,89);
        }
        trace.End(ticket,1);trace.End(ticket,999); // completed ticket cannot be recorded twice
        assert(camera.words==initial);
        const auto after=ComparisonCameraCollisionTrace::FloatingPointState();
        assert(fp.x87==after.x87 && (fp.mxcsr&0xffffffc0u)==(after.mxcsr&0xffffffc0u));
        assert(trace.Active() && !trace.Arm(124));
        // A real camera update writes its matrix/eye after environment returns.
        camera.At(0x48)=std::bit_cast<std::uint32_t>(100.0f+frame);
        if(frame+1==ComparisonCameraCollisionTrace::kFrames)assert(!trace.ReadyToPublish());
        }
        const auto gate_after=gate.Observe();
        assert(gate_after.completed==gate_before.completed &&
               gate_after.depth==gate_before.depth && gate_after.callbacks==gate_before.callbacks);
        (void)gate.EndFrame();
        camera.words=initial;
    }
    assert(trace.ReadyToPublish());
    assert(!trace.Active() && trace.Count()==ComparisonCameraCollisionTrace::kFrames && !trace.Begin(&camera).active);
    assert(!trace.BeginQuery(0,ray.data(),1234).active);
    std::ostringstream out;trace.Write(out);const auto json=out.str();
    assert(json.find("\"frame_gate_enabled\":true")!=std::string::npos);
    assert(json.find("\"outer_completed\":15,\"outer_callbacks\":16,\"outer_depth\":1")!=std::string::npos);
    gate=previous_gate;
    assert(json.find("\"dropped\":1")!=std::string::npos);
    assert(json.find("\"has_delta\":true")!=std::string::npos);
    assert(json.find("\"advance_complete\":true")!=std::string::npos);
    assert(json.find("1120403456")!=std::string::npos); // post-advance100.0, absent from environment sample
    assert(json.find("\"hit\":[1056964608,1147895808,1117519872,3271098368]")!=std::string::npos);
    ComparisonCameraCollisionTrace aborted(true,{900,920,65,80,-180,-150});
    assert(aborted.Arm(125));
    unsigned calls=0;
    try {ComparisonCameraCollisionScope scope(aborted,&camera);++calls;throw 7;}catch(int value){assert(value==7);}
    assert(calls==1 && !aborted.Active() && aborted.Count()==1);
    std::ostringstream failure;aborted.Write(failure);assert(failure.str().find("\"aborted\":true")!=std::string::npos);
    assert(failure.str().find("\"has_delta\":false")!=std::string::npos);
    assert(camera.words==initial);
}
