#pragma once
#include "pal4/comparison_frame_gate.h"
#include "pal4/comparison_camera_mode_trace.h"
#include "pal4/comparison_clump_query_trace.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <locale>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#endif

namespace pal4 {
// Captures actual 5EA710 calls and their 661A50/5FEA10 queries. Addresses are
// process-local provenance. All arithmetic data is serialized as raw words.
class ComparisonCameraCollisionTrace final {
public:
    static constexpr unsigned kFrames=16, kQueries=1024, kCameraWords=0x2ecu/4u;
    struct Fp {std::uint32_t x87{},mxcsr{};};
    static Fp FloatingPointState() noexcept {
        Fp result;
#if defined(_MSC_VER) && defined(_M_IX86)
        unsigned short control{};
        __asm fnstcw control
        result.x87=control;
#endif
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        result.mxcsr=_mm_getcsr();
#endif
        return result;
    }
    struct Query {
        std::uint32_t kind{},owner{},result{},triangle{},atomic{};
        std::array<std::uint32_t,6> ray{};
        std::array<std::uint32_t,4> hit{}; // fraction,x,y,z, meaningful only on hit
        Fp before{},after{};
    };
    struct Frame {
        std::uintptr_t camera{};
        std::uint64_t outer_completed{}, outer_callbacks{};
        std::uint32_t outer_depth{};
        bool frame_gate_enabled{};
        std::array<std::uint32_t,kCameraWords> before{},after{},after_advance{};
        Fp fp_before{},fp_after{};
        std::uint32_t result{},count{},dropped{};
        bool aborted{},has_delta{},advance_complete{};std::uint32_t delta_bits{};
        std::array<Query,kQueries> queries{};
    };
    struct Ticket {bool active{};unsigned index{};unsigned clump_query=ComparisonClumpQueryTrace::kNone;};
    struct AdvanceContext {const void* camera{};std::uint32_t delta_bits{};};
    AdvanceContext PushAdvance(const void* camera,float delta)noexcept{
        const auto prior=advance_;if(enabled_){advance_.camera=camera;std::memcpy(&advance_.delta_bits,&delta,4u);}return prior;
    }
    void RestoreAdvance(AdvanceContext prior)noexcept{advance_=prior;}
    void CompleteAdvance(const void* camera)noexcept{
        if(!enabled_ || !count_)return;
        auto& frame=frames_[count_-1];
        if(frame.has_delta && !frame.advance_complete && frame.camera==reinterpret_cast<std::uintptr_t>(camera)){
            std::memcpy(frame.after_advance.data(),camera,0x2ecu);frame.advance_complete=true;
        }
    }
    explicit ComparisonCameraCollisionTrace(bool enabled,std::array<float,6> box={},std::uint32_t startup_request=0u)
        :enabled_(enabled),box_(box) {
        if(!enabled)return;
        for(unsigned axis=0;axis<3;++axis)
            if(!(box[2*axis]<=box[2*axis+1]))throw std::invalid_argument("camera trace target box");
        if(startup_request)(void)Arm(startup_request);
    }
    bool Enabled()const noexcept{return enabled_;}
    bool Active()const noexcept{return pending_ || (count_ && frames_[count_-1].has_delta && !frames_[count_-1].advance_complete);}
    bool ReadyToPublish()const noexcept{
        if(count_!=kFrames || Active())return false;
        for(unsigned i=0;i<count_;++i)if(frames_[i].has_delta && !frames_[i].advance_complete)return false;
        return true;
    }
    unsigned Count()const noexcept{return count_;}
    bool Arm(std::uint32_t request) noexcept {
        if(!enabled_ || !request)return false;
        if(request==request_)return true;
        if(Active())return false;
        try{if(!frames_)frames_=std::make_unique<Frame[]>(kFrames);}catch(...){return false;}
        for(unsigned i=0;i<kFrames;++i)frames_[i]=Frame{};
        count_=0;nested_=0;skipped_=0;published_=false;request_=request;armed_=true;
        GetComparisonCameraModeTrace().Arm(request,enabled_);return true;
    }
    Ticket Begin(const void* camera) noexcept {
        if(!enabled_ || !armed_ || !camera)return {};
        std::array<float,3> target{};
        std::memcpy(target.data(),static_cast<const unsigned char*>(camera)+0x74u,12u);
        for(unsigned axis=0;axis<3;++axis)
            if(!(target[axis]>=box_[2*axis] && target[axis]<=box_[2*axis+1]))return {};
        if(Active()){++nested_;return {};}
        if(count_==kFrames){++skipped_;return {};}
        auto& frame=frames_[count_];frame.camera=reinterpret_cast<std::uintptr_t>(camera);
        const auto& gate=GetComparisonFrameGate();
        const auto outer=gate.Observe();
        frame.frame_gate_enabled=gate.Enabled();
        frame.outer_completed=outer.completed;
        frame.outer_callbacks=outer.callbacks;
        frame.outer_depth=outer.depth;
        frame.has_delta=advance_.camera==camera;frame.delta_bits=advance_.delta_bits;
        std::memcpy(frame.before.data(),camera,0x2ecu);frame.fp_before=FloatingPointState();
        pending_=true;return {true,count_};
    }
    void End(Ticket& ticket,std::uint32_t result,bool aborted=false) noexcept {
        if(!ticket.active)return;
        ticket.active=false;auto& frame=frames_[ticket.index];frame.result=result;frame.aborted=aborted;
        frame.fp_after=FloatingPointState();
        std::memcpy(frame.after.data(),reinterpret_cast<const void*>(frame.camera),0x2ecu);
        pending_=false;++count_;
    }
    Ticket BeginQuery(unsigned kind,const void* ray,std::uintptr_t owner) noexcept {
        const auto fp=FloatingPointState();
        const auto clump=kind==0u?GetComparisonClumpQueryTrace().Begin(ray,owner,fp.x87,fp.mxcsr):ComparisonClumpQueryTrace::kNone;
        if(!pending_)return {false,0u,clump};
        auto& frame=frames_[count_];
        if(frame.count==kQueries){++frame.dropped;return {false,0u,clump};}
        auto& query=frame.queries[frame.count];query.kind=kind;query.owner=static_cast<std::uint32_t>(owner);
        std::memcpy(query.ray.data(),ray,24u);query.before=FloatingPointState();
        return {true,frame.count++,clump};
    }
    void EndQuery(Ticket& ticket,std::uint32_t result,const float* hit,
                  std::uint32_t atomic=0,std::uint32_t triangle=0) noexcept {
        const auto fp=FloatingPointState();
        GetComparisonClumpQueryTrace().End(ticket.clump_query,result,hit,atomic,triangle,fp.x87,fp.mxcsr);
        ticket.clump_query=ComparisonClumpQueryTrace::kNone;
        if(!ticket.active)return;
        ticket.active=false;auto& query=frames_[count_].queries[ticket.index];
        query.after=FloatingPointState();query.result=result;query.atomic=atomic;query.triangle=triangle;
        if(result && hit)std::memcpy(query.hit.data(),hit,16u);
    }
    void Write(std::ostream& out)const {
        out.imbue(std::locale::classic());
        out<<"{\"schema\":2,\"enabled\":"<<(enabled_?"true":"false")<<",\"request\":"<<request_<<",\"count\":"<<count_
           <<",\"pending\":"<<(pending_?"true":"false")<<",\"nested\":"<<nested_<<",\"skipped\":"<<skipped_<<",\"frames\":[";
        for(unsigned i=0;i<count_;++i){if(i)out<<',';const auto& f=frames_[i];
            out<<"{\"camera\":"<<f.camera<<",\"result\":"<<f.result<<",\"aborted\":"<<(f.aborted?"true":"false")
               <<",\"frame_gate_enabled\":"<<(f.frame_gate_enabled?"true":"false")
               <<",\"outer_completed\":"<<f.outer_completed<<",\"outer_callbacks\":"<<f.outer_callbacks
               <<",\"outer_depth\":"<<f.outer_depth
               <<",\"has_delta\":"<<(f.has_delta?"true":"false")<<",\"delta_bits\":"<<f.delta_bits
               <<",\"advance_complete\":"<<(f.advance_complete?"true":"false")
               <<",\"dropped\":"<<f.dropped<<",\"fp_before\":";FpWrite(out,f.fp_before);out<<",\"fp_after\":";FpWrite(out,f.fp_after);
            out<<",\"before\":";Words(out,f.before);out<<",\"after\":";Words(out,f.after);
            out<<",\"after_advance\":";Words(out,f.after_advance);out<<",\"queries\":[";
            for(unsigned q=0;q<f.count;++q){if(q)out<<',';const auto& x=f.queries[q];
                out<<"{\"kind\":"<<x.kind<<",\"owner\":"<<x.owner<<",\"result\":"<<x.result<<",\"atomic\":"<<x.atomic
                   <<",\"triangle\":"<<x.triangle<<",\"ray\":";Words(out,x.ray);out<<",\"hit\":";Words(out,x.hit);
                out<<",\"fp_before\":";FpWrite(out,x.before);out<<",\"fp_after\":";FpWrite(out,x.after);out<<'}';}
            out<<"]}";}
        out<<"],\"mode_trace\":";GetComparisonCameraModeTrace().WriteJSON(out);out<<'}';
    }
    // Export only after all bounded frames finish, outside the observed call.
    // Failure is diagnostic only and must not change the game's return path.
    void Publish(std::string_view side) noexcept {
        if(published_ || !ReadyToPublish())return;
        published_=true;
        try {
            const auto* directory=std::getenv("PAL4_COMPARE_STATE_DIR");
            const auto* id=std::getenv("PAL4_COMPARE_CAMERA_TRACE_ID");
            if(!directory || !id)return;
            const auto path=std::filesystem::path(directory)/("compare-"+std::string(side)+"-camera-collision-"+id+"-"+std::to_string(request_)+".json");
            auto temp=path;temp+=".tmp";std::ofstream file(temp);Write(file);file.flush();
            const bool good=bool(file);file.close();if(good)std::filesystem::rename(temp,path);
        }catch(...){}
    }
    void PublishArm(std::string_view side,std::uint32_t request,bool accepted)const noexcept {
        try {
            const auto* directory=std::getenv("PAL4_COMPARE_STATE_DIR");
            const auto* id=std::getenv("PAL4_COMPARE_CAMERA_TRACE_ID");
            if(!directory || !id)return;
            const auto path=std::filesystem::path(directory)/("compare-"+std::string(side)+"-camera-arm-"+id+"-"+std::to_string(request)+".json");
            auto temp=path;temp+=".tmp";std::ofstream file(temp);
            file<<"{\"request\":"<<request<<",\"accepted\":"<<(accepted?"true":"false")<<'}';file.flush();
            const bool good=bool(file);file.close();if(good)std::filesystem::rename(temp,path);
        }catch(...){}
    }
private:
    template<class T>static void Words(std::ostream& out,const T& values){out<<'[';bool first=true;for(auto x:values){if(!first)out<<',';first=false;out<<x;}out<<']';}
    static void FpWrite(std::ostream& out,Fp fp){out<<"{\"x87\":"<<fp.x87<<",\"mxcsr\":"<<fp.mxcsr<<'}';}
    bool enabled_{},pending_{},published_{},armed_{};unsigned count_{};std::uint32_t request_{};std::uint64_t nested_{},skipped_{};
    std::array<float,6> box_{};std::unique_ptr<Frame[]> frames_;
    AdvanceContext advance_{};
};
inline ComparisonCameraCollisionTrace& GetComparisonCameraCollisionTrace(){
    static ComparisonCameraCollisionTrace trace=[] {
        const auto* value=std::getenv("PAL4_COMPARE_CAMERA_TARGET_BOX");std::array<float,6> box{};
        if(!std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") || !value)return ComparisonCameraCollisionTrace(false);
        for(unsigned i=0;i<box.size();++i){char* end{};box[i]=std::strtof(value,&end);
            if(end==value || (i<5 ? *end!=',' : *end!='\0'))throw std::invalid_argument("camera trace six-number target box");
            value=end+(i<5?1:0);}
        std::uint32_t request{};
        if(const auto* startup=std::getenv("PAL4_COMPARE_CAMERA_STARTUP_REQUEST")){
            char* end{};const auto parsed=std::strtoul(startup,&end,10);
            if(end==startup || *end || !parsed || parsed>0x7ffffffful)
                throw std::invalid_argument("camera startup trace request");
            request=static_cast<std::uint32_t>(parsed);
        }
        return ComparisonCameraCollisionTrace(true,box,request);
    }();return trace;
}
class ComparisonCameraCollisionScope final {
public:
    ComparisonCameraCollisionScope(ComparisonCameraCollisionTrace& trace,const void* camera):trace_(trace),ticket_(trace.Begin(camera)){}
    ComparisonCameraCollisionScope(const ComparisonCameraCollisionScope&)=delete;
    ComparisonCameraCollisionScope& operator=(const ComparisonCameraCollisionScope&)=delete;
    ~ComparisonCameraCollisionScope(){trace_.End(ticket_,0u,true);}
    void Complete(std::uint32_t result)noexcept{trace_.End(ticket_,result);}
private:
    ComparisonCameraCollisionTrace& trace_;ComparisonCameraCollisionTrace::Ticket ticket_;
};
class ComparisonCameraAdvanceScope final {
public:
    ComparisonCameraAdvanceScope(ComparisonCameraCollisionTrace& trace,const void* camera,float delta,std::string_view side="native")
        :trace_(trace),prior_(trace.PushAdvance(camera,delta)),camera_(camera),side_(side){}
    ComparisonCameraAdvanceScope(const ComparisonCameraAdvanceScope&)=delete;
    ComparisonCameraAdvanceScope& operator=(const ComparisonCameraAdvanceScope&)=delete;
    ~ComparisonCameraAdvanceScope(){trace_.CompleteAdvance(camera_);trace_.RestoreAdvance(prior_);trace_.Publish(side_);}
private:
    ComparisonCameraCollisionTrace& trace_;ComparisonCameraCollisionTrace::AdvanceContext prior_;
    const void* camera_;std::string_view side_;
};
} // namespace pal4
