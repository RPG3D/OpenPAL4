#include "pal4/portable/sound.h"
#include "pal4/script_sound_2d.h"
#include <algorithm>
#include <bit>
#include <deque>
#include <limits>
#include <map>
#include <ostream>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t offset,unsigned count=4) {
    Require(offset<=bytes.size() && count<=bytes.size()-offset,"truncated WAV");std::uint32_t value{};
    for(unsigned i=0;i<count;++i)value|=std::uint32_t(bytes[offset+i])<<(i*8);
    return value;
}
bool Tag(std::span<const std::uint8_t> bytes,std::size_t offset,std::string_view tag) {
    return offset<=bytes.size() && tag.size()<=bytes.size()-offset && std::equal(tag.begin(),tag.end(),bytes.begin()+offset);
}
enum class Phase {queued,prepared,playing,paused,completed,stopped,failed};
const char* Name(Phase phase) {
    switch(phase){case Phase::queued:return "queued";case Phase::prepared:return "prepared";case Phase::playing:return "playing";case Phase::paused:return "paused";case Phase::completed:return "completed";case Phase::stopped:return "stopped";case Phase::failed:return "failed";}
    return "unknown";
}
bool Terminal(Phase phase){return phase==Phase::completed || phase==Phase::stopped || phase==Phase::failed;}
}
WaveInfo InspectWave(std::span<const std::uint8_t> bytes) {
    Require(bytes.size()>=12 && bytes.size()<=64u*1024*1024 && Tag(bytes,0,"RIFF") && Tag(bytes,8,"WAVE"),"unsupported WAV container");
    Require(Word(bytes,4)==bytes.size()-8,"incorrect WAV RIFF length");WaveInfo result;bool format{},data{};
    for(std::size_t offset=12;offset<bytes.size();) {
        const auto size=Word(bytes,offset+4);Require(size<=bytes.size()-offset-8,"truncated WAV chunk");const auto payload=offset+8;
        if(Tag(bytes,offset,"fmt ")) {
            Require(!format && size>=16,"invalid WAV format chunk");format=true;
            result.codec=Word(bytes,payload,2);result.channels=Word(bytes,payload+2,2);result.sample_rate=Word(bytes,payload+4);result.bits=Word(bytes,payload+14,2);
            Require(result.channels>0 && result.channels<=8 && result.sample_rate>0 && result.sample_rate<=384000 && result.codec>0,"invalid WAV format");
            if(result.codec==1){const auto align=Word(bytes,payload+12,2);Require(result.bits>0 && result.bits%8==0 && align==result.channels*(result.bits/8) && Word(bytes,payload+8)==result.sample_rate*align,"invalid PCM WAV alignment");}
        } else if(Tag(bytes,offset,"data")){Require(!data && size>0,"invalid WAV data chunk");data=true;result.data_bytes=size;}
        offset=payload+size; if(size&1u){Require(offset<bytes.size(),"missing WAV chunk padding");++offset;}
    }
    Require(format && data,"missing WAV format/data");
    if(result.codec==1)Require(result.data_bytes%(result.channels*(result.bits/8))==0,"partial PCM WAV frame");
    return result;
}
struct SoundServices::Impl final:pal4::IScriptSound2dRuntime {
    struct Sample {std::uint32_t handle{};std::string member;std::int32_t loops{};std::size_t bytes{};WaveInfo wave;Phase phase{Phase::queued};unsigned starts{},pauses{};std::string error;};
    static constexpr std::uint32_t executor=0x1000,source=0x2000,audio=1;
    IScriptContextServices& next;const std::uint64_t generation;SoundReader read;
    std::string requested_name;std::uint32_t next_handle{1},next_script_id{},current{},calls{};
    std::map<std::uint32_t,Sample> samples;
    std::map<std::int32_t,std::uint32_t> ids;
    std::deque<SoundCommand> commands;
    Impl(IScriptContextServices& n,std::uint64_t g,SoundReader r):next(n),generation(g),read(std::move(r)){}
    std::string_view SoundName(std::uint32_t token) override{Require(token==source,"invalid sound name handle");return requested_name;}
    std::uint32_t AudioManager() override{return audio;}
    std::uint32_t ScriptExecutor() override{return executor;}
    std::uint32_t PlaySound(std::uint32_t manager,std::uint32_t name,std::string_view path,std::int32_t mode) override {
        Require(manager==audio && name==source && mode>=0,"invalid 2D sound binding");
        Require(next_handle<std::numeric_limits<std::uint32_t>::max(),"sound handle limit");
        auto bytes=read(path);const auto wave=InspectWave(bytes);const auto handle=next_handle++;
        Sample sample;sample.handle=handle;sample.member=requested_name+".wav";sample.loops=mode;sample.bytes=bytes.size();sample.wave=wave;
        samples.emplace(handle,sample);commands.push_back({SoundCommandKind::play,generation,handle,sample.member,mode,std::move(bytes)});return handle;
    }
    std::int32_t InsertSoundId(std::uint32_t owner,std::uint32_t handle) override {
        Require(owner==executor && samples.contains(handle) && next_script_id<=std::uint32_t(std::numeric_limits<std::int32_t>::max()),"invalid script sound ID");
        const auto id=static_cast<std::int32_t>(next_script_id++);ids.emplace(id,handle);return id;
    }
    void SetCurrentHandle(std::uint32_t address,std::uint32_t handle) override{Require(address==executor+0x20 && samples.contains(handle),"invalid current sound handle");current=handle;}
    std::uint32_t CheckActive(std::uint32_t address) override{Require(address==executor+0x20,"invalid sound status address");return current && samples.contains(current) && !Terminal(samples.at(current).phase);}
    void Stop(std::uint32_t handle) {
        const auto found=samples.find(handle);if(found==samples.end() || Terminal(found->second.phase))return;
        found->second.phase=Phase::stopped;commands.push_back({SoundCommandKind::stop,generation,handle,found->second.member,0,{}});if(current==handle)current=0;
    }
    std::uint32_t StopCurrent(std::uint32_t address) override{Require(address==executor+0x20,"invalid sound stop address");const auto handle=current;Stop(handle);return handle?1:0;}
    std::uint32_t RemoveSoundId(std::uint32_t owner,std::int32_t id) override {Require(owner==executor,"invalid script sound owner");const auto found=ids.find(id);if(found==ids.end())return 0;Stop(found->second);ids.erase(found);return 1;}
};
SoundServices::SoundServices(IScriptContextServices& next,std::uint64_t generation,SoundReader read):impl_(std::make_unique<Impl>(next,generation,std::move(read))){}
SoundServices::~SoundServices()=default;
void SoundServices::OnCall(const ScriptRequest& request){impl_->next.OnCall(request);}
ScriptResponse SoundServices::Invoke(const ScriptRequest& request) {
    const auto& call=request.call;
    if(call.system_id==133) {
        Require(call.arguments.size()==2,"invalid 2D sound arguments");const auto& name=std::get<std::string>(call.arguments[0]);
        Require(!name.empty() && name.size()<=128 && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==std::string::npos,"invalid 2D sound resource name");
        impl_->requested_name=name;++impl_->calls;
        return {pal4::ScriptSound2d::Play(Impl::source,std::get<std::int32_t>(call.arguments[1]),*impl_),false};
    }
    if(call.system_id==134){Require(call.arguments.empty(),"invalid sound stop arguments");++impl_->calls;(void)pal4::ScriptSound2d::StopCurrent(*impl_);return {};}
    if(call.system_id==135){Require(call.arguments.size()==1,"invalid sound ID arguments");++impl_->calls;(void)pal4::ScriptSound2d::StopId(std::get<std::int32_t>(call.arguments[0]),*impl_);return {};}
    return impl_->next.Invoke(request);
}
std::uint32_t SoundServices::PlayUiSound(std::string_view name,std::uint32_t repeats){
    Require(!name.empty() && name.size()<=128 && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==std::string_view::npos && repeats>0 && repeats<=std::uint32_t(std::numeric_limits<std::int32_t>::max()),"invalid UI sound request");
    impl_->requested_name=name;
    return impl_->PlaySound(Impl::audio,Impl::source,"gamedata\\PALSound\\"+std::string(name)+".wav",static_cast<std::int32_t>(repeats));
}
std::optional<SoundCommand> SoundServices::TakeCommand(){if(impl_->commands.empty())return {};auto command=std::move(impl_->commands.front());impl_->commands.pop_front();return command;}
bool SoundServices::Event(std::uint64_t generation,std::uint32_t handle,std::string_view event,std::string_view error) {
    if(generation!=impl_->generation)return false;
    const auto found=impl_->samples.find(handle);if(found==impl_->samples.end())return false;auto& sample=found->second;
    if(Terminal(sample.phase))return false;
    if(event=="prepared"){if(sample.phase!=Phase::queued)return false;sample.phase=Phase::prepared;}
    else if(event=="started"){if(sample.phase!=Phase::prepared && sample.phase!=Phase::paused)return false;sample.phase=Phase::playing;++sample.starts;}
    else if(event=="paused"){if(sample.phase!=Phase::playing)return false;sample.phase=Phase::paused;++sample.pauses;}
    else if(event=="completed"){if(sample.phase!=Phase::playing && sample.phase!=Phase::paused)return false;sample.phase=Phase::completed;if(impl_->current==handle)impl_->current=0;}
    else if(event=="failed"){sample.phase=Phase::failed;sample.error=error;if(impl_->current==handle)impl_->current=0;}
    else throw std::invalid_argument("unknown sound platform event");
    return true;
}
void SoundServices::WriteReport(std::ostream& out) const {
    out<<"{\"generation\":"<<impl_->generation<<",\"calls\":"<<impl_->calls<<",\"current_handle\":"<<impl_->current<<",\"script_ids\":"<<impl_->ids.size()<<",\"samples\":[";bool first=true;
    for(const auto& [handle,s]:impl_->samples){if(!first)out<<',';first=false;
        out<<"{\"handle\":"<<handle<<",\"member\":\""<<s.member<<"\",\"loops\":"<<s.loops<<",\"bytes\":"<<s.bytes<<",\"codec\":"<<s.wave.codec<<",\"channels\":"<<s.wave.channels
            <<",\"sample_rate\":"<<s.wave.sample_rate<<",\"bits\":"<<s.wave.bits<<",\"data_bytes\":"<<s.wave.data_bytes<<",\"phase\":\""<<Name(s.phase)<<"\",\"starts\":"<<s.starts<<",\"pauses\":"<<s.pauses<<'}';
    }
    out<<"]}";
}
}
