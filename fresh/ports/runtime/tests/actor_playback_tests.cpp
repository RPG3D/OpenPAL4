#include "pal4/portable/actor_playback.h"
#include "pal4/portable/package.h"
#include "pal4/game_object_animation_event_loader.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("actor playback contract failed");}
Actor Fixture() {
    const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};Actor actor;
    actor.frames.push_back({-1,identity});actor.palettes.push_back({0,identity});
    actor.mesh.vertices.resize(1);actor.mesh.vertices[0].position[0]=1;actor.influences.push_back({{0,0,0,0},{1,0,0,0}});
    actor.clips.push_back({"J08",4,{{0,{{0,{0,0,0,1},{0,0,0}},{4,{0,0,0,1},{4,0,0}}}}}});return actor;
}
std::vector<std::uint8_t> Message() {
    const auto signature=pal4::GameObjectAnimationEventLoader::kSignature;
    std::vector<std::uint8_t> bytes(8+signature.size()+332);
    const auto put=[&](std::size_t i,unsigned value){for(unsigned b=0;b<4;++b)bytes[i+b]=static_cast<std::uint8_t>(value>>(b*8));};
    put(0,static_cast<unsigned>(signature.size()));std::memcpy(bytes.data()+4,signature.data(),signature.size());put(4+signature.size(),1);
    const auto base=8+signature.size();std::memcpy(bytes.data()+base,"101",3);std::memcpy(bytes.data()+base+64,"J08",3);
    std::memcpy(bytes.data()+base+128,"HOLD",4);put(base+192,std::bit_cast<unsigned>(1.f));return bytes;
}
template<class F>void Reject(F f){bool threw{};try{f();}catch(const std::logic_error&){threw=true;}Check(threw);}
void BoundImages(){
    const auto actor=Fixture();const auto bytes=Message();
    const auto read=[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(name.ends_with(".amd"))return {};return bytes;};
    ActorPlayback playback(actor,"101",read);std::array<std::uint32_t,31> image{};image[0]=0x842c8c;image[20]=0xa1b2c3d4;
    const auto empty=image;Reject([&]{(void)playback.BindAnimationImage(image,0x80170,0);});Check(image==empty);Reject([&]{(void)playback.BindAnimationImage(image,1,0x200000);});Check(image==empty);
    image[21]=1;Reject([&]{(void)playback.BindAnimationImage(image,0x80170,0x200000);});image[21]=0;
    const auto cookie=playback.BindAnimationImage(image,0x80170,0x200000);Check(playback.OwnsAnimationImage(cookie) && image[0]==empty[0] && image[20]==empty[20]);
    Check(image[1]==0x200000 && image[2] && image[12]==std::bit_cast<std::uint32_t>(1.f));
    Reject([&]{(void)playback.BindAnimationImage(image,0x80170,0x200000);});Reject([&]{playback.UnbindAnimationImage(cookie+1);});
    Check(playback.Select("J08") && image[3]);playback.SetRestart(true);playback.AttachHoldListener();Check(image[17]==1 && image[7] && image[8]==image[7]+4);
    playback.Advance(1);Check(playback.MessageName()=="hold" && image[19]==1);playback.SetPause(false);playback.RemoveHoldListener();Check(!image[7] && !image[8] && !image[19]);
    playback.SetRestart(false);playback.Advance(5);Check(playback.Completed() && image[18]==1);playback.Seek(.5f);
    const auto valid=image;image[19]=123;Reject([&]{playback.SetPause(false);});Check(image[19]==123 && playback.Time()==.5f);image=valid;
    Check(playback.UnbindAnimationImage(cookie) && image==empty);const auto detached=image;playback.SetPause(true);Check(image==detached && !playback.OwnsAnimationImage(cookie));
    std::array<std::uint32_t,31> other{};const auto next=playback.BindAnimationImage(other,0x90170,0x200000);Check(next!=cookie && other[19]==1);Check(playback.UnbindAnimationImage(next) && other==std::array<std::uint32_t,31>{});
    const auto damaged=playback.BindAnimationImage(other,0x90170,0x200000);other[19]=123;const auto overwritten=other;
    Check(!playback.UnbindAnimationImage(damaged) && other==overwritten && !playback.OwnsAnimationImage(damaged));playback.SetPause(false);Check(other==overwritten);
    std::array<std::uint32_t,31> populated{};for(unsigned i=0;i<populated.size();++i)if(i<21 || i>23)populated[i]=0xa00000+i*4;
    const auto original=populated;const auto temporary=playback.BindAnimationImage(populated,0x90170,0x200000);
    Check(playback.Select("J08"));playback.Advance(.1f);Check(playback.UnbindAnimationImage(temporary) && populated==original);
    ActorPlayback* current{};std::uint64_t current_cookie{};std::array<std::uint32_t,31> callback_image{};bool nested{};
    ActorPlayback callback(actor,"101",read,[&](auto){Reject([&]{current->UnbindAnimationImage(current_cookie);});current->SetPause(true);nested=true;});current=&callback;
    current_cookie=callback.BindAnimationImage(callback_image,0xa0170,0x200000);Check(callback.Select("J08"));callback.Advance(1);Check(nested && callback_image[19]==1);callback.UnbindAnimationImage(current_cookie);
    ActorPlayback truncated(actor,"101",[](auto)->std::optional<std::vector<std::uint8_t>>{return std::vector<std::uint8_t>{};});std::array<std::uint32_t,31> failed{};
    const auto failed_cookie=truncated.BindAnimationImage(failed,0xb0170,0x200000);const auto before=failed;Reject([&]{(void)truncated.Select("J08");});Check(failed==before);truncated.UnbindAnimationImage(failed_cookie);
}

