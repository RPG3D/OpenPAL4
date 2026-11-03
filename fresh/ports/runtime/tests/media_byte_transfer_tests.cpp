#include "pal4/portable/media.h"
#include "pal4/portable/media_byte_transfer.h"
#include "pal4/portable/sound.h"
#include <iostream>
#include <source_location>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value,std::source_location line=std::source_location::current()){if(!value)throw std::runtime_error("media byte transfer contract failed at line "+std::to_string(line.line()));}
struct Missing final:IScriptContextServices { ScriptResponse Invoke(const ScriptRequest&) override {throw std::runtime_error("unbound service");} };
struct Next final:IScriptContextServices { ScriptResponse Invoke(const ScriptRequest&) override {return {std::int32_t(7),false};} };
std::vector<std::uint8_t> Wave(){return {'R','I','F','F',40,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x22,0x56,0,0,0x44,0xac,0,0,2,0,16,0,'d','a','t','a',4,0,0,0,0,0,0,0};}
}
int main(){try {
    MediaByteTransferState transfer;
    Check(!transfer.Pending());
    transfer.Begin({MediaByteTransferKind::media,9,1},false);
    Check(!transfer.Pending() && !transfer.Complete());

    Missing missing;ScriptGlobals globals{};ScriptStack stack(globals);MediaServices media(missing,9);
    stack.Compile("media","void Main(){giPlayMovie(\"1A.bik\");global_mvar=42;}");
    Check(stack.Push("media","void Main()",media) && stack.Tick(0)==ScriptState::suspended);
    const auto media_command=*media.TakeCommand();
    transfer.Begin({MediaByteTransferKind::media,media_command.generation,media_command.id},true);
    bool overlap_rejected=false;
    try {transfer.Begin({MediaByteTransferKind::sound,10,1},true);} catch(const std::logic_error&) {overlap_rejected=true;}
    Check(overlap_rejected && transfer.Pending());
    const auto completed_transfer=transfer.Complete();
    Check(completed_transfer && completed_transfer->kind==MediaByteTransferKind::media && !transfer.Pending());
    Check(!media.Complete(completed_transfer->generation-1,completed_transfer->id,true));
    auto completion=media.Complete(completed_transfer->generation,completed_transfer->id,true);
    Check(completion && stack.Complete(completion->token) && stack.Tick(0)==ScriptState::finished && globals[0]==42);stack.Clear();

    globals.fill(0);MediaServices failed_media(missing,11);stack.Compile("failed_media","void Main(){giPlayMovie(\"2A.bik\");global_mvar=99;}");
    Check(stack.Push("failed_media","void Main()",failed_media) && stack.Tick(0)==ScriptState::suspended);
    const auto failed_command=*failed_media.TakeCommand();
    transfer.Begin({MediaByteTransferKind::media,failed_command.generation,failed_command.id},true);
    const auto failed_transfer=transfer.Fail();
    Check(failed_transfer && failed_transfer->generation==11 && !transfer.Pending());
    completion=failed_media.Complete(failed_transfer->generation,failed_transfer->id,false,"injected byte-array allocation failure");
    Check(completion && stack.Complete(completion->token,completion->error) && stack.Tick(0)==ScriptState::failed &&
        stack.Error()=="injected byte-array allocation failure" && globals[0]==0);stack.Clear();

    Next next;const auto wave=Wave();SoundServices sounds(next,13,[&](std::string_view){return wave;});
    auto response=sounds.Invoke({{1,1},ScriptCall{133,0,{std::string("WJ001"),std::int32_t(1)}}});
    Check(!response.suspend);
    auto sound_command=sounds.TakeCommand();Check(sound_command && sound_command->kind==SoundCommandKind::play);
    transfer.Begin({MediaByteTransferKind::sound,sound_command->generation,sound_command->handle},true);
    const auto sound_success=transfer.Complete();
    Check(sound_success && sound_success->kind==MediaByteTransferKind::sound && sounds.Event(sound_success->generation,sound_success->id,"prepared") &&
        sounds.Event(sound_success->generation,sound_success->id,"started"));

    response=sounds.Invoke({{1,2},ScriptCall{133,0,{std::string("WJ001"),std::int32_t(1)}}});
    sound_command=sounds.TakeCommand();Check(sound_command && sound_command->kind==SoundCommandKind::play);
    transfer.Begin({MediaByteTransferKind::sound,sound_command->generation,sound_command->handle},true);
    const auto sound_failure=transfer.Fail();
    Check(sound_failure && !sounds.Event(sound_failure->generation+1,sound_failure->id,"failed","stale") &&
        sounds.Event(sound_failure->generation,sound_failure->id,"failed","injected byte-array allocation failure"));
    std::ostringstream report;sounds.WriteReport(report);
    Check(report.str().find("\"phase\":\"failed\"")!=std::string::npos);

    response=sounds.Invoke({{1,3},ScriptCall{133,0,{std::string("WJ001"),std::int32_t(1)}}});
    sound_command=sounds.TakeCommand();Check(sound_command && sound_command->kind==SoundCommandKind::play);
    transfer.Begin({MediaByteTransferKind::sound,sound_command->generation,sound_command->handle},true);
    const auto stop_setup=transfer.Complete();
    Check(stop_setup && sounds.Event(stop_setup->generation,stop_setup->id,"prepared") && sounds.Event(stop_setup->generation,stop_setup->id,"started"));
    response=sounds.Invoke({{1,4},ScriptCall{134,0,{}}});
    sound_command=sounds.TakeCommand();Check(sound_command && sound_command->kind==SoundCommandKind::stop);
    transfer.Begin({MediaByteTransferKind::sound,sound_command->generation,sound_command->handle},false);
    Check(!transfer.Pending() && !transfer.Fail());
    transfer.Begin({MediaByteTransferKind::media,12,8},true);transfer.Reset();
    Check(!transfer.Pending());
    std::cout<<"media byte transfer tickets, empty commands, success, failure, stale generations and overlap passed\n";
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
