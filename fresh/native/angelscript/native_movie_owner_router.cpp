#include "pal4/native_movie_owner_router.h"
#include "pal4/native_script_movie_player.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace pal4 {
void NativeMovieOwnerRouter::Register(NativeScriptMoviePlayer& player,
    std::function<void(float)> process_waits) {
    if(!process_waits || player.Waiting() || player.CompletionPending())
        throw std::invalid_argument("movie route requires an idle owner and continuation");
    owners_.insert_or_assign(&player,std::move(process_waits));
}
void NativeMovieOwnerRouter::Forget(NativeScriptMoviePlayer& player) noexcept {
    if(selected_==&player)selected_=nullptr;
    owners_.erase(&player);
}
NativeScriptMoviePlayer* NativeMovieOwnerRouter::Current() const noexcept {
    return selected_ && owners_.contains(selected_)?selected_:nullptr;
}
NativeScriptMoviePlayer* NativeMovieOwnerRouter::Select(NativeScriptMoviePlayer& player) {
    if(!owners_.contains(&player))throw std::logic_error("movie owner is not registered");
    auto* previous=Current();
    if(previous && (previous->Waiting() || previous->CompletionPending()))
        throw std::logic_error("movie playback or completion still belongs to another request");
    selected_=&player;
    return previous;
}
void NativeMovieOwnerRouter::Restore(NativeScriptMoviePlayer& player,
    NativeScriptMoviePlayer* previous) noexcept {
    if(selected_==&player)selected_=previous && owners_.contains(previous)?previous:nullptr;
}
bool NativeMovieOwnerRouter::Playing() const {
    auto* player=Current();return player && player->Waiting();
}
bool NativeMovieOwnerRouter::Tick(bool clock) {
    auto* player=Current();return player && player->Tick(clock);
}
bool NativeMovieOwnerRouter::Stop() {
    auto* player=Current();return player && player->Stop();
}
bool NativeMovieOwnerRouter::ProcessWaits(float elapsed) {
    if(!std::isfinite(elapsed) || elapsed<0.0f)
        throw std::invalid_argument("movie route frame time is invalid");
    auto* player=Current();if(!player)return false;
    auto process=owners_.at(player);
    process(elapsed);
    return true;
}
}
