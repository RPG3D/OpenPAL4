#include "pal4/effect_render_parameters.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_dialog_settings_file_writer.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_background_music_runtime.h"
#include "pal4/native_miles_music_stream.h"
#include "pal4/native_movie_flag_runtime.h"
#include "pal4/native_script_flash_runtime.h"
#include "pal4/native_script_movie_player.h"
#include "pal4/native_movie_owner_router.h"
#include "pal4/pal_screen_effect.h"
#include "pal4/script_music_transport.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Music final : public pal4::IScriptMusicTransportRuntime {
public:
    std::uint32_t BackgroundMusicPlayer() override { return 0x1000u; }
    std::uint8_t StopSound(std::uint32_t slot) override {
        Check(slot == 0x1030u, "NewGame music pause slot");
        ++pauses;
        return 1u;
    }
    std::uint8_t LoadAndPlay(std::uint32_t) override { return 1u; }
    int pauses{};
};

class Frames final : public pal4::INativeMovieFrameSink {
public:
    bool Upload(std::span<const std::uint8_t> pixels,
                std::uint32_t width, std::uint32_t height) override {
        Check(width == 800u && height == 600u &&
              pixels.size() == static_cast<std::size_t>(width) * height * 4u,
              "decoded original movie frame shape");
        ++uploads;
        return true;
    }
    int uploads{};
};

class Host final : public pal4::INativeMovieSessionHost {
public:
    explicit Host(pal4::NativeMilesDigitalDriver& miles,pal4::NativeBackgroundMusicRuntime* background=nullptr)
        : miles_(miles),background_(background) {}
    void* DigitalDriver() override { return miles_.Handle(); }
    float MuteGameAudio() override {
        ++mutes;
        if(background_){const auto saved=background_->Volume();background_->SetVolume(0.0f);return saved;}
        return 0.75f;
    }
    void RestoreGameAudio(float volume) override {
        if(background_)background_->SetVolume(volume);
        else Check(volume == 0.75f, "restore saved movie audio volume");
        ++restores;
    }
    void EnterState(std::uint32_t state) override {
        Check(state == 12u || state == 0u, "movie game state");
        if(reject_entry)throw std::runtime_error("movie state entry failure");
        states.push_back(state);
        if(on_state)on_state(state);
    }
    void ResetScreenEffect() override { ++screen_resets; }

    int mutes{};
    int restores{};
    int screen_resets{};
    bool reject_entry{};
    std::function<void(std::uint32_t)> on_state;
    std::vector<std::uint32_t> states;
private:
    pal4::NativeMilesDigitalDriver& miles_;
    pal4::NativeBackgroundMusicRuntime* background_;
};

