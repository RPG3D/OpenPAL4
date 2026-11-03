#include "pal4/portable/arena_skill_state.h"
#include "pal4/portable/game_services.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace pal4::portable;
namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void Reject(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}Check(failed,"missing arena skill service was accepted");}
}
int main(){
    try{
        std::ostringstream trace;GameServices services(trace);
        const ScriptCall call{180,0,{}};const ScriptRequest request{{1,1},call};
        Reject([&]{(void)services.Invoke(request);});
        ArenaSkillState owner;unsigned cleanups{};
        services.hide_skill_object=[&]{(void)owner.HideSkillObject();++cleanups;};
        Check(!services.Invoke(request).suspend && cleanups==1,
            "giHideGASkillObject did not clean through its arena manager");
        Check(!services.Invoke(request).suspend && cleanups==2,
            "repeated arena skill cleanup failed");
        owner.Close();
        owner.Close();
        Reject([&]{(void)owner.HideSkillObject();});
        std::cout<<"giHideGASkillObject routes to the shared empty arena manager and closes its owner\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
