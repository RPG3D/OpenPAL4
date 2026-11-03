#include "pal4/portable/minimap.h"
#include "pal4/portable/package.h"
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("minimap contract failed");}
void Word(std::vector<std::uint8_t>& out,unsigned value){for(unsigned b=0;b<4;++b)out.push_back(static_cast<std::uint8_t>(value>>(b*8)));}
std::vector<std::uint8_t> Layers(unsigned count=1,unsigned scale=2) {
    std::vector<std::uint8_t> out;Word(out,count);
    for(unsigned i=0;i<count;++i){Word(out,scale);for(float value:{0.f,0.f,0.f,10.f,0.f,-20.f})Word(out,std::bit_cast<unsigned>(value));}return out;
}
std::vector<std::uint8_t> Image() {
    // 64x64 synthetic RGBA(30,80,120,0), generated with Python zlib, no game data.
    const std::string hex="89504e470d0a1a0a0000000d4948445200000040000000400806000000aa6971de000000ac49444154789ce5ce210100000803b0272113711e9f181388f965b6f98c07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed07840e3018d07341ed0a203da01e0d860d363ce320c0000000049454e44ae426082";
    std::vector<std::uint8_t> out;for(std::size_t i=0;i<hex.size();i+=2)out.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));return out;
}
using Files=std::map<std::string,std::vector<std::uint8_t>>;
MinimapMemberReader Reader(const Files& files){return [&files](std::string_view member)->std::optional<std::vector<std::uint8_t>>{const auto i=files.find(std::string(member));if(i==files.end())return {};return i->second;};}
void Rejected(const Files& files){bool failed=false;try{SceneMinimap map("Q01","Q01",Reader(files));}catch(const std::runtime_error&){failed=true;}Check(failed);}
}
int main(int argc,char** argv) {
    try {
        if(argc==10){
            std::ifstream scenes(argv[1],std::ios::binary),ui(argv[2],std::ios::binary);PackageReader scene_package(scenes),ui_package(ui);
            const auto reader=[](PackageReader& package){return [&package](auto name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);};};
            SceneMinimap map("Q01","Q01",reader(scene_package),reader(ui_package));
            map.UpdatePlayer({std::stof(argv[4]),std::stof(argv[5]),std::stof(argv[6])},{std::stof(argv[7]),std::stof(argv[8]),std::stof(argv[9])});
            std::ofstream output(argv[3]);map.WriteReport(output);Check(bool(output));return 0;
        }
        if(argc==3){
            std::ifstream stream(argv[1],std::ios::binary);PackageReader package(stream);
            SceneMinimap map("Q01","Q01",[&](auto name)->std::optional<std::vector<std::uint8_t>>{if(!package.Find(name))return {};return package.Read(name);});
            Check(map.ActiveImage()!=nullptr);std::ofstream output(argv[2]);map.WriteReport(output);Check(bool(output));
            std::cout<<"original Q01 minimap "<<map.ActiveImage()->width<<'x'<<map.ActiveImage()->height<<'\n';return 0;
        }
        Files files{{"Q01\\Q01\\MiniMap.df",Layers()},{"Q01\\Q01\\MiniMap_0.png",Image()}};
        SceneMinimap map("Q01","Q01",Reader(files));const auto* image=map.ActiveImage();Check(image && image->width==64 && image->height==64 && image->rgba.size()==64*64*4);
        Check(image->rgba[0]==30 && image->rgba[1]==80 && image->rgba[2]==120 && image->rgba[3]==0);
        std::vector<MinimapObject> objects{{"entry",{15,999,-13},3,1,1},{"hidden",{},3,0,1},{"used",{},3,1,0},{"unmarked",{},0,1,1},{"unknown",{},4,1,1}};
        map.RefreshObjects(objects);Check(map.Icons().size()==1 && map.Icons()[0].name=="entry" && map.Icons()[0].image=="Entry.PNG" && map.Icons()[0].x==2 && map.Icons()[0].y==3);
        objects[0].position={5,0,-27};map.RefreshObjects(objects);Check(map.Icons()[0].x==-2 && map.Icons()[0].y==-3);
        objects[0].visible=0;map.RefreshObjects(objects);Check(map.Icons().empty());
        objects[0].visible=1;objects[0].uses=0;map.RefreshObjects(objects);Check(map.Icons().empty());
        objects.clear();for(unsigned marker:{2,3,5,6,7,8,9,10})objects.push_back({std::to_string(marker),{},marker,1,1});
        map.RefreshObjects(objects);Check(map.Icons().size()==8 && map.Icons()[0].image=="Grocer.PNG" && map.Icons()[7].image=="SaveSpot.png");
        Check(map.RevealAt({74,0,44}) && image->rgba[(32*64+32)*4+3]==254);
        Check(image->rgba[(32*64+57)*4+3]==126 && image->rgba[(32*64+63)*4+3]==0);
        Check(!map.RevealAt({-1000,0,-1000}) && image->rgba[(32*64+32)*4+3]==254);
        Files empty;SceneMinimap absent("Q01","N01",Reader(empty));Check(!absent.ActiveImage() && !absent.RevealAt({}));absent.RefreshObjects({});Check(absent.Icons().empty());
        absent.UpdatePlayer({1,2,3},{4,5,6});Check(!absent.Player().valid && !absent.PlayerImage());
        const auto icon=[](std::string_view member)->std::optional<std::vector<std::uint8_t>>{Check(member=="minimapIcon\\actor.png");return Image();};
        SceneMinimap player("Q01","Q01",Reader(files),icon);
        player.UpdatePlayer({15,999,-13},{1,318,3});const auto first=player.Player();
        Check(first.valid && first.grid_x==2 && first.grid_y==3 && first.left==-30 && first.top==-29);
        Check(first.direction_bits==std::array<unsigned,3>{std::bit_cast<unsigned>(1.f),std::bit_cast<unsigned>(318.f),std::bit_cast<unsigned>(3.f)});
        Check(player.PlayerImage()->width==64 && player.ActiveImage()->rgba[(3*64+2)*4+3]==254);
        player.UpdatePlayer({138,0,108},{0,0,0});Check(player.Player().grid_x==first.grid_x && player.Player().direction_bits==first.direction_bits);
        std::ostringstream player_report;player.WriteReport(player_report);Check(player_report.str().find("\"calls\":2,\"updates\":1")!=std::string::npos);
        player.ConfigureViewport(8,8);
        Check(player.ViewportImage() && player.ViewportImage()->width==8 && player.ViewportImage()->height==8);
        Check(player.ViewportRegion()==std::array<std::int32_t,4>{-2,-1,8,8});
        const auto viewport=player.ViewportImage()->rgba;
        for(unsigned x=0;x<8*4;++x)Check(viewport[x]==0);
        Check(viewport[(1*8+2)*4]==30 && viewport[(1*8+2)*4+1]==80);
        Check(viewport[(4*8+4)*4+3]==player.ActiveImage()->rgba[(3*64+2)*4+3]);
        const auto revision=player.ViewportRevision();player.ConfigureViewport(8,8);Check(player.ViewportRevision()==revision);
        player.UpdatePlayer({25,0,-3},{0,90,0});
        Check(player.ViewportRevision()>revision && player.ViewportRegion()==std::array<std::int32_t,4>{3,4,8,8});
        player.ConfigureViewport(128,128);Check(player.ViewportImage()->rgba.size()==128*128*4);
        Check(player.ViewportImage()->rgba[(64*128+64)*4]==30);
        bool invalid_viewport=false;try{player.ConfigureViewport(0,8);}catch(const std::runtime_error&){invalid_viewport=true;}
        Check(invalid_viewport && player.ViewportImage()->width==128);
        absent.ConfigureViewport(8,8);Check(!absent.ViewportImage());
        SceneMinimap full("Q01","Q01",Reader(files),icon);
        full.SetSceneType(0);Check(full.DisplayMode()==1);
        full.UpdatePlayer({15,0,-13},{0,174,0});Check(full.ActiveImage()->rgba[(3*64+2)*4+3]==0);
        full.ConfigureViewport(8,8);Check(!full.ViewportHasAlpha());
        const auto full_revision=full.ViewportRevision();full.SetSceneType(2);
        Check(full.DisplayMode()==2 && full.ViewportHasAlpha() && full.ViewportRevision()>full_revision);
        full.UpdatePlayer({15,0,-13},{0,174,0});Check(full.ViewportImage()->rgba[(4*8+4)*4+3]==254);
        full.SetSceneType(1);Check(full.DisplayMode()==0 && !full.ViewportHasAlpha());
        full.SetSceneType(3);Check(full.DisplayMode()==0); // Original switch has no branch for >2.
        auto negative=files;negative["Q01\\Q01\\MiniMap.df"]=Layers(1,static_cast<unsigned>(-2));SceneMinimap reversed("Q01","Q01",Reader(negative),icon);
        reversed.UpdatePlayer({5,0,-27},{0,180,0});Check(reversed.Player().grid_x==2 && reversed.Player().grid_y==3);
        bool unbound=false;try{map.UpdatePlayer({},{ });}catch(const std::runtime_error&){unbound=true;}Check(unbound);
        auto bad=files;bad.begin()->second=Layers(1,0);Rejected(bad);
        for(std::size_t n=0;n<files.begin()->second.size();++n){bad=files;bad.begin()->second.resize(n);Rejected(bad);}
        bad=files;bad["Q01\\Q01\\MiniMap_0.png"][45]^=1;Rejected(bad);
        files["Q01\\Q01\\MiniMap.df"]=Layers(2);SceneMinimap prefix("Q01","Q01",Reader(files));Check(prefix.ActiveImage() && prefix.ActiveImage()->width==64);
        std::ostringstream report;prefix.WriteReport(report);Check(report.str().find("\"missing_image\":true")!=std::string::npos && report.str().find("\"layers\":1")!=std::string::npos);
        bool failed=false;try{prefix.SelectLayer(1);}catch(const std::runtime_error&){failed=true;}Check(failed);
        std::cout<<"minimap original handlers, PNG, icons and fog contracts passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
