#include "pal4/portable/tutorial_assets.h"
#include "pal4/portable/dialog_text.h"
#include <tinyxml.h>
#include "pal4/combat_hint_window.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
std::string Attr(const TiXmlElement& e,const char* name){const auto* s=e.Attribute(name);Require(s,"missing tutorial XML attribute");return s;}
std::string Property(const TiXmlElement& e,const char* name,bool required=true){for(const auto* p=e.FirstChildElement("Property");p;p=p->NextSiblingElement("Property"))if(Attr(*p,"Name")==name)return Attr(*p,"Value");Require(!required,"missing tutorial layout property");return {};}
void Xml(DialogAssets& assets,std::string_view member,TiXmlDocument& doc){const auto b=assets.Read(member);Require(b.size()<=2*1024*1024 && std::find(b.begin(),b.end(),0)==b.end(),"invalid tutorial XML length/NUL");doc.Parse(std::string(b.begin(),b.end()).c_str(),nullptr,TIXML_ENCODING_LEGACY);Require(!doc.Error() && doc.RootElement(),"invalid tutorial XML");}
std::string Imageset(std::string_view name){
    if(name=="maimai0")return "imagesets/maimai_0.imageset";
    if(name=="biaoti0")return "imagesets/biaoti_0.imageset";
    if(name=="kuang0")return "imagesets/kuang_0.imageset";
    if(name=="duihuakuang0")return "imagesets/duihuakuang_0.imageset";
    if(name=="OiramLook")return "imagesets/OiramLook.imageset";
    throw std::runtime_error("unbound tutorial imageset: "+std::string(name));
}
UiImage Image(DialogAssets& assets,const std::string& ref){std::istringstream in(ref);std::string set,image,extra;Require(bool(in>>set>>image) && !(in>>extra) && set.starts_with("set:") && image.starts_with("image:"),"invalid tutorial image reference");return assets.Image(Imageset(std::string_view(set).substr(4)),image.substr(6));}
TutorialRect Rect(const std::string& text,const TutorialRect& parent){auto value=text;for(auto& c:value)if(c=='{'||c=='}'||c==',')c=' ';std::istringstream in(value);float v[8]{};for(auto& f:v)Require(bool(in>>f) && std::isfinite(f),"invalid tutorial rectangle");std::string extra;Require(!(in>>extra),"trailing tutorial rectangle values");
    for(unsigned i=0;i<8;++i)Require(std::abs(v[i])<=(i%2?16384.f:4.f),"tutorial rectangle exceeds limits");
    const float w=parent[2]-parent[0],h=parent[3]-parent[1];TutorialRect result{parent[0]+w*v[0]+v[1],parent[1]+h*v[2]+v[3],parent[0]+w*v[4]+v[5],parent[1]+h*v[6]+v[7]};Require(result[2]>result[0] && result[3]>result[1],"inverted tutorial rectangle");return result;}
