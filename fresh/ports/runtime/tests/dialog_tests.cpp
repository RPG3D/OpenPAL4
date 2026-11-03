#include "pal4/portable/dialog.h"
#include "pal4/portable/package.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("dialog contract failed");}
template<class F>void Reject(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}Check(failed);}
struct Next:IScriptContextServices{unsigned calls{};ScriptResponse Invoke(const ScriptRequest&) override{++calls;return {};}};
std::map<std::string,std::string> Files(){return {
    {"layouts/ScenarioDialog.xml",R"(<GUILayout><Window Name="Root"><Window Name="ScenarioDialog"><Property Name="Image" Value="set:zhujiemian0 image:duihuakuang"/><Property Name="UnifiedAreaRect" Value="{{0,0},{0,474},{0,800},{0,600}}"/><Window Name="EditDialog"><Property Name="Font" Value="dialog_simsun"/><Property Name="LineSpaceExtent" Value="3"/><Property Name="NormalTextColour" Value="FFFBFFD8"/></Window><Window Name="ImgClick"><Property Name="Image" Value="set:zhujiemian0 image:sanjiaodian2"/></Window></Window></Window></GUILayout>)"},
    {"imagesets/zhujiemian_0.imageset",R"(<Imageset Name="zhujiemian0" Imagefile="gamedata\ui\imagesets\zhujiemian0.png"><Image Name="duihuakuang" XPos="0" YPos="772" Width="800" Height="2"/><Image Name="sanjiaodian2" XPos="0" YPos="500" Width="26" Height="23"/><Image Name="default" XPos="0" YPos="0" Width="199" Height="275"/></Imageset>)"},
    {"fonts/Dialog_SIMSUN.font",R"(<Font Filename="gamedata\ui\fonts\kai.ttf" Size="20"/>)"},
    {"portrait/portrait0_0.imageset",R"(<Imageset Name="portrait00" Imagefile="gamedata\ui\imagesets\portrait00.png"><Image Name="101G" XPos="199" YPos="237" Width="199" Height="237"/></Imageset>)"},
    {"portrait/portrait1_0.imageset",R"(<Imageset Name="portrait10" Imagefile="gamedata\ui\imagesets\portrait10.png" AutoScaled="true" NativeHorzRes="1600" NativeVertRes="900"><Image Name="101F" XPos="0" YPos="0" Width="398" Height="474"/></Imageset>)"}
};}
}
int main(int argc,char** argv){try {
    if(argc==3){std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);DialogAssets assets([&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{std::string key(member);std::replace(key.begin(),key.end(),'/','\\');if(!package.Find(key))return {};return package.Read(key);});Next next;DialogServices dialog(next,assets,1);ScriptCall portrait{123,0,{std::string("101G"),true}};dialog.Invoke({{1,1},portrait});ScriptCall talk{240,0,{std::string("原资源对白"),std::string("40002")}};dialog.Invoke({{1,2},talk});std::ofstream out(argv[2]);dialog.WriteReport(out);Check(assets.Portrait("101G").height>0);return 0;}
    auto files=Files();DialogAssets assets([&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{auto found=files.find(std::string(member));if(found==files.end())return {};return std::vector<std::uint8_t>(found->second.begin(),found->second.end());});
    Check(assets.Style().panel_top==474 && assets.Style().background.height==2);
    Check(assets.Portrait("101G").x==199 && assets.Portrait("101G").height==237 && assets.Portrait("missing").height==275);
    Check(assets.Portrait("101F").logical_width==199 && assets.Portrait("101F").logical_height==316);
    Reject([&]{assets.Portrait("../bad");});Reject([&]{assets.Read("../outside");});
    Next next;DialogServices dialog(next,assets,9);ScriptGlobals globals{};ScriptStack stack(globals);
    dialog.Prepare();const auto revision=dialog.Revision();dialog.Prepare();Check(revision>0 && revision==dialog.Revision());
    stack.Compile("talk",R"(void Main(){giSetPortrait("101G",true);giTalk("你好abcdef", "40002");global_mvar=7;giTalk("second", "");global_mvar=8;})");
    Check(bool(stack.Push("talk","void Main()",dialog)) && stack.Tick(0)==ScriptState::suspended && globals[0]==0);
    ScriptCall ignoredPortrait{123,0,{std::string("101F"),false}};
    dialog.Invoke({{9,9},ignoredPortrait});std::ostringstream visiblePortrait;dialog.WriteReport(visiblePortrait);
    Check(visiblePortrait.str().find("\"portrait_name\":\"101G\"")!=std::string::npos);
    const std::uint32_t pages[]{4,8};Reject([&]{const std::uint32_t bad[]{8,4};dialog.Configure(9,1,bad,10);});
    Check(!dialog.Configure(8,1,pages,10) && !dialog.Confirm(9,1) && dialog.Configure(9,1,pages,10));
    dialog.Advance(.1f);Check(dialog.View().shown==0);dialog.Advance(.01f);Check(dialog.View().shown==1);Check(dialog.Confirm(9,1));dialog.Advance(0);Check(!dialog.TakeCompletion() && stack.Tick(.1)==ScriptState::suspended && globals[0]==0);
    Check(dialog.Confirm(9,1));dialog.Advance(2);Check(dialog.View().shown==5 && !dialog.TakeCompletion());
    Check(dialog.Confirm(9,1));Check(dialog.Confirm(9,1));dialog.Advance(0);const auto token=dialog.TakeCompletion();Check(token && stack.Complete(*token) && !stack.Complete(*token));
    Check(stack.Tick(0)==ScriptState::suspended && globals[0]==7 && !dialog.Confirm(9,1));
    const std::uint32_t second[]{6};Check(dialog.Configure(9,2,second,10));dialog.Advance(2);Check(dialog.View().shown==1);Check(dialog.Confirm(9,2));Check(dialog.Confirm(9,2));dialog.Advance(0);Check(stack.Complete(*dialog.TakeCompletion()) && stack.Tick(0)==ScriptState::finished && globals[0]==8);
    std::ostringstream out;dialog.WriteReport(out);Check(out.str().find("\"completed\":2")!=std::string::npos && out.str().find("\"portrait_name\":\"\"")!=std::string::npos);
    Reject([&]{dialog.Advance(-1);});
    // 解析失败不分配原等待处理器；后续合法对白仍可正常完成。
    ScriptCall malformed{240,0,{std::string("<unknown>bad</unknown>"),std::string()}};
    Reject([&]{dialog.Invoke({{9,20},malformed});});
    dialog.Advance(0);Check(!dialog.TakeCompletion() && !dialog.View().visible);
    ScriptCall colored{240,0,{std::string("A<colour red=255 green=187 blue=0 alpha=255>B</colour><dc0>C</dc0>"),std::string()}};
    Check(dialog.Invoke({{9,21},colored}).suspend);
    const auto view=dialog.View();Check(view.text!=view.display_text && view.display_text=="ABC" && view.characters==3 && view.text_runs.size()==3 && view.text_runs[1].color==0xffffbb00);
    Check(view.text_nodes.size()==3 && view.text_nodes[0].begin==0 && view.text_nodes[0].end==1 && view.text_nodes[2].end==3);
    const std::uint32_t colored_page[]{3};Check(dialog.Configure(9,view.id,colored_page,10));dialog.Advance(2);
    Check(dialog.Confirm(9,view.id));Check(dialog.Confirm(9,view.id));dialog.Advance(0);const auto colored_token=dialog.TakeCompletion();Check(colored_token && colored_token->sequence==21);
    ScriptCall same_colour{240,0,{std::string("<dc0>你好<dc0>你 A</dc0></dc0>"),std::string()}};
    Check(dialog.Invoke({{9,22},same_colour}).suspend);const auto same_view=dialog.View();
    Check(same_view.characters==5 && same_view.text_runs.size()==1 && same_view.text_nodes.size()==2 && same_view.text_nodes[0].end==2 && same_view.text_nodes[1].begin==2 && same_view.text_nodes[1].end==5);
    const std::uint32_t same_page[]{5};Check(dialog.Configure(9,same_view.id,same_page,10));
    Check(dialog.Confirm(9,same_view.id) && dialog.Confirm(9,same_view.id));dialog.Advance(0);const auto same_token=dialog.TakeCompletion();Check(same_token && same_token->sequence==22);

    // 节点/行过渡尚未完成时，确认只补全当前页，不翻页或释放脚本等待。
    DialogServices rich(next,assets,11);ScriptCall rich_call{240,0,{std::string("<dc0>你好<dc1>你 A</dc1></dc0>"),std::string()}};
    Check(rich.Invoke({{11,1},rich_call}).suspend);const auto rich_id=rich.View().id;
    const DialogPageLayout rich_pages[]{{3,{{0,2,0},{2,3,0}}},{5,{{3,5,0}}}};
    auto bad_page=rich_pages[0];bad_page.nodes[0].line_break=true;const auto before_rich=rich.Revision();
    Reject([&]{rich.ConfigureRich(11,rich_id,std::span(&bad_page,1),30);});Check(!rich.View().configured && rich.Revision()==before_rich);
    Check(!rich.ConfigureRich(12,rich_id,rich_pages,30) && rich.ConfigureRich(11,rich_id,rich_pages,30));
    for(unsigned i=0;i<4;++i)rich.Advance(.11f);
    Check(rich.View().shown==3 && rich.View().typing && !rich.View().typing->finished && rich.View().page==0);
    Check(rich.Confirm(11,rich_id) && rich.View().typing->finished && rich.View().page==0);rich.Advance(0);Check(!rich.TakeCompletion());
    Check(rich.Confirm(11,rich_id) && rich.View().page==1 && rich.View().shown==3 && !rich.View().typing->finished);
    rich.Advance(.11f);rich.Advance(.11f);Check(rich.View().shown==5 && !rich.View().typing->finished);
    Check(rich.Confirm(11,rich_id) && rich.View().visible && rich.View().typing->finished);
    Check(rich.Confirm(11,rich_id) && !rich.View().visible && !rich.View().typing);rich.Advance(0);
    const auto rich_token=rich.TakeCompletion();Check(rich_token && rich_token->sequence==1 && !rich.TakeCompletion());
    Check(rich.Invoke({{11,2},rich_call}).suspend);const auto second_rich_id=rich.View().id;
    const DialogPageLayout rich_full{5,{{0,2,0},{2,3,0},{3,5,1}}};Check(rich.ConfigureRich(11,second_rich_id,std::span(&rich_full,1),30));
    const unsigned revealed[]{1,2,2,3,3,3,4,5,5,5};
    for(unsigned i=0;i<10;++i){rich.Advance(.11f);Check(rich.View().shown==revealed[i] && rich.View().typing->finished==(i==9) && rich.View().visible && !rich.TakeCompletion());}
    Check(rich.Confirm(11,second_rich_id));rich.Advance(0);const auto rich_second=rich.TakeCompletion();Check(rich_second && rich_second->sequence==2);
    ScriptCall trailing{240,0,{std::string("A\n"),std::string()}};Check(rich.Invoke({{11,3},trailing}).suspend);const auto trailing_id=rich.View().id;
    const DialogPageLayout trailing_pages[]{{2,{{0,1,0},{1,2,0,true}}},{2,{{2,2,0}}}};
    Check(rich.ConfigureRich(11,trailing_id,trailing_pages,30));for(unsigned i=0;i<4;++i)rich.Advance(.11f);
    Check(rich.View().typing->finished && rich.Confirm(11,trailing_id));
    Check(rich.View().page==1 && rich.View().page_start==2 && rich.View().page_end==2 && rich.View().shown==2 && !rich.View().typing->finished);
    rich.Advance(.11f);Check(!rich.View().typing->finished && !rich.TakeCompletion());rich.Advance(.11f);Check(rich.View().typing->finished && rich.View().visible);
    Check(rich.Confirm(11,trailing_id));rich.Advance(0);const auto trailing_token=rich.TakeCompletion();Check(trailing_token && trailing_token->sequence==3);

    ScriptCall speaker{240,0,{std::string("A：B"),std::string()}};
    Check(rich.Invoke({{11,4},speaker}).suspend);const auto speaker_id=rich.View().id;
    const DialogPageLayout speaker_page{3,{{0,2,0},{2,2,0,true},{2,3,1}}};
    auto invalid_speaker=speaker_page;invalid_speaker.nodes[0].end=1;invalid_speaker.nodes[1]={1,1,0,true};invalid_speaker.nodes[2].begin=1;
    Reject([&]{rich.ConfigureRich(11,speaker_id,std::span(&invalid_speaker,1),30);});
    Check(rich.ConfigureRich(11,speaker_id,std::span(&speaker_page,1),30));
    // Game HandleTextDisplay loops changeLine until the current page is ready.
    Check(rich.Confirm(11,speaker_id) && rich.View().shown==3 && rich.View().typing->line==2 && rich.View().typing->finished && rich.View().visible);
    rich.Advance(0);Check(!rich.TakeCompletion());
    Check(rich.Confirm(11,speaker_id));rich.Advance(0);Check(rich.TakeCompletion()->sequence==4);
    std::cout<<"dialog original wait, type/reveal/page/close, portrait clear, stale callbacks and user resource XML passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
