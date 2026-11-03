#include "pal4/comparison_scene_relocation.h"
#include <iostream>
#include <sstream>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Write(const std::filesystem::path& root,unsigned id,const std::string& text){
    std::ofstream out(root/("compare-relocate-request-"+std::to_string(id)+".txt"));out<<text;
}
}
int main(){
    const std::string valid="PAL4_SCENE_RELOCATE_1\nM02 3\n-350 -690 -1350\n";
    for(const auto& text:{std::string("bad M02 3 1 2 3"),std::string("PAL4_SCENE_RELOCATE_1 ../M02 3 1 2 3"),
        std::string("PAL4_SCENE_RELOCATE_1 M02 3 nan 2 3"),std::string("PAL4_SCENE_RELOCATE_1 M02 3 1000001 2 3"),valid+"trailing"}){
        bool rejected=false;try{std::istringstream in(text);(void)pal4::ComparisonSceneRelocation::Read(in,1u);}
        catch(const std::invalid_argument&){rejected=true;}Check(rejected,"malformed relocation accepted");
    }
    const auto root=std::filesystem::path("H:/PAL4/objs/tmp")/("relocation-protocol-"+std::to_string(GetCurrentProcessId()));
    Check(std::filesystem::create_directory(root),"isolated protocol directory");
    _putenv_s("PAL4_COMPARE_STATE_DIR",root.string().c_str());
    pal4::ComparisonSceneRelocation owner("test");Write(root,1u,valid);owner.Accept(1u);
    unsigned calls{};const auto apply=[&](const std::array<float,3>& p){++calls;Check(p==std::array{-350.f,-690.f,-1350.f},"relocation argument changed");};
    owner.TryApply(false,"M02","3",apply);owner.TryApply(true,"M02","1",apply);
    Check(!calls && owner.Pending(),"locked/wrong-scene relocation applied");
    owner.Accept(2u);Check(owner.Pending(),"busy request lost pending operation");
    owner.TryApply(true,"M02","3",apply);Check(calls==1u && !owner.Pending(),"matching Scene did not apply exactly once");
    owner.Accept(1u);owner.TryApply(true,"M02","3",apply);Check(calls==1u,"duplicate request replayed");
    Write(root,3u,valid);owner.Accept(3u);bool failed=false;
    try{owner.TryApply(true,"M02","3",[](const auto&){throw std::runtime_error("intentional binding failure");});}
    catch(const std::runtime_error&){failed=true;}Check(failed && !owner.Pending(),"failed binding remained replayable");
    std::cout<<"PASS: relocation parser, scene/control gate, queue, duplicate and failure ownership\n";
}
