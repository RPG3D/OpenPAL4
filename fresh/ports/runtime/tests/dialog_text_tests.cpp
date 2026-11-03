#include "pal4/portable/dialog_text.h"
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool v){if(!v)throw std::runtime_error("dialog rich text contract failed");}
template<class F>void Reject(F f){bool rejected=false;try{f();}catch(const std::runtime_error&){rejected=true;}Check(rejected);}
int main(){try{
    constexpr unsigned normal=0xcc336699;const auto plain=ParseDialogText("云天河：好！",normal);Check(plain.characters==6 && plain.runs.size()==1 && plain.runs[0].end==6 && plain.runs[0].color==normal);
    const auto mixed=ParseDialogText("A<colour red=255 green=187 blue=0 alpha=255>B</colour><dc0>C</dc0>",normal);
    Check(mixed.text=="ABC" && mixed.characters==3 && mixed.runs.size()==3 && mixed.runs[0].color==normal && mixed.runs[1].color==0xffffbb00 && mixed.runs[2].color==normal);
    for(unsigned i=0;i<4;++i){const auto text=ParseDialogText("<dc"+std::to_string(i)+">A</dc"+std::to_string(i)+">",normal);const unsigned colours[]{normal,0xffff0000,0xff00ff00,0xff0000ff};Check(text.text=="A" && text.runs[0].color==colours[i]);}
    Check(ParseDialogText("<dc0> A\n  B &amp; C </dc0>",normal).text==" A\n  B & C ");
    Check(ParseDialogText("<dc0><df2>hut</df2></dc0>",normal,true).text=="hut");
    Reject([&]{ParseDialogText("<df2>hut</df2>",normal);});
    const auto same=ParseDialogText("<dc0>你好<dc0>你 A</dc0></dc0>",normal);
    Check(same.runs.size()==1 && same.nodes.size()==2 && same.nodes[0].begin==0 && same.nodes[0].end==2 && same.nodes[1].begin==2 && same.nodes[1].end==5);
    Check(plain.nodes.size()==1 && plain.nodes[0].end==plain.characters);
    for(const char* bad:{"<dc0>A","<unknown>A</unknown>","<colour red=256 green=0 blue=0 alpha=255>A</colour>","<dc0 foo=1>A</dc0>","<dc0></dc0>"})Reject([&]{ParseDialogText(bad,normal);});
    std::cout<<"dialog raw-preserving colour presets, visible text, whitespace, XML and ranges passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
