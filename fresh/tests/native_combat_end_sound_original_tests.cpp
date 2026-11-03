#include "pal4/native_combat_end_sound_events.h"
#include "pal4/native_combat_system_tail.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_background_music_runtime.h"
#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_miles_music_stream.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
namespace {void Check(bool v,const char* m){if(!v)throw std::runtime_error(m);}}
int main(int argc,char** argv){try{
    Check(argc==2,"original root required");
    pal4::NativeMilesDigitalDriver driver(argv[1]);Check(driver.Open(),"Miles open");driver.SetMasterVolume(0.0f);
    pal4::NativeMilesMusicStream opening(argv[1],driver);
    pal4::NativeBackgroundMusicRuntime music(argv[1],driver,opening);
    pal4::BackgroundMusicPlayer player;
    pal4::NativeMilesSound2dRuntime sounds(argv[1],driver);
    pal4::NativeScriptEventRouter router;
    std::array<std::uint32_t,130> image{};
    const auto combat=reinterpret_cast<std::uint32_t>(image.data());
    pal4::NativeCombatSystemTail tail(std::span<std::uint32_t,18>(image.data()+108u,18u));
    tail.SetText(combat+452u,"P01-1");std::uint32_t sound_handle{};
    pal4::NativeCombatEndSoundServices services;
    services.acquire_combat=[&]{return combat;};
    services.inactive=[&](auto c){Check(c==combat,"scoped Combat fixture identity");return false;};
    services.read_word=[&](auto a){Check(a==combat+460u,"fixture music length");return image[115];};
    services.text=[&](auto a)->std::string{Check(a==combat+452u,"native tail special track");return std::string(tail.Text(a));};
    services.special_music=[&](auto track,float dt,bool restore){
        (void)player.FadeInSpecialBackgroundMusic(track,dt,restore,music);
    };
    services.restore_music=[](auto){throw std::logic_error("normal scene branch belongs to production music script");};
    services.play_sound=[&](std::string_view path,auto mode){
        constexpr std::string_view prefix="gamedata\\palSound\\";Check(path.starts_with(prefix),"original effect path");
        sound_handle=sounds.PlayNamedSound(path.substr(prefix.size()),static_cast<std::int32_t>(mode));
        Check(sound_handle && sounds.IsPlaying(sound_handle),"original WA14 playback");return sound_handle;
    };
    pal4::NativeCombatEndSoundEvents owner(combat,image[105],router,std::move(services));
    router.Dispatch("combat_prepare",0u);
    Check(player.CurrentTrack()=="P01-1" && music.IsAudioActive(player.AudioHandle()) && sound_handle,
          "original prepare music and WA14 active");
    for(std::uint32_t cue=0;cue<3u;++cue){
        router.Dispatch("combat_over",reinterpret_cast<std::uint32_t>(&cue));
        Check(player.CurrentTrack()==(cue?"wa15.mp3":"wa16.mp3") && music.IsAudioActive(player.AudioHandle()),
              "original packaged result music active");
        Check(!owner.TemporaryTexts(),"temporary host strings retired after real transport");
    }
    owner.Close();Check(!image[105] && !router.SubscriptionCount(),"audio owner teardown");
    tail.Close();Check(!image[114] && !image[115],"native tail text view retired");
    sounds.StopAll();for(auto* stream:music.Streams())stream->Stop();music.Update(0.0f);
    Check(!sounds.ActiveCount() && music.Streams().size()==1u && !opening.Active(),"actual samples and music streams retired");
    std::cout<<"PASS: muted original Combat WA14, P01-1, wa15/wa16 through native event owner and Miles\n";
    return EXIT_SUCCESS;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}}
