#include "pal4/game_frame_updater.h"
#include "pal4/comparison_frame_clock.h"
#include "pal4/comparison_frame_gate.h"
#include "pal4/comparison_camera_euler_trace.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameFrameTimingRuntime {
    std::vector<std::uint32_t> times;
    std::vector<std::string> trace;
    std::size_t next_time = 0u;
    double callback_elapsed = -1.0;
    std::uint32_t callback_flag = 0xffffffffu;

    std::uint32_t TimeGetTime() override {
        Check(next_time < times.size(), "clock sample count");
        trace.push_back("clock");
        return times[next_time++];
    }
    void ProcessInputs() override { trace.push_back("inputs"); }
    void InvokeFrame(double elapsed, std::uint32_t flag) override {
        trace.push_back("frame");
        callback_elapsed = elapsed;
        callback_flag = flag;
    }
};

void SetCap(pal4::GameRenderState& state, float value) {
    state.Write(pal4::GameFrameUpdater::kDeltaCapOffset,
                std::bit_cast<std::uint32_t>(value));
}

void DisabledAndFirstFrame() {
    pal4::GameConfigWords config{};
    pal4::GameRenderState state(0x3000u);
    float accumulator = 0.0f;
    Runtime runtime;
    runtime.times = {1000u, 1017u};
    state.Write(pal4::GameFrameUpdater::kFirstFrameOffset, 1u);
    SetCap(state, 1.0f);
    pal4::GameFrameUpdater updater(config, state, accumulator);

    config[3] = 0x101u;
    updater.Update(runtime);
    Check(runtime.trace.empty() &&
              state.Read(pal4::GameFrameUpdater::kFirstFrameOffset) == 1u,
          "nonzero config low byte disables all frame work");

    config[3] = 0x100u;
    updater.Update(runtime);
    const float expected_elapsed = static_cast<float>(
        17.0 * static_cast<double>(std::bit_cast<float>(0x3a83126fu)));
    Check(runtime.next_time == 2u &&
              runtime.trace == std::vector<std::string>{
                  "clock", "clock", "inputs", "frame"} &&
              state.Read(pal4::GameFrameUpdater::kWindowStartOffset) == 1000u &&
              state.Read(pal4::GameFrameUpdater::kCurrentTimeOffset) == 1017u &&
              state.Read(pal4::GameFrameUpdater::kPreviousTimeOffset) == 1017u &&
              state.Read(pal4::GameFrameUpdater::kFrameCountOffset) == 1u &&
              state.Read(pal4::GameFrameUpdater::kFirstFrameOffset) == 0u &&
              runtime.callback_flag == 0u &&
              runtime.callback_elapsed == static_cast<double>(expected_elapsed) &&
              accumulator == 0.0f,
          "first frame seeds two times, processes inputs and clears accumulator");
}

void ExactAdmissionThreshold() {
    pal4::GameConfigWords config{};
    pal4::GameRenderState state(0x3000u);
    float accumulator = std::bit_cast<float>(0x3c888888u);
    Runtime runtime;
    runtime.times = {1000u, 1000u};
    state.Write(pal4::GameFrameUpdater::kPreviousTimeOffset, 1000u);
    SetCap(state, 1.0f);
    pal4::GameFrameUpdater updater(config, state, accumulator);

    updater.Update(runtime);
    Check(runtime.trace == std::vector<std::string>{"clock"} &&
              accumulator == std::bit_cast<float>(0x3c888888u),
          "one float step below threshold does not dispatch");

    accumulator = std::bit_cast<float>(0x3c888889u);
    updater.Update(runtime);
    Check(runtime.trace == std::vector<std::string>{
              "clock", "clock", "inputs", "frame"} &&
              runtime.callback_elapsed ==
                  static_cast<double>(std::bit_cast<float>(0x3c888889u)) &&
              accumulator == 0.0f,
          "exact float threshold dispatches stored accumulator");
}

