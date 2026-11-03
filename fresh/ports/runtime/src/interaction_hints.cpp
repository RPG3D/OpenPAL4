#include "pal4/portable/interaction_hints.h"
#include <stdexcept>
#include <cmath>
namespace pal4::portable {
InteractionHints::InteractionHints(std::uint64_t generation,Lookup lookup,Play play)
    :lookup_(std::move(lookup)),play_(std::move(play)){
    if(!lookup_ || !play_)throw std::invalid_argument("interaction hint services are missing");
    view_.generation=generation;view_.id=std::uint64_t{1}<<48;view_.revision=view_.id;
}
void InteractionHints::Advance(float seconds){
    if(!std::isfinite(seconds) || seconds<0)throw std::invalid_argument("invalid hint elapsed time");
    if(view_.visible){elapsed_+=seconds;if(elapsed_>=view_.duration)(void)Close(view_.generation,view_.id);}
}
bool InteractionHints::Close(std::uint64_t generation,std::uint64_t id){
    if(generation!=view_.generation || id!=view_.id || !view_.visible)return false;
    view_.visible=false;++view_.revision;return true;
}
void InteractionHints::ShowMessage(std::string_view text,float x,float y,float duration){
    if(view_.visible)(void)Close(view_.generation,view_.id);
    InitializeHintWindow(1,text,x,y,duration,0,1);
}
std::uint32_t InteractionHints::CreateHintWindow(){return view_.visible?0:1;}
std::optional<std::string> InteractionHints::TriggerText(std::uint32_t id){
    if(id!=0x109 && id!=0x10a)throw std::invalid_argument("unsupported interaction hint key");
    const auto text=lookup_(id);if(!text)throw std::runtime_error("original GameString hint is missing");return text;
}
std::string InteractionHints::ConvertToUtf8(std::string_view text){return std::string(text);}
void InteractionHints::InitializeHintWindow(std::uint32_t window,std::string_view text,float x,float y,float duration,std::uint32_t zero,std::uint32_t one){
    if(window!=1 || view_.visible || x!=.5f || y!=.15f || duration!=1 || zero!=0 || one!=1)throw std::logic_error("interaction hint window binding mismatch");
    ++view_.id;++view_.revision;view_.text=text;view_.x=x;view_.y=y;view_.duration=duration;elapsed_=0;view_.visible=true;
}
void InteractionHints::PlayHintSound(std::string_view path,std::uint32_t repeat){
    if(path!=pal4::PalActorControlGameEvent::kSoundPath || repeat!=1)throw std::logic_error("interaction hint sound mismatch");
    play_("WA05",repeat);
}
}
