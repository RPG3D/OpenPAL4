#include "pal4/portable/dialog.h"
#include "pal4/script_talk_handler.h"
#include "pal4/script_scenario_portrait.h"
#include "pal4/scenario_dialog.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Json(std::ostream& out,std::string_view text){
    constexpr char hex[]="0123456789abcdef";out<<'"';for(const unsigned char c:text){if(c=='"' || c=='\\')out<<'\\'<<char(c);else if(c<32)out<<"\\u00"<<hex[c>>4]<<hex[c&15];else out<<char(c);}out<<'"';
}
void Image(std::ostream& out,const UiImage& image){out<<"{\"member\":";Json(out,image.member);out<<",\"x\":"<<image.x<<",\"y\":"<<image.y<<",\"width\":"<<image.width<<",\"height\":"<<image.height<<",\"logical_width\":"<<image.logical_width<<",\"logical_height\":"<<image.logical_height<<'}';}
void Rect(std::ostream& out,const pal4::ScenarioRect& r){out<<'['<<r.left<<','<<r.top<<','<<r.right<<','<<r.bottom<<']';}
}
struct DialogServices::Impl final:pal4::IScriptTalkHandlerRuntime,pal4::IScriptScenarioPortraitRuntime,pal4::IScenarioTextDisplayRuntime,pal4::IScenarioLayoutRuntime {
    IScriptContextServices& next;DialogAssets& assets;std::uint64_t generation,id{},revision{};
    pal4::ScriptTalkHandler talk{*this};std::unique_ptr<pal4::ScenarioDialogWaitState> handler;
    std::optional<ScriptToken> pending,completed;bool closed{},visible{},configured{},prepared{},left{true};
    std::optional<DialogText> staged_text;DialogText formatted_text;
    std::string text,voice,portrait_name,requested_portrait;UiImage portrait;std::uint32_t characters{},shown{},page{},calls{},portrait_calls{},confirmations{},finished{};
    std::vector<std::uint32_t> ends;float elapsed{},font_extent{};
    std::vector<DialogPageLayout> rich_pages;DialogTypewriter typewriter;
    pal4::ScenarioRect text_rect{},portrait_rect{},click_rect{};
    static constexpr std::uint32_t object=0x1000,manager=0x2000,executor=0x3000,edit=0x4000,image_window=0x5000,panel=0x6000,click=0x7000;
    Impl(IScriptContextServices& n,DialogAssets& a,std::uint64_t g):next(n),assets(a),generation(g){}
    void Layout(){(void)pal4::ScenarioDialog::Layout(object,left?1:0,*this);}
    std::uint32_t PageStart()const{return page?ends.at(page-1):0;}
    std::uint32_t PageEnd()const{return configured?ends.at(page):0;}
    std::uint8_t VoiceFlag() override{return 0;} // Current portable settings have voice disabled.
    std::uint32_t ScriptExecutor() override{return executor;}
    std::uint32_t AudioManager() override{throw std::logic_error("dialog voice is disabled");}
    std::uint32_t PlayMusic(std::uint32_t,std::string_view,std::uint32_t) override{throw std::logic_error("dialog voice playback is not bound");}
    void StoreVoiceHandle(std::uint32_t,std::uint32_t) override{throw std::logic_error("dialog voice is disabled");}
    pal4::ScenarioDialogWaitState* Allocate(std::uint32_t size) override{Require(size==32 && !handler,"dialog wait already exists");handler=std::make_unique<pal4::ScenarioDialogWaitState>();closed=false;return handler.get();}
    void ConstructBase(pal4::ScenarioDialogWaitState& h,std::uint32_t mode,std::uint32_t queue) override{Require(&h==handler.get() && mode==1 && queue==executor+0x54,"invalid dialog wait");}
    void Flush(pal4::ScenarioDialogWaitState& h) override{Require(&h==handler.get() && h.vtable_address==pal4::ScriptTalkHandler::kVtable,"invalid dialog handler");}
    std::uint32_t AddHandler(std::uint32_t queue,pal4::ScenarioDialogWaitState* h) override{Require(queue==executor+0x54 && h==handler.get(),"invalid dialog queue");return 1;}
    std::uint32_t UiFrameManager() override{return manager;}
    std::uint32_t CreateScenarioDialog(std::uint32_t owner) override{Require(owner==manager,"invalid dialog manager");(void)assets.Style();return object;}
    std::uint32_t ScenarioDialog(std::uint32_t owner) override{return CreateScenarioDialog(owner);}
    std::uint8_t IsVisible(std::uint32_t dialog) override{Require(dialog==object,"invalid dialog object");return visible;}
    std::uint8_t SetText(std::uint32_t dialog,std::string_view value,std::uint32_t display) override{
        Require(dialog==object && display==pal4::ScriptTalkHandler::kDisplayValue,"invalid dialog display mode");
        Require(staged_text.has_value(),"dialog text was not validated");
        text=value;formatted_text=std::move(*staged_text);staged_text.reset();characters=formatted_text.characters;shown=page=0;ends.clear();rich_pages.clear();typewriter={};configured=false;elapsed=0;visible=true;++id;++revision;Layout();return 1;
    }
    void HideDialog(std::uint32_t dialog,std::uint32_t a,std::uint32_t b) override{Require(dialog==object && a==0 && b==0,"invalid dialog hide");visible=false;++revision;}
    void SetClosedFlag(pal4::ScenarioDialogWaitState& h) override{Require(&h==handler.get(),"unknown dialog wait");closed=true;}
    std::uint32_t ReadWord(std::uint32_t address) override{
        if(address==object+4)return 0x9000;
        if(address==object+44)return image_window;
        if(address==object+48)return click;
        if(address==object+52)return panel;
        if(address==object+56)return edit;
        if(address==image_window+0x6b0)return portrait.member.empty()?0:0xa000;
        throw std::logic_error("invalid dialog slot");
    }
    std::uint32_t EmptyStringAddress() override{return 0x9000;}
    std::uint8_t SetPortrait(std::uint32_t dialog,std::uint32_t name,std::int32_t argument) override{Require(dialog==object && name==0x9000,"invalid portrait token");portrait=assets.Portrait(requested_portrait);portrait_name=requested_portrait;left=argument!=0;Layout();++revision;return 1;}
    bool PageFinished()const{return rich_pages.empty()?shown==PageEnd():typewriter.Cursor().finished;}
    bool IsTypeEnd(std::uint32_t) override{return configured && PageFinished() && page+1==ends.size();}
    bool NeedChangePage(std::uint32_t) override{return configured && PageFinished() && page+1<ends.size();}
    void ChangePage(std::uint32_t) override{Require(NeedChangePage(edit),"invalid dialog page advance");++page;elapsed=0;if(!rich_pages.empty()){typewriter.Configure(rich_pages.at(page).nodes,PageStart(),PageEnd());shown=typewriter.Shown();}++revision;}
    void ChangeLine(std::uint32_t) override{Require(configured,"dialog text layout not ready");if(!rich_pages.empty()){typewriter.FinishLine();shown=typewriter.Shown();}else shown=PageEnd();++revision;}
    void RequestRedraw(std::uint32_t) override{}
    void HideAndClear(std::uint32_t dialog) override{Require(dialog==object,"invalid dialog close");visible=false;portrait={};portrait_name.clear();configured=false;rich_pages.clear();typewriter={};++finished;++revision;}
    float FontExtentAA(std::uint32_t) override{return font_extent;}
    float ReadFloat(std::uint32_t address) override{Require(address==0xa000+32,"invalid portrait height slot");return portrait.logical_height;}
    void SetVisible(std::uint32_t,bool) override{}
    std::uint32_t SetRect(std::uint32_t window,const pal4::ScenarioRect& rect) override{if(window==edit)text_rect=rect;else if(window==image_window)portrait_rect=rect;else if(window==click)click_rect=rect;else throw std::logic_error("invalid dialog rectangle");return 1;}
    void WriteByte(std::uint32_t,std::uint8_t) override{}
};
DialogServices::DialogServices(IScriptContextServices& next,DialogAssets& assets,std::uint64_t generation):impl_(std::make_unique<Impl>(next,assets,generation)){}
DialogServices::~DialogServices()=default;
void DialogServices::Prepare(){if(!impl_->prepared){(void)impl_->assets.Style();impl_->prepared=true;++impl_->revision;}}
void DialogServices::OnCall(const ScriptRequest& request){impl_->next.OnCall(request);}
ScriptResponse DialogServices::Invoke(const ScriptRequest& request){auto& s=*impl_;const auto& c=request.call;
    if(c.system_id==123){Require(c.arguments.size()==2,"invalid portrait arguments");s.requested_portrait=std::get<std::string>(c.arguments[0]);++s.portrait_calls;(void)pal4::ScriptScenarioPortrait::Set(Impl::object,std::get<bool>(c.arguments[1])?1:0,s);return {};}
    if(c.system_id!=240)return s.next.Invoke(request);
    Require(c.arguments.size()==2 && !s.pending,"dialog service already waiting");++s.calls;
    s.staged_text=ParseDialogText(std::get<std::string>(c.arguments[0]),s.assets.Style().color);
    s.voice=std::get<std::string>(c.arguments[1]);
    try{(void)s.talk.Invoke(std::get<std::string>(c.arguments[0]),s.voice);s.pending=request.token;s.staged_text.reset();}
    catch(...){s.staged_text.reset();s.handler.reset();s.closed=false;s.visible=false;s.configured=false;s.ends.clear();s.rich_pages.clear();s.typewriter={};++s.revision;throw;}
    return {{},true};
}
bool DialogServices::Configure(std::uint64_t generation,std::uint64_t id,std::span<const std::uint32_t> ends,float extent){auto& s=*impl_;
    if(generation!=s.generation || id!=s.id || !s.visible || s.configured)return false;
    Require(!ends.empty() && ends.size()<=4096 && ends.back()==s.characters && std::isfinite(extent) && extent>=0 && extent<200,"invalid dialog page layout");
    std::uint32_t previous{};for(const auto end:ends){Require(end>previous && end<=s.characters,"invalid dialog page boundary");previous=end;}
    s.ends.assign(ends.begin(),ends.end());s.configured=true;s.font_extent=extent;s.Layout();++s.revision;return true;
}
bool DialogServices::ConfigureRich(std::uint64_t generation,std::uint64_t id,std::span<const DialogPageLayout> pages,float extent){auto& s=*impl_;
    if(generation!=s.generation || id!=s.id || !s.visible || s.configured)return false;
    Require(!pages.empty() && pages.size()<=4096 && std::isfinite(extent) && extent>=0 && extent<200,"invalid rich dialog layout");
    std::vector<bool> newlines(s.characters),auto_breaks(s.characters+1);std::uint32_t codepoint=0;
    for(std::size_t offset=0;offset<s.formatted_text.text.size();++offset){
        const auto byte=static_cast<unsigned char>(s.formatted_text.text[offset]);
        if((byte&0xc0)==0x80)continue;
        newlines.at(codepoint)=byte=='\n';
        if(s.formatted_text.text.compare(offset,3,"\xef\xbc\x9a")==0)auto_breaks.at(codepoint+1)=true;
        ++codepoint;
    }
    std::vector<std::uint32_t> ends;std::uint32_t begin=0;std::size_t total_nodes=0;DialogTypewriter validator;
    for(const auto& page:pages){
        Require(page.end>=begin && page.end<=s.characters,"invalid rich dialog page boundary");
        total_nodes+=page.nodes.size();Require(total_nodes<=131072,"too many rich dialog nodes");
        validator.Configure(page.nodes,begin,page.end);
        for(const auto& node:page.nodes){
            if(node.line_break)Require(node.begin==node.end?auto_breaks.at(node.begin):newlines.at(node.begin),"dialog line-break node is not LF or an original scenario delimiter");
            else for(auto i=node.begin;i<node.end;++i)Require(!newlines.at(i),"dialog LF requires a line-break node");
        }
        ends.push_back(page.end);begin=page.end;
    }
    Require(begin==s.characters,"incomplete rich dialog layout");
    std::vector<DialogPageLayout> next(pages.begin(),pages.end());
    DialogTypewriter first;first.Configure(next.front().nodes,0,next.front().end);
    s.rich_pages=std::move(next);s.typewriter=std::move(first);s.ends=std::move(ends);s.configured=true;s.font_extent=extent;s.Layout();++s.revision;return true;
}
bool DialogServices::Confirm(std::uint64_t generation,std::uint64_t id){auto& s=*impl_;if(generation!=s.generation || id!=s.id || !s.visible || !s.configured)return false;++s.confirmations;pal4::ScenarioDialog::HandleTextDisplay(Impl::object,s);return true;}
void DialogServices::Advance(float elapsed){auto& s=*impl_;Require(std::isfinite(elapsed) && elapsed>=0,"invalid dialog elapsed");
    if(s.visible && s.configured && !s.rich_pages.empty()){
        if(s.typewriter.Advance(elapsed,std::bit_cast<float>(pal4::ScriptTalkHandler::kDisplayValue))){s.shown=s.typewriter.Shown();++s.revision;}
    }else if(s.visible && s.configured && s.shown<s.PageEnd()){
        s.elapsed+=elapsed;const auto interval=std::bit_cast<float>(pal4::ScriptTalkHandler::kDisplayValue);
        if(s.elapsed>interval){++s.shown;s.elapsed=0;++s.revision;}
    }
    if(s.handler){s.talk.Update(*s.handler);if(s.closed){s.completed=s.pending;s.pending.reset();s.handler.reset();++s.revision;}}
}
std::optional<ScriptToken> DialogServices::TakeCompletion(){return std::exchange(impl_->completed,{});}
std::uint64_t DialogServices::Revision() const{return impl_->revision;}
DialogView DialogServices::View() const{const auto& s=*impl_;return {s.generation,s.id,s.revision,s.visible,s.configured,s.left,s.text,s.formatted_text.text,s.formatted_text.runs,s.portrait,s.characters,s.shown,s.page,s.configured?s.PageStart():0,s.configured?s.PageEnd():0,s.font_extent,{s.text_rect.left,s.text_rect.top,s.text_rect.right,s.text_rect.bottom},{s.portrait_rect.left,s.portrait_rect.top,s.portrait_rect.right,s.portrait_rect.bottom},s.formatted_text.nodes,s.rich_pages.empty()?std::optional<DialogTypingCursor>{}:s.typewriter.Cursor(),{s.click_rect.left,s.click_rect.top,s.click_rect.right,s.click_rect.bottom}};}
std::vector<std::string> DialogServices::ResourceMembers() const{const auto& style=impl_->assets.Style();std::vector<std::string> result{style.font,style.background.member};if(!impl_->portrait.member.empty() && impl_->portrait.member!=style.background.member)result.push_back(impl_->portrait.member);return result;}
void DialogServices::WriteReport(std::ostream& out) const{const auto& s=*impl_;out<<"{\"generation\":"<<s.generation<<",\"id\":"<<s.id<<",\"revision\":"<<s.revision<<",\"visible\":"<<(s.visible?"true":"false")<<",\"configured\":"<<(s.configured?"true":"false")<<",\"text\":";Json(out,s.text);out<<",\"display_text\":";Json(out,s.formatted_text.text);out<<",\"text_runs\":[";for(std::size_t i=0;i<s.formatted_text.runs.size();++i){if(i)out<<',';const auto& run=s.formatted_text.runs[i];out<<"{\"begin\":"<<run.begin<<",\"end\":"<<run.end<<",\"color\":"<<run.color<<'}';}out<<"],\"voice\":";Json(out,s.voice);out<<",\"portrait_name\":";Json(out,s.portrait_name);
    out<<",\"portrait\":";Image(out,s.portrait);out<<",\"portrait_left\":"<<(s.left?"true":"false")<<",\"characters\":"<<s.characters<<",\"shown\":"<<s.shown<<",\"page\":"<<s.page<<",\"page_start\":"<<(s.configured?s.PageStart():0)<<",\"page_end\":"<<(s.configured?s.PageEnd():0)<<",\"page_count\":"<<s.ends.size()<<",\"font_extent\":"<<s.font_extent<<",\"calls\":"<<s.calls<<",\"portrait_calls\":"<<s.portrait_calls<<",\"confirmations\":"<<s.confirmations<<",\"completed\":"<<s.finished<<",\"waiting\":"<<(s.pending?"true":"false")<<",\"voice_enabled\":false,\"text_rect\":";Rect(out,s.text_rect);out<<",\"portrait_rect\":";Rect(out,s.portrait_rect);out<<",\"click_rect\":";Rect(out,s.click_rect);
    if(!s.rich_pages.empty()){const auto cursor=s.typewriter.Cursor();out<<",\"typing\":{\"line\":"<<cursor.line<<",\"node\":"<<cursor.node<<",\"character\":"<<cursor.character<<",\"finished\":"<<(cursor.finished?"true":"false")<<'}';}
    if(s.id || s.prepared){const auto& style=s.assets.Style();out<<",\"style\":{\"background\":";Image(out,style.background);out<<",\"indicator\":";Image(out,style.indicator);out<<",\"font\":";Json(out,style.font);out<<",\"font_size\":"<<style.font_size<<",\"line_space\":"<<style.line_space<<",\"color\":"<<style.color<<",\"panel_top\":"<<style.panel_top<<",\"panel_height\":"<<style.panel_height<<'}';}out<<'}';
}
}
