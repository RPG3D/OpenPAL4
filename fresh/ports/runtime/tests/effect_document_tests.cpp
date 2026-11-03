#include "pal4/portable/effect_document.h"
#include "pal4/portable/scene_data.h"
#include "pal4/portable/package.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool v){if(!v)throw std::runtime_error("effect document contract failed");}
BinaryXmlConfig Config(BinaryXmlNode root){const auto bytes=BinaryXmlConfig::Serialize(root);Check(bool(bytes));BinaryXmlConfig x;Check(x.Parse(*bytes));return x;}
template<class F> void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed);}
}
int main(int argc,char** argv){try{
    if(argc==3){
        std::ifstream cpk(argv[1],std::ios::binary);Check(bool(cpk));PackageReader package(cpk);
        EffectDocuments documents([&](auto member)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(member))return {};return package.Read(member);});
        {
            const auto definition=documents.Load(argv[2]);const auto plan=ReadEffectPlan(*definition);
            std::cout<<"effect="<<definition->name<<" bytes="<<definition->source_bytes<<" objects="<<plan.object_ids.size()<<" tasks="<<plan.task_ids.size()<<" templates="<<plan.templates.size()<<'\n';
            for(const auto& [id,node]:plan.templates)std::cout<<"template="<<id<<" type="<<node.name<<'\n';
        }
        Check(documents.Size()==0);return 0;
    }
    if(argc==4){
        std::ifstream gobs(argv[1],std::ios::binary),cpk(argv[2],std::ios::binary);Check(bool(gobs) && bool(cpk));
        PackageReader package(cpk);EffectDocuments documents([&](auto member)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(member))return {};return package.Read(member);});
        const std::filesystem::path output(argv[3]);std::filesystem::create_directories(output);
        std::ofstream index(output/"gob-effects.tsv");index<<"object\teffect\ttimes\tsaver\tdistance_clip\tvisible\tbytes\n";std::size_t processed=0;
        for(const auto& gob:ReadSceneGobs(gobs))if(gob.type==8){
            const auto config=ReadGobEffectParameters(gob.config);if(!config)throw std::runtime_error("effect config missing for "+gob.name);
            ++processed;
            const auto definition=documents.Load(config->name);const auto plan=ReadEffectPlan(*definition);
            std::ofstream report(output/(config->name+".json"));WriteEffectDocumentReport(report,*definition);
            std::ofstream graph(output/(config->name+"-plan.tsv"));graph<<"objects\t"<<plan.object_ids.size()<<"\ntasks\t"<<plan.task_ids.size()<<"\nparameters\t"<<plan.task_parameters.size()<<"\ntemplates\t"<<plan.templates.size()<<"\n";
            for(std::size_t i=0;i<plan.object_ids.size();++i)graph<<"object\t"<<i<<'\t'<<plan.object_ids[i]<<'\n';
            for(std::size_t i=0;i<plan.task_ids.size();++i)graph<<"task\t"<<i<<'\t'<<plan.task_ids[i]<<'\t'<<plan.task_parameters.at(i).name<<'\n';
            const auto bytes=package.Read(definition->member);std::ofstream raw(output/(config->name+".eft"),std::ios::binary);raw.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
            index<<gob.name<<'\t'<<config->name<<'\t'<<config->times<<'\t'<<config->saver<<'\t'<<config->distance_clip<<'\t'<<gob.flags[0]<<'\t'<<definition->source_bytes<<'\n';
        }
        std::cout<<"original effect records: "<<processed<<'\n';return 0;
    }
    BinaryXmlNode root;root.name="PAL4-GameObject-Parameters";Check(!ReadGobEffectParameters(Config(root)));
    root.children.push_back({"PAL4_GameObject-effect",{}, {}});auto defaults=ReadGobEffectParameters(Config(root));Check(defaults && defaults->name.empty() && defaults->times==0 && defaults->saver==0 && defaults->distance_clip==1);
    root.children[0].attributes={{"PAL4_GameObject-effect-name",3,std::string("H_077")},{"PAL4_GameObject-effect-times",1,std::int32_t(-1)},{"PAL4_GameObject-effect-saver",1,std::int32_t(1)},{"PAL4_GameObject-effect-distance-clip",1,std::int32_t(0)}};
    const auto values=ReadGobEffectParameters(Config(root));Check(values && values->name=="H_077" && values->times==0xffffffffu && values->saver==1 && values->distance_clip==0);
    BinaryXmlNode wrapper;wrapper.name="document";wrapper.children.push_back(root);Check(ReadGobEffectParameters(Config(wrapper))->name=="H_077");
    wrapper.name="unrelated";root.children[0].children.push_back(wrapper);Check(!ReadGobEffectParameters(Config(BinaryXmlNode{"unrelated",{},{{"nested",{},{root}}}})));
    const auto source=BinaryXmlConfig::Serialize(root);unsigned reads{};bool broken=false,missing=false;
    EffectDocuments documents([&](auto member)->std::optional<std::vector<std::uint8_t>>{Check(member=="script\\H_077.eft");++reads;if(missing)return {};if(broken)return std::vector<std::uint8_t>{1,2,3};return source;});
    broken=true;Reject([&]{documents.Load("H_077");});Check(documents.Size()==0);
    broken=false;missing=true;Reject([&]{documents.Load("H_077");});Check(documents.Size()==0);
    missing=false;auto document=documents.Load("H_077");Check(reads==3 && document->source_bytes==source->size() && documents.Load("H_077")==document && reads==3);
    Reject([&]{documents.Load("../H_077");});Reject([&]{documents.Load("");});Check(reads==3);
    std::ostringstream report;WriteEffectDocumentReport(report,*document);Check(report.str().find("\"playback_bound\":false")!=std::string::npos && report.str().find("H_077")!=std::string::npos);
    std::weak_ptr<const EffectDocument> released=document;document.reset();Check(released.expired() && documents.Size()==0);
    document=documents.Load("H_077");Check(reads==4 && document->xml.Root().name==root.name);
    EffectDocument effect;effect.member="fixture";
    BinaryXmlNode e;e.name="eFfEcT";
    BinaryXmlNode objects{"ObjectList",{},{{"Object",{{"ObjectId",3,std::string("smoke")}}, {}}}};
    BinaryXmlNode tasks{"TaskList",{},{{"TASK",{{"TaskId",1,std::int32_t(0)}}, {}}}};
    BinaryXmlNode params{"TaskParamList",{},{{"ShowobjTask",{}, {}}}};
    BinaryXmlNode templates{"OxnList",{},{{"RtkPrtStdAtomicObject",{{"Attr_Label",3,std::string("smoke")}}, {}}}};
    e.children={objects,tasks,params,templates};e.attributes={{"CameraStart",2,-2.f},{"DistanceClip",1,std::int32_t(-1)}};effect.xml=Config(e);
    auto plan=ReadEffectPlan(effect);Check(plan.object_ids==std::vector<std::string>{"smoke"} && plan.task_ids==std::vector<std::int32_t>{0} && plan.task_parameters[0].name=="ShowobjTask" && plan.templates.contains("smoke") && !plan.distance_clip && plan.camera_start==0);
    e.attributes.clear();effect.xml=Config(e);plan=ReadEffectPlan(effect);Check(plan.distance_clip && plan.camera_start==1);
    e.children[3].children.push_back(e.children[3].children.front());effect.xml=Config(e);Reject([&]{ReadEffectPlan(effect);});
    e.children.pop_back();effect.xml=Config(e);Reject([&]{ReadEffectPlan(effect);});
    std::cout<<"effect config scope/defaults, original parser, cache lifetime and failed-load retry passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