std::uint32_t Tint(const TiXmlElement& node){const auto s=Property(node,"BackgroundColours",false);if(s.empty())return 0xffffffff;std::istringstream in(s);std::string item;std::uint32_t result{};unsigned count{};while(in>>item){Require(item.size()==11 && item[2]==':',"invalid tutorial tint");const auto value=static_cast<std::uint32_t>(std::stoul(item.substr(3),nullptr,16));if(count)Require(value==result,"unsupported tutorial gradient");result=value;++count;}Require(count==4,"invalid tutorial corners");return result;}
void Walk(DialogAssets& assets,const TiXmlElement& node,const TutorialRect& parent,TutorialPresentation& out,unsigned depth){Require(depth<16,"tutorial layout nesting limit");const auto name=Attr(node,"Name");const auto rect=Rect(Property(node,"UnifiedAreaRect"),parent);
    if(name=="EditInfo"){out.text_rect=rect;out.line_space=std::stof(Property(node,"LineSpaceExtent"));out.color=static_cast<unsigned>(std::stoul(Property(node,"NormalTextColour"),nullptr,16));Require(Property(node,"VScrollVisible")=="True","tutorial scrollbar is disabled");}
    else if(name=="BtnOk"){out.button_rect=rect;out.button=Image(assets,Property(node,"olNormalImage"));}
    else if(const auto image=Property(node,"Image",false);!image.empty())out.patches.push_back({Image(assets,image),rect,Tint(node)});
    for(const auto* child=node.FirstChildElement("Window");child;child=child->NextSiblingElement("Window"))Walk(assets,*child,rect,out,depth+1);
}
void Text(DialogAssets& assets,std::string_view source,TutorialPresentation& out){Require(!source.empty() && source.size()<=256*1024,"invalid tutorial source size");std::uint32_t characters{};
    for(std::size_t p=0;p<source.size();){const auto end=source.find('<',p);const auto n=(end==source.npos?source.size():end)-p;if(n){const auto plain=ParseDialogText(source.substr(p,n),out.color);out.text+=plain.text;characters+=plain.characters;p+=n;}
        if(end==source.npos)break;
        const auto close=source.find('>',p);Require(close!=source.npos && close-p<512,"invalid tutorial tag");const auto tag=source.substr(p,close-p+1);
        if(tag!="<dc0>" && tag!="</dc0>"){TiXmlDocument doc;doc.Parse(std::string(tag).c_str(),nullptr,TIXML_ENCODING_UTF8);const auto* image=doc.RootElement();Require(!doc.Error() && image && std::string_view(image->Value())=="image" && !image->FirstChild(),"unsupported tutorial tag");for(const auto* a=image->FirstAttribute();a;a=a->Next())Require(std::string_view(a->Name())=="set" || std::string_view(a->Name())=="name","unsupported tutorial image attribute");out.images.push_back({characters,assets.Image(Imageset(Attr(*image,"set")),Attr(*image,"name"))});out.text+="\xef\xbf\xbc";++characters;}
        p=close+1;
    }
}
void Json(std::ostream& out,std::string_view value){out<<'"';constexpr char hex[]="0123456789abcdef";for(unsigned char c:value){if(c=='"'||c=='\\')out<<'\\'<<char(c);else if(c<32)out<<"\\u00"<<hex[c>>4]<<hex[c&15];else out<<char(c);}out<<'"';}
void RectJson(std::ostream& out,const TutorialRect& r){out<<'['<<r[0]<<','<<r[1]<<','<<r[2]<<','<<r[3]<<']';}
void ImageJson(std::ostream& out,const UiImage& i){out<<"{\"member\":";Json(out,i.member);out<<",\"x\":"<<i.x<<",\"y\":"<<i.y<<",\"width\":"<<i.width<<",\"height\":"<<i.height<<",\"logical_width\":"<<i.logical_width<<",\"logical_height\":"<<i.logical_height<<'}';}
}
TutorialPresentation PrepareTutorialPresentation(DialogAssets& assets,std::string_view source){TutorialPresentation out;TiXmlDocument layout;Xml(assets,"layouts/TutorialWindow.xml",layout);Require(std::string_view(layout.RootElement()->Value())=="GUILayout","invalid tutorial layout root");const auto* root=layout.RootElement()->FirstChildElement("Window");Require(root && !root->NextSiblingElement("Window"),"invalid tutorial layout window");Walk(assets,*root,{0,0,800,600},out,0);
    TiXmlDocument font;Xml(assets,"fonts/system.font",font);out.font=Attr(*font.RootElement(),"Filename");std::replace(out.font.begin(),out.font.end(),'\\','/');Require(out.font.starts_with("gamedata/ui/fonts/") && out.font.find("..") ==out.font.npos,"invalid tutorial font");out.font.erase(0,12);out.font_size=std::stof(Attr(*font.RootElement(),"Size"));Require(std::isfinite(out.font_size)&&out.font_size>0&&out.font_size<128&&std::isfinite(out.line_space)&&out.line_space>=0&&!out.button.member.empty()&&out.text_rect[2]>out.text_rect[0],"incomplete tutorial style");Text(assets,source,out);return out;}