void AccumulationAndFpsBoundary() {
    pal4::GameConfigWords config{};
    pal4::GameRenderState state(0x3000u);
    float accumulator = 0.0f;
    Runtime runtime;
    runtime.times = {1008u, 1016u, 1017u, 2000u, 2001u};
    state.Write(pal4::GameFrameUpdater::kPreviousTimeOffset, 1000u);
    state.Write(pal4::GameFrameUpdater::kWindowStartOffset, 1000u);
    SetCap(state, 1.0f);
    pal4::GameFrameUpdater updater(config, state, accumulator);

    updater.Update(runtime);
    Check(runtime.trace == std::vector<std::string>{"clock"} &&
              accumulator > 0.007f && accumulator < 0.009f,
          "subthreshold delta accumulates without callback");
    updater.Update(runtime);
    Check(runtime.trace == std::vector<std::string>{"clock", "clock"} &&
              accumulator > 0.015f && accumulator < 0.017f,
          "second short delta remains below one-sixtieth threshold");
    updater.Update(runtime);
    Check(runtime.trace == std::vector<std::string>{
              "clock", "clock", "clock", "inputs", "frame"} &&
              runtime.callback_elapsed > 0.016f &&
              accumulator == 0.0f &&
              state.Read(pal4::GameFrameUpdater::kFrameCountOffset) == 3u,
          "threshold crossing dispatches one frame");
    updater.Update(runtime);
    Check(state.Read(pal4::GameFrameUpdater::kWindowStartOffset) == 1000u &&
              state.Read(pal4::GameFrameUpdater::kFrameCountOffset) == 4u,
          "FPS window stays open at exact 1000 millisecond boundary");
    updater.Update(runtime);
    Check(state.Read(pal4::GameFrameUpdater::kWindowStartOffset) == 2001u &&
              state.Read(pal4::GameFrameUpdater::kPreviousFrameCountOffset) == 4u &&
              state.Read(pal4::GameFrameUpdater::kFrameCountOffset) == 1u,
          "FPS window rolls only after strict boundary");
}

void CapAndTimerWrap() {
    pal4::GameConfigWords config{};
    pal4::GameRenderState capped_state(0x3000u);
    float capped_accumulator = 0.0f;
    Runtime capped;
    capped.times = {100u, 200u};
    SetCap(capped_state, 0.01f);
    pal4::GameFrameUpdater cap_updater(config, capped_state,
                                      capped_accumulator);
    cap_updater.Update(capped);
    Check(capped_accumulator == 0.01f &&
              capped.trace == std::vector<std::string>{"clock"},
          "large delta is capped below frame admission threshold");
    cap_updater.Update(capped);
    Check(capped.trace == std::vector<std::string>{
              "clock", "clock", "inputs", "frame"} &&
              capped.callback_elapsed > 0.019f &&
              capped_accumulator == 0.0f,
          "two capped deltas dispatch the stored float accumulator");

    pal4::GameRenderState wrapped_state(0x3000u);
    float wrapped_accumulator = 0.0f;
    Runtime wrapped;
    wrapped.times = {0x10u};
    wrapped_state.Write(pal4::GameFrameUpdater::kPreviousTimeOffset,
                        0xfffffff0u);
    SetCap(wrapped_state, 1.0f);
    pal4::GameFrameUpdater wrap_updater(config, wrapped_state,
                                       wrapped_accumulator);
    wrap_updater.Update(wrapped);
    Check(wrapped.trace == std::vector<std::string>{
              "clock", "inputs", "frame"} &&
              std::fabs(wrapped.callback_elapsed - 0.032) < 0.00001,
          "unsigned timer difference wraps across 32-bit boundary");
}

void DiagnosticResetDomain() {
    pal4::ComparisonFrameClock clock(17u),normal(0u);
    Check(normal.ResetTimestamp(100000u)==100000u,"disabled diagnostic leaves reset on public clock");
    pal4::GameConfigWords config{};pal4::GameRenderState state(0x3000u);
    float accumulated=0;pal4::GameFrameUpdater updater(config,state,accumulated);SetCap(state,.1f);
    state.Write(pal4::GameFrameUpdater::kPreviousTimeOffset,clock.Next());
    Runtime runtime;runtime.times={clock.Next()};updater.Update(runtime);
    Check(runtime.callback_elapsed>.016 && runtime.callback_elapsed<.018,"ordinary controlled frame delta");
    // Original reset handlers write the chosen public timestamp themselves.
    // A real boot-clock value mixed into this sequence causes unsigned underflow.
    state.Write(pal4::GameFrameUpdater::kPreviousTimeOffset,100000u);
    runtime.times.push_back(clock.Next());updater.Update(runtime);
    Check(runtime.callback_elapsed==static_cast<double>(.1f),"mixed clock reset reproduces the observed cap");
    state.Write(pal4::GameFrameUpdater::kPreviousTimeOffset,clock.ResetTimestamp(100000u));
    Check(clock.Current()==51u,"reset read does not advance controlled time");
    runtime.times.push_back(clock.Next());updater.Update(runtime);
    Check(runtime.callback_elapsed>.016 && runtime.callback_elapsed<.018,"same-domain reset retains original frame arithmetic without cap compensation");
    for(const auto rva:{0x1d3846u,0x1d32b5u,0x18f204u,0x1b676cu}){
        Check(pal4::ComparisonFrameClock::IsOriginalResetCaller(0x400000u+rva,0x400000u) &&
              pal4::ComparisonFrameClock::IsOriginalResetCaller(0x500000u+rva,0x500000u) &&
              !pal4::ComparisonFrameClock::IsOriginalResetCaller(0x400001u+rva,0x400000u),"reset call filter preserves ASLR and exact return address");
    }
    Check(!pal4::ComparisonFrameClock::IsOriginalResetCaller(0x4092a8u,0x400000u),"frame advance and reset reads remain distinct");
}

