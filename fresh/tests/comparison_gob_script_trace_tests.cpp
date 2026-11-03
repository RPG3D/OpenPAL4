#include "pal4/comparison_gob_script_trace_io.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
int main(){
    try{
        _putenv_s("PAL4_COMPARE_SCRIPTED_INPUT","1");
        _putenv_s("PAL4_COMPARE_ATB_TRACE","1");
        _putenv_s("PAL4_COMPARE_FRAME_STEP_MS","17");
        _putenv_s("PAL4_COMPARE_FRAME_GATE","1");
        const std::array<std::uint32_t,5> args{0x3f800000u,0xc0000000u,0x7fc12345u,0xffffffffu,1u};
        pal4::ComparisonGobScriptTrace disabled;
        disabled.Record(-308,false,reinterpret_cast<const char*>(1u),args,5u);
        Check(disabled.Count()==0u && disabled.Sequence()==0u,"disabled trace reads no invalid text and records nothing");
        auto& gate=pal4::GetComparisonFrameGate();
        Check(gate.BeginFrame(),"diagnostic gate accepts bootstrap frame");
        gate.RequestArm(1u);Check(gate.EndFrame() && gate.Grant(123u,1u) && gate.BeginFrame(),
            "diagnostic gate enters one credited frame");
        gate.NoteCallback();
        auto& live=pal4::GetComparisonGobScriptTrace();
        Check(live.Enabled(),"trace requires scripted and ATB environments");
        live.Record(-308,false,"JA01",args,5u);
        live.Record(-308,true,std::string_view("JA01"),args,5u);
        Check(live.Count()==2u && live.Sequence()==2u && live.Dropped()==0u,"before and after retain actual call order");
        const auto& first=live.Entries()[0];const auto& second=live.Entries()[1];
        Check(first.id==-308 && !first.after && second.after && first.args==args && first.argc==5u &&
            first.frame_gate_enabled && first.outer_completed==0u && first.outer_callbacks==1u && first.outer_depth==1u,
            "raw argument words and actual frame ownership are copied without interpretation");
        Check(gate.Observe().remaining==0u && gate.Observe().depth==1u && gate.Observe().callbacks==1u &&
            pal4::GetComparisonFrameClock().Current()==0u,"record does not advance gate, callback count or clock");
        Check(gate.EndFrame(),"record leaves credited frame completion intact");
        pal4::ComparisonGobScriptTrace bounded(true);
        const char long_name[32]={'J'};
        std::array<char,32> full;full.fill('A');
        bounded.Record(-176,false,full.data(),args,3u);
        Check(bounded.Entries()[0].name_truncated && bounded.Entries()[0].name_length==31u &&
            bounded.Entries()[0].name[31]=='\0',"bounded original name without terminator is explicitly truncated");
        bounded.Record(-176,true,std::string_view(long_name,32u),args,3u);
        Check(bounded.Entries()[1].name_truncated,"modern SDK long names also carry completeness failure");
        bounded.Record(-303,false,nullptr,args,0u);
        Check(bounded.Entries()[2].name_length==0u && !bounded.Entries()[2].name_truncated,
            "null original name maps to empty bounded text");
        const auto count=bounded.Count();
        bounded.Record(-298,false,"treasure01",args,6u);
        Check(bounded.Count()==count && bounded.Dropped()==1u,"invalid argument count is rejected without overflow");
        while(bounded.Count()<pal4::ComparisonGobScriptTrace::kCapacity)
            bounded.Record(-302,false,"JA01",args,2u);
        const auto first_sequence=bounded.Entries()[0].sequence;
        bounded.Record(-302,true,"JA01",args,2u);
        Check(bounded.Count()==512u && bounded.Dropped()==2u && bounded.Entries()[0].sequence==first_sequence,
            "capacity drops new events without overwriting the first evidence");
        pal4::ComparisonGobScriptTrace escaped(true);
        const char name[]={'J','A','"','\\','\n',static_cast<char>(0xff)};
        escaped.Record(-176,false,std::string_view(name,sizeof(name)),args,3u);
        std::ostringstream output;
        pal4::WriteComparisonGobScriptTrace(output,escaped,77u,88u);
        const auto json=output.str();
        Check(json.find("JA\\\"\\\\\\u000a\\u00ff")!=std::string::npos &&
            json.find("\"args\":[1065353216,3221225472,2143363909]")!=std::string::npos &&
            json.find("\"name_truncated\":false")!=std::string::npos,"JSON preserves escaped name bytes and integer argument payload");
        Check(escaped.Count()==1u && escaped.Sequence()==1u,"JSON output does not clear or mutate evidence");
        bool rejected{};try{pal4::PublishComparisonGobScriptTrace("invalid",1u);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"publication rejects invalid side before filesystem access");
        rejected=false;try{pal4::PublishComparisonGobScriptTrace("original",0u);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"publication rejects zero request identity");
        std::cout<<json;
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
