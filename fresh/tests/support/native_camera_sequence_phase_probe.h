#pragma once
#include "pal4/native_script_camera_sequence_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_script_end_action_queue.h"
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace pal4::test {
// Safe inside an existing active fixture: polls an incomplete task only and
// verifies no task timing/phase changes. It never advances camera playback.
inline void VerifyNativeCameraWaitPollDoesNotAdvance(
    NativeScriptCameraSequenceRuntime& camera,std::uint32_t task) {
    const auto require=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    require(task!=0u && camera.ActiveTask()==task,"camera poll probe requires current owned task");
    std::array<std::uint32_t,17> before{},after{};
    std::memcpy(before.data(),reinterpret_cast<const void*>(task),sizeof(before));
    const auto blocks=camera.BlocksScript();
    auto& waits=camera.WaitHandlers();
    require(waits.BlocksScript()==blocks,"camera wait adapter forwards blocking state");
    for(const float dt:{0.f,.017f,1.f})
        require(!waits.Tick(dt),"incomplete camera poll does not release wait");
    std::memcpy(after.data(),reinterpret_cast<const void*>(task),sizeof(after));
    require(before==after && camera.ActiveTask()==task && camera.BlocksScript()==blocks,
        "wait polling preserves task phase, clocks, identity and blocking state");
    for(const float dt:{-.1f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        bool rejected{};try{(void)waits.Tick(dt);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,"camera wait adapter rejects invalid frame times");
    }
    std::memcpy(after.data(),reinterpret_cast<const void*>(task),sizeof(after));
    require(before==after,"rejected poll leaves task timing unchanged");
}

// Use the existing playback owner so its embedded sentinel is never
// reinitialized by another allocator. Requires no active tasks or waits.
// Q01/N01 camera001 record002 is a known interpolated real-resource fixture.
inline void VerifyNativeCameraSequencePhases(NativeScriptCameraSequenceRuntime& playback_owner,
    const std::filesystem::path& root,
    NativeCameraManagerOwner& cameras,NativeGameArenaManagerOwner& arena,
    INativeCameraObjectResources& resources,std::string_view camera_file="camera001",
    std::string_view record="002") {
    const auto require=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    require(camera_file.find_first_of("\"\\\r\n")==std::string_view::npos &&
        record.find_first_of("\"\\\r\n")==std::string_view::npos,"phase probe requires literal camera names");
    require(!playback_owner.Waiting() && playback_owner.ActiveTask()==0u,
        "camera phase probe requires idle shared playback owner");
    NativeAngelScriptEngine engine;
    const std::string source="void Start(){global_mvar=1;giCameraPrepare(\""+
        std::string(camera_file)+"\");giCameraRunSingle(\""+std::string(record)+
        "\",true);global_mvar=2;}void Follow(){giCameraWait();global_mvar=3;}";
    require(engine.Raw()->AddScriptSection("camera_phase_probe","probe",source.c_str(),source.size())>=0 &&
        engine.Raw()->Build("camera_phase_probe")>=0,"compile real SDK camera phase probe");
    NativeScriptFrameRunner primary_runner(engine),secondary_runner(engine);
    NativeScriptCameraSequenceRuntime primary(root,cameras,arena,resources,
        [&]{return primary_runner.Context().Suspend()>=0;},&playback_owner);
    NativeScriptCameraSequenceRuntime secondary(root,cameras,arena,resources,
        [&]{return secondary_runner.Context().Suspend()>=0;},&playback_owner);
    NativeGameScriptServices primary_services,secondary_services;
    primary_services.camera_sequences=&primary;secondary_services.camera_sequences=&secondary;
    NativeGameScriptCallbacks primary_callbacks(primary_services),secondary_callbacks(secondary_services);
    engine.SetContextCallbacks(&primary_runner.Context(),&primary_callbacks);
    engine.SetContextCallbacks(&secondary_runner.Context(),&secondary_callbacks);
    primary_runner.Queue("camera_phase_probe","Start");
    require(primary_runner.Tick(.017f,&primary.WaitHandlers())==NativeScriptFrameStatus::suspended &&
        primary.BlocksScript() && primary.ActiveTask()!=0u,"SDK create queues blocking camera in script phase");
    const auto task=primary.ActiveTask();
    const auto word=[&](unsigned offset){return *reinterpret_cast<const std::uint32_t*>(task+offset);};
    require(word(0x28u)==2u && word(0x18u)==0u && word(0x40u)==0u,
        "new task remains phase2 with zero clocks until active-camera phase");
    playback_owner.AdvancePlayback(.017f);
    require(word(0x28u)==3u && word(0x18u)==std::bit_cast<std::uint32_t>(.017f) && word(0x40u)==0u,
        "same-frame active-camera phase enters new task exactly once");
    VerifyNativeCameraWaitPollDoesNotAdvance(primary,task);
    secondary_runner.Queue("camera_phase_probe","Follow");
    require(secondary_runner.Tick(.017f,&secondary.WaitHandlers())==NativeScriptFrameStatus::suspended &&
        secondary.ActiveTask()==task && secondary.BlocksScript(),"second SDK context waits on shared playback");
    VerifyNativeCameraWaitPollDoesNotAdvance(secondary,task);
    playback_owner.AdvancePlayback(.017f);
    require(word(0x40u)==std::bit_cast<std::uint32_t>(.017f),
        "single active-camera advance produces one playback delta despite both context polls");
    VerifyNativeCameraWaitPollDoesNotAdvance(primary,task);
    VerifyNativeCameraWaitPollDoesNotAdvance(secondary,task);
    // Complete through ordinary task phases; wait polling must remain separate.
    unsigned steps{};
    while(primary.ActiveTask()!=0u && steps++<10000u)playback_owner.AdvancePlayback(.017f);
    require(primary.ActiveTask()==0u && primary.Waiting() && secondary.Waiting() &&
        primary.BlocksScript() && secondary.BlocksScript() && engine.GlobalStoryVariable()==1,
        "playback completion does not itself remove context waits or resume SDK");
    require(primary_runner.Tick(0.f,&primary.WaitHandlers())==NativeScriptFrameStatus::finished &&
        !primary.Waiting() && secondary.Waiting() && engine.GlobalStoryVariable()==2,
        "subsequent primary wait poll releases only primary continuation");
    require(secondary_runner.Tick(0.f,&secondary.WaitHandlers())==NativeScriptFrameStatus::finished &&
        !secondary.Waiting() && engine.GlobalStoryVariable()==3,
        "subsequent secondary poll independently releases shared-camera wait");
    primary_runner.ClearExecution();secondary_runner.ClearExecution();
    secondary.ClearHandlers();primary.ClearHandlers();
    // Actual SDK camera registration and playback retain their owner when an
    // already-closed rotation tail releases the shared context execution flag.
    auto ordered_waits = std::make_shared<NativeScriptEndActionQueue>();
    primary.BindWaitQueue(ordered_waits);
    primary_runner.Queue("camera_phase_probe","Start");
    require(primary_runner.Tick(.017f,&primary.WaitHandlers())==NativeScriptFrameStatus::suspended &&
        primary.BlocksScript() && primary.ActiveTask()!=0u,
        "real camera wait joins the context's ordered lifecycle queue");
    const auto retained_task=primary.ActiveTask();
    ordered_waits->AppendCompletedHandler(0x849550u,2,"105-Player-Movment");
    require(ordered_waits->Process([](auto&){throw std::logic_error("unexpected end action");}) &&
        !primary.BlocksScript() && primary.Waiting() && primary.ActiveTask()==retained_task,
        "closed rotation tail clears camera pause while retaining incomplete playback");
    require(primary_runner.TickUntilBoundary(0.f,&primary.WaitHandlers(),[]{return false;})==NativeScriptFrameStatus::finished &&
        engine.GlobalStoryVariable()==2 && primary.Waiting() && primary.ActiveTask()==retained_task,
        "actual SDK continuation advances without waiting for retained camera completion");
    primary_runner.ClearExecution();
    primary.ClearHandlers();
    require(ordered_waits->Empty(),"camera context cleanup unregisters all borrowed polls");
    primary.ClearCameraCollection(primary.ActiveCameraObject(primary.CameraSubsystem()));
    require(primary.ActiveTask()==0u,"explicit fixture teardown retires remaining camera task");
    engine.SetContextCallbacks(&primary_runner.Context(),nullptr);
    engine.SetContextCallbacks(&secondary_runner.Context(),nullptr);
    require(playback_owner.ActiveTask()==0u && !playback_owner.Waiting(),
        "phase probe leaves owner idle without destroying its shared sentinel");
}
}
