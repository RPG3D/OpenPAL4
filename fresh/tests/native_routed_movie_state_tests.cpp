#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "support/event_arena_runtime.h"
#include "pal4/native_opening_video_state.h"
#include "pal4/native_movie_owner_router.h"
#include "pal4/native_script_movie_player.h"
#include "pal4/native_bink_frame_presenter.h"
#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_effect_render_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_global_key_down.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/pal_game_iv_state_transition.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/pal_console.h"
#include "pal4/pal_screen_effect.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T> std::uint32_t Address(T* value){return reinterpret_cast<std::uint32_t>(value);}
struct Preterit final : pal4::IPreteritRuntime {
    std::uint32_t FindTexture(std::uint32_t,std::string_view) override {throw std::logic_error("unexpected inactive effect texture");}
    std::int32_t Random() override {throw std::logic_error("unexpected inactive effect random");}
    void SetGameState(std::uint32_t) override {throw std::logic_error("unexpected inactive effect transition");}
};
struct UnusedWait final : pal4::INativeScriptHandlerWaitRuntime {
    bool Tick(float) override {throw std::logic_error("routed state must not tick fallback waits");}
};
struct Host final : pal4::INativeMovieSessionHost,pal4::INativeBinkStateHost,pal4::IPalGameIVStateRuntime {
    pal4::NativeMilesDigitalDriver& miles;
    std::uint32_t game;
    pal4::NativeOpeningVideoState* state{};
    pal4::NativePalGameIVStateList* states{};
    std::uint32_t manager{};
    std::array<std::uint32_t,2> scene{},logo{},loading{};
    pal4::PalGameIVStateLabels labels;
    unsigned returns{},documents{},destroyed{},mutes{},restores{},resets{},logo_entries{};
    Host(pal4::NativeMilesDigitalDriver& driver,std::uint32_t address):miles(driver),game(address){}
    void* DigitalDriver() override {return miles.Handle();}
    float MuteGameAudio() override {++mutes;return 1.0f;}
    void RestoreGameAudio(float value) override {Check(value==1.0f,"restore selected movie volume");++restores;}
    void EnterState(std::uint32_t id) override {Check(id==12u || id==0u,"movie transition target");Change(id);}
    void Change(std::uint32_t id){Check(states && pal4::PalGameIVStateTransition::Initialize(game,id,0u,labels,*this),"shared state transition succeeds");}
    std::uint32_t Mode(){return states->StateId(ReadWord(game+0x18cu));}
    void ResetScreenEffect() override {++resets;}
    std::uint32_t CreateBinkReturnDocument() override {
        Check(Mode()==12u,"movie return document is constructed before a later Loading transition");
        ++documents;return 0x1234u;
    }
    std::uint32_t BinkStateGameAddress() const override {return game;}
    void ReturnBinkToScene(std::uint32_t document) override {Check(document==0x1234u,"state return document");++returns;Change(1u);}
    std::uint32_t DestroyBinkReturnDocument(std::uint32_t document) override {Check(document==0x1234u,"state document release");++destroyed;return 1u;}
    void RegisterAtExit(std::uint32_t callback) override {Check(callback==0x5b67a0u,"original label cleanup registration");}
    std::uint32_t FindEntry(std::uint32_t address,std::uint32_t id) override {Check(address==game,"state registry owner");return states->Find(id);}
    std::uint32_t ReadWord(std::uint32_t address) override {Check(address==game+0x18cu,"state current pointer read");return *reinterpret_cast<std::uint32_t*>(address);}
    void WriteWord(std::uint32_t address,std::uint32_t value) override {Check(address==game+0x18cu,"state current pointer write");*reinterpret_cast<std::uint32_t*>(address)=value;}
    void Leave(std::uint32_t address,std::uint32_t) override {if(address==state->Address())state->Leave();}
    std::uint32_t StateId(std::uint32_t address) override {return states->StateId(address);}
    std::uint32_t UiFrameManager() override {return manager;}
    void SetCursor(std::uint32_t slot,std::string_view name) override {Check(slot==manager+0x114u && name=="default.cur","original transition cursor request");}
    void LogExchange(std::string_view,std::string_view,std::uint32_t line) override {Check(line==0x1a1u,"original state exchange log");}
    bool Enter(std::uint32_t address,std::uint32_t previous,std::uint32_t argument) override {
        if(address==state->Address())return state->Enter(previous,argument);
        if(address==Address(logo.data()))++logo_entries;
        return true; // Observing Scene/Logo/Loading entry endpoints, no asset loads.
    }
    std::uint32_t TimeGetTime() override {return 123u;}
    void WriteGameStateWord(std::uint32_t offset,std::uint32_t value) override {Check((offset==0x574u || offset==0x56cu) && value==123u,"original state clock reset");}
};
void Run(const std::filesystem::path& root){
    pal4::test::NativeFrameSession frame;Check(frame.Start(800,600),"start hidden actual movie GPU frame");
    Check(!IsWindowVisible(frame.window),"movie state test window remains hidden");
    pal4::NativeMilesDigitalDriver miles(root.string());Check(miles.Open(),"open original movie audio driver");miles.SetMasterVolume(0.0f);
    std::array<std::uint32_t,0x224u/4u> manager{};
    std::array<std::uint32_t,0x200u/4u> game{};
    Host host(miles,Address(game.data()));pal4::NativeMovieOwnerRouter routes;
    pal4::NativeBinkFramePresenter presenter;
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[]="void first(){giPlayMovie(\"1A.bik\");global_mvar+=10;} void second(){giPlayMovie(\"1A.bik\");global_mvar+=20;}"
        "void chain(){giPlayMovie(\"1A.bik\");global_mvar+=1;giPlayMovie(\"1A.bik\");global_mvar+=2;giArenaLoad(\"Q01\",\"N01\",\"N01\",false);global_mvar+=4;}"
        "void final(){giPlayMovieFinal(\"1A.bik\");global_mvar+=50;}";
    Check(engine.Raw()->AddScriptSection("state_movies","fixture",source,sizeof(source)-1)>=0 &&
        engine.Raw()->Build("state_movies")>=0,"compile movie state SDK contexts");
    asIScriptContext* first{};asIScriptContext* second{};
    Check(engine.Raw()->CreateContext(&first)>=0 && engine.Raw()->CreateContext(&second)>=0,"create independent SDK movie contexts");
    {
        pal4::NativeScriptMoviePlayer primary(root.string(),*first,presenter,host);
        auto child=std::make_unique<pal4::NativeScriptMoviePlayer>(root.string(),*second,presenter,host);
        pal4::test::EventArenaRuntime arena;
        arena.enter_loading=[&]{host.Change(6u);Check(first->Suspend()>=0,"Loading request suspends its current SDK continuation");};
        pal4::NativeGameScriptServices a{},b{};a.movie=&primary;a.arena=&arena;b.movie=child.get();
        pal4::NativeGameScriptCallbacks callbacks_a(a),callbacks_b(b);
        engine.SetContextCallbacks(first,&callbacks_a);engine.SetContextCallbacks(second,&callbacks_b);
        std::uint32_t mode_at_resume{};
        primary.BindRoute(routes,[&](float){if(primary.TakeCompletion()){
            mode_at_resume=host.Mode();const auto result=first->Execute();
            Check(result==asEXECUTION_FINISHED || (result==asEXECUTION_SUSPENDED && (primary.Waiting() || arena.loading)),"resume primary movie script");
        }});
        child->BindRoute(routes,[&](float){if(child->TakeCompletion()){
            Check(second->Execute()==asEXECUTION_FINISHED,"resume selected child movie script");child.reset();
        }});
        pal4::PalScreenEffectState screen{};pal4::NativeEffectRenderRuntime effects;Preterit preterit;
        pal4::PalConsoleState console;UnusedWait unused;
        pal4::NativeStateScriptFrameServices fallback{Address(first),[](float,auto&){throw std::logic_error("fallback script must not run");}};
        pal4::NativeOpeningVideoState state(Address(manager.data()),primary,presenter,std::move(fallback),unused,
            screen,effects,preterit,console,host,800,600);
        state.BindMovieRouter(routes);host.state=&state;host.manager=Address(manager.data());
        pal4::NativePalGameIVStateList states(Address(game.data()));
        Check(states.Register(state.Address(),12u,[]{}) && states.Register(Address(host.scene.data()),1u,[]{}) &&
            states.Register(Address(host.logo.data()),0u,[]{}) && states.Register(Address(host.loading.data()),6u,[]{}),
            "register Bink and observed destination identities");
        host.states=&states;host.Change(1u);
        pal4::NativeEventHandlerArray active([](auto,auto)->std::uint32_t{throw std::logic_error("unexpected HUD callback");},
            [](auto,auto,auto)->std::uint8_t{throw std::logic_error("unexpected HUD predicate");});
        unsigned skips{};
        pal4::NativeGlobalKeyDown keys(Address(game.data()),states,Address(manager.data()),active,
            {Address(&routes),[&]{return routes.Playing();},[&]{Check(routes.Stop(),"Escape closes selected owner");++skips;}});
        std::array<std::uint32_t,5> escape{};escape[4]=1u;
        Check(first->Prepare(engine.FunctionId("state_movies","first"))>=0 && first->Execute()==asEXECUTION_SUSPENDED &&
            state.Entered() && routes.Current()==&primary,"primary movie selects state12 route");
        (void)keys.Process(Address(escape.data()));(void)state.Update(0.016f);
        Check(!state.Entered() && host.returns==1u && engine.GlobalStoryVariable()==10,"primary Escape resumes and returns to Scene");
        Check(second->Prepare(engine.FunctionId("state_movies","second"))>=0 && second->Execute()==asEXECUTION_SUSPENDED &&
            child && routes.Current()==child.get() && !primary.Waiting(),"different context selects its actual movie owner");
        std::uint32_t pixels{};
        for(unsigned attempt=0;attempt<120u && !pixels;++attempt){
            Check(frame.Begin(),"begin selected movie camera update");
            state.Render(Address(&frame.camera),false);pixels=frame.CountNonBlack();
        }
        Check(pixels>0u,"routed child original movie changes actual GPU pixels");
        (void)keys.Process(Address(escape.data()));(void)state.Update(0.016f);
        Check(!child && !routes.Current() && !state.Entered() && host.returns==2u &&
            host.documents==host.destroyed && host.mutes==host.restores && skips==2u && engine.GlobalStoryVariable()==30,
            "child retirement during state update leaves no borrowed player and preserves original Escape/Scene return");
        {
            unsigned owners{},retired{};
            struct TimedMovie final : pal4::INativeTimedScriptContext,pal4::INativeScriptHandlerWaitRuntime {
                pal4::NativeScriptMoviePlayer movie;unsigned& retired;
                TimedMovie(const std::filesystem::path& root,asIScriptContext& context,
                    pal4::NativeBinkFramePresenter& frames,Host& host,unsigned& count)
                    :movie(root.string(),context,frames,host),retired(count){}
                ~TimedMovie() override {++retired;}
                pal4::NativeGameScriptServices BindServices(pal4::NativeGameScriptServices shared) override {
                    shared.movie=&movie;return shared;
                }
                pal4::INativeScriptFrameWaitRuntime* Waits() noexcept override {return this;}
                bool BlocksScript() const override {return movie.Waiting() || movie.CompletionPending();}
                bool Tick(float) override {return movie.TakeCompletion();}
                void ClearHandlers() override {(void)movie.Cancel();}
            };
            pal4::NativeTimedScriptStack timed(engine,root,[&](auto source,auto module){
                Check(source=="Q01.csb","timed movie source routing");
                constexpr char text[]="void timed(){giPlayMovie(\"1A.bik\");global_mvar+=5;}";
                const std::string name(module);
                Check(engine.Raw()->AddScriptSection(name.c_str(),"fixture",text,sizeof(text)-1)>=0 &&
                    engine.Raw()->Build(name.c_str())>=0,"compile actual timed movie callback");
            });
            timed.SetContextFactory([&](asIScriptContext& context){
                auto owner=std::make_unique<TimedMovie>(root,context,presenter,host,retired);++owners;
                owner->movie.BindRoute(routes,[&](float delta){(void)timed.ProcessSelected(delta);});
                return owner;
            });
            const char function[]="timed";const std::uint32_t record[]{0u,Address(function)};
            (void)pal4::ScriptTimedState::Start(0u,Address(record),timed.Timer());
            Check(timed.Process(0.0f,false,false,"Q01") && timed.Active() && routes.Playing() && state.Entered(),
                "actual timed SDK callback selects the Bink state movie route");
            timed.Timer().DisableForSceneRelease();
            Check(frame.Begin(),"begin timed movie GPU frame");state.Render(Address(&frame.camera),false);
            (void)frame.CountNonBlack();Check(routes.Current()->FrameReady(),"timed movie decodes through actual state render");
            (void)keys.Process(Address(escape.data()));(void)state.Update(0.016f);
            Check(!timed.Active() && timed.FinishedCount()==1u && owners==1u && retired==1u &&
                !routes.Current() && !state.Entered() && host.returns==3u && skips==3u && engine.GlobalStoryVariable()==35,
                "state update resumes and retires the actual timed SDK context without borrowing freed movie/waits");
        }
        Check(first->Prepare(engine.FunctionId("state_movies","chain"))>=0 && first->Execute()==asEXECUTION_SUSPENDED &&
            state.PreviousStateId()==1u,"movie chain starts with its real Scene predecessor");
        (void)keys.Process(Address(escape.data()));
        Check(host.Mode()==12u && primary.CompletionPending(),"Escape publishes completion before handler state changes");
        (void)state.Update(0.016f);
        Check(primary.Waiting() && host.Mode()==12u && state.PreviousStateId()==12u &&
            host.returns==3u && engine.GlobalStoryVariable()==36,"second movie records actual Bink predecessor without early Scene return");
        (void)keys.Process(Address(escape.data()));(void)state.Update(0.016f);
        Check(!state.Entered() && host.Mode()==6u && arena.requests==1 && arena.world=="Q01" && arena.area=="N01" &&
            host.returns==3u && engine.GlobalStoryVariable()==38,"chained movie continuation enters Loading without being overwritten by Scene return");
        host.Change(1u); // Observed Loading completion, not an asset load.
        Check(first->Execute()==asEXECUTION_FINISHED && engine.GlobalStoryVariable()==42,"Loading continuation resumes after its preserved SDK instruction");
        for(unsigned cycle=0;cycle<2u;++cycle){
            const auto logos=host.logo_entries,resets=host.resets,restores=host.restores;
            Check(first->Prepare(engine.FunctionId("state_movies","final"))>=0 && first->Execute()==asEXECUTION_SUSPENDED &&
                host.Mode()==12u && state.PreviousStateId()==12u,"final movie double entry keeps original Bink predecessor");
            if(!cycle)(void)keys.Process(Address(escape.data()));
            else{
                const auto count=primary.FrameInfo().frame_count;
                for(unsigned index=0;index<=count && primary.Waiting();++index){
                    Check(frame.Begin(),"begin natural final movie GPU frame");state.Render(Address(&frame.camera),false);
                    Check(RwCameraEndUpdate(frame.camera)!=nullptr,"end natural final movie GPU frame");frame.updating=false;
                }
                (void)frame.CountNonBlack();
            }
            Check(!primary.Waiting() && primary.CompletionPending() && host.Mode()==12u && host.logo_entries==logos &&
                host.resets==resets && host.restores==restores,"skip/natural end defer audio and Logo effects until wait update");
            (void)state.Update(0.016f);
            Check(!primary.CompletionPending() && !state.Entered() && mode_at_resume==0u && host.Mode()==0u &&
                host.logo_entries==logos+1u && host.resets==resets+1u && host.restores==restores+1u &&
                engine.GlobalStoryVariable()==92+static_cast<int>(cycle)*50,"final handler enters Logo before SDK continuation, once per movie");
        }
        {
            // 58ACB0 calls7DF4B0/7DF540: ordinary mode executes one SDK step,
            // then returns stopped Bink to Scene before later script lines.
            constexpr char text[] =
                "void after_movie(){\n giPlayMovie(\"1A.bik\");\n global_mvar+=1;\n"
                " giArenaLoad(\"M01\",\"3\",\"\",true);\n global_mvar+=2;\n}";
            Check(engine.Raw()->AddScriptSection("post_movie","fixture",text,sizeof(text)-1)>=0 &&
                engine.Raw()->Build("post_movie")>=0,"compile ordinary single-movie Loading continuation");
            pal4::NativeScriptFrameRunner runner(engine);
            pal4::NativeScriptMoviePlayer movie(root.string(),runner.Context(),presenter,host);
            struct Wait final : pal4::INativeScriptHandlerWaitRuntime {
                pal4::NativeScriptMoviePlayer& movie;
                explicit Wait(pal4::NativeScriptMoviePlayer& value):movie(value){}
                bool BlocksScript() const override {return movie.Waiting() || movie.CompletionPending();}
                bool Tick(float) override {return movie.TakeCompletion();}
            } wait(movie);
            pal4::test::EventArenaRuntime next_arena;
            next_arena.enter_loading=[&]{host.Change(6u);Check(runner.Context().Suspend()>=0,"Loading suspends ordinary continuation");};
            pal4::NativeGameScriptServices services{};services.movie=&movie;services.arena=&next_arena;
            pal4::NativeGameScriptCallbacks callbacks(services);
            engine.SetContextCallbacks(&runner.Context(),&callbacks);
            runner.SetImmediateModeProvider([&]{return callbacks.ImmediateMode()==1u;});
            const auto boundary=[&]{return next_arena.loading;};
            movie.BindRoute(routes,[&](float delta){
                Check(runner.TickOriginal(delta,&wait,boundary)!=pal4::NativeScriptFrameStatus::failed,
                    "ordinary movie handler uses original SDK step");
            });
            const auto story=engine.GlobalStoryVariable();
            const auto returns=host.returns;
            host.Change(1u);
            runner.Queue("post_movie","after_movie");
            for(unsigned i=0;i<128u && !movie.Waiting();++i)
                Check(runner.TickOriginal(0.016f,&wait,boundary)!=pal4::NativeScriptFrameStatus::failed,"start stepped movie");
            Check(movie.Waiting() && state.PreviousStateId()==1u,"single movie starts from Scene in ordinary mode");
            (void)keys.Process(Address(escape.data()));
            (void)state.Update(0.016f);
            Check(!state.Entered() && host.Mode()==1u && host.returns==returns+1u && !next_arena.requests,
                "stopped single movie returns to Scene before giArenaLoad executes");
            for(unsigned i=0;i<128u && !next_arena.loading;++i)
                Check(runner.TickOriginal(0.016f,&wait,boundary)!=pal4::NativeScriptFrameStatus::failed,"advance post-movie script");
            Check(host.Mode()==6u && next_arena.requests==1u && next_arena.world=="M01" && next_arena.area=="3" &&
                engine.GlobalStoryVariable()==story+1,"later script frame enters Loading once without stale Bink return");
            host.Change(1u);next_arena.loading=false;
            for(unsigned i=0;i<128u && runner.Status()!=pal4::NativeScriptFrameStatus::finished;++i)
                Check(runner.TickOriginal(0.016f,&wait,boundary)!=pal4::NativeScriptFrameStatus::failed,"resume loaded scene continuation");
            Check(runner.Status()==pal4::NativeScriptFrameStatus::finished && next_arena.requests==1u &&
                engine.GlobalStoryVariable()==story+3,"Loading continuation preserves its SDK instruction and does not replay load");
            engine.SetContextCallbacks(&runner.Context(),nullptr);
        }
        Check(host.mutes==host.restores && host.documents==host.destroyed,"chained/final movies balance audio and temporary state documents");
        engine.SetContextCallbacks(first,nullptr);engine.SetContextCallbacks(second,nullptr);
        active.ReleaseStorage();
        std::cout<<"Routed Bink state: "<<pixels<<" GPU pixels, real Escape, timed retirement, single-movie original stepping, shared chained/Loading transitions and deferred final Logo verified\n";
    }
    (void)first->Release();(void)second->Release();
}
}
int main(int argc,char** argv){try{Check(argc==2,"provide original game directory");pal4::test::NativeDesktopObservation desktop;
    Run(argv[1]);desktop.Check("routed movie state preserves desktop focus and cursor");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
