#include "pal4/native_combat_system_access.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
namespace {
void Check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void Reject(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}Check(caught,"expected rejection");}
}
int main(){try{
    Reject([]{pal4::NativeCombatSystemAccess bad(0u,[]{});});
    Reject([]{pal4::NativeCombatSystemAccess bad(123u,{});});
    bool ready=false;unsigned checks{};pal4::NativeCombatSystemAccess* active{};
    pal4::NativeCombatSystemAccess owner(123u,[&]{
        ++checks;if(!ready)throw std::logic_error("unconstructed dependency");
        Reject([&]{active->Acquire();});Reject([&]{active->Close();});
    });active=&owner;
    Reject([&]{owner.Acquire();});Check(!owner.RegistrationCount(),"preflight failure does not register partial object");
    ready=true;Check(owner.Acquire()==123u && owner.Acquire()==123u && owner.RegistrationCount()==1u,"shared singleton registers once");
    ready=false;Reject([&]{owner.Acquire();});Check(checks==4u,"cached acquisition still checks borrowed lifetime");
    owner.Close();owner.Close();Check(!owner.RegistrationCount(),"native registration retired");Reject([&]{owner.Acquire();});
    std::cout<<"PASS: Combat shared singleton guard, native registration, preflight/reentry and retirement\n";
    return EXIT_SUCCESS;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}}
