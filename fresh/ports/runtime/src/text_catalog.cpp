#include "pal4/portable/text_catalog.h"
#include "pal4/portable/gbk.h"
#include <istream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
unsigned Word(std::istream& input){unsigned char b[4]{};Require(bool(input.read(reinterpret_cast<char*>(b),4)),"truncated text catalog");return b[0]|(unsigned(b[1])<<8)|(unsigned(b[2])<<16)|(unsigned(b[3])<<24);}
}
TextCatalog ReadTextCatalog(std::istream& input,std::string_view magic){
    char signature[4]{};Require(magic.size()==4 && bool(input.read(signature,4)) && std::string_view(signature,4)==magic && Word(input)==1,"unsupported text catalog");
    const auto count=Word(input);Require(count>0 && count<=4096,"invalid text catalog count");
    TextCatalog result;unsigned total{};
    for(unsigned i=0;i<count;++i){const auto key=Word(input),size=Word(input);Require(size>0 && size<=256*1024 && total<=8*1024*1024-size,"invalid catalog text size");total+=size;
        std::string raw(size,'\0');Require(bool(input.read(raw.data(),size)) && raw.find('\0')==std::string::npos,"invalid catalog text");
        Require(result.emplace(key,DecodeGbk(raw)).second,"duplicate text key");
    }
    Require(input.peek()==std::char_traits<char>::eof(),"trailing text catalog bytes");return result;
}
}