void DiagnosticFrameCredits(){
    using Gate=pal4::ComparisonFrameGate;using Phase=Gate::Phase;
    Check(!Gate::ParseEnabled(nullptr,false,false),"default frame gate stays disabled");
    for(const auto value:{"0","true","2"}){
        bool rejected=false;try{(void)Gate::ParseEnabled(value,true,true);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"invalid frame gate toggle rejected");
    }
    for(const auto flags:{std::pair{false,true},std::pair{true,false}}){
        bool rejected=false;try{(void)Gate::ParseEnabled("1",flags.first,flags.second);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"frame credit requires scripted fixed-clock experiment");
    }
    Gate normal(false);Check(normal.BeginFrame(),"normal update admitted");normal.RequestArm(100u);
    Check(!normal.EndFrame() && normal.Observe().phase==Phase::disabled,"normal update and countdown unchanged");
    Gate gate(true);Check(gate.BeginFrame(),"startup runs before countdown starts");
    gate.RequestArm(100u);Check(gate.Observe().phase==Phase::waiting && gate.Observe().arm_pending,"start does not hold inside game callback");
    Check(gate.BeginFrame(),"actual nested startup update retained");
    Check(!gate.EndFrame() && gate.Observe().phase==Phase::waiting,"nested end cannot publish arm");
    Check(gate.EndFrame() && gate.Observe().phase==Phase::held,"outer end publishes initial hold");
    Check(!gate.BeginFrame() && !gate.Grant(1u,0u) && !gate.Grant(1u,10001u),"no credit leaves simulation held");
    const auto held=gate.Observe();gate.RequestArm(999u);
    Check(gate.Observe().arm_period==100u && gate.Observe().arm_calls==1u && gate.Observe().completed==held.completed,"later countdown start cannot reset epoch");
    Check(gate.Grant(101u,2u) && !gate.Grant(102u,1u),"one bounded credit request at a time");
    Check(gate.BeginFrame(),"first credited update admitted");gate.NoteClockRead();gate.NoteClockRead();gate.NoteCallback();
    Check(gate.BeginFrame(),"nested update retains game behavior");gate.NoteClockRead();gate.NoteCallback();
    Check(!gate.EndFrame() && !gate.EndFrame(),"nested and first outer end do not complete two-frame request");
    Check(gate.Observe().completed==1u && gate.Observe().remaining==1u && gate.Observe().nested==1u,"nested work consumes no second credit");
    Check(gate.BeginFrame(),"second outer credit admitted");gate.NoteClockRead();
    Check(gate.EndFrame() && gate.Observe().completed==2u && gate.Observe().callbacks==2u && gate.Observe().clock_reads==4u,"complete only after whole outer updates, independent of callback/read counts");
    Check(!gate.BeginFrame() && !gate.Grant(101u,1u),"completed request cannot replay credit");
    Check(gate.Grant(103u,1u) && gate.BeginFrame(),"next request resumes held world");gate.Fail();
    Check(gate.EndFrame() && gate.Observe().depth==0u && gate.Observe().phase==Phase::faulted && !gate.BeginFrame(),"failure unwinds depth and remains an explicit stopped fault");
    Gate outside(true);outside.RequestArm(20u);
    Check(outside.Observe().phase==Phase::waiting && outside.Observe().completed==0u,"query after start outside frame cannot arm");
    Check(outside.BeginFrame() && outside.EndFrame() && outside.Observe().phase==Phase::held,"outside start waits for next whole frame end");
}

