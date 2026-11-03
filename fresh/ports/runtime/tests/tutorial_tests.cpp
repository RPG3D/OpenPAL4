#include "pal4/portable/tutorial.h"
#include "pal4/portable/tutorial_assets.h"
#include "pal4/portable/package.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool v){if(!v)throw std::runtime_error("tutorial contract failed");}
struct Next:IScriptContextServices{unsigned calls{};ScriptResponse Invoke(const ScriptRequest&)override{++calls;return {};}};
void Word(std::string& b,unsigned v){for(unsigned n=0;n<32;n+=8)b+=static_cast<char>(v>>n);}
std::string Fixture(){std::string b="P4TU";Word(b,1);Word(b,1);Word(b,10);Word(b,4);b+="text";return b;}
int main(int argc,char** argv){try{
    if(argc!=1 && argc!=3 && !(argc==5 && std::string_view(argv[1])=="presentation"))throw std::runtime_error("invalid tutorial probe arguments");
    if(argc==5 && std::string_view(argv[1])=="presentation"){
        std::ifstream input(argv[2],std::ios::binary);const auto catalog=ReadTutorialCatalog(input);std::ifstream archive(argv[3],std::ios::binary);PackageReader package(archive);
        DialogAssets assets([&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{std::string name(member);std::replace(name.begin(),name.end(),'/','\\');if(!package.Find(name))return {};return package.Read(name);});
        const auto view=PrepareTutorialPresentation(assets,catalog.at(10));Check(view.images.size()==2 && view.patches.size()==4 && view.font=="fonts/simsun.ttc" && view.font_size==13);
        Check(view.text_rect[0]>100 && view.text_rect[0]<120 && view.text_rect[2]-view.text_rect[0]>500 && view.text_rect[3]-view.text_rect[1]>300 && view.button_rect[1]>400);
        Check(view.text.find("<image")==std::string::npos && view.text.find("<dc0>")==std::string::npos && view.images[0].image.member=="imagesets/maimai0.png");
        for(const auto& member:TutorialResourceMembers(view))Check(!assets.Read(member).empty());
        for(const auto& [key,text]:catalog){(void)key;(void)PrepareTutorialPresentation(assets,text);}
        for(const auto* bad:{"<unknown/>","<image set=../bad name=x/>","<image set=maimai0 name=missing/>","<image set=maimai0/>"}){bool failed=false;try{PrepareTutorialPresentation(assets,bad);}catch(const std::exception&){failed=true;}Check(failed);}
        std::ofstream output(argv[4]);WriteTutorialPresentation(output,view);return 0;
    }
    if(argc==3){std::ifstream input(argv[1],std::ios::binary);const auto catalog=ReadTutorialCatalog(input);const auto& source=catalog.at(10);Check(source.find("<image set=maimai0 name=ma_zou2/>")!=std::string::npos && source.find("<dc0>")!=std::string::npos && source.find('\n')!=std::string::npos);
        Next next;unsigned saves{};TutorialServices tutorial(next,catalog,7,true,[&](unsigned key){Check(key==10);++saves;});ScriptCall call{309,10100,{std::int32_t(10)}};Check(tutorial.Invoke({{1,175},call}).suspend);Check(tutorial.View().visible && tutorial.View().source==source && saves==1);Check(!tutorial.Advance());Check(tutorial.Close(7,tutorial.View().id));Check(tutorial.Advance()==std::optional(ScriptToken{1,175}));
        std::ofstream report(argv[2]);report<<"{\"passed\":true,\"records\":"<<catalog.size()<<",\"key\":10,\"original_markup_preserved\":true,\"waits_for_explicit_close\":true,\"text_bytes\":"<<source.size()<<"}";return 0;
    }
    const auto bytes=Fixture();std::istringstream input(bytes);Check(ReadTutorialCatalog(input).at(10)=="text");
    for(std::size_t n=0;n<bytes.size();++n){bool failed=false;try{std::istringstream in(bytes.substr(0,n));ReadTutorialCatalog(in);}catch(const std::exception&){failed=true;}Check(failed);}
    {auto b=bytes;b[8]=2;b+=bytes.substr(12);bool failed=false;try{std::istringstream in(b);ReadTutorialCatalog(in);}catch(const std::exception&){failed=true;}Check(failed);}
    {bool failed=false;try{std::istringstream in(bytes+"x");ReadTutorialCatalog(in);}catch(const std::exception&){failed=true;}Check(failed);}
    const TutorialCatalog catalog{{10,"text<image set=maimai0 name=ma_zou2/>"},{1,"other"},{0xffffffffu,"wide"}};Next next;unsigned saves{};bool saved_before_visible=false;TutorialServices* owner=nullptr;
    TutorialServices tutorial(next,catalog,9,true,[&](unsigned){++saves;saved_before_visible=!owner->View().visible;});owner=&tutorial;
    ScriptCall call{309,0,{std::int32_t(10)}};const ScriptToken token{4,8};Check(tutorial.Invoke({token,call}).suspend);auto view=tutorial.View();Check(view.visible && view.waiting && view.key==10 && saved_before_visible && saves==1);
    for(unsigned i=0;i<3;++i)Check(!tutorial.Advance());Check(!tutorial.Close(8,view.id) && !tutorial.Close(9,view.id+1));Check(tutorial.Close(9,view.id) && !tutorial.Close(9,view.id));Check(tutorial.Advance()==std::optional(token) && !tutorial.Advance());
    Check(tutorial.Invoke({{4,9},call}).suspend);Check(!tutorial.View().visible && saves==1);Check(tutorial.Advance()==std::optional(ScriptToken{4,9}));
    call.arguments[0]=std::int32_t(-1);Check(tutorial.Invoke({{4,10},call}).suspend);Check(tutorial.View().visible && tutorial.View().key==0xffffffffu && saves==1);tutorial.Cancel();Check(!tutorial.Advance() && !tutorial.View().visible);
    TutorialServices disabled(next,catalog,10,false,[](unsigned){throw std::runtime_error("disabled tutorial must not save");});call.arguments[0]=std::int32_t(10);Check(disabled.Invoke({token,call}).suspend);Check(!disabled.View().visible && disabled.Advance()==std::optional(token));
    ScriptGlobals globals{};ScriptStack stack(globals);TutorialServices script(next,catalog,11,true,[](unsigned){});stack.Compile("tutorial","void Probe(){giShowTutorial(10);giShowTutorial(1);}");Check(stack.Push("tutorial","void Probe()",script).has_value());
    Check(stack.Tick(0)==ScriptState::suspended && script.View().key==10);Check(!script.Advance());Check(script.Close(11,script.View().id));const auto first=script.Advance();Check(first && stack.Complete(*first));Check(stack.Tick(0)==ScriptState::suspended && script.View().key==1);Check(script.Close(11,script.View().id));const auto second=script.Advance();Check(second && *second!=*first && stack.Complete(*second));Check(stack.Tick(0)==ScriptState::finished);
    std::cout<<"tutorial cache, original gates and script continuation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
