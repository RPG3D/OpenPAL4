#include "pal4/effect_render_parameters.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_flash_runtime.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/pal_screen_effect.h"
#include "pal4/script_music_transport.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <array>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

class RenderRuntime final : public pal4::IEffectRenderRuntime {
public:
    std::uint32_t DestroyTexture(std::uint32_t texture) override {
        Check(texture == 1u, "release flash texture");
        return 1u;
    }
    std::uint32_t TextureDictionary() override { return 1u; }
    std::uint32_t CreateTexture(std::uint32_t dictionary,
                                std::uint32_t width, std::uint32_t height,
                                std::uint32_t format,
                                std::uint32_t color) override {
        Check(dictionary == 1u && width == 32u && height == 32u &&
                  format == 1280u && color == 0xffffffffu,
              "initialize flash texture through existing effect handler");
        return 1u;
    }
    std::int32_t FloatToLong(float value) override {
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t DrawTexturedRectangle(
        std::uint32_t, std::int32_t, std::int32_t, std::int32_t,
        std::int32_t, std::uint32_t, std::uint8_t r, std::uint8_t g,
        std::uint8_t b, std::int32_t alpha) override {
        last_color={r,g,b};last_alpha=alpha;
        return 1u;
    }
    float Interpolate(float first, float last, float fraction,
                      std::uint32_t mode) override {
        Check(mode == 0u, "PAL4 linear fade interpolation");
        return first + (last - first) * fraction;
    }
    std::array<std::uint8_t,3> last_color{};
    int last_alpha{};
};

class MusicRuntime final : public pal4::IScriptMusicTransportRuntime {
public:
    std::uint32_t BackgroundMusicPlayer() override { return 0x1000u; }
    std::uint8_t StopSound(std::uint32_t slot) override {
        Check(slot == 0x1030u, "music slot after fade");
        ++pauses;
        return 1u;
    }
    std::uint8_t LoadAndPlay(std::uint32_t) override { return 1u; }
    int pauses{};
};

}  // namespace

