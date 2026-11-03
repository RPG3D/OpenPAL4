#include "pal4/comparison_flee_task_trace.h"
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main(){try{
    pal4::ComparisonFleeTaskTrace disabled(false,true);
    Check(!disabled.Begin(0u,0.f,[](auto)->std::uint32_t{throw std::runtime_error("disabled read");}),
        "disabled observer never reads target memory");
    std::map<std::uint32_t,std::uint32_t> memory{{108u,1u},{112u,0u},{104u,200u},{200u,300u},{300u,200u},{308u,400u}};
    for(unsigned i=0;i<15u;++i)memory[400u+4u*i]=i;
    memory[400u]=0x845740u;
    const auto base_only=[&](auto a){return memory.at(a);};
    pal4::ComparisonFleeTaskTrace ordinary(true);
    Check(!ordinary.Begin(100u,.017f,base_only),"ordinary flee observation does not read beyond Run base or capture approach");
    for(unsigned i=0;i<7u;++i)memory[460u+4u*i]=0xff000000u+i;
    pal4::ComparisonFleeTaskTrace approach(true,true);
    const auto before=pal4::GetComparisonFrameGate().Observe();
    auto* record=approach.Begin(100u,.017f,base_only);
    Check(record && record->before.tasks[0].run_sampled && record->before.tasks[0].run_words[6]==0xff000006u,
        "actual88-byte Run task exposes exact target, actor and direction words");
    memory[440u]=4u;approach.End(record,base_only);
    const auto after=pal4::GetComparisonFrameGate().Observe();
    Check(record->completed && record->after.tasks[0].words[10]==4u &&
          record->outer_completed==before.completed && before.completed==after.completed && before.callbacks==after.callbacks,
          "observation preserves before/after phase and never advances frame state");
    std::ostringstream json;approach.Write(json);
    Check(json.str().find("\"run_sampled\":true")!=std::string::npos &&
          json.str().find("\"outer_completed\":")!=std::string::npos,"explicit publication includes raw extension and frame metadata");
    for(unsigned i=0;i<7u;++i)memory.erase(460u+4u*i);
    memory[400u]=0x845760u;
    auto* face=approach.Begin(100u,.017f,base_only);
    Check(face && !face->before.tasks[0].run_sampled,"Face observation never reads the larger Run-only extension");
    approach.End(face,base_only);
    for(unsigned i=2u;i<3000u;++i){
        auto* next=approach.Begin(100u,.017f,base_only);
        Check(next!=nullptr,"multi-command battle retains records beyond the old2048 bound");
        approach.End(next,base_only);
    }
    std::ostringstream extended;approach.Write(extended);
    Check(extended.str().find("\"capacity\":16384,\"dropped\":0")!=std::string::npos,
          "extended approach trace publishes its actual bounded capacity without drops");
    std::cout<<json.str()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
