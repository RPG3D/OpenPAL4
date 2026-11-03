#pragma once
#include "pal4/portable/script_stack.h"

namespace pal4::portable {
enum class MediaKind {pause_music,resume_music,movie,play_music};
struct MusicPlayback {
    std::string member;
    bool loop{};
    float fade_in_seconds{},fade_out_seconds{};
    std::uint32_t restore_fade_bits{};
};
struct MediaCommand {
    std::uint64_t generation{},id{};
    MediaKind kind{};
    std::string movie;
    MusicPlayback music;
};
struct MediaCompletion {ScriptToken token;MediaKind kind;std::string error;};
// Platform playback is asynchronous. IDs distinguish separate operations;
// generation distinguishes game restarts, even when script tokens repeat.
class MediaServices final:public IScriptContextServices {
public:
    MediaServices(IScriptContextServices& next,std::uint64_t generation);
    void OnCall(const ScriptRequest& request) override {next_.OnCall(request);}
    ScriptResponse Invoke(const ScriptRequest& request) override;
    std::optional<MediaCommand> TakeCommand();
    std::optional<MediaCompletion> Complete(std::uint64_t generation,std::uint64_t id,bool success,std::string error={});
    std::optional<MediaCompletion> SkipMovie(std::uint64_t generation,std::uint64_t id);
    void Cancel();
    bool Waiting() const {return pending_.has_value();}
    bool MusicPaused() const {return music_paused_;}
    unsigned MoviesCompleted() const {return movies_completed_;}
    unsigned MoviesSkipped() const {return movies_skipped_;}
    unsigned MusicStarted() const {return music_started_;}
    std::uint64_t MusicOperation() const {return music_operation_;}
    std::string_view MusicTrack() const;
    std::uint64_t Generation() const {return generation_;}
private:
    std::optional<MediaCompletion> Finish(std::uint64_t generation,std::uint64_t id,bool success,std::string error,bool skipped);
    struct MusicState;
    struct Pending {MediaCommand command;ScriptToken token;bool dispatched{};std::shared_ptr<MusicState> music;};
    IScriptContextServices& next_;
    const std::uint64_t generation_;
    std::uint64_t next_id_{};
    std::optional<Pending> pending_;
    bool music_paused_{};
    unsigned movies_completed_{},movies_skipped_{};
    unsigned music_started_{};
    std::uint64_t music_operation_{};
    std::shared_ptr<MusicState> music_;
};
} // namespace pal4::portable
