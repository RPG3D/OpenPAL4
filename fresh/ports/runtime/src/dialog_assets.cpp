#include "pal4/portable/dialog_assets.h"
#include "pal4/scenario_dialog.h"
#include <tinyxml.h>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool value,const char* error){if(!value)throw std::runtime_error(error);}
std::string Attribute(const TiXmlElement& node,const char* name){const auto* value=node.Attribute(name);Require(value,"missing UI XML attribute");return value;}
std::string Member(std::string value){
    std::replace(value.begin(),value.end(),'\\','/');
    constexpr std::string_view prefix="gamedata/ui/";
    Require(value.starts_with(prefix),"UI resource outside original ui package");value.erase(0,prefix.size());
    Require(value.find("..") == std::string::npos && value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_./-")==std::string::npos,"invalid UI member");return value;
}
std::string Property(const TiXmlElement& node,const char* name){
    for(auto* p=node.FirstChildElement("Property");p;p=p->NextSiblingElement("Property"))if(Attribute(*p,"Name")==name)return Attribute(*p,"Value");
    throw std::runtime_error("missing dialog layout property");
}
const TiXmlElement& Window(const TiXmlElement& parent,const char* name){
    for(auto* p=parent.FirstChildElement("Window");p;p=p->NextSiblingElement("Window"))if(Attribute(*p,"Name")==name)return *p;
    throw std::runtime_error("missing original dialog window");
}
}
struct DialogAssets::Impl:pal4::IScenarioPortraitRuntime {
    UiReader reader;DialogStyle style;bool loaded{};
    struct Imageset {std::string member;std::map<std::string,UiImage> images;};
    std::map<std::string,Imageset> sets;
    std::map<std::string,std::string> image_files;
    std::map<std::string,pal4::ScenarioPortraitCacheEntry> cache;
    std::vector<UiImage> handles{UiImage{}};std::uint32_t selected{};
    explicit Impl(UiReader r):reader(std::move(r)){}
    std::vector<std::uint8_t> Read(std::string_view member){auto bytes=reader(member);if(!bytes)throw std::runtime_error("missing original UI member: "+std::string(member));Require(!bytes->empty() && bytes->size()<=32u*1024*1024,"invalid UI member size");return std::move(*bytes);}
    void Xml(std::string_view member,TiXmlDocument& doc){const auto bytes=Read(member);Require(bytes.size()<=2u*1024*1024 && std::find(bytes.begin(),bytes.end(),0)==bytes.end(),"invalid UI XML size/NUL");const std::string text(bytes.begin(),bytes.end());doc.Parse(text.c_str(),nullptr,TIXML_ENCODING_LEGACY);Require(!doc.Error() && doc.RootElement(),"cannot parse original UI XML");}
    void CreateImageset(const std::string& file) override {
        TiXmlDocument doc;Xml(Member(file),doc);const auto& node=*doc.RootElement();Require(std::string_view(node.Value())=="Imageset","invalid imageset root");
        Imageset set;set.member=Member(Attribute(node,"Imagefile"));
        int native_width=800,native_height=600;node.QueryIntAttribute("NativeHorzRes",&native_width);node.QueryIntAttribute("NativeVertRes",&native_height);
        Require(native_width>0 && native_height>0,"invalid imageset native resolution");
        const bool scaled=node.Attribute("AutoScaled") && std::string_view(node.Attribute("AutoScaled"))=="true";
        for(auto* p=node.FirstChildElement("Image");p;p=p->NextSiblingElement("Image")) {
            Require(set.images.size()<4096,"imageset exceeds image limit");UiImage image;image.member=set.member;
            Require(p->QueryIntAttribute("XPos",&image.x)==TIXML_SUCCESS && p->QueryIntAttribute("YPos",&image.y)==TIXML_SUCCESS && p->QueryIntAttribute("Width",&image.width)==TIXML_SUCCESS && p->QueryIntAttribute("Height",&image.height)==TIXML_SUCCESS && image.x>=0 && image.y>=0 && image.width>0 && image.height>0 && image.width<=16384 && image.height<=16384,"invalid UI image rectangle");
            image.logical_width=image.width*(scaled?800.f/static_cast<float>(native_width):1.f);image.logical_height=image.height*(scaled?600.f/static_cast<float>(native_height):1.f);
            Require(set.images.emplace(Attribute(*p,"Name"),image).second,"duplicate UI image");
        }
        const auto set_name=Attribute(node,"Name");
        Require(sets.emplace(set_name,std::move(set)).second,"duplicate UI imageset");image_files.emplace(Member(file),set_name);
    }
    void Load(){
        if(loaded)return;
        TiXmlDocument doc;Xml("layouts/ScenarioDialog.xml",doc);const auto& root=Window(*doc.RootElement(),"Root");const auto& panel=Window(root,"ScenarioDialog");const auto& edit=Window(panel,"EditDialog");
        auto rect=Property(panel,"UnifiedAreaRect");float a{},b{},c{},d{},e{},f{},g{},h{};
        for(auto& ch:rect)if(ch=='{' || ch=='}' || ch==',')ch=' ';
        std::istringstream values(rect);
        Require(bool(values>>a>>b>>c>>d>>e>>f>>g>>h) && a==0 && b==0 && c==0 && e==0 && g==0 && f==800 && d>=0 && h==600,"unsupported dialog panel rectangle");
        style.panel_top=d;style.panel_height=h-d;style.line_space=std::stof(Property(edit,"LineSpaceExtent"));
        const auto color=Property(edit,"NormalTextColour");Require(color.size()==8,"invalid dialog text color");style.color=static_cast<std::uint32_t>(std::stoul(color,nullptr,16));
        Require(Property(edit,"Font")=="dialog_simsun" && Property(panel,"Image")=="set:zhujiemian0 image:duihuakuang","unsupported dialog skin");
        if(!sets.contains("zhujiemian0"))CreateImageset("gamedata/ui/imagesets/zhujiemian_0.imageset");
        style.background=sets.at("zhujiemian0").images.at("duihuakuang");
        Require(Property(Window(panel,"ImgClick"),"Image")=="set:zhujiemian0 image:sanjiaodian2","unsupported dialog indicator");
        style.indicator=sets.at("zhujiemian0").images.at("sanjiaodian2");
        TiXmlDocument font;Xml("fonts/Dialog_SIMSUN.font",font);style.font=Member(Attribute(*font.RootElement(),"Filename"));style.font_size=std::stof(Attribute(*font.RootElement(),"Size"));
        Require(std::isfinite(style.font_size) && style.font_size>0 && style.font_size<128 && std::isfinite(style.line_space) && style.line_space>=0,"invalid dialog font metrics");loaded=true;
    }
    void BeginResources() override{Load();}
    std::uint32_t EndResources() override{return 1;}
    std::uint32_t ReadWord(std::uint32_t address) override{if(address==0x102c)return 0x2000;if(address==0x26b0)return selected;throw std::logic_error("invalid portrait slot");}
    void SetImage(std::uint32_t window,std::uint32_t image) override{Require(window==0x2000 && image<handles.size(),"invalid portrait image");selected=image;}
    std::optional<pal4::ScenarioPortraitCacheEntry> FindCache(std::uint32_t,const std::string& name) override{const auto found=cache.find(name);return found==cache.end()?std::nullopt:std::optional(found->second);}
    void InsertCache(std::uint32_t,const std::string& name,const pal4::ScenarioPortraitCacheEntry& entry) override{cache[name]=entry;}
    bool ImagesetPresent(const std::string& name) override{return sets.contains(name);}
    bool PortraitFileExists(const std::string& file) override{return reader(Member(file)).has_value();}
    bool ImageDefined(const std::string& set,const std::string& name) override{return sets.at(set).images.contains(name);}
    std::uint32_t GetImage(const std::string& set,const std::string& name) override{handles.push_back(sets.at(set).images.at(name));return static_cast<std::uint32_t>(handles.size()-1);}
    void SetTextAndImagePositions(std::uint32_t,std::uint32_t) override{}
};
DialogAssets::DialogAssets(UiReader reader):impl_(std::make_unique<Impl>(std::move(reader))){}
DialogAssets::~DialogAssets()=default;
const DialogStyle& DialogAssets::Style(){impl_->Load();return impl_->style;}
UiImage DialogAssets::Image(const std::string& file,const std::string& name){
    const auto member=Member("gamedata/ui/"+file);Require(member.starts_with("imagesets/") && member.ends_with(".imageset"),"invalid imageset file");
    if(!impl_->image_files.contains(member))impl_->CreateImageset("gamedata/ui/"+member);
    return impl_->sets.at(impl_->image_files.at(member)).images.at(name);
}
UiImage DialogAssets::Portrait(const std::string& name){Require(name.size()<=128 && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==std::string::npos,"invalid portrait name");(void)pal4::ScenarioDialog::SetPortrait(0x1000,name,1,*impl_);return impl_->handles.at(impl_->selected);}
std::vector<std::uint8_t> DialogAssets::Read(std::string_view member){Require(!member.empty() && member.front()!='/' && member.find("..") == std::string_view::npos && member.find(':')==std::string_view::npos,"invalid UI read path");return impl_->Read(member);}
}
