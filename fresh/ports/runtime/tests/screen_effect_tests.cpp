#include "pal4/portable/screen_effect.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool condition) {if(!condition)throw std::runtime_error("screen effect contract failed");}
struct Missing:IScriptContextServices {
    ScriptResponse Invoke(const ScriptRequest& request) override {throw std::runtime_error("unbound game service "+std::to_string(request.call.system_id));}
};
int main() {
    try {
        Missing missing;ScreenEffectServices services(missing);
        ScriptGlobals globals{};ScriptStack stack(globals);
        stack.Compile("fade",R"(
            void Main(){giFlashOutBlack(1,true,true);global_mvar=1;giFlashInBlack(1,true);global_mvar=2;}
            void Zero(){giFlashOutWhite(0,true,true);global_mvar=3;}
            void NewGame(){giFlashOutBlack(0.1,true,true);giScriptMusicPause();global_mvar=99;}
        )");
        stack.Push("fade","void Main()",services);
        Check(stack.Tick(0)==ScriptState::suspended && globals[0]==0 && services.State().phase==0);
        Check(services.Advance(.5f).empty() && services.State().alpha==127.5f);
        Check(stack.Tick(.5)==ScriptState::suspended && globals[0]==0);
        const auto completed=services.Advance(.5f);
        Check(completed.size()==1 && services.State().alpha==255 && services.State().active==1);
        Check(stack.Complete(completed[0]) && stack.Tick(0)==ScriptState::suspended && globals[0]==1);
        Check(services.State().alpha==255 && services.State().phase==0);
        Check(services.Advance(.25f).empty() && services.State().alpha==191.25f);
        const auto in=services.Advance(1);
        Check(in.size()==1 && services.State().alpha==0 && services.State().active==0 && stack.Complete(in[0]));
        Check(stack.Tick(0)==ScriptState::finished && globals[0]==2);stack.PopFinished();
        stack.Push("fade","void Zero()",services);
        Check(stack.Tick(0)==ScriptState::suspended && globals[0]==2 && services.State().alpha==255);
        Check(services.State().red==255 && services.State().green==255);
        const auto zero=services.Advance(0);Check(zero.size()==1 && stack.Complete(zero[0]));
        Check(stack.Tick(0)==ScriptState::finished && globals[0]==3);stack.PopFinished();
        services.Reset();stack.Push("fade","void NewGame()",services);
        Check(stack.Tick(0)==ScriptState::suspended && services.Advance(.05f).empty());
        const auto first=services.Advance(.05f);Check(first.size()==1 && stack.Complete(first[0]));
        Check(stack.Tick(0)==ScriptState::failed && globals[0]==3 && stack.Error()=="unbound game service 237");
        Check(services.State().active==1 && services.State().alpha==255);stack.Clear();services.Reset();
        const auto request=[](int id,std::vector<ScriptValue> args,std::uint64_t sequence) {
            return std::pair{ScriptCall{id,0,std::move(args)},ScriptToken{77,sequence}};
        };
        auto red=request(295,{1.f,true,false},1);
        Check(!services.Invoke({red.second,red.first}).suspend && services.State().red==229 && services.State().green==10);
        auto ignored=request(293,{.25f,false,true},2);
        Check(!services.Invoke({ignored.second,ignored.first}).suspend && services.State().red==229);
        Check(services.Advance(1).empty());
        auto red_in=request(296,{1.f,true,true},3);
        Check(services.Invoke({red_in.second,red_in.first}).suspend);
        Check(services.State().keep_active==0);
        Check(services.Advance(.5f).empty() && services.State().alpha==127.5f);
        services.Reset();Check(services.Advance(100).empty() && !services.State().active);
        bool invalid=false;try{services.Advance(std::numeric_limits<float>::infinity());}catch(const std::invalid_argument&){invalid=true;}Check(invalid);
        auto nan=request(291,{std::numeric_limits<float>::quiet_NaN(),true,true},4);
        invalid=false;try{services.Invoke({nan.second,nan.first});}catch(const std::invalid_argument&){invalid=true;}Check(invalid);
        std::cout<<"screen fade interpolation, hold, zero-time yield, blocking completion, busy gate and actual NewGame boundary passed\n";
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
