#include "pal4/native_combat_world_names.h"
#include "pal4/native_combat_system_image.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void Check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void Reject(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}Check(caught,"expected rejection");}
}
int main(){try{
    pal4::NativeCombatSystemImage image;
    const auto combat=reinterpret_cast<std::uint32_t>(&image);
    image.world.tail.back()=12345u;image.translator.words[0]=67890u;
    auto& words=image.world_text_storage;
    using Owner=pal4::NativeCombatWorldNames;
    {
        std::array<std::uint32_t,8> foreign{};
        Reject([&]{Owner invalid(combat,foreign);});
        Owner owner(combat,words);Check(owner.Matches("",""),"initial empty names");
        Reject([&]{Owner duplicate(combat,words);});
        owner.Assign("CombatWorld","M01");Check(owner.Matches("CombatWorld","M01"),"original names retained");
        Check(words[2]==11u && words[6]==3u &&
            std::string(reinterpret_cast<const char*>(words[1]))=="CombatWorld" &&
            std::string(reinterpret_cast<const char*>(words[5]))=="M01","actual native name pointers/lengths");
        owner.Assign(owner.Scene(),owner.Region());Check(owner.Matches("M01","CombatWorld"),"cross-aliased assignment");
        owner.Assign(std::string(1000u,'r'),std::string(2000u,'s'));
        Check(words[2]==1000u && words[6]==2000u,"host growth republishes views");
        const auto saved=words[1];words[1]=42u;Reject([&]{owner.Close();});words[1]=saved;
        owner.Close();owner.Close();Reject([&]{owner.Assign("a","b");});
        Reject([&]{Owner duplicate(combat,words);});
    }
    for(auto w:words)Check(!w,"close clears all native views");
    {Owner restart(combat,words);restart.Assign("CombatWorld","M01");}
    for(auto w:words)Check(!w,"destructor clears active views");
    Check(image.world.tail.back()==12345u && image.translator.words[0]==67890u,"neighbor fields preserved");
    std::cout<<"PASS: actual WorldManager348/364 host names, aliases, views, guards and cleanup\n";
    return EXIT_SUCCESS;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}}