void DiagnosticCheckpointEpoch(){
    using Gate=pal4::ComparisonFrameGate;using Phase=Gate::Phase;
    Check(!Gate::ParseCheckpointEnabled(nullptr,false),"checkpoint bootstrap is disabled by default");
    Check(Gate::ParseCheckpointEnabled("1",true),"explicit checkpoint toggle accepted with frame gate");
    for(const auto value:{"0","true","2"}){
        bool rejected=false;try{(void)Gate::ParseCheckpointEnabled(value,true);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"invalid checkpoint policy rejected");
    }
    bool rejected=false;try{(void)Gate::ParseCheckpointEnabled("1",false);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected,"checkpoint policy requires an enabled diagnostic frame gate");
    Check(Gate::ParseStartupRequest(nullptr,false)==0u && Gate::ParseStartupRequest("",false)==0u &&
          Gate::ParseStartupRequest("2147483647",true)==0x7fffffffu,"startup request is explicit and bounded");
    for(const auto value:{"0","-1","+1"," 1","1x","2147483648","999999999999999999999"}){
        bool invalid=false;try{(void)Gate::ParseStartupRequest(value,true);}catch(const std::invalid_argument&){invalid=true;}
        Check(invalid,"invalid startup request rejected");
    }
    bool missing_policy=false;try{(void)Gate::ParseStartupRequest("1",false);}catch(const std::invalid_argument&){missing_policy=true;}
    Check(missing_policy,"startup request cannot enable ordinary gameplay diagnostics");
    Gate automatic(true,true,123u);
    Check(!automatic.RequestStartupCheckpointArm(true) && automatic.BeginFrame(),"auto hold requires real outer end");
    Check(!automatic.RequestStartupCheckpointArm(false) && automatic.BeginFrame() &&
          !automatic.RequestStartupCheckpointArm(true) && !automatic.EndFrame(),"not-ready and nested frames cannot arm");
    Check(automatic.RequestStartupCheckpointArm(true) && !automatic.RequestStartupCheckpointArm(true) &&
          automatic.EndFrame() && automatic.Observe().phase==Phase::held && automatic.Observe().request==123u &&
          automatic.Observe().completed==0u && automatic.Observe().arm_calls==1u,
          "first ready outer end holds exactly once without simulation credit");
    Check(!automatic.RequestStartupCheckpointArm(true) && automatic.Grant(124u,1u) && automatic.BeginFrame() &&
          !automatic.RequestStartupCheckpointArm(true) && automatic.EndFrame() && automatic.Observe().completed==1u,
          "subsequent real frames cannot reset startup epoch");
    Gate normal(false),countdown_only(true);
    Check(!normal.RequestCheckpointArm(1u) && !countdown_only.RequestCheckpointArm(1u),
          "ordinary play and countdown-only experiments cannot arm from checkpoint messages");
    Gate gate(true,true);const auto waiting=gate.Observe();
    Check(waiting.checkpoint_allowed && waiting.arm_source==Gate::ArmSource::none && !gate.RequestCheckpointArm(0u),
          "waiting query and zero request do not arm checkpoint");
    Check(gate.BeginFrame() && !gate.RequestCheckpointArm(11u) && !gate.EndFrame(),
          "checkpoint message cannot arm inside an active or nested game frame");
    Check(gate.RequestCheckpointArm(12u) && gate.Observe().phase==Phase::waiting &&
          gate.Observe().arm_pending && gate.Observe().request==12u && gate.Observe().arm_calls==1u &&
          gate.Observe().arm_period==0u && gate.Observe().arm_source==Gate::ArmSource::checkpoint,
          "checkpoint request records distinct diagnostic provenance without granting simulation");
    Check(!gate.RequestCheckpointArm(13u) && !gate.Grant(14u,1u),"pending checkpoint cannot be replaced or credited");
    gate.RequestArm(900u);
    Check(gate.Observe().request==12u && gate.Observe().arm_source==Gate::ArmSource::checkpoint,
          "later timer signal cannot replace pending checkpoint epoch");
    Check(gate.BeginFrame() && gate.BeginFrame() && !gate.EndFrame() && gate.Observe().phase==Phase::waiting,
          "checkpoint arm retains nested startup work until complete outer end");
    Check(gate.EndFrame() && gate.Observe().phase==Phase::held && gate.Observe().depth==0u &&
          gate.Observe().completed==0u && gate.Observe().callbacks==0u && gate.Observe().clock_reads==0u &&
          gate.Observe().nested==0u && gate.Observe().request==12u && !gate.Observe().arm_pending,
          "complete outer boundary publishes zero-count checkpoint epoch");
    Check(!gate.RequestCheckpointArm(15u) && !gate.Grant(12u,1u) && gate.Grant(16u,1u),
          "held epoch rejects rearm/bootstrap replay and accepts only fresh credit");
    Check(gate.BeginFrame(),"checkpoint credit admitted");gate.NoteClockRead();gate.NoteCallback();
    Check(!gate.RequestCheckpointArm(17u) && gate.EndFrame() && gate.Observe().completed==1u &&
          gate.Observe().clock_reads==1u && gate.Observe().callbacks==1u,
          "checkpoint credit advances exactly one full frame");
    Gate timer(true,true);timer.RequestArm(7u);
    Check(!timer.RequestCheckpointArm(18u) && timer.Observe().arm_source==Gate::ArmSource::countdown,
          "actual countdown arming retains its original epoch semantics");
}

