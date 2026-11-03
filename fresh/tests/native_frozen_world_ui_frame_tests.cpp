#include "pal4/native_frozen_world_ui_frame.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void Reject(F&& f,const char* message){
    bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}
    Check(rejected,message);
}
void Verify(){
    using pal4::NativeFrozenWorldUiFrame;
    const auto world=[](std::uint32_t){};
    const auto ui=[](std::uint32_t){return 1u;};
    Reject([&]{NativeFrozenWorldUiFrame frame(0u,2u,world,ui);},"missing arena owner");
    Reject([&]{NativeFrozenWorldUiFrame frame(1u,0u,world,ui);},"missing UI owner");
    Reject([&]{NativeFrozenWorldUiFrame frame(1u,2u,{},ui);},"missing world renderer");
    Reject([&]{NativeFrozenWorldUiFrame frame(1u,2u,world,{});},"missing UI renderer");
    std::vector<std::string> calls;
    std::uint32_t camera=0x1234u;
    const auto slot=reinterpret_cast<std::uint32_t>(&camera);
    bool world_error{},ui_error{},nested{};
    NativeFrozenWorldUiFrame* active{};
    NativeFrozenWorldUiFrame frame(1u,2u,[&](std::uint32_t value){
        Check(value==camera,"world receives borrowed camera value");calls.push_back("world");
        if(nested)Reject([&]{(void)active->Render(slot);},"reject reentrant render");
        if(world_error)throw std::runtime_error("world renderer failure");
    },[&](std::uint32_t value){
        Check(value==slot,"UI receives camera slot, not camera value");calls.push_back("ui");
        if(ui_error)throw std::runtime_error("UI renderer failure");
        return 0x7788u;
    });
    active=&frame;
    Reject([&]{(void)frame.Render(0u);},"reject null slot");
    camera=0u;Reject([&]{(void)frame.Render(slot);},"reject missing camera");
    Check(calls.empty(),"invalid camera executes no renderer");camera=0x1234u;
    Check(frame.Render(slot)==0x7788u && calls==std::vector<std::string>{"world","ui"},"shared Save renderer order/result");
    calls.clear();nested=true;
    Check(frame.Render(slot)==0x7788u && calls==std::vector<std::string>{"world","ui"},"rejected nesting preserves outer render");nested=false;
    calls.clear();world_error=true;
    Reject([&]{(void)frame.Render(slot);},"propagate world failure");
    Check(calls==std::vector<std::string>{"world"},"world failure skips UI");world_error=false;
    calls.clear();ui_error=true;
    Reject([&]{(void)frame.Render(slot);},"propagate UI failure");ui_error=false;
    calls.clear();Check(frame.Render(slot)==0x7788u && calls==std::vector<std::string>{"world","ui"},"exception releases borrowed frame scope");
    Check(camera==0x1234u,"render does not rewrite borrowed camera");
}
}
int main(){try{Verify();std::cout<<"Frozen world/UI render owner, ordering, return, nesting and exception cleanup pass\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
