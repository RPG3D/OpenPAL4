#include "pal4/native_combat_system_tail.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void Check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void Reject(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}Check(caught,"expected rejection");}
}
int main(){try{
    std::array<std::uint32_t,20> data;data.fill(0xa5a5a5a5u);
    auto storage=std::span<std::uint32_t,18>(data.data()+1u,18u);
    const auto tail=reinterpret_cast<std::uint32_t>(storage.data());
    using Owner=pal4::NativeCombatSystemTail;
    Owner owner(storage);
    Check(owner.Address()==tail && storage[0]==0u && storage[1]==0u && storage[2]==0u && storage[3]==0u && storage[4]==1u,
          "original leading scalar defaults");
    auto* bytes=reinterpret_cast<const std::uint8_t*>(storage.data());
    Check(bytes[68]==1u && bytes[69]==0u && bytes[70]==0u && bytes[71]==0xa5u,"original tail flags and untouched padding");
    Check(data.front()==0xa5a5a5a5u && data.back()==0xa5a5a5a5u,"tail range preserves adjacent fields");
    Reject([&]{Owner duplicate(storage);});
    for(unsigned i=0;i<3u;++i){
        const auto record=tail+20u+i*16u;
        Check(owner.Text(record).empty() && !storage[6u+i*4u] && !storage[7u+i*4u],"initial empty host view");
        const std::string text(1000u+i*57u,static_cast<char>('a'+i));
        owner.SetText(record,text);
        Check(owner.Text(record)==text && storage[7u+i*4u]==text.size(),"host growth publishes actual length");
        Check(std::string(reinterpret_cast<const char*>(storage[6u+i*4u]))==text,"native read-only C string view");
        owner.SetText(record,owner.Text(record).substr(2u,40u));
        Check(owner.Text(record)==std::string(40u,static_cast<char>('a'+i)),"aliasing source assignment");
    }
    Reject([&]{owner.SetText(tail+24u,"bad");});
    const auto before_reset=storage[6];storage[6]=42u;Reject([&]{owner.Reset();});storage[6]=before_reset;
    storage[0]=9u;storage[4]=99u;
    owner.Reset();owner.Reset();
    Check(!storage[0] && storage[4]==1u && bytes[68]==1u && !bytes[69] && !bytes[70] && bytes[71]==0xa5u,"EndCombat reset reuses original scalar/byte defaults without touching padding");
    for(unsigned i=0;i<3u;++i)Check(owner.Text(tail+20u+i*16u).empty() && !storage[6u+i*4u] && !storage[7u+i*4u],"EndCombat reset clears all three previously nonempty host strings");
    Check(data.front()==0xa5a5a5a5u && data.back()==0xa5a5a5a5u,"EndCombat reset preserves adjacent result/system fields");
    owner.SetText(tail+20u,"after-reset");
    const auto saved=storage[6];storage[6]=42u;Reject([&]{owner.Text(tail+20u);});Reject([&]{owner.Close();});storage[6]=saved;
    owner.Close();owner.Close();Check(owner.Closed(),"explicit close idempotent");
    for(unsigned i=0;i<3u;++i)Check(!storage[6u+i*4u] && !storage[7u+i*4u],"close clears borrowed views");
    Reject([&]{owner.Text(tail+20u);});
    {
        auto old=std::make_unique<Owner>(storage);old->Close();
        Owner replacement(storage);
        // Destroying an already closed owner must not release the new claim.
        old.reset();Reject([&]{Owner duplicate(storage);});
        replacement.SetText(tail+20u,"pending-at-destruction");
    }
    Check(!storage[6] && !storage[7],"destructor clears pending text view");
    std::cout<<"PASS: Combat tail native defaults, three host texts, growth, aliases and retirement\n";
    return EXIT_SUCCESS;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}}
