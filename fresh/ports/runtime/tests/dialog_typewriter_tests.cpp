#include "pal4/portable/dialog_typewriter.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("dialog typing contract failed");}
template<class F>void Reject(F action){bool failed=false;try{action();}catch(const std::invalid_argument&){failed=true;}Check(failed);}
void Trace(std::span<const DialogRevealNode> nodes,std::uint32_t end,std::span<const std::array<unsigned,4>> expected){
    DialogTypewriter writer;writer.Configure(nodes,0,end);
    for(std::size_t i=0;i<expected.size();++i){
        Check(writer.Advance(.11f,.1f));const auto cursor=writer.Cursor();const auto& row=expected[i];
        Check(writer.Shown()==row[0] && cursor.line==row[1] && cursor.node==row[2] && cursor.character==row[3] && cursor.finished==(i+1==expected.size()));
    }
    Check(!writer.Advance(1,.1f));
}
}
int main(){try{
    const DialogRevealNode plain[]{{0,3,0},{3,5,1}};
    const std::array<unsigned,4> plain_trace[]{{1,0,0,1},{2,0,0,2},{3,0,0,3},{3,0,1,0},{3,1,0,0},{4,1,0,1},{5,1,0,2},{5,1,1,0},{5,2,0,0}};
    Trace(plain,5,plain_trace);
    const DialogRevealNode rich[]{{0,2,0},{2,3,0},{3,5,1}};
    const std::array<unsigned,4> rich_trace[]{{1,0,0,1},{2,0,0,2},{2,0,1,0},{3,0,1,1},{3,0,2,0},{3,1,0,0},{4,1,0,1},{5,1,0,2},{5,1,1,0},{5,2,0,0}};
    Trace(rich,5,rich_trace);
    const DialogRevealNode lf[]{{0,2,0},{2,3,0,true},{3,5,1}};
    const std::array<unsigned,4> lf_trace[]{{1,0,0,1},{2,0,0,2},{2,0,1,0},{3,0,2,0},{3,1,0,0},{4,1,0,1},{5,1,0,2},{5,1,1,0},{5,2,0,0}};
    Trace(lf,5,lf_trace);
    const DialogRevealNode automatic[]{{0,2,0},{2,2,0,true},{2,3,1}};
    const std::array<unsigned,4> automatic_trace[]{{1,0,0,1},{2,0,0,2},{2,0,1,0},{2,0,2,0},{2,1,0,0},{3,1,0,1},{3,1,1,0},{3,2,0,0}};
    Trace(automatic,3,automatic_trace);
    DialogTypewriter automatic_writer;automatic_writer.Configure(automatic,0,3);automatic_writer.FinishLine();
    Check(automatic_writer.Shown()==2 && automatic_writer.Cursor().line==1 && !automatic_writer.Cursor().finished);
    const DialogRevealNode trailing[]{{0,1,0},{1,2,0,true},{2,2,1}};
    const std::array<unsigned,4> trailing_trace[]{{1,0,0,1},{1,0,1,0},{2,0,2,0},{2,1,0,0},{2,1,1,0},{2,2,0,0}};
    Trace(trailing,2,trailing_trace);
    const DialogRevealNode leading[]{{0,0,0},{0,1,0,true},{1,2,1}};
    const std::array<unsigned,4> leading_trace[]{{0,0,1,0},{1,0,2,0},{1,1,0,0},{2,1,0,1},{2,1,1,0},{2,2,0,0}};
    Trace(leading,2,leading_trace);
    const DialogRevealNode blank[]{{0,1,0},{1,2,0,true},{2,2,1},{2,3,1,true},{3,4,2}};
    const std::array<unsigned,4> blank_trace[]{{1,0,0,1},{1,0,1,0},{2,0,2,0},{2,1,0,0},{2,1,1,0},{3,1,2,0},{3,2,0,0},{4,2,0,1},{4,2,1,0},{4,3,0,0}};
    Trace(blank,4,blank_trace);
    DialogTypewriter writer;writer.Configure(plain,0,5);
    Check(!writer.Advance(.05f,.1f) && !writer.Advance(.05f,.1f) && writer.Advance(.1f,.1f) && writer.Shown()==1);
    Check(!writer.Advance(.1f,.1f) && writer.Advance(1,.1f) && writer.Shown()==2);
    const DialogRevealNode bad[]{{0,2,0},{3,5,1}};
    Reject([&]{writer.Configure(bad,0,5);});Check(writer.Shown()==2);
    Reject([&]{writer.Advance(-1,.1f);});Reject([&]{writer.Advance(std::numeric_limits<float>::infinity(),.1f);});
    Reject([&]{writer.Advance(0,0);});writer.Finish();Check(writer.Shown()==5 && writer.Cursor().finished && !writer.Advance(1,.1f));
    const DialogRevealNode page[]{{5,7,0}};writer.Configure(page,5,7);Check(writer.Shown()==5 && !writer.Cursor().finished);writer.Advance(.11f,.1f);Check(writer.Shown()==6);
    writer.Configure(lf,0,5);Check(!writer.Advance(.05f,.1f));writer.FinishLine();
    Check(writer.Shown()==3 && writer.Cursor().line==1 && !writer.Cursor().finished);
    Check(writer.Advance(.06f,.1f) && writer.Shown()==4); // finish-line preserves the elapsed remainder.
    writer.FinishLine();Check(writer.Shown()==5 && writer.Cursor().line==2 && writer.Cursor().finished);
    writer.FinishLine();Check(writer.Shown()==5 && writer.Cursor().line==2);
    std::cout<<"dialog typing PC node/line/LF/empty traces, strict timer, one-step, finish, next page and invalid input passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
