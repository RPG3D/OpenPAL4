#include "pal4/portable/player_free_movement.h"
#include "pal4/portable/package.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T Read(const std::filesystem::path& path,T(*reader)(std::istream&)){
    std::ifstream input(path,std::ios::binary);Require(bool(input),"missing user cache");return reader(input);
}
struct Reset final:IControllerResetServices {
    void SetDefaultCursor() override{}
    void ClearPlatformInput() override{}
    void HideInteractionUi() override{}
    void SetMonsterPursuitStopped(bool) override{}
};
// 无平台的CPU资源探针：这里只记录外部服务调用，不冒充实际相机或HUD。
struct Services final:IPlayerPlacementServices {
    std::uint32_t follows{},attributes{},animations{};const CollisionMesh& mesh;
    explicit Services(const CollisionMesh& value):mesh(value){}
    void FollowPlayer(std::uint32_t) override{++follows;}
    void UpdateFloorAttributes(unsigned child,const FloorHit& hit) override{Require(child==0 && hit.triangle<mesh.floor.size(),"invalid free probe floor source");++attributes;}
    void UpdateGrid(Point3,Point3) override{throw std::logic_error("unexpected minimap call from vector movement");}
    void SelectAnimation(std::uint32_t,std::string_view) override{++animations;}
};
}
int main(int argc,char** argv){try{
    Require(argc==9,"usage: free_movement_probe data_dir actors_dir dummy_cache collision_cache palactor_cpk world scene output_json");
    const auto roles=Read(std::filesystem::path(argv[1])/"roles.p4r",ReadRoles);
    std::ifstream growth_file(std::filesystem::path(argv[1])/"upgrades.p4u",std::ios::binary);const auto growth=ReadUpgrades(growth_file);
    PlayerRoster players;players.Prepare(roles,growth,[&](std::string_view model){return Read(std::filesystem::path(argv[2])/("actor_"+std::string(model)+".p4a"),ReadActor);});
    const auto mesh=Read(argv[4],ReadCollision);const auto& triangle=mesh.floor.front();Point3 start{};
    for(unsigned axis=0;axis<3;++axis)start[axis]=static_cast<float>((double(triangle.points[0][axis])+triangle.points[1][axis]+triangle.points[2][axis])/3);
    players.EnterScene(argv[6],argv[7]);players.SetPosition(0,start);
    ActorController controller;controller.Initialize(1280,720,[&]{return Read(argv[3],ReadActor);});controller.SelectActor(0,1);
    Reset reset;controller.ResetAndSync(true,reset);
    std::ifstream package_file(argv[5],std::ios::binary);PackageReader package(package_file);std::map<std::string,std::uint32_t> source_members;
    PlayerActions actions(players,[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{
        if(!package.Find(member))return {};
        auto bytes=package.Read(member);++source_members[std::string(member)];return bytes;
    });
    std::ifstream scene_file(std::filesystem::path(argv[5]).parent_path()/"scenedata.cpk",std::ios::binary);PackageReader scene_package(scene_file);
    const auto prefix=std::string(argv[6])+"\\"+argv[7]+"\\"+argv[7];
    Require(scene_package.Find(prefix+"_floor.dff") && scene_package.Find(prefix+"_wall.dff"),"original scene namespace was not verified");
    Require(!scene_package.Find(prefix+"_intri.dff"),"original intri exists: prepare and register it before this probe");
    SceneMovement floor(mesh.floor);SceneCollision collision;
    for(std::size_t first=0;first<mesh.wall.size();){auto end=first+1;while(end<mesh.wall.size() && mesh.wall[end].atomic==mesh.wall[first].atomic)++end;
        collision.Add(CollisionGroup::wall,std::span(mesh.wall).subspan(first,end-first));first=end;}
    Services services(mesh);unsigned camera_accesses{},active_inputs{},accepted{};
    PlayerFreeMovement movement(players,controller,actions,floor,collision,services,[&]{++camera_accesses;});
    ControlCameraBasis camera{{1,0,0},{0,0,1}};FreeControlInput input;
    for(unsigned frame=0;frame<80;++frame){
        input={};
        // 先释放再改变方向，隔离原组件的跨帧方向积累；不运行游戏脚本。
        if(frame%20!=0){switch(frame/20){case 0:input.forward=true;break;case 1:input.right=true;break;case 2:input.backward=true;break;default:input.left=true;break;}}
        if(controller.UpdateFreeInput(input,camera,players.Slots()[0].position,.01f,[&](Point3 direction,float dt){if(movement.Advance(direction,dt))++accepted;}))++active_inputs;
        (void)actions.Advance(.01f);
    }
    Require(accepted>0 && services.attributes==accepted,"original cache movement did not commit a floor hit");
    Require(!source_members.empty(),"movement failed to read original animation messages");
    std::ofstream out(argv[8]);out<<std::setprecision(9)<<"{\"probe\":\"offline_user_cache\",\"world\":"<<std::quoted(argv[6])<<",\"scene\":"<<std::quoted(argv[7])<<",\"game_calls\":0,\"camera_and_hud_bound\":false,\"configured_objects_bound\":false,\"intri_absence_verified\":true,\"active_inputs\":"<<active_inputs<<",\"camera_accesses\":"<<camera_accesses<<",\"follows\":"<<services.follows<<",\"attributes\":"<<services.attributes<<",\"start\":["<<start[0]<<','<<start[1]<<','<<start[2]<<"],\"end\":[";
    const auto end=players.Slots()[0].position;out<<end[0]<<','<<end[1]<<','<<end[2]<<"],\"movement\":";movement.WriteReport(out);out<<",\"collision\":";collision.WriteReport(out);
    out<<",\"original_members\":[";bool first=true;for(const auto& [member,count]:source_members){if(!first)out<<',';first=false;out<<"{\"member\":"<<std::quoted(member)<<",\"reads\":"<<count<<'}';}out<<"]}";Require(bool(out),"cannot write free movement probe report");
    std::cout<<"offline user-cache free movement passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
