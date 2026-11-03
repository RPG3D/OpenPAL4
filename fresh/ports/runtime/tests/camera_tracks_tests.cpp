#include "pal4/portable/camera_tracks.h"
#include "pal4/binary_xml_config.h"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <source_location>
#include <stdexcept>

using namespace pal4::portable;
namespace {
void Check(bool value,std::source_location line=std::source_location::current()){if(!value)throw std::runtime_error("camera track contract line "+std::to_string(line.line()));}
void Word(std::string& data,std::uint32_t value){for(unsigned i=0;i<32;i+=8)data+=static_cast<char>(value>>i);}
void Real(std::string& data,float value){Word(data,std::bit_cast<std::uint32_t>(value));}
void Text(std::string& data,std::string_view text){Word(data,static_cast<std::uint32_t>(text.size()+1));data+=text;data+='\0';}
std::vector<std::uint8_t> Fixture() {
    std::string data;Word(data,3);
    for(unsigned i=0;i<3;++i){
        Text(data,i==0?"001":i==1?"002":"003");
        for(float value:{10.f,20.f,30.f,0.f,0.f,0.f})Real(data,value);
        Word(data,0xffffffff);Word(data,0xffffffff);Real(data,i==0?100.f:200.f);
        Word(data,i==2?0:1);Real(data,i==0?2.f:1.f);Word(data,i==0?1:0);Word(data,0);
        pal4::BinaryXmlNode xml;xml.name="config";pal4::BinaryXmlNode child;child.name="SCLD_Extend";
        for(const auto& [name,value]:std::initializer_list<std::pair<const char*,float>>{
            {"SCLD_Extend_lookat_x",10.f},{"SCLD_Extend_lookat_y",20.f},{"SCLD_Extend_lookat_z",30.f},
            {"SCLD_Extend_eye_x",10.f},{"SCLD_Extend_eye_y",20.f},{"SCLD_Extend_eye_z",-170.f}})
            child.attributes.push_back({name,2,value});
        xml.children.push_back(child);const auto bytes=pal4::BinaryXmlConfig::Serialize(xml);Check(bool(bytes));data.append(bytes->begin(),bytes->end());
    }
    return {data.begin(),data.end()};
}
bool Near(float a,float b){return std::abs(a-b)<.003f;}
}
int main(int argc,char** argv){try {
    if(argc==4){
        std::ifstream input(argv[1],std::ios::binary);std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        GameCameraSystem cameras;cameras.SetCollision(false);
        CameraTracks tracks(cameras,[&](auto)->std::optional<std::vector<std::uint8_t>>{return bytes;},true);
        Check(std::get<std::int32_t>(tracks.Prepare("Q01","N01","MC001").value)==1);
        std::ofstream frames(std::string(argv[3])+".frames.jsonl");std::istringstream names(argv[2]);std::string name;unsigned sequence{};
        while(std::getline(names,name,',')) {
            Check(tracks.RunSingle({1,++sequence},name,true).suspend);unsigned done{};
            for(unsigned frame=0;frame<10000 && !done;++frame){done+=static_cast<unsigned>(tracks.Advance(.05f).size());cameras.Update(0,{});tracks.WriteReport(frames,false);frames<<'\n';}
            Check(done==1 && !tracks.Playing());
        }
        Check(sequence>0);std::ofstream out(argv[3]);tracks.WriteReport(out);return 0;
    }
    const auto bytes=Fixture();GameCameraSystem cameras;cameras.SetCollision(false);cameras.SetParameters(0,{0,0,0},{0,0,0},100);
    CameraTracks tracks(cameras,[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(member!="Q01\\N01\\MC001.cam")return {};return bytes;},true);
    Check(std::get<std::int32_t>(tracks.Prepare("Q01","N01","MC001").value)==1);
    {std::ostringstream summary,diagnostic;tracks.WriteReport(summary,false);tracks.WriteReport(diagnostic,true);
        Check(summary.str().find("source_hex")==std::string::npos && diagnostic.str().find("source_hex")!=std::string::npos);}
    {GameCameraSystem separate;CameraTracks normal(separate,[&](auto)->std::optional<std::vector<std::uint8_t>>{return bytes;});
        Check(std::get<std::int32_t>(normal.Prepare("Q01","N01","MC001").value)==1);std::ostringstream report;normal.WriteReport(report,true);
        Check(report.str().find("source_hex")==std::string::npos && report.str().find("\"bytes\":"+std::to_string(bytes.size()))!=std::string::npos);}
    const auto before=cameras.Pose();Check(tracks.RunSingle({10,1},"001",true).suspend && cameras.Pose().target==before.target);
    Check(tracks.Advance(0).empty());Check(cameras.Pose().target==std::array<float,3>{10,20,30});
    Check(tracks.Advance(0).empty());auto done=tracks.Advance(0);Check(done.size()==1 && done[0].token==ScriptToken{10,1} && done[0].error.empty());
    Check(!tracks.Playing() && tracks.Advance(100).empty());
    const auto idle_pose=cameras.Pose();
    Check(tracks.WaitCurrent({11,1}).suspend && !tracks.Playing() && tracks.Waiting());
    done=tracks.Advance(0);Check(done.size()==1 && done[0].token==ScriptToken{11,1} && !tracks.Waiting());
    Check(cameras.Pose().eye==idle_pose.eye && cameras.Pose().target==idle_pose.target && tracks.Advance(1).empty());
    cameras.SetParameters(0,{0,0,0},{0,0,0},100);
    Check(tracks.RunSingle({20,1},"002",true).suspend);Check(tracks.Advance(0).empty());
    Check(tracks.Advance(.5f).empty());auto mid=cameras.Pose();Check(Near(mid.target[0],5) && Near(mid.distance,150));
    Check(tracks.Advance(.5f).empty());done=tracks.Advance(0);Check(done.size()==1 && done[0].token.context==20);
    Check(Near(cameras.Pose().target[0],10) && Near(cameras.Pose().distance,200));
    cameras.SetParameters(0,{0,0,0},{0,0,0},100);
    Check(!tracks.RunSingle({21,1},"002",false).suspend);Check(tracks.Advance(0).empty());
    Check(tracks.Advance(.25f).empty());const auto waiting_pose=cameras.Pose();
    Check(tracks.WaitCurrent({22,1}).suspend && tracks.Playing() && tracks.Advance(.25f).empty());
    Check(cameras.Pose().distance>waiting_pose.distance && tracks.Playing());
    Check(tracks.Advance(.5f).empty());done=tracks.Advance(0);
    Check(done.size()==1 && done[0].token==ScriptToken{22,1} && !tracks.Playing() && tracks.Advance(0).empty());
    cameras.SetParameters(0,{0,0,0},{0,0,0},100);
    Check(tracks.RunSingle({30,1},"003",true).suspend);Check(tracks.Advance(0).empty());Check(tracks.Advance(.5f).empty());
    mid=cameras.Pose();Check(Near(mid.eye[0],5) && Near(mid.eye[1],10) && Near(mid.eye[2],-135));
    cameras.Update(0,{});Check(cameras.Pose().eye==mid.eye);Check(tracks.Advance(.5f).empty());done=tracks.Advance(0);Check(done.size()==1);
    Check(Near(cameras.Pose().eye[0],10) && Near(cameras.Pose().eye[2],-170));
    Check(tracks.RunSingle({40,1},"002",true).suspend);Check(!tracks.RunSingle({41,1},"001",false).suspend);
    Check(tracks.Advance(0).empty() && tracks.Advance(0).empty());
    done=tracks.Advance(0);Check(done.size()==1 && done[0].token.context==40 && done[0].error.empty() && !tracks.Playing());
    const auto missing=tracks.RunSingle({50,1},"missing",true);Check(!missing.suspend && std::get<std::int32_t>(missing.value)==0);
    Check(std::get<std::int32_t>(tracks.Prepare("Q01","N01","missing").value)==0);
    bool rejected=false;try{(void)tracks.Prepare("..","N01","MC001");}catch(const std::exception&){rejected=true;}Check(rejected);
    rejected=false;try{(void)tracks.Advance(-1);}catch(const std::exception&){rejected=true;}Check(rejected);
    {
        struct Services final:IScriptContextServices {
            CameraTracks& tracks;
            explicit Services(CameraTracks& t):tracks(t){}
            ScriptResponse Invoke(const ScriptRequest& r) override {
                if(r.call.system_id==287)return tracks.Prepare("Q01","N01",std::get<std::string>(r.call.arguments[0]));
                if(r.call.system_id==288)return tracks.RunSingle(r.token,std::get<std::string>(r.call.arguments[0]),std::get<bool>(r.call.arguments[1]));
                if(r.call.system_id==290)return tracks.WaitCurrent(r.token);
                throw std::logic_error("unexpected camera test service");
            }
        } services(tracks);
        ScriptGlobals globals{};ScriptStack stack(globals);
        stack.Compile("camera","void run(){giCameraPrepare(\"MC001\");giCameraRunSingle(\"001\",true);}");
        Check(bool(stack.Push("camera","void run()",services)) && stack.Tick(0)==ScriptState::suspended);
        Check(stack.WaitKind()==ScriptWaitKind::external && tracks.Advance(0).empty() && tracks.Advance(0).empty());
        done=tracks.Advance(0);Check(done.size()==1 && stack.Complete(done[0].token));
        Check(!stack.Complete(done[0].token) && stack.Tick(0)==ScriptState::finished);
        stack.PopFinished();
        stack.Compile("wait","void run(){giCameraPrepare(\"MC001\");giCameraRunSingle(\"002\",false);giCameraWait();global_mvar=7;}");
        Check(bool(stack.Push("wait","void run()",services)) && stack.Tick(0)==ScriptState::suspended && globals[0]==0);
        Check(tracks.Advance(0).empty() && tracks.Advance(.5f).empty() && globals[0]==0);
        Check(tracks.Advance(.5f).empty());done=tracks.Advance(0);
        Check(done.size()==1 && stack.Complete(done[0].token) && stack.Tick(0)==ScriptState::finished && globals[0]==7);
    }
    std::cout<<"camera original task phases, immediate/interpolated/eye anchor, wait completion and shared-container replacement passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