void DiagnosticCameraEulerTrace() {
    using Trace=pal4::ComparisonCameraEulerTrace;
    const std::array<std::uint32_t,4> input{0x80000000u,0x7fc12345u,0x3f800001u,0xbf800000u};
    const std::array<std::uint32_t,3> output{0x80000000u,0x7f800000u,0x7fa54321u};
    const auto cw=Trace::CaptureControlWord();
    Trace disabled;
    disabled.Record(input,output,0x007fu,0x027fu);
    Check(!disabled.Enabled() && !disabled.Count() && !disabled.Sequence() && !disabled.Dropped(),
          "Euler observer is disabled by default and counts no calls");
    Trace trace(true);
    trace.Record(input,output,cw,cw);
    Check(trace.Count()==1u && trace.Sequence()==1u && !trace.Dropped() &&
          trace.Entries()[0].sequence==1u && trace.Entries()[0].input==input &&
          trace.Entries()[0].output==output && trace.Entries()[0].cw_before==cw &&
          trace.Entries()[0].cw_after==cw,
          "Euler observer preserves raw signed-zero/NaN/infinity words and actual control word");
    for(std::uint32_t i=1u;i<Trace::kCapacity;++i)trace.Record(input,output,0x007fu,0x027fu);
    const auto overflow_input=std::array<std::uint32_t,4>{1u,2u,3u,4u};
    for(unsigned i=0;i<3u;++i)trace.Record(overflow_input,output,0x037fu,0x037fu);
    Check(trace.Count()==Trace::kCapacity && trace.Sequence()==Trace::kCapacity+3u && trace.Dropped()==3u &&
          trace.Entries()[0].input==input && trace.Entries()[Trace::kCapacity-1u].input==input &&
          trace.Entries()[Trace::kCapacity-1u].sequence==Trace::kCapacity,
          "Euler observer drops overflow without replacing captured entries or losing total sequence");
    Check(Trace::CaptureControlWord()==cw,
          "integer Euler recording and control-word observation never change actual FPU control");
}

}  // namespace

int main() {
    try {
        Check(pal4::ComparisonFrameClock::ParseStep(nullptr,false)==0u,
              "normal frame runtime keeps the real clock");
        for(const auto* invalid:{"0","16","101","17x","-17"}) {
            bool rejected=false;
            try{(void)pal4::ComparisonFrameClock::ParseStep(invalid,true);}
            catch(const std::invalid_argument&){rejected=true;}
            Check(rejected,"invalid diagnostic clock configuration rejected");
        }
        bool rejected=false;
        try{(void)pal4::ComparisonFrameClock::ParseStep("17",false);}
        catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"diagnostic clock requires scripted input");
        pal4::ComparisonFrameClock clock(pal4::ComparisonFrameClock::ParseStep("17",true));
        Check(clock.Enabled() && clock.Next()==17u && clock.Next()==34u && clock.Next()==51u,
              "both diagnostic adapters share the same millisecond input sequence");
        Check(pal4::ComparisonFrameClock::IsOriginalFrameCaller(0x4092a8u,0x400000u) &&
              pal4::ComparisonFrameClock::IsOriginalFrameCaller(0x5092ccu,0x500000u) &&
              !pal4::ComparisonFrameClock::IsOriginalFrameCaller(0x4092a9u,0x400000u) &&
              !pal4::ComparisonFrameClock::IsOriginalFrameCaller(0x401000u,0x400000u),
              "only the two verified frame time calls advance diagnostic time");
        DisabledAndFirstFrame();
        ExactAdmissionThreshold();
        AccumulationAndFpsBoundary();
        CapAndTimerWrap();
        DiagnosticResetDomain();
        DiagnosticFrameCredits();
        DiagnosticCheckpointEpoch();
        DiagnosticCameraEulerTrace();
        std::cout << "game_frame_updater_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
