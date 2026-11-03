#pragma once
#include <functional>
#include <unordered_map>

namespace pal4 {
class NativeScriptMoviePlayer;

// Borrows movie owners and their handler-only continuation endpoints. The router
// must outlive registered players. All calls run on the game/SDK thread.
class NativeMovieOwnerRouter final {
public:
    NativeMovieOwnerRouter() = default;
    NativeMovieOwnerRouter(const NativeMovieOwnerRouter&) = delete;
    NativeMovieOwnerRouter& operator=(const NativeMovieOwnerRouter&) = delete;
    [[nodiscard]] NativeScriptMoviePlayer* Current() const noexcept;
    [[nodiscard]] bool Playing() const;
    [[nodiscard]] bool Tick(bool obey_movie_clock = true);
    [[nodiscard]] bool Stop();
    // Copies the endpoint before calling it: a continuation may retire its own
    // movie/context. Do not retain Current() across this call.
    [[nodiscard]] bool ProcessWaits(float elapsed);
    [[nodiscard]] std::size_t RegisteredCount() const noexcept { return owners_.size(); }
private:
    friend class NativeScriptMoviePlayer;
    void Register(NativeScriptMoviePlayer&, std::function<void(float)> process_waits);
    void Forget(NativeScriptMoviePlayer&) noexcept;
    NativeScriptMoviePlayer* Select(NativeScriptMoviePlayer&);
    void Restore(NativeScriptMoviePlayer&, NativeScriptMoviePlayer* previous) noexcept;
    std::unordered_map<NativeScriptMoviePlayer*, std::function<void(float)>> owners_;
    NativeScriptMoviePlayer* selected_{};
};
}