void MovieMusicVolume(const std::string& root,pal4::NativeMilesDigitalDriver& miles){
    pal4::NativeMilesMusicStream opening(root,miles);
    Check(opening.Open("P01-1.mp3"),"open actual opening music for movie mute");
    pal4::NativeBackgroundMusicRuntime background(root,miles,opening);background.SetVolume(0.42f);
    const auto scene=background.PlaySound("gamedata\\Music\\P23.mp3",false,false);
    Check(scene && background.Streams().size()==2u,"movie mute covers two live original music streams");
    const auto levels=[&](float expected){for(auto* stream:background.Streams())
        Check(stream->VolumeLevels()==std::array<float,2>{expected,expected},"actual Miles stream levels match movie audio phase");};
    levels(0.42f);
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[]="void run(){giPlayMovie(\"1A.bik\");} void final(){giPlayMovieFinal(\"1A.bik\");}";
    Check(engine.Raw()->AddScriptSection("movie_volume","fixture",source,sizeof(source)-1)>=0 &&
        engine.Raw()->Build("movie_volume")>=0,"compile movie/BGM SDK lifecycle");
    asIScriptContext* context{};Check(engine.Raw()->CreateContext(&context)>=0,"create movie/BGM SDK context");
    {
        Frames frames;Host host(miles,&background);
        pal4::NativeScriptMoviePlayer movie(root,*context,frames,host);
        pal4::NativeGameScriptServices services{};services.movie=&movie;
        pal4::NativeGameScriptCallbacks callbacks(services);engine.SetContextCallbacks(context,&callbacks);
        const auto execute=[&](const char* name){Check(context->Prepare(engine.FunctionId("movie_volume",name))>=0,"prepare movie/BGM call");return context->Execute();};
        Check(execute("run")==asEXECUTION_SUSPENDED && background.Volume()==0.0f,"movie actually mutes background owner");levels(0.0f);
        Check(!movie.Tick(false) && movie.Stop() && movie.CompletionPending() && background.Volume()==0.0f,
            "video/skip completion keeps BGM muted until handler consumption");levels(0.0f);
        Check(movie.TakeCompletion() && background.Volume()==0.42f && context->Execute()==asEXECUTION_FINISHED,
            "movie wait restores saved BGM level before continuing SDK");levels(0.42f);
        Check(execute("run")==asEXECUTION_SUSPENDED && movie.Cancel() && !movie.CompletionPending() &&
            background.Volume()==0.42f && context->Abort()>=0,"cancelled wait restores real BGM immediately");levels(0.42f);
        host.reject_entry=true;
        Check(execute("run")==asEXECUTION_EXCEPTION && background.Volume()==0.42f && !movie.Waiting(),
            "state-entry failure restores actual music after muting");levels(0.42f);host.reject_entry=false;
        Check(execute("final")==asEXECUTION_SUSPENDED && movie.Stop() && background.Volume()==0.0f && !host.screen_resets,
            "final movie defers BGM restoration and Logo to its wait");levels(0.0f);
        Check(movie.TakeCompletion() && background.Volume()==0.42f && host.screen_resets==1u &&
            context->Execute()==asEXECUTION_FINISHED && host.mutes==host.restores,
            "final handler restores actual BGM and balances all failure/cancel paths");levels(0.42f);
        engine.SetContextCallbacks(context,nullptr);
    }
    (void)context->Release();
    std::cout<<"Movie/BGM integration: two original Miles streams mute, remain muted through completion, restore on wait/cancel/failure verified\n";
}

