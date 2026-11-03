#include "pal4/portable/dialog_text.h"
#include <tinyxml.h>
#include <array>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::uint32_t Characters(std::string_view text){
    std::uint32_t result{};
    for(std::size_t i=0;i<text.size();){const auto lead=static_cast<unsigned char>(text[i]);const auto n=lead<128?1u:lead>=0xc2 && lead<=0xdf?2u:lead>=0xe0 && lead<=0xef?3u:lead>=0xf0 && lead<=0xf4?4u:0u;
        Require(n && i+n<=text.size(),"invalid dialog UTF-8");for(unsigned j=1;j<n;++j)Require((static_cast<unsigned char>(text[i+j])&0xc0)==0x80,"invalid dialog UTF-8 trail");i+=n;++result;
    }return result;
}
void Append(DialogText& out,std::string_view text,std::uint32_t color){
    const auto count=Characters(text);if(!count)return;const auto begin=out.characters;out.characters+=count;out.text+=text;
    Require(out.nodes.size()<4096,"too many dialog text nodes");out.nodes.push_back({begin,out.characters});
    if(!out.runs.empty() && out.runs.back().color==color)out.runs.back().end=out.characters;
    else{Require(out.runs.size()<1024,"too many dialog colour runs");out.runs.push_back({begin,out.characters,color});}
}
void Visit(const TiXmlNode& node,DialogText& out,std::uint32_t color,std::uint32_t normal,unsigned depth,bool hint_font_fallback){
    Require(depth<32,"dialog markup nesting limit");
    if(const auto* text=node.ToText()){Append(out,text->Value(),color);return;}
    const auto* element=node.ToElement();Require(element!=nullptr,"unsupported dialog rich text node");
    const std::string_view name=element->Value();
    if(name=="colour"){
        unsigned channels[4]{};unsigned i{};
        for(const char* attr:{"alpha","red","green","blue"}){int v{};Require(element->QueryIntAttribute(attr,&v)==TIXML_SUCCESS && v>=0 && v<=255,"invalid dialog colour attribute");channels[i++]=static_cast<unsigned>(v);}
        color=(channels[0]<<24)|(channels[1]<<16)|(channels[2]<<8)|channels[3];
        for(const auto* attr=element->FirstAttribute();attr;attr=attr->Next()){const std::string_view key=attr->Name();Require(key=="alpha" || key=="red" || key=="green" || key=="blue","unsupported dialog colour attribute");}
    }else{
        Require(!element->FirstAttribute(),"unsupported dialog preset colour attribute");
        if(name=="dc0")color=normal;else if(name=="dc1")color=0xffff0000;else if(name=="dc2")color=0xff00ff00;else if(name=="dc3")color=0xff0000ff;else if(name!="df2" || !hint_font_fallback)throw std::runtime_error("unsupported dialog rich text tag: "+std::string(name));
    }
    for(const auto* child=node.FirstChild();child;child=child->NextSibling())Visit(*child,out,color,normal,depth+1,hint_font_fallback);
}
}
DialogText ParseDialogText(std::string_view source,std::uint32_t normal_color,bool hint_font_fallback){
    Require(!source.empty() && source.size()<=65536 && source.find('\0')==std::string_view::npos,"invalid dialog text length");DialogText result;
    if(source.find('<')==std::string_view::npos){Append(result,source,normal_color);return result;}
    // 所有原UI/XML装载与对白解析由同一游戏线程串行执行；恢复TinyXML公共全局选项。
    struct Whitespace {bool old=TiXmlBase::IsWhiteSpaceCondensed();Whitespace(){TiXmlBase::SetCondenseWhiteSpace(false);}~Whitespace(){TiXmlBase::SetCondenseWhiteSpace(old);}} whitespace;
    TiXmlDocument doc;const std::string xml="<text>"+std::string(source)+"</text>";doc.Parse(xml.c_str(),nullptr,TIXML_ENCODING_UTF8);Require(!doc.Error() && doc.RootElement(),"invalid dialog rich text XML");
    for(const auto* child=doc.RootElement()->FirstChild();child;child=child->NextSibling())Visit(*child,result,normal_color,normal_color,0,hint_font_fallback);
    Require(result.characters>0,"empty dialog text");return result;
}
}
