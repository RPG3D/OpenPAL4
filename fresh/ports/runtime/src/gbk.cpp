#include "pal4/portable/gbk.h"
#include <array>
#include <cstdint>
#include <stdexcept>
namespace pal4::portable {
namespace {
constexpr std::array<std::uint16_t,126*190> mapping={
#include "cp936_table.inc"
};
void Append(std::string& output,std::uint32_t point) {
    if(point<128)output+=static_cast<char>(point);
    else if(point<0x800){output+=static_cast<char>(0xc0|(point>>6));output+=static_cast<char>(0x80|(point&63));}
    else {output+=static_cast<char>(0xe0|(point>>12));output+=static_cast<char>(0x80|((point>>6)&63));output+=static_cast<char>(0x80|(point&63));}
}
}
std::string DecodeGbk(std::string_view input) {
    std::string output;output.reserve(input.size());
    for(std::size_t i=0;i<input.size();) {
        const auto lead=static_cast<unsigned char>(input[i++]);
        if(lead<128){Append(output,lead);continue;}
        if(lead==0x80){Append(output,0x20ac);continue;}
        if(lead<0x81 || lead>0xfe || i==input.size())throw std::runtime_error("invalid or truncated CP936 lead");
        const auto trail=static_cast<unsigned char>(input[i++]);
        if(trail<0x40 || trail>0xfe || trail==0x7f)throw std::runtime_error("invalid CP936 trail");
        const auto point=mapping[(lead-0x81)*190+trail-0x40-(trail>0x7f)];
        if(!point)throw std::runtime_error("unmapped CP936 pair");
        Append(output,point);
    }
    return output;
}
}