TutorialPresentation PrepareHintPresentation(DialogAssets& assets,std::string_view source,float x,float y){
    Require(std::isfinite(x) && std::isfinite(y) && x>=0 && x<=1 && y>=0 && y<=1,"invalid hint anchor");
    TiXmlDocument layout;Xml(assets,"layouts/hintWindow.xml",layout);const auto* root=layout.RootElement()->FirstChildElement("Window");
    Require(root && Attr(*root,"Name")=="ImgFrame","invalid hint frame");
    auto frame=Rect(Property(*root,"UnifiedAreaRect"),{0,0,800,600});
    const auto width=frame[2]-frame[0],height=frame[3]-frame[1];frame={x*800-width*.5f,y*600-height*.5f,x*800+width*.5f,y*600+height*.5f};
    const auto* edit=root->FirstChildElement("Window");Require(edit && Attr(*edit,"Name")=="EditMessage","invalid hint text window");
    TutorialPresentation out;out.text_rect=Rect(Property(*edit,"UnifiedAreaRect"),frame);
    out.line_space=std::stof(Property(*edit,"LineSpaceExtent"));out.color=static_cast<std::uint32_t>(std::stoul(Property(*edit,"NormalTextColour"),nullptr,16));
    const char* names[9]={"TopLeftFrameImage","TopFrameImage","TopRightFrameImage","LeftFrameImage","Image","RightFrameImage","BottomLeftFrameImage","BottomFrameImage","BottomRightFrameImage"};
    UiImage pieces[9];for(unsigned i=0;i<9;++i)pieces[i]=Image(assets,Property(*root,names[i]));
    const float xs[4]={frame[0],frame[0]+pieces[0].logical_width,frame[2]-pieces[2].logical_width,frame[2]};
    const float ys[4]={frame[1],frame[1]+pieces[0].logical_height,frame[3]-pieces[6].logical_height,frame[3]};
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)out.patches.push_back({pieces[row*3+col],{xs[col],ys[row],xs[col+1],ys[row+1]},Tint(*root)});
    TiXmlDocument font;Xml(assets,"fonts/"+Property(*edit,"Font")+".font",font);
    out.font=Attr(*font.RootElement(),"Filename");std::replace(out.font.begin(),out.font.end(),'\\','/');
    Require(out.font.starts_with("gamedata/ui/fonts/") && out.font.find("..")==out.font.npos,"invalid hint font");out.font.erase(0,12);
    out.font_size=std::stof(Attr(*font.RootElement(),"Size"));Require(std::isfinite(out.font_size) && out.font_size>0 && out.font_size<128,"invalid hint font size");
    std::string markup(source);for(std::size_t pos=0;(pos=markup.find("<br/>",pos))!=markup.npos;){markup.replace(pos,5,"\n");++pos;}
    const auto rich=ParseDialogText(markup,out.color,true);out.text=rich.text;out.text_runs=rich.runs;return out;
}
void ResizeHintPresentation(TutorialPresentation& p,float width,float height){
    Require(p.button.member.empty() && p.patches.size()==9 && std::isfinite(width) && std::isfinite(height) && width>0 && height>0 && width<4096 && height<4096,"invalid hint text measurement");
    const TutorialRect frame{p.patches[0].rect[0],p.patches[0].rect[1],p.patches[8].rect[2],p.patches[8].rect[3]};
    struct Metrics final:pal4::IHintWindowResizeRuntime {
        float ew,eh,fw,fh,width,height;
        Metrics(const TutorialRect& f,const TutorialRect& e,float w,float h):ew(e[2]-e[0]),eh(e[3]-e[1]),fw(f[2]-f[0]),fh(f[3]-f[1]),width(w),height(h){}
        std::uint32_t ReadWord(std::uint32_t a) override{Require(a==0x300+40 || a==0x300+44,"invalid hint resize field");return a==0x300+40?0x100:0x200;}
        float TextHeight(std::uint32_t e) override{Require(e==0x100,"invalid hint text metrics");return height;}
        float MaxLineWidth(std::uint32_t e) override{Require(e==0x100,"invalid hint width metrics");return width;}
        float ReadFloat(std::uint32_t a) override{switch(a){case 0x100+268:return ew;case 0x100+272:return eh;case 0x200+268:return fw;case 0x200+272:return fh;default:throw std::logic_error("invalid hint resize read");}}
        void SetHeight(std::uint32_t e,std::uint32_t mode,float v) override{Require(mode==1 && (e==0x100 || e==0x200),"invalid hint height binding");(e==0x100?eh:fh)=v;}
        void SetWidth(std::uint32_t e,std::uint32_t mode,float v) override{Require(mode==1 && (e==0x100 || e==0x200),"invalid hint width binding");(e==0x100?ew:fw)=v;}
    } metrics(frame,p.text_rect,width,height);
    pal4::CombatHintWindow::Resize(0x300,metrics);
    const float x=(frame[0]+frame[2])*.5f,y=(frame[1]+frame[3])*.5f;
    const float left=x-metrics.fw*.5f,top=y-metrics.fh*.5f;
    const float text_x=left+p.text_rect[0]-frame[0],text_y=top+p.text_rect[1]-frame[1];
    p.text_rect={text_x,text_y,text_x+metrics.ew,text_y+metrics.eh};
    const float xs[4]={left,left+p.patches[0].image.logical_width,left+metrics.fw-p.patches[2].image.logical_width,left+metrics.fw};
    const float ys[4]={top,top+p.patches[0].image.logical_height,top+metrics.fh-p.patches[6].image.logical_height,top+metrics.fh};
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)p.patches[row*3+col].rect={xs[col],ys[row],xs[col+1],ys[row+1]};
    p.hint_sized=true;
}
std::vector<std::string> TutorialResourceMembers(const TutorialPresentation& p){std::set<std::string> result{p.font};if(!p.button.member.empty())result.insert(p.button.member);for(const auto& patch:p.patches)result.insert(patch.image.member);for(const auto& image:p.images)result.insert(image.image.member);return {result.begin(),result.end()};}
void WriteTutorialPresentation(std::ostream& out,const TutorialPresentation& p){out<<"{\"hint_sized\":"<<(p.hint_sized?"true":"false")<<",\"text\":";Json(out,p.text);out<<",\"text_runs\":[";for(std::size_t i=0;i<p.text_runs.size();++i){if(i)out<<',';const auto& r=p.text_runs[i];out<<"{\"begin\":"<<r.begin<<",\"end\":"<<r.end<<",\"color\":"<<r.color<<'}';}out<<"],\"font\":";Json(out,p.font);out<<",\"font_size\":"<<p.font_size<<",\"line_space\":"<<p.line_space<<",\"color\":"<<p.color<<",\"text_rect\":";RectJson(out,p.text_rect);out<<",\"button_rect\":";RectJson(out,p.button_rect);out<<",\"button\":";ImageJson(out,p.button);out<<",\"patches\":[";bool first=true;for(const auto& patch:p.patches){if(!first)out<<',';first=false;out<<"{\"image\":";ImageJson(out,patch.image);out<<",\"rect\":";RectJson(out,patch.rect);out<<",\"color\":"<<patch.color<<'}';}out<<"],\"images\":[";first=true;for(const auto& image:p.images){if(!first)out<<',';first=false;out<<"{\"character\":"<<image.character<<",\"image\":";ImageJson(out,image.image);out<<'}';}out<<"]}";}
}
