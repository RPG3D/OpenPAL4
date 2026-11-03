#include "pal4/portable/sound.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <source_location>
using namespace pal4::portable;
namespace {
void Check(bool value,std::source_location line=std::source_location::current()){if(!value)throw std::runtime_error("sound contract line "+std::to_string(line.line()));}
void Word(std::vector<std::uint8_t>& b,std::uint32_t value,unsigned count=4){for(unsigned i=0;i<count;++i)b.push_back(static_cast<std::uint8_t>(value>>(8*i)));}
void Tag(std::vector<std::uint8_t>& b,std::string_view text){b.insert(b.end(),text.begin(),text.end());}
std::vector<std::uint8_t> Wave(){std::vector<std::uint8_t> b;Tag(b,"RIFF");Word(b,40);Tag(b,"WAVEfmt ");Word(b,16);Word(b,1,2);Word(b,1,2);Word(b,22050);Word(b,44100);Word(b,2,2);Word(b,16,2);Tag(b,"data");Word(b,4);Word(b,0);return b;}
struct Next final:IScriptContextServices{unsigned calls{};ScriptResponse Invoke(const ScriptRequest&) override{++calls;return {std::int32_t(7),false};}};
}
int main(int argc,char** argv){try {
    if(argc==2){std::ifstream in(argv[1],std::ios::binary);std::vector<std::uint8_t> b{std::istreambuf_iterator<char>(in),{}};const auto info=InspectWave(b);std::cout<<"{\"codec\":"<<info.codec<<",\"channels\":"<<info.channels<<",\"sample_rate\":"<<info.sample_rate<<",\"bits\":"<<info.bits<<",\"data_bytes\":"<<info.data_bytes<<'}';return 0;}
    const auto wave=Wave();const auto info=InspectWave(wave);Check(info.sample_rate==22050 && info.channels==1 && info.bits==16 && info.data_bytes==4);
    for(std::size_t i=0;i<wave.size();++i){bool failed=false;try{(void)InspectWave(std::span(wave).first(i));}catch(const std::exception&){failed=true;}Check(failed);}
    Next next;unsigned reads{};SoundServices sounds(next,9,[&](std::string_view path){Check(path=="gamedata\\PALSound\\WJ001.wav");++reads;return wave;});
    const auto call=[&](int id,std::vector<ScriptValue> args){ScriptCall c{id,0,std::move(args)};return sounds.Invoke({{1,1},c});};
    auto response=call(133,{std::string("WJ001"),std::int32_t(1)});Check(!response.suspend && std::get<std::int32_t>(response.value)==-1);
    auto command=sounds.TakeCommand();Check(command && command->handle==1 && command->loops==1 && command->bytes==wave && !sounds.TakeCommand());
    Check(!sounds.Event(8,1,"prepared") && !sounds.Event(9,2,"prepared") && !sounds.Event(9,1,"started"));
    Check(sounds.Event(9,1,"prepared") && sounds.Event(9,1,"started") && !sounds.Event(9,1,"started"));
    Check(sounds.Event(9,1,"paused") && sounds.Event(9,1,"started") && sounds.Event(9,1,"completed") && !sounds.Event(9,1,"completed"));
    response=call(133,{std::string("WJ001"),std::int32_t(-2)});Check(std::get<std::int32_t>(response.value)==0);
    command=sounds.TakeCommand();Check(command && command->handle==2 && command->loops==0);
    response=call(133,{std::string("WJ001"),std::int32_t(3)});command=sounds.TakeCommand();Check(std::get<std::int32_t>(response.value)==-1 && command && command->loops==3 && command->handle==3);
    (void)call(134,{});command=sounds.TakeCommand();Check(command && command->kind==SoundCommandKind::stop && command->handle==3 && !sounds.Event(9,3,"started"));
    (void)call(135,{std::int32_t(0)});command=sounds.TakeCommand();Check(command && command->handle==2);
    (void)call(135,{std::int32_t(0)});(void)call(135,{std::int32_t(-1)});Check(!sounds.TakeCommand());
    bool failed=false;try{(void)call(133,{std::string("../WJ001"),std::int32_t(1)});}catch(const std::exception&){failed=true;}Check(failed && reads==3);
    Check(std::get<std::int32_t>(call(140,{}).value)==7 && next.calls==1);
    std::ostringstream out;sounds.WriteReport(out);Check(out.str().find("\"current_handle\":0")!=std::string::npos && out.str().find("\"script_ids\":0")!=std::string::npos);
    {
        ScriptGlobals globals{};ScriptStack stack(globals);stack.Compile("sound","void run(){gi2DSoundPlay(\"WJ001\",1);}");
        Check(bool(stack.Push("sound","void run()",sounds)) && stack.Tick(0)==ScriptState::finished && sounds.TakeCommand().has_value());
    }
    {
        SoundServices ui(next,10,[&](auto path){Check(path=="gamedata\\PALSound\\WA05.wav");return wave;});
        const auto handle=ui.PlayUiSound("WA05",1);const auto queued=ui.TakeCommand();
        Check(handle==1 && queued && queued->handle==handle && queued->generation==10 && queued->member=="WA05.wav" && queued->loops==1);
        std::ostringstream report;ui.WriteReport(report);Check(report.str().find("\"current_handle\":0")!=std::string::npos && report.str().find("\"script_ids\":0")!=std::string::npos);
        bool rejected=false;try{ui.PlayUiSound("../WA05",1);}catch(const std::exception&){rejected=true;}Check(rejected);
    }
    std::cout<<"sound original modes, nonblocking script, handles, events, stops and WAV bounds passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
