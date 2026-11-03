#include "pal4/portable/scene_data.h"
#include "pal4/portable/package.h"
#include <filesystem>
#include <bit>
#include <limits>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace {
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("scene data contract failed");}
void Word(std::string& b,std::uint32_t value){for(unsigned n=0;n<32;n+=8)b+=static_cast<char>(value>>n);}
void Text(std::string& b,std::string_view text){Word(b,static_cast<std::uint32_t>(text.size()+1));b+=text;b+='\0';}
std::string Xml(){pal4::BinaryXmlNode root;root.name="config";const auto bytes=pal4::BinaryXmlConfig::Serialize(root);Check(bool(bytes));return {bytes->begin(),bytes->end()};}
template<class F> void Reject(F read,const std::string& bytes){bool failed=false;try{std::istringstream input(bytes);(void)read(input);}catch(const std::exception&){failed=true;}Check(failed);}
template<class F> void Boundaries(F read,const std::string& bytes){for(std::size_t i=0;i<bytes.size();++i)Reject(read,bytes.substr(0,i));Reject(read,bytes+"x");}
template<class F> auto Load(const std::filesystem::path& path,F read){
    std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("missing scene file "+path.string());
    try{return read(input);}catch(const std::exception& error){throw std::runtime_error(path.string()+": "+error.what());}
}
}
int main(int argc,char** argv){
    try {
        if(argc==5 && std::string_view(argv[1])=="--monsters") {
            std::ifstream package_file(argv[2],std::ios::binary);PackageReader package(package_file);
            const auto bytes=package.Read(argv[3]);
            std::istringstream input(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
            SceneCatalog catalog;catalog.monsters=ReadSceneMonsters(input);
            std::ofstream output(argv[4]);WriteSceneCatalogReport(output,catalog);return 0;
        }
        if(argc==3 || argc==4){
            const std::filesystem::path directory(argv[1]);SceneCatalog c;
            c.gobs=Load(directory/"GameObjs.gob",ReadSceneGobs);c.npcs=Load(directory/"npcInfo.npc",ReadSceneNpcs);
            c.monsters=Load(directory/"mstInfo.mst",ReadSceneMonsters);
            c.paths=Load(directory/"pathway.pwy",ReadScenePaths);
            c.events=Load(directory/(directory.filename().string()+".evf"),ReadSceneEvents);
            std::vector<std::filesystem::path> cameras;
            for(const auto& f:std::filesystem::directory_iterator(directory))if(f.path().extension()==".cam")cameras.push_back(f.path());
            std::sort(cameras.begin(),cameras.end());
            for(const auto& path:cameras)c.cameras.emplace_back(path.filename().string(),Load(path,ReadSceneCameras));
            if(argc==4) {
                const std::filesystem::path target(argv[3]);std::filesystem::create_directories(target);
                for(std::size_t i=0;i<c.events.size();++i)if(!c.events[i].normalized_clump.empty()) {
                    std::ofstream file(target/("event_"+std::to_string(i)+".dff"),std::ios::binary);
                    const auto& bytes=c.events[i].normalized_clump;
                    file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
                    Check(bool(file));
                }
            }
            std::ofstream output(argv[2]);WriteSceneCatalogReport(output,c);return 0;
        }
        const auto monster_record=[](std::string_view name,std::uint32_t count){
            std::string b;Text(b,name);Text(b,"401");
            for(float f:{1.f,-2.f,3.f,0.f,73.f,180.f})Word(b,std::bit_cast<std::uint32_t>(f));
            for(auto value:{0x80000001u,2u,0u,0xffffffffu,count})Word(b,value);
            if(count<4)for(unsigned i=0;i<count;++i)Word(b,0x80000000u+i);
            b+=Xml();b+=Xml();return b;
        };
        std::string mst;Word(mst,1);mst+=monster_record("m01-01-01",2);
        std::istringstream monster_input(mst);const auto monsters=ReadSceneMonsters(monster_input);
        Check(monsters.size()==1 && monsters[0].name=="m01-01-01" && monsters[0].resource=="401");
        Check(monsters[0].position==std::array<float,3>{1,-2,3} && monsters[0].rotation==std::array<float,3>{0,73,180});
        Check(monsters[0].fields==std::array<std::uint32_t,5>{0x80000001u,2,0,0xffffffffu,2});
        Check(monsters[0].values==std::vector<std::uint32_t>{0x80000000u,0x80000001u});
        Check(monsters[0].behavior.Root().name=="config" && monsters[0].cache.Root().name=="config");
        Boundaries(ReadSceneMonsters,mst);
        auto mst_pair=mst;mst_pair[0]=2;mst_pair+=monster_record("m01-01-02",0);
        std::istringstream monster_pair(mst_pair);const auto pair_records=ReadSceneMonsters(monster_pair);
        Check(pair_records.size()==2 && pair_records[1].values.empty());
        Reject(ReadSceneMonsters,mst_pair+"x");
        auto duplicate=mst;duplicate[0]=2;duplicate+=monster_record("m01-01-01",0);Reject(ReadSceneMonsters,duplicate);
        for(auto count:{0xffffffffu,0x7fffffffu,0x40000000u}){std::string invalid;Word(invalid,1);invalid+=monster_record("bad",count);Reject(ReadSceneMonsters,invalid);}
        std::string empty;Word(empty,0);std::istringstream empty_input(empty);Check(ReadSceneMonsters(empty_input).empty());
        std::string gob;Word(gob,1);Word(gob,0);
        for(const auto* s:{"MO001","gamedata\\PALObject\\OJ29\\","OJ29","OJ29"})Text(gob,s);
        for(unsigned i=0;i<6;++i)Word(gob,0);Text(gob,"func2002");for(unsigned i=0;i<4;++i)Word(gob,i);gob+=Xml();
        std::istringstream g(gob);auto gobs=ReadSceneGobs(g);Check(gobs.size()==1 && gobs[0].model=="OJ29" && gobs[0].config.Root().name=="config");Boundaries(ReadSceneGobs,gob);
        std::string npc;Word(npc,1);for(const auto* s:{"MN001","201J","func2002"})Text(npc,s);
        for(unsigned i=0;i<9;++i)Word(npc,0);npc+=Xml();npc+=Xml();
        std::istringstream n(npc);const auto records=ReadSceneNpcs(n);Check(records[0].resource=="201J" && records[0].script=="func2002");Boundaries(ReadSceneNpcs,npc);
        std::string camera;Word(camera,1);Text(camera,"001");for(unsigned i=0;i<13;++i)Word(camera,i);camera+=Xml();
        std::istringstream c(camera);Check(ReadSceneCameras(c)[0].values[12]==12);Boundaries(ReadSceneCameras,camera);
        std::string event;Word(event,1);Text(event,"entrance1");Word(event,2);Word(event,3);Text(event,"");Word(event,0);Word(event,1);
        Text(event,"Q01");Text(event,"N01");Text(event,"func1001");Word(event,1);for(unsigned i=0;i<20;++i)Word(event,i);Word(event,0);Word(event,0);
        std::istringstream e(event);Check(ReadSceneEvents(e)[0].properties[0][19]==19);Boundaries(ReadSceneEvents,event);
        auto broken=gob;broken[12]='\0';Reject(ReadSceneGobs,broken);
        broken=gob;broken[0]=char(0xff);broken[1]=char(0xff);Reject(ReadSceneGobs,broken);
        broken=event;Word(broken,16);Word(broken,99);Word(broken,0);Reject(ReadSceneEvents,broken);
        const auto chunk=[](unsigned kind,const std::string& body,unsigned omitted=0){std::string b;Word(b,kind);Word(b,static_cast<unsigned>(body.size())+omitted);Word(b,0x1c020065);return b+body;};
        std::string empty_plugins;for(unsigned i=0;i<4;++i){Word(empty_plugins,0x2101+i);Word(empty_plugins,i==2?8:12);Word(empty_plugins,0x1c020065);}
        std::string cs;Word(cs,1);Word(cs,0);Word(cs,0);
        const auto atomic=chunk(20,chunk(1,std::string(16,'\0'))+chunk(3,empty_plugins,44),44);
        const auto clump=chunk(16,chunk(1,cs)+chunk(14,"")+chunk(26,"")+atomic+chunk(3,""),44);
        auto two=event+clump+event.substr(4);two[0]=2;
        // 第二条必须有独立名称；更改完整同长度的字符串。
        two.replace(two.rfind("entrance1"),9,"entrance2");
        std::istringstream pair(two);const auto events=ReadSceneEvents(pair);Check(events.size()==2 && events[0].clump==std::vector<std::uint8_t>(clump.begin(),clump.end()) && events[1].name=="entrance2");
        Check(events[0].normalized_clump.size()==events[0].clump.size());
        const auto read_word=[](const auto& bytes,std::size_t pos){unsigned word{};for(unsigned i=0;i<4;++i)word|=unsigned(bytes.at(pos+i))<<(8*i);return word;};
        Check(read_word(events[0].normalized_clump,4)+12==events[0].normalized_clump.size());
        const auto plugin_offset=clump.find(empty_plugins);
        for(unsigned i=0;i<4;++i)Check(read_word(events[0].normalized_clump,plugin_offset+i*12+4)==0);
        std::string normalized_event=event+std::string(events[0].normalized_clump.begin(),events[0].normalized_clump.end());
        std::istringstream normalized_input(normalized_event);const auto normalized=ReadSceneEvents(normalized_input);
        Check(normalized[0].clump==events[0].normalized_clump && normalized[0].normalized_clump==events[0].normalized_clump);
        broken=two;const auto plugin=broken.find(empty_plugins);broken[plugin]=char(0xff);Reject(ReadSceneEvents,broken);
        std::cout<<"scene data checks passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