void FailedMessageRetry(){
    auto actor=Fixture();auto second=actor.clips.front();second.name="C01";actor.clips.push_back(second);
    const auto complete=Message();int failure=1;unsigned c01_reads=0;
    ActorPlayback playback(actor,"101",[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{
        if(name.ends_with(".amd")){if(name.ends_with("C01.amd") && failure==2)return std::vector<std::uint8_t>{0};return {};}
        if(name.ends_with("C01.amf")){++c01_reads;if(failure==1)return std::vector<std::uint8_t>(complete.begin(),complete.end()-1);}
        return complete;
    });
    Check(playback.Select("J08"));playback.Advance(.5f);
    Reject([&]{(void)playback.Select("C01");});
    Check(playback.Action()=="J08" && playback.Time()==.5f && c01_reads==1);
    playback.Advance(.25f);Check(playback.Time()==.75f);
    failure=2;Reject([&]{(void)playback.Select("C01");});Check(playback.Action()=="J08" && playback.Time()==.75f && c01_reads==2);
    failure=0;Check(playback.Select("C01") && c01_reads==3 && playback.Action()=="C01");
    playback.Advance(.5f);Check(playback.Time()==.5f);
}

void MessageLifetime(){
    auto actor=Fixture();auto second=actor.clips.front();second.name="C01";actor.clips.push_back(second);
    const auto bytes=Message();unsigned j08=0,c01=0;
    const auto read=[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{
        if(name.ends_with(".amd"))return {};
        if(name.ends_with("J08.amf"))++j08;else if(name.ends_with("C01.amf"))++c01;else throw std::logic_error("unexpected message resource");
        return bytes;
    };
    ActorPlayback playback(actor,"101",read);
    Check(playback.Select("J08") && j08==1);Check(playback.SelectBlended("C01",.2f) && c01==1);
    Check(playback.Select("J08") && j08==1);Check(playback.SelectBlended("C01",.2f) && c01==2);
    playback.Advance(.25f);Check(playback.Action()=="C01");Check(playback.Select("J08") && j08==2);
    ActorPlayback* active{};unsigned nested=0;
    ActorPlayback callback(actor,"101",read,[&](std::span<const std::byte> event){
        Check(event[128]==std::byte{'H'});Check(active->Select("C01"));Check(event[128]==std::byte{'H'});++nested;
    });
    active=&callback;Check(callback.Select("J08"));callback.Advance(1);Check(nested==1 && callback.Action()=="C01");
    const auto previous=j08;Check(callback.Select("J08") && j08==previous+1);
}

void ClipLifetime(){
    auto model=Fixture();auto second=model.clips.front();second.name="C01";second.tracks[0].keys[0].translation[0]=10;model.clips.push_back(second);
    const auto clips=model.clips;for(auto& entry:model.clips){entry.duration=0;entry.tracks.clear();}
    std::array<std::weak_ptr<const AnimationClip>,2> cache;std::array<unsigned,2> loads{};bool fail=false;
    const auto loader=[&](std::string_view name)->std::shared_ptr<const AnimationClip>{
        const unsigned index=name=="J08"?0:1;if(index==1 && fail)throw std::logic_error("controlled animation failure");
        if(auto clip=cache[index].lock())return clip;auto clip=std::make_shared<AnimationClip>(clips[index]);cache[index]=clip;++loads[index];return clip;
    };
    const auto no_messages=[](std::string_view)->std::optional<std::vector<std::uint8_t>>{return {};};
    ActorPlayback a(model,"101",no_messages,{},false,loader),b(model,"101",no_messages,{},false,loader);
    Check(a.Select("J08") && b.Select("J08") && loads[0]==1 && !cache[0].expired());
    a.Advance(.5f);fail=true;Reject([&]{a.Select("C01");});Check(a.Action()=="J08" && a.Time()==.5f && !cache[0].expired());
    fail=false;Check(a.SelectBlended("C01",.2f) && loads[1]==1 && !cache[0].expired() && !cache[1].expired());
    std::vector<Vertex> pose;a.Advance(.1f);a.Sample(pose);a.Advance(.2f);a.Sample(pose);Check(a.Action()=="C01" && !cache[0].expired());
    Check(b.Select("C01") && loads[1]==1 && cache[0].expired());
    Check(a.Select("J08") && loads[0]==2 && !cache[0].expired());
    Check(b.Select("J08") && cache[1].expired());
    ActorPlayback bad_messages(model,"101",[](std::string_view name)->std::optional<std::vector<std::uint8_t>>{
        if(name.ends_with("C01.amf"))return std::vector<std::uint8_t>{0};return {};
    },{},false,loader);
    Check(bad_messages.Select("J08"));Reject([&]{bad_messages.Select("C01");});
    Check(bad_messages.Action()=="J08" && cache[1].expired());
}

}
int main(int argc,char** argv) {
    try {
        if(argc==5 && std::string_view(argv[1])=="hold-cycle"){
            std::ifstream archive(argv[2],std::ios::binary),cache(argv[3],std::ios::binary);PackageReader package(archive);const auto actor=ReadActor(cache);
            ActorPlayback playback(actor,"101",[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);});
            Check(playback.Select("C08"));playback.SetRestart(true);bool c08_done=false;
            for(unsigned i=0;i<300 && !c08_done;++i){playback.Advance(1.f/60);c08_done=playback.Completed();}Check(c08_done);
            Check(playback.Select("C07"));playback.AttachHoldListener();playback.SetRestart(true);
            for(unsigned i=0;i<300 && playback.MessageName()!="hold";++i)playback.Advance(1.f/60);
            const auto held=playback.Time();Check(playback.MessageName()=="hold" && held>0 && held<playback.Duration());
            for(unsigned i=0;i<60;++i)playback.Advance(1.f/60);Check(playback.Time()==held);
            playback.SetPause(false);playback.RemoveHoldListener();bool resumed=false;
            for(unsigned i=0;i<300 && !resumed;++i){playback.Advance(1.f/60);resumed=playback.Completed();}Check(resumed);
            Check(playback.Select("C01"));std::vector<Vertex> pose;playback.Sample(pose);Check(!pose.empty());
            std::ofstream out(argv[4]);out<<"{\"independent_fixture\":true,\"game_calls\":0,\"C08_completed\":true,\"C07_held_time\":"<<held<<",\"C07_unheld_completed\":true,\"playback\":";playback.WriteReport(out);out<<'}';return 0;
        }
        if(argc==5 || argc==6) {
            std::ifstream package(argv[1],std::ios::binary),cache(argv[2],std::ios::binary);PackageReader reader(package);const auto actor=ReadActor(cache);
            ActorPlayback playback(actor,"101",[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(!reader.Find(name))return {};return reader.Read(name);});
            Check(playback.Select(argc==6?argv[5]:"J08"));playback.AttachHoldListener();playback.SetRestart(true);playback.Advance(std::stof(argv[3]));
            std::vector<Vertex> pose;playback.Sample(pose);double checksum=0;
            for(std::size_t i=0;i<pose.size();++i)for(unsigned axis=0;axis<3;++axis)checksum+=pose[i].position[axis]*double((i%17+1)*(axis+1));
            std::ofstream out(argv[4]);out<<"{\"playback\":";playback.WriteReport(out);out<<",\"pose_checksum\":"<<std::setprecision(14)<<checksum<<'}';return 0;
        }
        BoundImages();FailedMessageRetry();MessageLifetime();ClipLifetime();
        const auto actor=Fixture();const auto bytes=Message();
        const auto read=[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{Check(name=="101\\J08.amf" || name=="101\\J08.amd");if(name.ends_with(".amd"))return {};return bytes;};
        ActorPlayback playback(actor,"101",read,{},true);Check(!playback.Select("missing") && playback.Select("J08"));
        {std::ostringstream summary,diagnostic;playback.WriteReport(summary,false);playback.WriteReport(diagnostic,true);
            Check(summary.str().find("\"source_hex\":\"\"")!=std::string::npos && diagnostic.str().find("\"source_hex\":\"\"")==std::string::npos);}
        {ActorPlayback normal(actor,"101",read);Check(normal.Select("J08"));std::ostringstream report;normal.WriteReport(report,true);
            Check(report.str().find("\"source_hex\":\"\"")!=std::string::npos && report.str().find("\"bytes\":"+std::to_string(bytes.size()))!=std::string::npos);}
        playback.AttachHoldListener();playback.SetRestart(true);playback.Advance(.75f);Check(playback.MessageName().empty());
        std::vector<Vertex> pose;playback.Sample(pose);Check(std::abs(pose[0].position[0]-1.75f)<1e-5f);
        std::vector<Matrix> frames;
        playback.Advance(.25f);Check(playback.MessageName()=="hold" && playback.Time()==1);playback.Advance(10);playback.Sample(pose,&frames);Check(playback.Time()==1 && pose[0].position[0]==2 && frames[0][12]==1);
        const auto held_frames=frames;frames.clear();playback.Sample(pose,&frames);Check(frames==held_frames);
        std::ostringstream held;playback.WriteReport(held);Check(held.str().find("\"hold_events\":1")!=std::string::npos);
        Check(playback.Select("J08"));playback.Advance(1);Check(playback.Time()==1);std::ostringstream replay;playback.WriteReport(replay);Check(replay.str().find("\"hold_events\":2")!=std::string::npos);
        playback.RemoveHoldListener();Check(playback.Select("J08"));playback.Advance(1);Check(playback.Time()==1 && playback.MessageName().empty());
        playback.Advance(.5f);Check(playback.Time()==1.5f);
        unsigned fallback_calls{};
        ActorPlayback fallback(actor,"101",read,[&](std::span<const std::byte> event){Check(event.size()==332 && event[128]==std::byte{'H'} && event[196]==std::byte{1});++fallback_calls;});
        Check(fallback.Select("J08"));fallback.Advance(1);fallback.Advance(.5f);Check(fallback_calls==1 && fallback.Time()==1.5f);
        bool rejected=false;
        ActorPlayback no_events(actor,"101",[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});Check(no_events.Select("J08"));no_events.Advance(5);Check(no_events.Time()==4 && no_events.Completed());no_events.Sample(pose);Check(pose[0].position[0]==5);
        Check(no_events.Select("J08"));no_events.SetRestart(true);no_events.Advance(5);Check(no_events.Time()==0 && no_events.Completed());no_events.Advance(.5f);Check(!no_events.Completed());
        {
            auto blended=Fixture();blended.clips.push_back({"C01",2,{{0,{{0,{0,0,0,1},{10,0,0}},{2,{0,0,0,1},{12,0,0}}}}}});
            ActorPlayback p(blended,"101",[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
            std::array<std::uint32_t,31> blend_image{};const auto binding=p.BindAnimationImage(blend_image,0xc0170,0x200000);
            Check(p.Select("J08"));p.Advance(.75f);Check(p.SelectBlended("C01",.2f) && blend_image[15]);
            p.Advance(.1f);p.Sample(pose);Check(pose[0].position[0]==1); // original uses pre-decrement remaining time
            p.Advance(.05f);p.Sample(pose,&frames);Check(std::abs(pose[0].position[0]-6)<1e-5f && std::abs(frames[0][12]-5)<1e-5f);
            p.Advance(.1f);p.Sample(pose);Check(p.Action()=="C01" && p.Time()==0 && pose[0].position[0]==11);
            p.Advance(.5f);p.Sample(pose);Check(std::abs(pose[0].position[0]-11.5f)<1e-5f);
            Check(!blend_image[15]);p.UnbindAnimationImage(binding);
            auto scaled=blended;scaled.frames[0].local[0]=2;scaled.clips[0].tracks.clear();scaled.clips[1].tracks.clear();
            SampleBlendedActorVertices(scaled,0,1,.5f,pose);Check(pose[0].position[0]==2);
            // Opposed rotations must blend in bone space, preserving the radius.
            blended.clips[0].tracks[0].keys={ {0,{0,0,0,1},{0,0,0}} };
            blended.clips[1].tracks[0].keys={ {0,{0,0,1,0},{0,0,0}} };
            SampleBlendedActorVertices(blended,0,1,.5f,pose);
            Check(std::abs(pose[0].position[0])<1e-5f && std::abs(std::abs(pose[0].position[1])-1)<1e-5f);
            const auto first=blended.clips[0],second=blended.clips[1];auto detached=blended;detached.clips.clear();std::vector<Vertex> independent;
            SampleBlendedActorClipVertices(detached,first,second,.5f,independent);Check(independent.size()==pose.size() && std::equal(std::begin(independent[0].position),std::end(independent[0].position),std::begin(pose[0].position)));
            auto bad=second;bad.tracks[0].keys.clear();bool invalid=false;try{SampleBlendedActorClipVertices(detached,first,bad,.5f,independent);}catch(const std::exception&){invalid=true;}Check(invalid);
        }
        for(std::size_t size=0;size<bytes.size();++size) {
            ActorPlayback truncated(actor,"101",[&](auto)->std::optional<std::vector<std::uint8_t>>{return std::vector<std::uint8_t>(bytes.begin(),bytes.begin()+size);});
            rejected=false;try{(void)truncated.Select("J08");}catch(const std::logic_error&){rejected=true;}Check(rejected);
        }
        std::cout<<"original AMF load, one-shot HOLD, pause/rewind, endpoint pose and truncation contracts passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