int main() {
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] =
        "void Wait() { giFlashOutBlack(0.1f, true, true); "
        "giScriptMusicPause(); } "
        "void NoWait() { giFlashOutBlack(0.1f, true, false); "
        "giScriptMusicPause(); } "
        "void ZeroWait() { giFlashOutBlack(0.0f, true, true); "
        "giScriptMusicPause(); }"
        "void InBlack() { giFlashInBlack(0.1f,true); giScriptMusicPause(); }"
        "void OutWhite() { giFlashOutWhite(0.1f,true,true); giScriptMusicPause(); }"
        "void InWhite() { giFlashInWhite(0.1f,true); giScriptMusicPause(); }";
    Check(engine.Raw()->AddScriptSection("flash_wait", "flash_wait.cs",
                                         source, sizeof(source) - 1) >= 0 &&
              engine.Raw()->Build("flash_wait") >= 0,
          "compile flash wait script");
    pal4::NativeScriptFrameRunner runner(engine);

    pal4::PalScreenEffectState effect{};
    RenderRuntime render;
    (void)pal4::EffectRenderControl::Initialize(effect.render, 640, 480);
    pal4::EffectRenderControl::InitializeTexture(effect.render, render);
    pal4::NativeScriptFlashRuntime flash(effect, render, runner.Context());
    MusicRuntime music;
    pal4::NativeGameScriptCallbacks callbacks({&flash.Flash(), &music});
    engine.SetCallbacks(&callbacks);

    runner.Queue("flash_wait", "Wait");
    Check(runner.Tick(0.0f, &flash) ==
                  pal4::NativeScriptFrameStatus::suspended &&
              flash.Waiting() && music.pauses == 0 &&
              effect.render.phase == 0u && effect.render.active == 1u,
          "blocking fade suspends the VM before the next game callback");
    Check(runner.Tick(0.05f, &flash) ==
                  pal4::NativeScriptFrameStatus::suspended &&
              flash.Waiting() &&
              effect.render.phase == 0u && effect.render.alpha > 120.0f &&
              effect.render.alpha < 135.0f,
          "half fade keeps the original script suspended");
    Check(runner.Tick(0.05f, &flash) ==
                  pal4::NativeScriptFrameStatus::finished &&
              !flash.Waiting() && music.pauses == 1 &&
              effect.render.phase == 1u && effect.render.alpha == 255.0f &&
              effect.render.active == 1u,
          "completed fade resumes script and holds black");

    runner.Queue("flash_wait", "NoWait");
    Check(runner.Tick(0.0f, &flash) ==
                  pal4::NativeScriptFrameStatus::finished &&
              flash.Waiting() && music.pauses == 2,
          "nonblocking fade registers a handler without suspending script");
    Check(runner.Tick(0.1f, &flash) ==
                  pal4::NativeScriptFrameStatus::finished &&
              !flash.Waiting(),
          "nonblocking fade handler is released after completion");

    runner.Queue("flash_wait", "ZeroWait");
    Check(runner.Tick(0.0f, &flash) ==
                  pal4::NativeScriptFrameStatus::suspended &&
              effect.render.phase == 1u && effect.render.alpha == 255.0f &&
              runner.Tick(0.0f, &flash) ==
                  pal4::NativeScriptFrameStatus::finished &&
              !flash.Waiting() && music.pauses == 3,
          "zero-duration fade closes without a division by zero");

    runner.Queue("flash_wait", "Wait");
    Check(runner.Tick(0.0f, &flash) == pal4::NativeScriptFrameStatus::suspended &&
              flash.Waiting(), "prepare a flash wait for execution cleanup");
    const auto pauses_before_cleanup = music.pauses;
    runner.ClearExecution();
    flash.ClearHandlers();
    Check(!flash.Waiting() && effect.render.active == 1u &&
              runner.Tick(0.0f) == pal4::NativeScriptFrameStatus::idle &&
              music.pauses == pauses_before_cleanup,
          "cleanup closes flash waits without completing the old script or cancelling the effect");
    flash.ClearHandlers();

    // Production Scene owns one shared clock. Two independent context wait
    // owners must not shorten the fade or stop it when a script finishes.
    // The preceding cancellation intentionally left the global fade running.
    // Finish it before asking the original callback to start another fade.
    pal4::EffectRenderControl::AdvanceFade(effect.render, 0.1f, render);
    flash.UseWorldClock(true);
    pal4::NativeScriptFlashRuntime second(effect, render, runner.Context());
    second.UseWorldClock(true);
    runner.Queue("flash_wait", "Wait");
    Check(runner.Tick(0.0f, &flash) == pal4::NativeScriptFrameStatus::suspended,
          "world-clock fade starts from actual SDK callback");
    (void)runner.Tick(0.05f, &flash); (void)second.Tick(0.05f);
    Check(effect.render.elapsed == 0.0f, "context waits never advance the shared world clock");
    pal4::EffectRenderControl::AdvanceFade(effect.render, 0.05f, render);
    Check(runner.Tick(0.05f, &flash) == pal4::NativeScriptFrameStatus::suspended &&
          effect.render.elapsed == 0.05f && effect.render.alpha > 120 && effect.render.alpha < 135,
          "one world update produces half alpha independent of context count");
    pal4::EffectRenderControl::AdvanceFade(effect.render, 0.05f, render);
    Check(runner.Tick(0.0f, &flash) == pal4::NativeScriptFrameStatus::finished && !flash.Waiting(),
          "world completion resumes the blocking caller");
    runner.Queue("flash_wait", "NoWait");
    Check(runner.Tick(0.0f, &flash) == pal4::NativeScriptFrameStatus::finished && flash.Waiting(),
          "nonblocking caller finishes before its world-owned fade");
    pal4::EffectRenderControl::AdvanceFade(effect.render, 0.1f, render);
    (void)flash.Tick(0.0f);
    Check(!flash.Waiting() && effect.render.alpha == 255.0f && effect.render.active == 1u,
          "world-owned fade completes and holds black after the caller finished");
    for (const auto name : {"InBlack", "OutWhite", "InWhite"}) {
        runner.Queue("flash_wait", name);
        Check(runner.Tick(0.0f, &flash) == pal4::NativeScriptFrameStatus::suspended,
              "black/white fade callback suspends until world completion");
        pal4::EffectRenderControl::AdvanceFade(effect.render, 0.05f, render);
        Check(pal4::EffectRenderControl::Draw(effect.render, 0x1000u, render) != 0 &&
                  render.last_alpha >= 126 && render.last_alpha <= 128 &&
                  render.last_color == (std::string(name)=="InBlack"
                      ? std::array<std::uint8_t,3>{0,0,0} : std::array<std::uint8_t,3>{255,255,255}),
              "actual shared draw boundary submits correct half alpha and black/white color");
        pal4::EffectRenderControl::AdvanceFade(effect.render, 0.05f, render);
        Check(runner.Tick(0.0f, &flash) == pal4::NativeScriptFrameStatus::finished &&
                  effect.render.active == (std::string(name)=="OutWhite" ? 1u : 0u) &&
                  effect.render.alpha == (std::string(name)=="OutWhite" ? 255.0f : 0.0f),
              "fade-out holds its color and fade-in releases the overlay");
    }

    engine.SetCallbacks(nullptr);
    (void)pal4::EffectRenderControl::DestroyTexture(effect.render, render);
    return EXIT_SUCCESS;
}
