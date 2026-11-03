#include "pal4/portable/gbk.h"
#include "pal4/portable/csb.h"
#include "pal4/portable/csb_execution.h"
#include "pal4/portable/package.h"
#include "pal4/portable/script_stack.h"
#include "pal4/portable/script_immediate.h"
#include <fstream>
#include <bit>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value,const char* message="CSB contract failed"){if(!value)throw std::runtime_error(message);}
void Word(std::vector<std::uint8_t>& b,std::uint32_t value){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(value>>(i*8)));}
void Text(std::vector<std::uint8_t>& b,std::string_view value){Word(b,static_cast<std::uint32_t>(value.size()));b.insert(b.end(),value.begin(),value.end());}
void Type(std::vector<std::uint8_t>& b){b.push_back(0);Word(b,69);Text(b,"");b.insert(b.end(),4,0);}
void Function(std::vector<std::uint8_t>& b,std::string_view name,int id,bool jump=false){
    Text(b,name);Type(b);Word(b,0);Word(b,static_cast<std::uint32_t>(id));Word(b,jump?14:6);
    if(jump){b.push_back(14);Word(b,0);}b.push_back(13);b.insert(b.end(),2,0);
    Word(b,0);Word(b,0);Text(b,"");Word(b,0);
}
std::vector<std::uint8_t> Fixture(){
    std::vector<std::uint8_t> b(4);Word(b,0);Word(b,0);Word(b,0);Word(b,0);
    Function(b,"@init",-1);Function(b,"@exit",-1);Word(b,1);Function(b,"Main",0,true);Word(b,1);Text(b,std::string_view("\xc4\xe3\xba\xc3\0",5));Word(b,0);
    const auto size=static_cast<std::uint32_t>(b.size()-4);for(unsigned i=0;i<4;++i)b[i]=static_cast<std::uint8_t>(size>>(i*8));return b;
}
template<class F> void Reject(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}Check(failed);}
void Inspect(const CsbModule& module,std::string_view entry){
    const auto* f=module.Find(entry);if(!f)throw std::runtime_error("entry missing");
    std::array<std::uint64_t,123> counts{};std::uint64_t instructions=0;
    for(const auto& function:module.functions)for(const auto& i:function.instructions){++counts[i.opcode];++instructions;}
    std::cout<<"functions="<<module.functions.size()<<" strings="<<module.strings.size()<<" imports="<<module.imports.size()<<" instructions="<<instructions<<" entry="<<f->name<<" parameters="<<f->parameters.size()<<" stack="<<f->stack_slots<<'\n';
    for(const auto& i:f->instructions)std::cout<<"instruction "<<i.offset<<' '<<unsigned(i.opcode)<<' '<<i.operand<<'\n';
}
std::shared_ptr<CsbModule> StringAllocationFixture() {
    auto module=std::make_shared<CsbModule>();module->used_types={"string"};
    CsbFunction function;function.name="Main";function.result.token=69;function.stack_slots=1;
    const auto constructor=std::uint64_t(0xfffffff2u)<<32;
    function.instructions={{0,0,1,6,1},{6,6,7,6,1},{12,12,109,12,constructor},
        {24,24,4,6,1},{30,30,97,8,static_cast<std::uint32_t>(-154)},
        {38,38,7,6,1},{44,44,110,4,0},{48,48,13,6,0}};
    module->functions.push_back(std::move(function));return module;
}
struct StringAllocationHost:IScriptServices {
    bool fail{};unsigned calls{};
    ScriptResponse Invoke(const ScriptCall& call) override {
        Check(call.system_id==154 && call.arguments.size()==1 &&
            std::get<std::string>(call.arguments[0]).empty());
        ++calls;if(fail)throw std::runtime_error("controlled allocation callback failure");
        return {true,false};
    }
};
void CheckNarrowIntegerOpcode(std::uint8_t opcode,std::uint32_t input,std::int32_t expected) {
    auto module=std::make_shared<CsbModule>();CsbFunction function;
    function.name="Main";function.result.token=69;
    function.instructions={{0,0,2,8,input},{8,8,opcode,4,0},
        {12,12,100,8,static_cast<std::uint32_t>(-1)},{20,20,13,6,0}};
    module->functions.push_back(std::move(function));ScriptGlobals globals{};
    StringAllocationHost host;CsbExecution execution(module,"Main",globals,host);
    Check(execution.Execute()==ScriptState::finished && globals[0]==expected,
        "signed/unsigned narrow integer conversion failed");
}
void CheckReferenceByteOps() {
    auto module=std::make_shared<CsbModule>();CsbFunction function;
    function.name="Main";function.result.token=69;
    function.instructions={{0,0,1,6,1},{6,6,2,8,0x1234u},{14,14,7,6,1},
        {20,20,57,4,0},{24,24,0,6,1},{30,30,7,6,1},{36,36,116,6,0},
        {42,42,100,8,static_cast<std::uint32_t>(-1)},{50,50,13,6,0}};
    module->functions.push_back(std::move(function));ScriptGlobals globals{};
    StringAllocationHost host;CsbExecution execution(module,"Main",globals,host);
    Check(execution.Execute()==ScriptState::finished && globals[0]==0x34,
        "BC_WRT1/BC_RD1 reference semantics failed");
}
}
int main(int argc,char** argv){
    try {
        if(argc==5 && std::string_view(argv[3])=="--extract") {
            std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);
            const auto bytes=package.Read(argv[2]);std::ofstream output(argv[4],std::ios::binary|std::ios::trunc);
            if(!output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))
                throw std::runtime_error("cannot write extracted CSB");
            std::cout<<"extracted "<<bytes.size()<<" CSB bytes\n";return 0;
        }
        if((argc==5 || argc==6) && std::string_view(argv[4])=="--steps"){
            std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);
            auto module=std::make_shared<const CsbModule>(ReadCsb(package.Read(argv[2])));
            struct Recording final:IScriptServices {
                ScriptImmediateState immediate;std::string trace;unsigned frame{},calls{};
                std::uint32_t ImmediateMode() const override{return immediate.Current();}
                ScriptResponse Invoke(const ScriptCall& call) override {
                    ++calls;trace+="step "+std::to_string(frame)+" "+EncodeScriptCall(call);
                    if(call.system_id==55)immediate.Begin();else if(call.system_id==56)immediate.End();
                    if(call.system_id==133 || call.system_id==287 || call.system_id==288)return {std::int32_t(1),false};return {};
                }
            } recording;
            ScriptGlobals globals{};if(argc==6)globals[0]=std::stoi(argv[5]);CsbExecution execution(module,argv[3],globals,recording,DecodeGbk);
            for(unsigned frame=1;frame<=10000;++frame){
                recording.frame=frame;const auto state=execution.Execute(1000000,ScriptRunMode::original_steps);
                recording.trace+="frame "+std::to_string(frame)+" mode "+std::to_string(recording.ImmediateMode())+" saved "+std::to_string(recording.immediate.Saved())+" story "+std::to_string(globals[0])+" calls "+std::to_string(recording.calls)+" state "+(state==ScriptState::finished?"finished":state==ScriptState::suspended?"suspended":"failed")+"\n";
                if(state==ScriptState::failed){std::cout<<recording.trace;throw std::runtime_error(execution.Error());}
                if(state==ScriptState::finished){std::cout<<recording.trace<<"final "<<globals[0]<<'\n';return 0;}
            }throw std::runtime_error("legacy step fixture frame limit exceeded");
        }
        if((argc==5 || argc==6) && std::string_view(argv[4])=="--execute") {
            std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);
            auto module=std::make_shared<CsbModule>(ReadCsb(package.Read(argv[2])));
            struct Recording:IScriptServices {
                unsigned calls{};std::string trace;
                ScriptResponse Invoke(const ScriptCall& call) override {
                    ++calls;trace+=EncodeScriptCall(call);
                    // Same explicit query fixtures as the source/stack opening oracle.
                    if(call.system_id==133 || call.system_id==287 || call.system_id==288)return {std::int32_t(1),true};
                    return {{},true};
                }
            } recording;
            ScriptGlobals globals{};if(argc==6)globals[0]=std::stoi(argv[5]);CsbExecution execution(module,argv[3],globals,recording,DecodeGbk);
            for(unsigned tick=0;tick<10001;++tick) {
                const auto state=execution.Execute(10000);
                if(state==ScriptState::failed){std::cout<<recording.trace;throw std::runtime_error(execution.Error());}
                if(state==ScriptState::finished){std::cout<<recording.trace<<"final "<<globals[0]<<'\n';return 0;}
            }
            throw std::runtime_error("legacy execution call budget exhausted");
        }
        if(argc==4){std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);Inspect(ReadCsb(package.Read(argv[2])),argv[3]);return 0;}
        const auto fixture=Fixture();const auto module=ReadCsb(fixture);
        Check(module.functions.size()==1 && module.Find("Main") && !module.Find("missing"));
        Check(module.Find("Main")->instructions[1].offset==8 && module.Find("Main")->instructions[1].file_offset>8);
        Check(module.strings[0]==std::string("\xc4\xe3\xba\xc3\0",5));
        CheckNarrowIntegerOpcode(53,0x80u,-128);
        CheckNarrowIntegerOpcode(54,0x8001u,-32767);
        CheckNarrowIntegerOpcode(55,0x1234ff80u,0x80);
        CheckNarrowIntegerOpcode(56,0x1234ff80u,0xff80);
        CheckReferenceByteOps();
        for(std::size_t i=0;i<fixture.size();++i)Reject([&]{ReadCsb(std::span(fixture).first(i));});
        auto bad=fixture;bad.push_back(0);Reject([&]{ReadCsb(bad);});
        const auto offset=module.Find("Main")->instructions[0].file_offset;
        bad=fixture;bad[offset]=123;Reject([&]{ReadCsb(bad);});
        bad=fixture;bad[offset+1]=1;Reject([&]{ReadCsb(bad);});
        bad=fixture;bad[offset+1]=0xff;bad[offset+2]=0xff;bad[offset+3]=0xff;bad[offset+4]=0xff;Reject([&]{ReadCsb(bad);});
        for(unsigned op=0;op<123;++op)Check(CsbInstructionBytes(static_cast<std::uint8_t>(op))>=4);
        Reject([]{CsbInstructionBytes(255);});
        struct Empty:IScriptServices {ScriptResponse Invoke(const ScriptCall&) override {throw std::runtime_error("unexpected service");}} empty;
        ScriptGlobals globals{};auto shared=std::make_shared<CsbModule>(module);
        CsbExecution small(shared,"Main",globals,empty);
        Check(small.Execute(1)==ScriptState::suspended && small.PauseReason()==ScriptPause::budget);
        Check(small.Execute(1)==ScriptState::finished);Reject([&]{small.Execute();});
        shared->functions[0].instructions={{0,0,97,8,static_cast<std::uint32_t>(-237)},
            {8,0,2,8,123},{16,0,100,8,0xffffffffu},{24,0,13,6,0}};
        struct Pause:IScriptServices {unsigned calls{};ScriptResponse Invoke(const ScriptCall& call) override {Check(call.system_id==237);++calls;return {{},true};}} pause;
        CsbExecution resumed(shared,"Main",globals,pause);
        Check(resumed.Execute()==ScriptState::suspended && resumed.PauseReason()==ScriptPause::service && globals[0]==0);
        Check(resumed.Execute()==ScriptState::finished && globals[0]==123 && pause.calls==1);
        CsbExecution failed(shared,"Main",globals,empty);Check(failed.Execute()==ScriptState::failed);Reject([&]{failed.Execute();});
        shared->functions[0].instructions[0].opcode=122;CsbExecution unsupported(shared,"Main",globals,pause);
        Check(unsupported.Execute()==ScriptState::failed && pause.calls==1);
        shared->functions[0].instructions={{0,0,12,8,1},{8,0,13,6,0}};
        CsbFunction helper;helper.name="Helper";helper.id=1;helper.result.token=69;
        helper.instructions={{0,0,97,8,static_cast<std::uint32_t>(-237)},{8,0,13,6,0}};shared->functions.push_back(helper);
        CsbExecution nested(shared,"Main",globals,pause);
        Check(nested.Execute()==ScriptState::suspended && pause.calls==2);
        Check(nested.Execute()==ScriptState::finished && pause.calls==2);
        shared->functions[0].instructions={{0,0,2,8,42},{8,0,12,8,1},{16,0,88,4,0},{20,0,100,8,0xffffffffu},{28,0,13,6,0}};
        shared->functions[1].instructions={{0,0,4,6,0},{6,0,86,4,0},{10,0,13,6,1}};
        CsbType integer;integer.token=62;shared->functions[1].parameters={integer};shared->functions[1].result=integer;
        CsbExecution arguments(shared,"Main",globals,empty);Check(arguments.Execute()==ScriptState::finished && globals[0]==42);
        // SET8 must preserve both halves and the original low-word-first stack.
        struct Wait:IScriptServices {
            unsigned calls{};
            ScriptResponse Invoke(const ScriptCall& call) override {
                Check(call.system_id==239 && call.arguments.size()==1);
                Check(std::get<double>(call.arguments[0])==-1.23456789012345);
                ++calls;return {{},true};
            }
        } wait;
        shared->functions[0].instructions={{0,0,66,12,std::bit_cast<std::uint64_t>(-1.23456789012345)},
            {12,0,97,8,static_cast<std::uint32_t>(-239)},{20,0,13,6,0}};
        CsbExecution wide(shared,"Main",globals,wait);
        Check(wide.Execute()==ScriptState::suspended && wide.PauseReason()==ScriptPause::service && wait.calls==1);
        Check(wide.Execute()==ScriptState::finished && wait.calls==1);
        {
            ScriptGlobals allocated_globals{};StringAllocationHost allocated_host;
            CsbExecution allocated(StringAllocationFixture(),"Main",allocated_globals,allocated_host);
            Check(allocated.Execute()==ScriptState::finished && allocated_host.calls==1,
                "BC_ALLOC/BC_FREE string lifecycle failed");
        }
        {
            ScriptGlobals failed_globals{};StringAllocationHost failed_host;failed_host.fail=true;
            CsbExecution failed(StringAllocationFixture(),"Main",failed_globals,failed_host);
            Check(failed.Execute()==ScriptState::failed && failed_host.calls==1 &&
                failed.Error().find("controlled allocation callback failure")!=std::string::npos,
                "BC_ALLOC exception cleanup failed");
        }
        struct Host:IScriptContextServices {ScriptResponse Invoke(const ScriptRequest&) override {throw std::runtime_error("unexpected stack service");}} host;
        ScriptStack stack(globals);stack.LoadCsb("original",fixture);Check(bool(stack.Push("original","void Main()",host)));
        Reject([&]{stack.LoadCsb("original",fixture);});
        Check(stack.Tick(0,1)==ScriptState::suspended && stack.WaitKind()==ScriptWaitKind::none);
        Check(stack.Tick(0,1)==ScriptState::finished && stack.PopFinished());
        Check(bool(stack.Push("original","void Main()",host)));stack.Clear();Check(stack.Depth()==0);
        std::cout<<"CSB exact lengths, wire widths, raw GBK, instructions, jumps and truncation passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
