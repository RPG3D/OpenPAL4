#include "pal4/portable/effect_tasks.h"
#include "pal4/portable/effect_cylinder.h"
#include "pal4/portable/package.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4;using namespace pal4::portable;
namespace {
void Check(bool v){if(!v)throw std::runtime_error("effect task scheduling contract failed");}
template<class F> void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed);}
struct Target final:EffectVisualTarget {
    bool visible{};std::array<float,3> scale{1,1,1};std::array<std::uint8_t,4> color{251,238,0,254};unsigned shows{},hides{};
    void Show(bool v) override{visible=v;if(v)++shows;else ++hides;}
    std::array<float,3> Scale() const override{return scale;}
    void SetScale(std::array<float,3> v,bool additive) override{for(unsigned i=0;i<3;++i)scale[i]=v[i]+(additive?scale[i]:0);}
    std::array<std::uint8_t,4> Color() const override{return color;}
    void SetColor(std::array<std::uint8_t,4> v) override{color=v;}
    void SetColorMode(std::uint8_t) override{}
};
BinaryXmlNode Config(std::string name,int mode,float wait,float duration){BinaryXmlNode n;n.name=name;n.attributes={{"Attr_Class_Name",3,name},{"Attr_Task_Wait_Mode",1,std::int32_t(mode)},{"Attr_Task_Wait_Time",2,wait},{"Attr_Task_Expect_Time",2,duration}};return n;}
std::vector<EffectTaskBinding> CylinderBindings(const EffectPlan& p,EffectCylinder& cylinder){
    Check(p.task_ids.size()==p.task_parameters.size());std::vector<EffectTaskBinding> b;
    for(std::size_t i=0;i<p.task_ids.size();++i)if(p.task_ids[i]==2)b.push_back({p.task_parameters[i],&cylinder});Check(b.size()==4);return b;
}
}
int main(int argc,char** argv){try{
    if(argc==3){
        std::ifstream stream(argv[1],std::ios::binary);PackageReader package(stream);auto reader=[&](std::string_view member){return package.Read(member);};
        EffectDocuments documents(reader);EffectTextures textures(reader);EffectObjectCounts counts;
        const auto document=documents.Load("H_077");const auto plan=ReadEffectPlan(*document);
        EffectCylinder cylinder(plan.templates.at("yzt3"),textures,counts);EffectTasks tasks(CylinderBindings(plan,cylinder));
        std::ofstream out(argv[2]);out<<"{\"projection\":\"H_077/yzt3 only\",\"frames\":[";
        bool first=true;unsigned frames=0;float max_x=0;bool partial_alpha=false;
        while(!tasks.Finished() && frames<180){tasks.Advance(1.f/60);cylinder.Advance(1./60);++frames;max_x=std::max(max_x,cylinder.Scale()[0]);const auto alpha=cylinder.Color()[3];partial_alpha|=alpha>0 && alpha<254;
            if(!first)out<<',';first=false;out<<"{\"frame\":"<<frames<<",\"object\":";cylinder.WriteReport(out);out<<",\"scheduler\":";tasks.WriteReport(out);out<<'}';}
        out<<"],\"final\":";tasks.WriteReport(out);out<<",\"chapter_gameplay_validated\":false}";
        Check(bool(out) && tasks.Finished() && !cylinder.Visible() && cylinder.Color()[3]==0 && max_x>10 && partial_alpha && counts.created==1);
        std::cout<<"original H_077 cylinder four tasks completed in "<<frames<<" frames\n";return 0;
    }
    Target target;auto show=Config("ShowobjTask",2,0,0);auto color=Config("ChangeColorTask",2,.1f,.2f);
    color.attributes.push_back({"Attr_Task_ChangeColor_Alpha",1,std::int32_t(1)});auto hide=Config("HideobjTask",1,0,0);
    EffectTasks tasks({{show,&target},{color,&target},{hide,&target}});
    Check(!tasks.Finished() && !target.visible);tasks.Advance(.05f);Check(target.visible && target.shows==1 && target.color[3]==254);
    tasks.Advance(.05f);Check(target.visible && target.color[3]==254);tasks.Advance(.05f);Check(target.visible && target.color[3]<254 && target.color[3]>0);
    for(int i=0;i<20 && !tasks.Finished();++i)tasks.Advance(.05f);
    Check(tasks.Finished() && !target.visible && target.hides==1 && target.color==std::array<std::uint8_t,4>{251,238,0,0});
    std::ostringstream report;tasks.WriteReport(report);Check(report.str().find("\"deleted\":3")!=std::string::npos);
    auto scale=Config("ScaleTask",2,0,.2f);scale.attributes.insert(scale.attributes.end(),{{"Attr_Task_Scale_Result",1,std::int32_t(1)},{"Attr_Task_Scale_x",2,2.f},{"Attr_Task_Scale_y",2,3.f},{"Attr_Task_Scale_z",2,4.f}});
    {EffectTasks scaling({{scale,&target}});for(int i=0;i<20 && !scaling.Finished();++i)scaling.Advance(.05f);Check(scaling.Finished() && target.scale==std::array<float,3>{2,3,4});}
    Reject([&]{EffectTasks bad({{Config("UnknownTask",2,0,0),&target}});});Reject([&]{EffectTasks bad({{show,nullptr}});});
    Reject([&]{tasks.Advance(-1);});{EffectTasks stopped({{Config("ShowobjTask",2,0,-1),&target}});stopped.Advance(.1f);Check(!stopped.Finished());stopped.Stop();Check(stopped.Finished());}
    // 后一任务继承前一任务的等待/预期时间，而非自己的XML值。
    auto inherited=scale;inherited.attributes.push_back({"Attr_Inherit_Pre_Task",1,std::int32_t(1)});
    EffectTasks inheritance({{Config("ShowobjTask",2,.3f,.7f),&target},{inherited,&target}});
    std::ostringstream inheritance_report;inheritance.WriteReport(inheritance_report);Check(inheritance_report.str().find("\"expect_time\":0.699999988")!=std::string::npos);
    std::cout<<"original Dialog initialization/GOM phases, visual facets, timing inheritance and cleanup passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
