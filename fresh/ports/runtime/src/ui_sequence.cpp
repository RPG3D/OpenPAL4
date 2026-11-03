#include "pal4/portable/ui_sequence.h"
#include <tinyxml.h>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
std::string Attribute(const TiXmlElement& node,const char* name){const auto* value=node.Attribute(name);Require(value && *value,"missing UI sequence attribute");return value;}
std::uint32_t Number(const std::string& value){std::uint32_t number{};const auto result=std::from_chars(value.data(),value.data()+value.size(),number);Require(result.ec==std::errc{} && result.ptr==value.data()+value.size(),"invalid UI sequence integer");return number;}
float Duration(const std::string& value){char* end{};const float number=std::strtof(value.c_str(),&end);Require(end==value.c_str()+value.size() && std::isfinite(number) && number>0 && number<=3600,"invalid UI sequence duration");return number;}
void Name(const std::string& value){Require(!value.empty() && value.size()<=255 && value.find('\0')==std::string::npos,"invalid UI sequence name");}
}
UiSequenceDefinition ParseUiSequence(std::string_view xml){
    Require(!xml.empty() && xml.size()<=65536 && xml.find('\0')==std::string_view::npos,"invalid UI sequence XML size");
    const std::string source(xml);TiXmlDocument document;document.Parse(source.c_str(),nullptr,TIXML_ENCODING_UTF8);
    const auto* root=document.RootElement();Require(!document.Error() && root && std::string_view(root->Value())=="SequenceImage","invalid UI sequence XML");
    UiSequenceDefinition result;result.name=Attribute(*root,"Name");Name(result.name);result.loop_count=Number(Attribute(*root,"LoopCount"));
    const auto count=Number(Attribute(*root,"FrameCount"));Require(count>0 && count<=4096,"invalid UI sequence frame count");result.frames.resize(count);std::vector<bool> seen(count);
    for(const auto* a=root->FirstAttribute();a;a=a->Next()){const std::string_view key=a->Name();Require(key=="Name" || key=="LoopCount" || key=="FrameCount","unsupported UI sequence attribute");}
    for(const auto* node=root->FirstChildElement();node;node=node->NextSiblingElement()){
        Require(std::string_view(node->Value())=="ImageFrame" && !node->FirstChildElement(),"unsupported UI sequence element");
        const auto index=Number(Attribute(*node,"Index"));Require(index<count && !seen[index],"invalid UI sequence frame index");seen[index]=true;
        auto& frame=result.frames[index];frame.duration=Duration(Attribute(*node,"Duration"));frame.imageset=Attribute(*node,"Imageset");frame.image=Attribute(*node,"Image");Name(frame.imageset);Name(frame.image);
        for(const auto* a=node->FirstAttribute();a;a=a->Next()){const std::string_view key=a->Name();Require(key=="Index" || key=="Duration" || key=="Imageset" || key=="Image","unsupported UI sequence frame attribute");}
    }
    for(bool present:seen){Require(present,"incomplete UI sequence frames");}
    return result;
}
UiSequencePlayback::UiSequencePlayback(UiSequenceDefinition definition):definition_(std::move(definition)){
    Name(definition_.name);Require(!definition_.frames.empty() && definition_.frames.size()<=4096,"invalid UI sequence frames");
    for(const auto& frame:definition_.frames){Require(std::isfinite(frame.duration) && frame.duration>0 && frame.duration<=3600,"invalid UI sequence duration");Name(frame.imageset);Name(frame.image);duration_+=frame.duration;}
    Require(std::isfinite(duration_) && duration_>0,"invalid UI sequence total duration");
}
std::size_t UiSequencePlayback::Advance(float seconds){
    Require(std::isfinite(seconds) && seconds>=0,"invalid UI sequence elapsed time");if(Finished())return frame_;
    float time=elapsed_+seconds;Require(std::isfinite(time),"UI sequence time overflow");
    if(time>=duration_){
        if(definition_.loop_count && ++loops_>=definition_.loop_count){elapsed_=duration_;return frame_;}
        time-=duration_; // 原版每次只减一个周期，大时间步的剩余时间留给后续更新。
    }
    elapsed_=time;frame_=0;float stop=0;
    for(std::size_t i=0;i<definition_.frames.size();++i){stop+=definition_.frames[i].duration;if(time<stop){frame_=i;break;}}
    return frame_;
}
}
