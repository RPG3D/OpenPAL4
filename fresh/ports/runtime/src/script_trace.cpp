#include "pal4/portable/script.h"
#include <bit>
#include <iomanip>
#include <sstream>

namespace pal4::portable {
std::string EncodeScriptCall(const ScriptCall& call) {
    std::ostringstream out; out<<"call "<<call.system_id<<" story "<<call.story_before;
    for (const auto& value:call.arguments) {
        out<<' ';
        if (const auto* p=std::get_if<std::int32_t>(&value)) out<<"i:"<<*p;
        else if (const auto* boolean=std::get_if<bool>(&value)) out<<"b:"<<(*boolean?1:0);
        else if (const auto* real=std::get_if<float>(&value)) out<<"f:"<<std::hex<<std::setw(8)<<std::setfill('0')<<std::bit_cast<std::uint32_t>(*real)<<std::dec;
        else if (const auto* wide=std::get_if<double>(&value)) out<<"d:"<<std::hex<<std::setw(16)<<std::setfill('0')<<std::bit_cast<std::uint64_t>(*wide)<<std::dec;
        else if (const auto* text=std::get_if<std::string>(&value)) {
            out<<"s:";
            for (unsigned char c:*text) out<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c);
            out<<std::dec;
        } else out<<"void";
    }
    return out.str()+"\n";
}
}  // namespace pal4::portable
