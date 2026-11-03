#include "pal4/portable/effect_particle.h"
#include "pal4/portable/package.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <source_location>
#include <sstream>
#include <stdexcept>
using namespace pal4;using namespace pal4::portable;
namespace {
void Check(bool v,std::source_location where=std::source_location::current()){if(!v)throw std::runtime_error("original particle adapter contract failed at "+std::to_string(where.line()));}
template<class F> void Reject(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}Check(caught);}
struct Random final:ParticleRandom {std::mt19937 engine;unsigned seeds{},calls{};unsigned Time() override{return 1234;}void Seed(unsigned n) override{engine.seed(n);++seeds;}int Next15() override{++calls;return static_cast<int>(engine() & 32767);}};
BinaryXmlNode Fixture(){BinaryXmlNode n;n.name="RtkPrtStdAtomicObject";n.attributes={{"Attr_Prt_Texture",3,std::string("test.png")},{"Attr_Prt_MaxPrt",1,std::int32_t(8)},{"Attr_Prt_PrtEmit",1,std::int32_t(2)},{"Attr_Prt_PrtEmit_Bias",1,std::int32_t(0)},{"Attr_Prt_EmitGap",2,1.f},{"Attr_Prt_PrtLife",2,.5f},{"Attr_Prt_PrtLife_Bias",2,0.f},{"Attr_Prt_InitVel",2,2.f},{"Attr_Prt_InitDir_y",2,1.f},{"Attr_Prt_ColorGradual",1,std::int32_t(0)},{"Attr_Prt_Atomic_Position_x",2,10.f},{"Attr_Prt_Atomic_Position_y",2,20.f}};return n;}
std::vector<std::uint8_t> Png(){const std::string hex="89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c4890000000d49444154789c63f8ffffff7f0009fb03fd2a86e38a0000000049454e44ae426082";std::vector<std::uint8_t> b;for(std::size_t i=0;i<hex.size();i+=2)b.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));return b;}
}
int main(int argc,char** argv){try{
    if(argc==3 || argc==4){std::vector<double> deltas(180,1./60);if(argc==4){deltas.clear();std::ifstream replay(argv[3]);double dt;while(replay>>dt){Check(std::isfinite(dt)&&dt>=0&&deltas.size()<4096);deltas.push_back(dt);}Check(!deltas.empty());}std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);auto read=[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(member))return {};return package.Read(member);};EffectDocuments documents(read);EffectTextures textures(read);const auto plan=ReadEffectPlan(*documents.Load("H_077"));ParticleCounts counts;Random random;std::ofstream out(argv[2]);out<<"{\"templates\":[";bool first=true;
        for(const auto& name:{"P_01","lz"}){{EffectParticle particle(plan.templates.at(name),textures,random,counts);particle.Show(true);unsigned maximum=0;bool variation=false;out<<(first?"":",")<<"{\"name\":\""<<name<<"\",\"frames\":[";first=false;
            for(std::size_t i=0;i<deltas.size();++i){particle.Advance(deltas[i]);const auto snapshot=particle.Snapshot();maximum=std::max(maximum,static_cast<unsigned>(std::count_if(snapshot.begin(),snapshot.end(),[](const auto& p){return p.color[3]>0;})));for(const auto& p:snapshot)variation|=p.color[3]>0 && p.color[3]<255;if(i)out<<',';particle.WriteReport(out);}
            Check(maximum>0 && variation && particle.Capacity()==100 && particle.SourceBlend()==5 && particle.DestinationBlend()==6);particle.SetEmission(false);for(unsigned i=0;i<150;++i)particle.Advance(1./60);for(const auto& p:particle.Snapshot())Check(p.color[3]==0);
            out<<"],\"drained\":";particle.WriteReport(out);out<<'}';}Check(counts.instances==0);}
        out<<"],\"created\":"<<counts.created<<",\"random_calls\":"<<random.calls<<",\"original_sdk_random_equivalence\":false,\"chapter_gameplay_validated\":false}";Check(bool(out));std::cout<<"two original H_077 particle configurations/emission/simulation/streams/drain passed\n";return 0;
    }
    EffectTextures textures([](std::string_view path)->std::optional<std::vector<std::uint8_t>>{if(path.ends_with(".dds"))return {};return Png();});ParticleCounts counts;Random random;
    {EffectParticle particle(Fixture(),textures,random,counts);Check(!particle.Visible());particle.Advance(.5);Check(particle.Snapshot().empty());particle.Show(true);Check(counts.instances==1 && random.seeds==1 && particle.SourceBlend()==5 && particle.DestinationBlend()==2);particle.Advance(.1);const auto initial=particle.Snapshot();Check(initial.size()==2 && initial[0].color[3]>0 && std::abs(initial[0].position[0]-10)<1e-5);
        particle.Show(false);particle.Advance(2);Check(particle.Snapshot()[0].position==initial[0].position);particle.Show(true);particle.SetEmission(false);particle.Advance(.2);Check(particle.Snapshot()[0].position[1]>initial[0].position[1]);particle.Advance(.3);for(const auto& p:particle.Snapshot())Check(p.color[3]==0);
        particle.SetEmission(true);particle.Advance(1);particle.Advance(0);Reject([&]{particle.Advance(-1);});
        auto scene=CreateParticleScene(particle);const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};UpdateParticleScene(scene,particle,identity);Check(scene.vertices.size()==48 && scene.textures.size()==1);particle.Reset();Check(!particle.Visible());particle.SetEmissionTime(1,false);std::ostringstream decreasing;particle.WriteReport(decreasing);Check(decreasing.str().find("\"emission_rate\":8")!=std::string::npos);Reject([&]{particle.SetEmissionTime(0,false);});particle.Reset();particle.Show(true);particle.SetTimedMode(true);particle.Advance(.1);for(const auto& p:particle.Snapshot())Check(p.color[3]==0);
    }Check(counts.instances==0);
    auto bad=Fixture();for(auto& a:bad.attributes)if(a.name=="Attr_Prt_MaxPrt")a.value=std::int32_t(5000);Reject([&]{EffectParticle particle(bad,textures,random,counts);});Check(counts.instances==0);
    auto collection=Fixture();collection.attributes.push_back({"Attr_Prt_Is_Billboard",1,std::int32_t(1)});Reject([&]{EffectParticle particle(collection,textures,random,counts);});Check(counts.instances==0);
    std::cout<<"original particle configuration, transform, emission/simulation, dead slots, scene conversion and rollback passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
