#include "pal4/portable/effect_cylinder.h"
#include "pal4/portable/package.h"
#include "pal4/xml_float_to_int.h"
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool v){if(!v)throw std::runtime_error("effect cylinder owner contract failed");}
template<class F> void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed);}
BinaryXmlNode Fixture(){
    BinaryXmlNode node;node.name="CylinderObject";
    node.attributes={{"Attr_Cylinder_Texture",3,std::string("test.png")},{"Attr_Cylinder_BorderNumb",1,std::int32_t(8)},
      {"Attr_Cylinder_Asterialize",1,std::int32_t(0)},{"Attr_Cylinder_Center",1,std::int32_t(0)},
      {"Attr_Cylinder_Angel",2,360.f},{"Attr_Cylinder_Position_x",2,20.f},{"Attr_Cylinder_Scale_x",2,.2f},
      {"Attr_Cylinder_Vertex_Col_Red",1,std::int32_t(251)},{"Attr_Cylinder_Vertex_Col_Green",1,std::int32_t(238)},
      {"Attr_Cylinder_Vertex_Col_Blue",1,std::int32_t(0)},{"Attr_Auto_AlphaBlend",1,std::int32_t(0)}};return node;
}
std::vector<std::uint8_t> Png(){
    const std::string hex="89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c4890000000d49444154789c63f8ffffff7f0009fb03fd2a86e38a0000000049454e44ae426082";
    std::vector<std::uint8_t> bytes;for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));return bytes;
}
}
int main(int argc,char** argv){try{
    if(argc==3 || argc==4){
        std::ifstream input(argv[1],std::ios::binary);PackageReader package(input);
        const EffectMemberReader reader=[&](auto member)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(member))return {};return package.Read(member);};
        EffectDocuments documents(reader);EffectTextures textures(reader);EffectObjectCounts counts;
        const auto effect=documents.Load("H_077");const auto plan=ReadEffectPlan(*effect);const auto& node=plan.templates.at("yzt3");
        {EffectCylinder cylinder(node,textures,counts);Check(counts.cylinders==1 && !cylinder.Visible() && !cylinder.Additive());cylinder.Show(true);Check(cylinder.Mesh().vertices.size()==48 && cylinder.Color()==std::array<std::uint8_t,4>{251,238,0,254});
         std::ofstream out(argv[2]);cylinder.WriteReport(out);Check(bool(out));
         if(argc==4){const auto& bytes=cylinder.Mesh().textures[0].rgba;std::ofstream raw(argv[3],std::ios::binary);raw.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));Check(bool(raw));}}
        Check(counts.cylinders==0);return 0;
    }
    unsigned reads{};bool broken=true;EffectTextures textures([&](auto path)->std::optional<std::vector<std::uint8_t>>{if(path=="tex\\test.dds")return {};Check(path=="tex\\test.png");++reads;if(broken)return std::vector<std::uint8_t>{1,2,3};return Png();});
    Reject([&]{textures.Load("gamedata\\effect\\tex\\test.png");});Check(textures.Size()==0);broken=false;
    auto texture=textures.Load("gamedata/effect/tex/test.png");Check(texture->image.width==1 && texture->image.rgba==std::vector<std::uint8_t>(4,255));
    Check(textures.Load("gamedata\\effect\\tex\\test.png")==texture && reads==2);Reject([&]{textures.Load("gamedata\\effect\\tex\\..\\test.png");});
    EffectObjectCounts counts;const auto node=Fixture();
    {
        EffectCylinder cylinder(node,textures,counts);EffectCylinder second(node,textures,counts);Check(counts.cylinders==2 && reads==2);
        Check(!cylinder.Visible() && !cylinder.Additive() && cylinder.Color()==std::array<std::uint8_t,4>{251,238,0,254});
        cylinder.Show(true);Check(cylinder.Appearance()[0].visible && cylinder.Mesh().vertices.size()==48);
        Check(std::abs(cylinder.Transform()[0]-.2f)<1e-6 && cylinder.Transform()[12]==20);
        cylinder.SetScale({2,3,4});cylinder.SetRotation({0,90,0});cylinder.SetPosition({10,20,30});
        const auto matrix=cylinder.Transform();Check(std::abs(matrix[0])<1e-5 && std::abs(matrix[2]+2)<1e-5 && matrix[5]==3 && std::abs(matrix[8]-4)<1e-5 && matrix[12]==10 && matrix[13]==20 && matrix[14]==30);
        cylinder.SetPosition({1,2,3},true);Check(cylinder.Transform()[12]==11 && cylinder.Transform()[14]==33);
        cylinder.SetColor({20,30,40,100});cylinder.SetTimedMode(true);cylinder.Advance(.1);Check(cylinder.Color()==std::array<std::uint8_t,4>{20,30,40,87});
        cylinder.Show(false);cylinder.Advance(1);Check(cylinder.Color()[3]==87 && !cylinder.Visible());
        cylinder.Reset();Check(cylinder.Color()==std::array<std::uint8_t,4>{251,238,0,254} && cylinder.Transform()[12]==20 && !cylinder.Visible());
        Reject([&]{cylinder.SetScale({std::numeric_limits<float>::infinity(),1,1});});Reject([&]{cylinder.Advance(-1);});
    }
    Check(counts.cylinders==0);auto bad=node;bad.attributes.push_back({"Attr_Cylinder_Height",2,std::numeric_limits<float>::infinity()});Reject([&]{EffectCylinder cylinder(bad,textures,counts);});Check(counts.cylinders==0);
    std::weak_ptr<const EffectTexture> released=texture;texture.reset();Check(released.expired() && textures.Size()==0);
    auto reloaded=textures.Load("gamedata\\effect\\tex\\test.png");Check(reads==3 && reloaded->image.rgba==std::vector<std::uint8_t>(4,255));
    Check(XmlFloatToInt(std::bit_cast<unsigned>(360.f))==360 && XmlFloatToInt(std::bit_cast<unsigned>(-2.75f))==-2 && XmlFloatToInt(std::bit_cast<unsigned>(4294967296.f))==0 && XmlFloatToInt(std::bit_cast<unsigned>(std::numeric_limits<float>::infinity()))==0);
    std::cout<<"original cylinder owner, texture cache, facets, transform, timed alpha, reset and rollback passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
