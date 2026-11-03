#pragma once

#include "pal4/native_bink_decoder.h"
#include "pal4/native_bink_package_source.h"
#include "pal4/native_movie_frame_sink.h"
#include "pal4/script_movie_player.h"

#include <angelscript.h>

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace pal4 {
class NativeMovieOwnerRouter;

class INativeMovieSessionHost {
public:
    virtual ~INativeMovieSessionHost() = default;
    // The game audio owner keeps this driver alive until movie playback ends.
    [[nodiscard]] virtual void* DigitalDriver() = 0;
    [[nodiscard]] virtual float MuteGameAudio() = 0;
    virtual void RestoreGameAudio(float previous_volume) = 0;
    virtual void EnterState(std::uint32_t state) = 0;
    virtual void ResetScreenEffect() = 0;
};

// Owns the movie wait for one AngelScript context. The frame owner calls Tick,
// draws the frame sink inside its camera update, then resumes the script when
// Tick or Stop reports completion.
class NativeScriptMoviePlayer final : public IScriptMoviePlayer {
public:
    NativeScriptMoviePlayer(std::string original_root,
                            asIScriptContext& context,
                            INativeMovieFrameSink& frames,
                            INativeMovieSessionHost& host);
    ~NativeScriptMoviePlayer() override;
    // Optional selected-owner adapter; the router must outlive this player.
    void BindRoute(NativeMovieOwnerRouter&, std::function<void(float)> process_waits);

    [[nodiscard]] std::uint32_t Play(std::string_view movie_name) override;
    [[nodiscard]] std::uint32_t PlayFinal(
        std::string_view movie_name) override;
    [[nodiscard]] bool Tick(bool obey_frame_clock = true);
    [[nodiscard]] bool Stop();
    // Retire a displaced/aborted wait without resuming it or entering Logo.
    [[nodiscard]] bool Cancel();
    [[nodiscard]] bool Waiting() const { return active_; }
    [[nodiscard]] bool FrameReady() const { return frame_ready_; }
    // Render/skip publishes completion only. Handler consumption restores game
    // audio and applies final-Logo effects; a host callback may retire this player.
    [[nodiscard]] bool CompletionPending() const { return completion_pending_; }
    [[nodiscard]] bool TakeCompletion();
    [[nodiscard]] NativeBinkFrameInfo FrameInfo() const {
        return decoder_.FrameInfo();
    }

private:
    [[nodiscard]] std::uint32_t Start(std::string_view movie_name,
                                      bool final_movie);
    void Finish(bool notify = true);
    void CompleteWait(bool allow_logo);

    std::string root_;
    asIScriptContext& context_;
    INativeMovieFrameSink& frames_;
    INativeMovieSessionHost& host_;
    NativeBinkDecoder decoder_;
    NativeBinkPackageSource source_;
    std::vector<std::uint8_t> pixels_;
    float saved_volume_{};
    bool active_{};
    bool final_movie_{};
    bool finish_after_draw_{};
    bool frame_ready_{};
    bool completion_pending_{};
    NativeMovieOwnerRouter* route_{};
    bool volume_restore_pending_{};
    bool logo_return_pending_{};
};

}  // namespace pal4
