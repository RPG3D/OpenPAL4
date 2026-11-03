#include "pal4/portable/gbk.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using pal4::portable::DecodeGbk;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("CP936 contract failed");}
void Reject(std::string_view value){bool failed=false;try{(void)DecodeGbk(value);}catch(const std::exception&){failed=true;}Check(failed);}
}
int main(int argc,char** argv){try {
    if(argc==3){std::ifstream in(argv[1],std::ios::binary);std::string text{std::istreambuf_iterator<char>(in),{}};std::ofstream out(argv[2],std::ios::binary);const auto decoded=DecodeGbk(text);out.write(decoded.data(),static_cast<std::streamsize>(decoded.size()));Check(bool(in) && bool(out));return 0;}
    Check(DecodeGbk("")=="" && DecodeGbk("Q01_N01_init")=="Q01_N01_init");
    Check(DecodeGbk("\xd4\xc6\xcc\xec\xba\xd3")=="\xe4\xba\x91\xe5\xa4\xa9\xe6\xb2\xb3");
    Check(DecodeGbk("\x80")=="\xe2\x82\xac");Check(DecodeGbk(std::string("A\0B",3))==std::string("A\0B",3));
    Reject("\x81");Reject("\xff");Reject("\x81\x7f");Reject("\x81\x30");Reject("\xfe\xfe");
    std::cout<<"CP936 ASCII, Chinese, euro, embedded NUL, malformed and unmapped sequences passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
