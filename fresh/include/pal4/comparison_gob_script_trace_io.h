#pragma once
#include "pal4/comparison_gob_script_trace.h"
#include <filesystem>
#include <fstream>
#include <locale>
#include <ostream>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace pal4 {
inline void WriteComparisonGobScriptTrace(std::ostream& out,const ComparisonGobScriptTrace& trace,
    std::uint32_t pid,std::uint32_t request){
    out.imbue(std::locale::classic());
    out<<"{\"pid\":"<<pid<<",\"request\":"<<request<<",\"enabled\":"<<(trace.Enabled()?"true":"false")
        <<",\"capacity\":"<<ComparisonGobScriptTrace::kCapacity<<",\"count\":"<<trace.Count()
        <<",\"sequence\":"<<trace.Sequence()<<",\"dropped\":"<<trace.Dropped()<<",\"entries\":[";
    for(std::uint32_t i=0;i<trace.Count();++i){
        const auto& e=trace.Entries()[i];if(i)out<<',';
        out<<"{\"sequence\":"<<e.sequence<<",\"id\":"<<e.id<<",\"after\":"<<(e.after?"true":"false")
            <<",\"frame_gate_enabled\":"<<(e.frame_gate_enabled?"true":"false")
            <<",\"outer_completed\":"<<e.outer_completed<<",\"outer_callbacks\":"<<e.outer_callbacks
            <<",\"outer_depth\":"<<e.outer_depth<<",\"name\":\"";
        // JSON byte escaping preserves bounded arbitrary game-name bytes;
        // no UTF conversion or interpretation occurs in Record.
        constexpr char hex[]="0123456789abcdef";
        for(std::uint32_t j=0;j<e.name_length;++j){
            const auto c=static_cast<unsigned char>(e.name[j]);
            if(c=='"' || c=='\\')out<<'\\'<<static_cast<char>(c);
            else if(c<0x20u || c>=0x7fu)out<<"\\u00"<<hex[c>>4u]<<hex[c&15u];
            else out<<static_cast<char>(c);
        }
        out<<"\",\"name_length\":"<<e.name_length<<",\"name_truncated\":"<<(e.name_truncated?"true":"false")
            <<",\"argc\":"<<e.argc<<",\"args\":[";
        for(std::uint32_t j=0;j<e.argc;++j){if(j)out<<',';out<<e.args[j];}
        out<<"]}";
    }
    out<<"]}\n";
}
// Explicit observation message only; never run file IO in an actual callback.
inline void PublishComparisonGobScriptTrace(std::string_view side,std::uint32_t request){
    if((side!="original" && side!="native") || !request)
        throw std::invalid_argument("GOB script trace result identity");
    std::array<wchar_t,32768> root{};
    const auto length=GetEnvironmentVariableW(L"PAL4_COMPARE_STATE_DIR",root.data(),static_cast<DWORD>(root.size()));
    if(!length || length>=root.size())throw std::runtime_error("GOB script trace output directory absent");
    const auto directory=std::filesystem::path(root.data());
    if(!directory.is_absolute())throw std::runtime_error("GOB script trace output directory must be absolute");
    const auto path=directory/("compare-"+std::string(side)+"-gob-script-"+
        std::to_string(GetCurrentProcessId())+"-"+std::to_string(request)+".json");
    const auto temporary=std::filesystem::path(path.wstring()+L".tmp");
    {
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
        WriteComparisonGobScriptTrace(out,GetComparisonGobScriptTrace(),GetCurrentProcessId(),request);
        out.flush();if(!out)throw std::runtime_error("GOB script trace result write failed");
    }
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("GOB script trace result publication failed");
}
}
