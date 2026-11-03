#include "pal4/portable/media.h"
#include <iostream>
#include <stdexcept>
#include <bit>
#include <limits>
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("media contract failed");}
struct Missing:IScriptContextServices {
    ScriptResponse Invoke(const ScriptRequest&) override {throw std::runtime_error("unbound service");}
};
int main() {
    try {
        Missing missing;ScriptGlobals globals{};ScriptStack stack(globals);MediaServices media(missing,9);
        stack.Compile("movie",R"(void Main(){giScriptMusicPause();giPlayMovie("1A.bik");giPlayMovie("PAL4A.bik");giScriptMusicResume();global_mvar=123;})");
        stack.Push("movie","void Main()",media);
        Check(stack.Tick(0)==ScriptState::suspended && !media.Complete(9,1,true));
        const auto pause=*media.TakeCommand();Check(pause.kind==MediaKind::pause_music && !media.TakeCommand());
        Check(!media.Complete(8,pause.id,true) && !media.Complete(9,pause.id+1,true));
        Check(stack.Tick(100)==ScriptState::suspended && globals[0]==0);
        auto result=media.Complete(9,pause.id,true);Check(result && media.MusicPaused() && stack.Complete(result->token));
        Check(!media.Complete(9,pause.id,true) && stack.Tick(0)==ScriptState::suspended);
        const auto first=*media.TakeCommand();Check(first.kind==MediaKind::movie && first.movie=="1A.bik");
        Check(stack.Tick(10000)==ScriptState::suspended && media.MoviesCompleted()==0);
        result=media.Complete(9,first.id,true);Check(result && stack.Complete(result->token));
        Check(stack.Tick(0)==ScriptState::suspended && media.MoviesCompleted()==1);
        const auto second=*media.TakeCommand();Check(second.movie=="PAL4A.bik" && second.id>first.id);
        Check(!media.Complete(9,first.id,true));
        result=media.Complete(9,second.id,true);Check(result && stack.Complete(result->token));
        Check(stack.Tick(0)==ScriptState::suspended && media.MoviesCompleted()==2);
        const auto resume=*media.TakeCommand();Check(resume.kind==MediaKind::resume_music);
        result=media.Complete(9,resume.id,true);Check(result && stack.Complete(result->token) && !media.MusicPaused());
        Check(stack.Tick(0)==ScriptState::finished && globals[0]==123);stack.Clear();

        {
            globals.fill(0);MediaServices skipped(missing,12);stack.Push("movie","void Main()",skipped);stack.Tick(0);
            Check(!skipped.SkipMovie(12,1));const auto p=*skipped.TakeCommand();
            Check(!skipped.SkipMovie(12,p.id) && skipped.Waiting() && skipped.MoviesSkipped()==0);
            auto ack=skipped.Complete(12,p.id,true);Check(ack && stack.Complete(ack->token));stack.Tick(0);
            const auto clip=*skipped.TakeCommand();Check(!skipped.SkipMovie(11,clip.id) && !skipped.SkipMovie(12,clip.id+1));
            ack=skipped.SkipMovie(12,clip.id);Check(ack && ack->error.empty() && stack.Complete(ack->token));
            Check(skipped.MoviesSkipped()==1 && skipped.MoviesCompleted()==0 && !skipped.SkipMovie(12,clip.id));
            Check(stack.Tick(0)==ScriptState::suspended);const auto next=*skipped.TakeCommand();Check(next.movie=="PAL4A.bik");
            ack=skipped.Complete(12,next.id,true);Check(ack && stack.Complete(ack->token));stack.Tick(0);
            const auto resume=*skipped.TakeCommand();ack=skipped.Complete(12,resume.id,true);Check(ack && stack.Complete(ack->token));
            Check(stack.Tick(0)==ScriptState::finished && globals[0]==123 && skipped.MoviesCompleted()==1 && skipped.MoviesSkipped()==1);stack.Clear();
        }
        globals.fill(0);MediaServices failed(missing,10);stack.Push("movie","void Main()",failed);stack.Tick(0);
        const auto operation=*failed.TakeCommand();
        Check(operation.id==pause.id && !failed.Complete(9,operation.id,true));
        result=failed.Complete(10,operation.id,false,"decoder unavailable");
        Check(result && stack.Complete(result->token,result->error));
        Check(stack.Tick(0)==ScriptState::failed && globals[0]==0 && stack.Error()=="decoder unavailable");stack.Clear();
        stack.Push("movie","void Main()",failed);stack.Tick(0);const auto cancelled=*failed.TakeCommand();
        failed.Cancel();Check(!failed.Waiting() && !failed.Complete(10,cancelled.id,true));stack.Clear();
        for(const auto* name:{"../movie.bik","C:movie.bik","a/b.bik","movie.mp4",".bik"}) {
            bool rejected=false;
            try{failed.Invoke({{1,1},ScriptCall{297,0,{std::string(name)}}});}
            catch(const std::invalid_argument&){rejected=true;}Check(rejected);
        }
        globals.fill(0);MediaServices music(missing,11);
        stack.Compile("bgm",R"(void Main(){giScriptMusicPlay("P23",2,0.0f,2.5f);global_mvar=17;})");
        stack.Push("bgm","void Main()",music);Check(stack.Tick(0)==ScriptState::suspended);
        const auto song=*music.TakeCommand();
        Check(song.kind==MediaKind::play_music && song.music.member=="p23.mp3" && song.music.loop);
        Check(song.music.fade_in_seconds==0 && song.music.fade_out_seconds==1 && song.music.restore_fade_bits==0x40200000u);
        Check(music.MusicStarted()==0 && music.MusicTrack().empty() && !music.Complete(10,song.id,true));
        Check(stack.Tick(10)==ScriptState::suspended && globals[0]==0);
        result=music.Complete(11,song.id,true);Check(result && stack.Complete(result->token));
        Check(music.MusicStarted()==1 && music.MusicTrack()=="P23" && music.MusicOperation()==song.id);
        Check(stack.Tick(0)==ScriptState::finished && globals[0]==17);stack.Clear();
        const ScriptCall nextSong{127,0,{std::string("P24.MP3"),std::int32_t(2),0.5f,1.5f}};
        Check(music.Invoke({{8,1},nextSong}).suspend);const auto fading=*music.TakeCommand();
        Check(fading.music.member=="p24.mp3" && fading.music.fade_in_seconds==0.5f && fading.music.restore_fade_bits==std::bit_cast<std::uint32_t>(1.5f));
        Check(music.Complete(11,fading.id,false,"missing package") && music.MusicTrack()=="P23" && music.MusicStarted()==1);
        Check(!music.Invoke({{8,2},ScriptCall{127,0,{std::string(),std::int32_t(2),0.0f,3.0f}}}).suspend && !music.TakeCommand());
        for(const auto* name:{"../track","C:track","a/b",".track"}) {
            bool rejected=false;try{music.Invoke({{8,3},ScriptCall{127,0,{std::string(name),std::int32_t(2),0.0f,2.5f}}});}
            catch(const std::invalid_argument&){rejected=true;}Check(rejected);
        }
        bool rejected=false;try{music.Invoke({{8,4},ScriptCall{127,0,{std::string("P23"),std::int32_t(2),std::numeric_limits<float>::infinity(),2.5f}}});}
        catch(const std::invalid_argument&){rejected=true;}Check(rejected);
        rejected=false;try{music.Invoke({{8,5},ScriptCall{127,0,{std::string("P23"),std::int32_t(1),0.0f,2.5f}}});}
        catch(const std::runtime_error&){rejected=true;}Check(rejected && !music.Waiting());
        music.Invoke({{8,6},nextSong});const auto cancelledMusic=*music.TakeCommand();music.Cancel();
        Check(!music.Complete(11,cancelledMusic.id,true) && music.MusicOperation()==0);
        std::cout<<"media platform acknowledgement, exact continuation, generation, cancellation and failure contracts passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