void RoutedMovies(const std::string& root,pal4::NativeMilesDigitalDriver& miles) {
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[]=
        "void run(){giPlayMovie(\"1A.bik\");global_mvar+=10;}"
        "void final(){giPlayMovieFinal(\"1A.bik\");}"
        "void missing(){giPlayMovie(\"__missing_owner_probe__.bik\");}";
    Check(engine.Raw()->AddScriptSection("routed_movies","fixture",source,sizeof(source)-1)>=0 &&
        engine.Raw()->Build("routed_movies")>=0,"compile routed movie SDK fixture");
    asIScriptContext* first{};asIScriptContext* second{};
    Check(engine.Raw()->CreateContext(&first)>=0 && engine.Raw()->CreateContext(&second)>=0 &&
        first && second,"create independent movie SDK contexts");
    {
        pal4::NativeMovieOwnerRouter router;Frames frames;Host host(miles);
        pal4::NativeScriptMoviePlayer primary(root,*first,frames,host);
        auto child=std::make_unique<pal4::NativeScriptMoviePlayer>(root,*second,frames,host);
        pal4::NativeGameScriptServices first_services{},second_services{};
        first_services.movie=&primary;second_services.movie=child.get();
        pal4::NativeGameScriptCallbacks first_callbacks(first_services),second_callbacks(second_services);
        engine.SetContextCallbacks(first,&first_callbacks);engine.SetContextCallbacks(second,&second_callbacks);
        unsigned first_resumes{},second_resumes{};
        primary.BindRoute(router,[&](float){
            if(primary.TakeCompletion()){
                ++first_resumes;Check(first->Execute()==asEXECUTION_FINISHED,"resume selected primary SDK context");
            }
        });
        const auto bind_child=[&]{child->BindRoute(router,[&](float){
            if(child->TakeCompletion()){
                ++second_resumes;Check(second->Execute()==asEXECUTION_FINISHED,"resume selected child SDK context");
                child.reset(); // Erases the router's stored callback during dispatch.
            }
        });};
        bind_child();
        const auto execute=[&](asIScriptContext* context,const char* function){
            Check(context->Prepare(engine.FunctionId("routed_movies",function))>=0,"prepare routed movie request");
            return context->Execute();
        };
        Check(execute(first,"run")==asEXECUTION_SUSPENDED && router.Current()==&primary && router.Playing(),
            "first active SDK context selects its actual movie owner");
        const auto mutes=host.mutes;
        Check(execute(second,"run")==asEXECUTION_EXCEPTION && router.Current()==&primary && host.mutes==mutes &&
            !child->Waiting(),"overlapping context cannot replace active decoder or audio owner");
        const auto uploads=frames.uploads;
        Check(!router.Tick(false) && frames.uploads==uploads+1 && router.Stop() &&
            !router.Playing() && primary.CompletionPending(),"selected original movie decodes then publishes skip completion");
        Check(execute(second,"run")==asEXECUTION_EXCEPTION && router.Current()==&primary,
            "unconsumed movie completion cannot be stolen by another script");
        Check(router.ProcessWaits(0.0f) && first_resumes==1u && !second_resumes && engine.GlobalStoryVariable()==10,
            "movie completion resumes only its selected SDK context");
        Check(execute(second,"run")==asEXECUTION_SUSPENDED && router.Current()==child.get(),
            "consumed completion permits next context to select its own player");
        Check(!router.Tick(false) && router.Stop() && router.ProcessWaits(0.0f) && !child &&
            !router.Current() && router.RegisteredCount()==1u && second_resumes==1u && engine.GlobalStoryVariable()==20,
            "copied continuation survives its own movie owner and callback retirement");
        Check(execute(first,"missing")==asEXECUTION_EXCEPTION && !router.Current() && !primary.Waiting(),
            "missing original resource restores previous movie selection");
        host.reject_entry=true;
        Check(execute(first,"run")==asEXECUTION_EXCEPTION && !router.Current() && host.mutes==host.restores,
            "state-entry failure restores route, decoder and game-audio balance");
        host.reject_entry=false;
        Check(execute(first,"run")==asEXECUTION_SUSPENDED && router.Stop() && router.ProcessWaits(0.0f) &&
            first_resumes==2u,"failed requests can retry with the same live player");
        child=std::make_unique<pal4::NativeScriptMoviePlayer>(root,*second,frames,host);
        second_services.movie=child.get();second_callbacks.SetServices(second_services);bind_child();
        Check(execute(second,"run")==asEXECUTION_SUSPENDED && second->Abort()>=0,"prepare abrupt child teardown");
        child.reset();
        Check(!router.Current() && !router.Playing() && !router.Stop() && !router.ProcessWaits(0.0f) &&
            host.mutes==host.restores,"active child retirement clears borrowed route and restores audio");
        child=std::make_unique<pal4::NativeScriptMoviePlayer>(root,*second,frames,host);
        second_services.movie=child.get();second_callbacks.SetServices(second_services);bind_child();
        host.on_state=[&](auto state){if(state==0u){(void)second->Abort();child.reset();}};
        child->BindRoute(router,[&](float){Check(child->TakeCompletion(),"consume final completion before self retirement");});
        Check(execute(second,"final")==asEXECUTION_SUSPENDED && router.Stop() && child &&
            child->CompletionPending() && router.ProcessWaits(0.0f) && !child && !router.Current() &&
            host.mutes==host.restores,"final state transition can retire its own movie after completion is published");
        host.on_state={};
        child=std::make_unique<pal4::NativeScriptMoviePlayer>(root,*second,frames,host);
        second_services.movie=child.get();second_callbacks.SetServices(second_services);bind_child();
        const auto resets_before_cancel=host.screen_resets;
        Check(execute(second,"final")==asEXECUTION_SUSPENDED && child->Cancel() &&
            !child->CompletionPending() && host.screen_resets==resets_before_cancel && host.mutes==host.restores,
            "cancelling a displaced final movie clears its wait without entering Logo");
        Check(second->Abort()>=0 && execute(first,"run")==asEXECUTION_SUSPENDED && router.Current()==&primary &&
            router.Stop() && primary.CompletionPending() && !primary.Cancel() && !primary.CompletionPending(),
            "clearing a completed wait consumes its stale notification before another owner starts");
        Check(first->Abort()>=0,"abort cancelled primary fixture");child.reset();
        engine.SetContextCallbacks(first,nullptr);engine.SetContextCallbacks(second,nullptr);
    }
    (void)first->Release();(void)second->Release();
    std::cout<<"Movie owner router: original decode, independent SDK owners, skip/resume, rollback and callback retirement verified\n";
}

