#include "pal4/native_script_movie_player.h"
#include "pal4/native_movie_owner_router.h"

#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace pal4 {

NativeScriptMoviePlayer::NativeScriptMoviePlayer(
    std::string original_root, asIScriptContext& context,
    INativeMovieFrameSink& frames, INativeMovieSessionHost& host)
    : root_(std::move(original_root)), context_(context), frames_(frames),
      host_(host), decoder_((root_ + "\\binkw32.dll").c_str()),
      source_(decoder_) {}

NativeScriptMoviePlayer::~NativeScriptMoviePlayer() {
    if(route_)route_->Forget(*this);
    if (!active_ && !volume_restore_pending_) return;
    source_.Close();
    try {
        host_.RestoreGameAudio(saved_volume_);
    } catch (...) {
    }
}

void NativeScriptMoviePlayer::BindRoute(NativeMovieOwnerRouter& route,
    std::function<void(float)> process_waits) {
    if(route_ && route_!=&route)
        throw std::logic_error("movie cannot replace its owner router");
    route.Register(*this,std::move(process_waits));
    route_=&route;
}

std::uint32_t NativeScriptMoviePlayer::Play(
    const std::string_view movie_name) {
    return Start(movie_name, false);
}

std::uint32_t NativeScriptMoviePlayer::PlayFinal(
    const std::string_view movie_name) {
    return Start(movie_name, true);
}

std::uint32_t NativeScriptMoviePlayer::Start(
    const std::string_view movie_name, const bool final_movie) {
    if (active_ || completion_pending_ || volume_restore_pending_ || asGetActiveContext() != &context_)
        throw std::logic_error("PAL4 movie requires its active script context");
    if (movie_name.empty() || movie_name.find_first_of("/\\:") !=
                                  std::string_view::npos ||
        movie_name == "." || movie_name == "..")
        throw std::invalid_argument("invalid PAL4 movie member name");

    auto* previous=route_?route_->Select(*this):nullptr;
    bool route_committed=false;
    // Restore selection on every decoder/audio/state-entry failure. Retain a
    // completed selection until its handler consumes completion or retires it.
    const auto rollback=[&](void*){if(route_ && !route_committed)route_->Restore(*this,previous);};
    std::unique_ptr<void,decltype(rollback)> route_guard(this,rollback);

    if (!decoder_.ConfigureMilesSound(host_.DigitalDriver()))
        throw std::runtime_error("PAL4 Bink cannot use Miles sound: " +
                                 decoder_.LastError());
    const std::string name(movie_name);
    const auto first = root_ + "\\gamedata\\VideoA.cpk";
    const auto second = root_ + "\\gamedata\\VideoB.cpk";
    if (!source_.Open(first.c_str(), name.c_str()) &&
        !source_.Open(second.c_str(), name.c_str()))
        throw std::runtime_error("PAL4 movie cannot open: " + name +
                                 ": " + decoder_.LastError());

    const auto info = decoder_.FrameInfo();
    if (info.width == 0u || info.height == 0u ||
        info.width > 4096u || info.height > 4096u ||
        info.frame_count == 0u ||
        info.width > std::numeric_limits<std::uint32_t>::max() / 4u) {
        source_.Close();
        throw std::runtime_error("PAL4 movie has invalid dimensions");
    }
    bool audio_muted = false;
    try {
        pixels_.resize(static_cast<std::size_t>(info.width) * info.height * 4u);
        saved_volume_ = host_.MuteGameAudio();
        audio_muted = true;
        final_movie_ = final_movie;
        finish_after_draw_ = false;
        frame_ready_ = false;
        completion_pending_ = false;
        if (context_.Suspend() < 0)
            throw std::runtime_error("PAL4 movie cannot suspend script");
        active_ = true;
        host_.EnterState(12u);
        if (final_movie) host_.EnterState(12u);
    } catch (...) {
        source_.Close();
        active_ = false;
        final_movie_ = false;
        pixels_.clear();
        if (audio_muted) {
            try {
                host_.RestoreGameAudio(saved_volume_);
            } catch (...) {
            }
        }
        throw;
    }
    route_committed=true;
    return 1u;
}

bool NativeScriptMoviePlayer::Tick(const bool obey_frame_clock) {
    if (!active_) return false;
    if (finish_after_draw_) {
        Finish();
        return true;
    }
    if (obey_frame_clock && decoder_.WaitForFrame()) return false;
    if (!decoder_.DecodeFrame(pixels_))
        throw std::runtime_error("PAL4 movie frame decode failed");
    const auto info = decoder_.FrameInfo();
    if (!frames_.Upload(pixels_, info.width, info.height))
        throw std::runtime_error("PAL4 movie frame upload failed");
    frame_ready_ = true;
    if (info.frame_number == info.frame_count) {
        finish_after_draw_ = true;
    } else if (!decoder_.NextFrame()) {
        throw std::runtime_error("PAL4 movie frame advance failed");
    }
    return false;
}

bool NativeScriptMoviePlayer::TakeCompletion() {
    if(!completion_pending_)return false;
    CompleteWait(true);
    return true;
}

bool NativeScriptMoviePlayer::Stop() {
    if (!active_) return false;
    Finish();
    return true;
}

bool NativeScriptMoviePlayer::Cancel() {
    if(!active_){CompleteWait(false);return false;}
    Finish(false);
    return true;
}

void NativeScriptMoviePlayer::Finish(bool notify) {
    source_.Close();
    active_ = false;
    completion_pending_ = notify;
    volume_restore_pending_=true;
    logo_return_pending_=notify && final_movie_;
    final_movie_ = false;
    finish_after_draw_ = false;
    frame_ready_ = false;
    pixels_.clear();
    // IDA5E6970 publishes completion only; 5E6860 performs the game side
    // effects when the script handler consumes it. Cancellation restores now.
    if(!notify)CompleteWait(false);
}

void NativeScriptMoviePlayer::CompleteWait(bool allow_logo) {
    auto& host=host_;
    const auto saved_volume=saved_volume_;
    const bool restore=std::exchange(volume_restore_pending_,false);
    const bool pending_logo=std::exchange(logo_return_pending_,false);
    const bool return_to_logo=allow_logo && pending_logo;
    completion_pending_=false;
    // A host state callback may retire this player; no member access follows.
    if(restore)host.RestoreGameAudio(saved_volume);
    if (return_to_logo) {
        host.ResetScreenEffect();
        host.EnterState(0u);
    }
}

}  // namespace pal4
