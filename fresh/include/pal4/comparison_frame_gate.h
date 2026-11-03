#pragma once
#include "pal4/comparison_frame_clock.h"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace pal4 {
// Test scheduling only. A credit runs one whole original outer frame update;
// nested updates retain their real behavior and are counted separately.
class ComparisonFrameGate final {
public:
    static constexpr std::uint32_t kMessage=0x8518u;
    static constexpr std::uint32_t kCheckpointMessage=0x8519u;
    enum class ArmSource { none, countdown, checkpoint };
    enum class Phase { disabled, waiting, held, running, faulted };
    struct Observation {
        Phase phase{};
        std::uint32_t depth{},remaining{},request{},arm_period{},arm_calls{};
        std::uint64_t completed{},granted{},nested{},clock_reads{},callbacks{};
        bool arm_pending{},checkpoint_allowed{};
        ArmSource arm_source{};
    };
    static bool ParseEnabled(const char* value,bool scripted,bool fixed_clock){
        if(!value || !*value)return false;
        if(std::string_view(value)!="1" || !scripted || !fixed_clock)
            throw std::invalid_argument("frame gate requires value1, scripted input and fixed comparison clock");
        return true;
    }
    static bool EnvironmentEnabled(){return ParseEnabled(std::getenv("PAL4_COMPARE_FRAME_GATE"),
        std::getenv("PAL4_COMPARE_SCRIPTED_INPUT")!=nullptr,GetComparisonFrameClock().Enabled());}
    static bool ParseCheckpointEnabled(const char* value,bool frame_gate){
        if(!value || !*value)return false;
        if(std::string_view(value)!="1" || !frame_gate)
            throw std::invalid_argument("checkpoint frame gate requires explicit value1 and enabled frame gate");
        return true;
    }
    explicit ComparisonFrameGate(bool enabled,bool checkpoint=false,std::uint32_t startup_request=0u)
        :phase_(enabled?Phase::waiting:Phase::disabled),startup_request_(startup_request),checkpoint_allowed_(checkpoint){
        if(startup_request && (!checkpoint || startup_request>0x7fffffffu))throw std::invalid_argument("invalid startup checkpoint policy or request");
        if(checkpoint && !enabled)throw std::invalid_argument("checkpoint gate cannot be enabled without frame gate");
    }
    static ComparisonFrameGate FromEnvironment(){
        const bool enabled=EnvironmentEnabled();
        auto gate=ComparisonFrameGate(enabled,ParseCheckpointEnabled(
            std::getenv("PAL4_COMPARE_FRAME_GATE_CHECKPOINT"),enabled));
        gate.startup_request_=ParseStartupRequest(std::getenv("PAL4_COMPARE_FRAME_GATE_STARTUP_REQUEST"),gate.checkpoint_allowed_);
        return gate;
    }
    bool Enabled()const noexcept{return phase_!=Phase::disabled;}
    bool BeginFrame()noexcept{
        if(!Enabled())return true;
        if(phase_==Phase::faulted)return false;
        if(depth_){++depth_;if(phase_!=Phase::waiting)++nested_;return true;}
        if(phase_!=Phase::waiting){
            if(!remaining_)return false;
            --remaining_;phase_=Phase::running;
        }
        depth_=1u;return true;
    }
    // True requests a completion/initial-hold publication outside the game call.
    bool EndFrame()noexcept{
        if(!Enabled())return false;
        if(!depth_){Fail();return true;}
        if(--depth_)return false;
        if(phase_==Phase::faulted)return true;
        if(phase_==Phase::waiting){
            if(!arm_pending_)return false;
            arm_pending_=false;phase_=Phase::held;
            completed_=granted_=nested_=clock_reads_=callbacks_=0u;
            return true;
        }
        ++completed_;
        if(remaining_)return false;
        phase_=Phase::held;return true;
    }
    // Signal only after the actual countdown Start succeeds. Even outside a
    // frame this waits for the next whole outer frame, never arms from a query.
    void RequestArm(std::uint32_t period)noexcept{
        if(phase_!=Phase::waiting || arm_pending_)return;
        arm_pending_=true;arm_period_=period;arm_source_=ArmSource::countdown;++arm_calls_;
    }
    // Explicit diagnostic request after adapters verified actual Scene/control/
    // readiness. It holds only after the next complete outer frame; no query
    // or arbitrary input changes an epoch and no gameplay state is touched.
    bool RequestCheckpointArm(std::uint32_t request)noexcept{
        if(!checkpoint_allowed_ || phase_!=Phase::waiting || arm_pending_ || depth_ || !request)return false;
        request_=request;arm_pending_=true;arm_period_=0u;
        arm_source_=ArmSource::checkpoint;++arm_calls_;return true;
    }
    // Opt-in startup observation: adapters check real readiness after the whole
    // game update, while still inside the outer frame. No partial/nested hold.
    static std::uint32_t ParseStartupRequest(const char* value,bool checkpoint){
        if(!value || !*value)return 0u;
        if(!checkpoint)throw std::invalid_argument("startup checkpoint requires checkpoint policy");
        std::uint32_t request{};
        for(const char* p=value;*p;++p){
            if(*p<'0' || *p>'9' || request>(0x7fffffffu-static_cast<unsigned>(*p-'0'))/10u)
                throw std::invalid_argument("startup checkpoint request must be positive signed32 decimal");
            request=request*10u+static_cast<unsigned>(*p-'0');
        }
        if(!request)throw std::invalid_argument("startup checkpoint request cannot be zero");
        return request;
    }
    std::uint32_t StartupRequest()const noexcept{return startup_request_;}
    bool RequestStartupCheckpointArm(bool ready)noexcept{
        if(!ready || !startup_request_ || !checkpoint_allowed_ || phase_!=Phase::waiting ||
           arm_pending_ || depth_!=1u)return false;
        request_=startup_request_;arm_pending_=true;arm_period_=0u;
        arm_source_=ArmSource::checkpoint;++arm_calls_;return true;
    }
    bool Grant(std::uint32_t request,std::uint32_t frames)noexcept{
        if(phase_!=Phase::held || depth_ || !request || request==request_ ||
            !frames || frames>10000u)return false;
        request_=request;remaining_=frames;granted_+=frames;phase_=Phase::running;return true;
    }
    void NoteClockRead()noexcept{if(phase_==Phase::running && depth_)++clock_reads_;}
    void NoteCallback()noexcept{if(phase_==Phase::running && depth_)++callbacks_;}
    void Fail()noexcept{if(Enabled()){phase_=Phase::faulted;remaining_=0u;}}
    Observation Observe()const noexcept{return {phase_,depth_,remaining_,request_,arm_period_,arm_calls_,
        completed_,granted_,nested_,clock_reads_,callbacks_,arm_pending_,checkpoint_allowed_,arm_source_};}
    static const char* ArmSourceName(ArmSource source)noexcept{
        switch(source){case ArmSource::none:return "none";case ArmSource::countdown:return "countdown";
        case ArmSource::checkpoint:return "checkpoint";}return "invalid";
    }
    static const char* PhaseName(Phase phase)noexcept{
        switch(phase){case Phase::disabled:return "disabled";case Phase::waiting:return "waiting";
        case Phase::held:return "held";case Phase::running:return "running";default:return "faulted";}
    }
private:
    Phase phase_;
    std::uint32_t depth_{},remaining_{},request_{},arm_period_{},arm_calls_{},startup_request_{};
    std::uint64_t completed_{},granted_{},nested_{},clock_reads_{},callbacks_{};
    bool arm_pending_{},checkpoint_allowed_{};
    ArmSource arm_source_{};
};
inline ComparisonFrameGate& GetComparisonFrameGate(){
    static ComparisonFrameGate gate=ComparisonFrameGate::FromEnvironment();return gate;
}
}