class SettingsSource final : public pal4::INativeDialogSettingsStateSource {
public:
    void SynchronizeStoryData(pal4::DialogManagerState&) override { ++saves; }
    float AudioLevel(std::uint32_t) override { return 0.5f; }
    std::uint32_t FirstNode(const pal4::DialogManagerState& state,
                            std::size_t offset) override {
        return state.Word(offset + 4u);
    }
    std::uint32_t NextNode(std::uint32_t, bool) override {
        throw std::logic_error("unexpected dialog tree node");
    }
    std::uint32_t NodeValue(std::uint32_t) override {
        throw std::logic_error("unexpected dialog tree value");
    }
    std::uint32_t StoryWord(std::uint32_t) override {
        throw std::logic_error("unexpected story array");
    }
    int saves{};
};

class TemporarySettingsDirectory final {
public:
    TemporarySettingsDirectory() {
        const auto stamp = std::chrono::steady_clock::now()
                               .time_since_epoch().count();
        path_ = std::filesystem::path("H:/PAL4/objs") /
                ("pal4-script-settings-" + std::to_string(stamp));
        Check(std::filesystem::create_directory(path_),
              "create isolated script settings directory");
    }
    ~TemporarySettingsDirectory() {
        std::error_code error;
        std::filesystem::remove(path_ / "pal4.set", error);
        std::filesystem::remove(path_, error);
    }
    [[nodiscard]] const std::filesystem::path& Path() const { return path_; }

private:
    std::filesystem::path path_;
};

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "provide original PAL4 installation path");
        const std::string root(argv[1]);
        pal4::NativePackageArchive archive;
        Check(archive.Open((root + "\\gamedata\\script.cpk").c_str()),
              "open original script.cpk");
        const auto script = archive.ReadMember("script.csb");
        Check(script && script->size() > 4u, "read original script.csb");
        std::uint32_t size{};
        std::memcpy(&size, script->data(), sizeof(size));
        Check(size == script->size() - 4u, "original script length");

        pal4::NativeAngelScriptEngine engine;
        constexpr char module[] = "gamedata\\editData\\script\\script.csb";
        const auto loaded = engine.LoadBytecode(
            module, {reinterpret_cast<const std::byte*>(script->data() + 4u),
                     size});
        Check(loaded.code == 0, "restore original script bytecode");
        asIScriptContext* context{};
        Check(engine.Raw()->CreateContext(&context) >= 0 && context,
              "create original script context");

        pal4::PalScreenEffectState screen{};
        pal4::EffectRenderRuntimeStub render;
        (void)pal4::EffectRenderControl::Initialize(screen.render, 640, 480);
        screen.render.texture = 1u;
        pal4::NativeScriptFlashRuntime flash(screen, render, *context);
        Music music;
        Frames frames;
        pal4::NativeMilesDigitalDriver miles(root);
        Check(miles.Open(), "open shared original Miles driver");
        miles.SetMasterVolume(0.0f);
        Host host(miles);
        pal4::NativeScriptMoviePlayer movie(root, *context, frames, host);
        pal4::DialogManagerState global_data;
        TemporarySettingsDirectory settings_directory;
        SettingsSource settings_source;
        pal4::NativeDialogSettingsFileWriter settings(
            settings_directory.Path(), settings_source);
        pal4::NativeMovieFlagRuntime movie_flags(global_data, settings);
        pal4::NativeGameScriptCallbacks callbacks(
            {&flash.Flash(), &music, &movie, &movie_flags});
        engine.SetCallbacks(&callbacks);

        const auto new_game = engine.FunctionId(module, "NewGame");
        Check(new_game >= 0 && context->Prepare(new_game) >= 0 &&
              context->Execute() == asEXECUTION_SUSPENDED,
              "original NewGame starts with blocking fade");
        Check(flash.Tick(0.1f) &&
              context->Execute() == asEXECUTION_SUSPENDED &&
              movie.Waiting() && music.pauses == 1 &&
              host.states == std::vector<std::uint32_t>{12u},
              "first original movie suspends NewGame in state 12");
        const auto first_frame_count = movie.FrameInfo().frame_count;
        Check(first_frame_count == 855u, "first original movie frame count");
        for (std::uint32_t frame = 0u; frame < first_frame_count; ++frame)
            Check(!movie.Tick(false) && movie.FrameReady(),
                  "decode each frame of first original movie");
        Check(frames.uploads == static_cast<int>(first_frame_count) &&
              movie.Tick(false) && !movie.Waiting() && movie.CompletionPending() &&
              movie.TakeCompletion() && !movie.CompletionPending() && !movie.TakeCompletion(),
              "first original movie publishes completion once after its final drawn frame");

        Check(context->Execute() == asEXECUTION_SUSPENDED &&
              movie.Waiting() &&
              host.states == std::vector<std::uint32_t>{12u, 12u},
              "original script resumes into second movie");
        Check(!movie.Tick(false) &&
              frames.uploads == static_cast<int>(first_frame_count + 1u) &&
              movie.Stop() && movie.TakeCompletion(),
              "second original movie decodes and stops");
        const auto continuation = context->Execute();
        std::uint32_t saved_movie_flags{};
        std::ifstream saved_settings(
            settings_directory.Path() / "pal4.set", std::ios::binary);
        if (saved_settings) {
            saved_settings.seekg(24);
            saved_settings.read(reinterpret_cast<char*>(&saved_movie_flags),
                                sizeof(saved_movie_flags));
        }
        Check(continuation == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -57 is unavailable") == 0 &&
              global_data.Word(0x18u) == 1u && settings_source.saves == 1 &&
              saved_movie_flags == 1u &&
              host.mutes == 2 &&
              host.restores == 2,
              "original script saves movie flag before player initialization");

        constexpr char final_script[] =
            "void FinalMovie() { giPlayMovieFinal(\"1A.bik\"); }";
        Check(engine.Raw()->AddScriptSection(
                  "movie_final", "movie_final.cs", final_script,
                  sizeof(final_script) - 1u) >= 0 &&
              engine.Raw()->Build("movie_final") >= 0,
              "build final-movie script");
        const auto final_function = engine.FunctionId(
            "movie_final", "FinalMovie");
        Check(final_function >= 0 &&
              context->Prepare(final_function) >= 0 &&
              context->Execute() == asEXECUTION_SUSPENDED &&
              movie.Waiting(),
              "final movie uses blocking native playback");
        Check(movie.Stop() && movie.CompletionPending() &&
              host.screen_resets==0 && host.states.back()==12u && movie.TakeCompletion() &&
              host.states == std::vector<std::uint32_t>{
                  12u, 12u, 12u, 12u, 0u} &&
              host.screen_resets == 1 &&
              context->Execute() == asEXECUTION_FINISHED,
              "final movie returns to game state and resumes script");

        engine.SetCallbacks(nullptr);
        RoutedMovies(root,miles);
        MovieMusicVolume(root,miles);
        (void)context->Release();
        std::cout << "original NewGame crossed both movie waits\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
