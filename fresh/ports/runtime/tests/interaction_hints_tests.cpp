#include "pal4/portable/interaction_hints.h"
#include "pal4/portable/text_catalog.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include "pal4/portable/tutorial_assets.h"
#include "pal4/portable/package.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("interaction hint contract failed");}
int main(int argc,char** argv){try{
    TextCatalog catalog{{265,"first hint"},{266,"second hint"}};
    if(argc>=2){std::ifstream input(argv[1],std::ios::binary);catalog=ReadTextCatalog(input,"P4GS");Check(catalog.size()==2 && catalog.contains(265) && catalog.contains(266));}
    if(argc>=3){
        std::ifstream archive(argv[2],std::ios::binary);PackageReader package(archive);
        DialogAssets assets([&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{std::string name(member);std::replace(name.begin(),name.end(),'/','\\');if(!package.Find(name))return {};return package.Read(name);});
        for(const auto& [key,text]:catalog){(void)key;auto view=PrepareHintPresentation(assets,text);
            Check(view.text==text && view.button.member.empty() && view.patches.size()==9 && view.font=="fonts/SystemBold.ttf" && view.font_size==13);
            Check(view.text_rect[0]>300 && view.text_rect[2]<500 && view.text_rect[1]>60 && view.text_rect[3]<120);
            for(const auto& member:TutorialResourceMembers(view))Check(!member.empty() && !assets.Read(member).empty());
            ResizeHintPresentation(view,78,16);Check(view.hint_sized && view.text_rect[2]-view.text_rect[0]==85 && view.text_rect[3]-view.text_rect[1]==17);
            Check(std::abs((view.patches[0].rect[0]+view.patches[8].rect[2])*.5f-400)<.001f);
            if(argc==4 && key==265){std::ofstream report(argv[3]);WriteTutorialPresentation(report,view);}
        }
    }
    unsigned sounds{};InteractionHints hints(7,[&](auto key)->std::optional<std::string>{const auto found=catalog.find(key);return found==catalog.end()?std::nullopt:std::optional(found->second);},
        [&](auto name,auto repeats){Check(name=="WA05" && repeats==1);++sounds;});
    // 通过原提示处理器验证门控和窗口，不在便携服务内复制角色判断。
    struct Runtime final:pal4::IActorControlGameEventRuntime {
        InteractionHints& hints;int code{1};explicit Runtime(InteractionHints& h):hints(h){}
        std::int32_t EventCode(std::uint32_t) override{return code;}
        std::uint32_t CreateHintWindow() override{return hints.CreateHintWindow();}
        std::optional<std::string> TriggerText(std::uint32_t id) override{return hints.TriggerText(id);}
        std::string ConvertToUtf8(std::string_view text) override{return hints.ConvertToUtf8(text);}
        void InitializeHintWindow(std::uint32_t w,std::string_view text,float x,float y,float a,std::uint32_t z,std::uint32_t o) override{hints.InitializeHintWindow(w,text,x,y,a,z,o);}
        void PlayHintSound(std::string_view path,std::uint32_t repeats) override{hints.PlayHintSound(path,repeats);}
    } runtime(hints);
    pal4::PalActorControlState actor;
    Check(!pal4::PalActorControlGameEvent::Process(actor,1,runtime));auto first=hints.View();
    Check(first.visible && first.text==catalog.at(265) && first.x==.5f && first.y==.15f && sounds==1);
    Check(!pal4::PalActorControlGameEvent::Process(actor,1,runtime) && hints.View().id==first.id && sounds==1);
    Check(!hints.Close(6,first.id) && !hints.Close(7,first.id+1));
    hints.Advance(.75f);Check(hints.View().visible);hints.Advance(.25f);Check(!hints.View().visible && !hints.Close(7,first.id));
    runtime.code=3;Check(!pal4::PalActorControlGameEvent::Process(actor,1,runtime));
    Check(hints.View().id>first.id && hints.View().revision>first.revision && hints.View().text==catalog.at(266) && sounds==2);
    Check(!hints.Close(7,first.id));Check(hints.Close(7,hints.View().id));
    actor.words[19]=3;Check(pal4::PalActorControlGameEvent::Process(actor,1,runtime) && !hints.View().visible && sounds==2);
    if(argc>=2){std::ifstream input(argv[1],std::ios::binary);bool rejected=false;try{(void)ReadTextCatalog(input,"P4TU");}catch(const std::exception&){rejected=true;}Check(rejected);}
    std::cout<<"hint texts, original role gates, timed lifetime and sound contracts passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
