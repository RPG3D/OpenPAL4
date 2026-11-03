#include "pal4/portable/media.h"
#include "pal4/background_music_player.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
// Adapter for the existing music controller. It plans platform actions without
// pretending that scene music initializers or one-shot restoration are bound.
class MusicPlan final:public pal4::IBackgroundMusicRuntime {
public:
    MusicPlayback& playback;
    explicit MusicPlan(MusicPlayback& value):playback(value) {}
    std::string_view DefaultTrack() override{return {};}
    std::string_view MusicDirectory() override{return {};}
    std::uint32_t PlaySound(std::string_view path,bool once,bool) override {playback.member=path;playback.loop=!once;return 1;}
    std::uint32_t FadeInAudio(std::uint32_t,float seconds) override {playback.fade_in_seconds=seconds;return 0;}
    std::uint32_t FadeOutAudio(std::uint32_t,float seconds) override {playback.fade_out_seconds=seconds;return 0;}
    std::uint32_t StopAudioImmediately(std::uint32_t) override {playback.fade_out_seconds=0;return 0;}
    std::uint32_t CleanupTemporaryText() override{return 0;}
    std::uint32_t CleanupTemporaryPath() override{return 0;}
    [[noreturn]] static void Missing(){throw std::runtime_error("scene music/restoration service is not bound");}
    void InitializeScript() override{Missing();}
    std::uint32_t CreateInterpreter(std::string_view,std::string_view,std::string_view) override{Missing();}
    bool ExecuteInterpreter(std::uint32_t,pal4::BackgroundMusicPlayer&) override{Missing();}
    void HandleException(std::uint32_t) override{Missing();}
    void ReleaseException(std::uint32_t) override{Missing();}
    void CleanupScripts() override{Missing();}
    bool IsAudioActive(std::uint32_t) override{Missing();}
    std::uint8_t PauseAudio(std::uint32_t) override{Missing();}
    std::uint32_t SetAudioProperty(std::uint32_t,std::uint32_t) override{Missing();}
    std::uint8_t SavedTextAssignmentResult(std::string_view,bool) override{Missing();}
};
}
struct MediaServices::MusicState {pal4::BackgroundMusicPlayer player;};
MediaServices::MediaServices(IScriptContextServices& next,std::uint64_t generation):next_(next),generation_(generation),music_(std::make_shared<MusicState>()) {
    if(!generation)throw std::invalid_argument("media generation must be nonzero");
}
ScriptResponse MediaServices::Invoke(const ScriptRequest& request) {
    const auto& call=request.call;
    if(call.system_id!=127 && call.system_id!=237 && call.system_id!=238 && call.system_id!=297)return next_.Invoke(request);
    if(pending_)throw std::logic_error("another media operation is pending");
    MediaCommand command;command.generation=generation_;
    command.kind=call.system_id==127?MediaKind::play_music:call.system_id==297?MediaKind::movie:call.system_id==237?MediaKind::pause_music:MediaKind::resume_music;
    std::shared_ptr<MusicState> planned_music;
    if(call.system_id==127) {
        if(call.arguments.size()!=4)throw std::invalid_argument("invalid script music arguments");
        const auto& track=std::get<std::string>(call.arguments[0]);const auto mode=std::get<std::int32_t>(call.arguments[1]);
        const auto fade=std::get<float>(call.arguments[2]),restore=std::get<float>(call.arguments[3]);
        if(!std::isfinite(fade) || !std::isfinite(restore))throw std::invalid_argument("non-finite music fade");
        if(track.size()>128 || track.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-")!=std::string::npos || track.find("..")!=std::string::npos || (!track.empty() && track.front()=='.'))
            throw std::invalid_argument("invalid music member name");
        if(!track.empty() && mode==1)throw std::runtime_error("one-shot music restoration is not yet bound");
        planned_music=std::make_shared<MusicState>(*music_);MusicPlan plan(command.music);
        command.music.restore_fade_bits=std::bit_cast<std::uint32_t>(restore);
        (void)planned_music->player.PlayScriptTrack(track,static_cast<std::uint32_t>(mode),fade,command.music.restore_fade_bits,plan);
        if(track.empty()){music_=std::move(planned_music);return {};}
        for(auto& c:command.music.member)if(c>='A' && c<='Z')c=static_cast<char>(c+('a'-'A'));
    } else if(call.system_id==297) {
        if(call.arguments.size()!=1)throw std::invalid_argument("invalid movie arguments");
        command.movie=std::get<std::string>(call.arguments[0]);
        const auto& name=command.movie;
        if(name.size()<5 || name.size()>128 || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-")!=std::string::npos ||
            name.find("..")!=std::string::npos || name.front()=='.')throw std::invalid_argument("invalid movie member name");
        auto extension=name.substr(name.size()-4);
        for(auto& c:extension)if(c>='A' && c<='Z')c=static_cast<char>(c+('a'-'A'));
        if(extension!=".bik")throw std::invalid_argument("unsupported movie member extension");
    } else if(!call.arguments.empty())throw std::invalid_argument("invalid music transport arguments");
    if(next_id_==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("media operation ID exhausted");
    command.id=++next_id_;pending_=Pending{std::move(command),request.token,false,std::move(planned_music)};return {{},true};
}
std::optional<MediaCommand> MediaServices::TakeCommand() {
    if(!pending_ || pending_->dispatched)return {};
    pending_->dispatched=true;return pending_->command;
}
std::optional<MediaCompletion> MediaServices::Complete(std::uint64_t generation,std::uint64_t id,bool success,std::string error) {
    return Finish(generation,id,success,std::move(error),false);
}
std::optional<MediaCompletion> MediaServices::SkipMovie(std::uint64_t generation,std::uint64_t id){
    return Finish(generation,id,true,{},true);
}
std::optional<MediaCompletion> MediaServices::Finish(std::uint64_t generation,std::uint64_t id,bool success,std::string error,bool skipped){
    if(generation!=generation_ || !pending_ || !pending_->dispatched || pending_->command.id!=id)return {};
    const auto kind=pending_->command.kind;
    if(skipped && kind!=MediaKind::movie)return {};
    if(success) {
        if(kind==MediaKind::pause_music)music_paused_=true;
        if(kind==MediaKind::resume_music)music_paused_=false;
        if(kind==MediaKind::movie){if(skipped)++movies_skipped_;else ++movies_completed_;}
        if(kind==MediaKind::play_music){music_=std::move(pending_->music);++music_started_;music_operation_=id;music_paused_=false;}
        error.clear();
    } else if(error.empty())error="platform media operation failed";
    MediaCompletion result{pending_->token,kind,std::move(error)};pending_.reset();return result;
}
void MediaServices::Cancel() {pending_.reset();music_operation_=0;}
std::string_view MediaServices::MusicTrack() const{return music_->player.CurrentTrack();}
} // namespace pal4::portable
