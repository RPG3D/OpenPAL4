#include "pal4/portable/surface_motion.h"
#include "pal4/portable/scene.h"
#include "pal4/portable/actor.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
std::vector<CollisionTriangle> Fixture(){return {{{Point3{0,0,0},Point3{10,0,0},Point3{0,0,10}},0,0,0},{{Point3{10,0,0},Point3{10,0,10},Point3{0,0,10}},0,0,1}};}
float Distance(Point3 a,Point3 b){float d{};for(unsigned i=0;i<3;++i)d+=(a[i]-b[i])*(a[i]-b[i]);return std::sqrt(d);}
void Json(std::ostream& out,Point3 p){out<<'['<<p[0]<<','<<p[1]<<','<<p[2]<<']';}
}
int main(int argc,char** argv){try{
    if(argc==5){
        std::ifstream floor(argv[1],std::ios::binary),world_file(argv[2],std::ios::binary);const auto collision=ReadCollision(floor);const auto world=ReadScene(world_file);
        CollisionSurface surface(collision.floor);surface.EnableNavigation();
        const Point3 from{-575,61,-576};Point3 to{-670,53,-465};const auto target_face=surface.Snap(to);
        std::ifstream actor_file(argv[3],std::ios::binary);const auto actor=ReadActor(actor_file);const auto sphere=ActorBindSphere(actor);Check(sphere.has_value(),"original actor radius missing");
        unsigned queries{};SurfaceMotion motion(surface,from,(*sphere)[3],[&](Point3 a,Point3 b){++queries;return WorldSegmentBlocked(world,a,b);});
        Check(target_face && motion.Start(from,to,target_face),"real Q01 route failed");motion.SetSpeed(45);
        std::ofstream out(argv[4]);out<<std::setprecision(9)<<"{\"faces\":"<<surface.Size()<<",\"height\":"<<(*sphere)[3]<<",\"linked_edges\":"<<surface.LinkedEdges()<<",\"target_face\":"<<target_face<<",\"target\":";Json(out,to);out<<",\"route\":[";
        const auto route=motion.Path();for(std::size_t i=0;i<route.size();++i){if(i)out<<',';Json(out,route[i].position);}out<<"],\"frames\":[";
        unsigned steps{};float maximum_step{},maximum_horizontal{};
        while(motion.Active() && steps<1200){const auto old=motion.Position();motion.Advance(1.f/60);const auto next=motion.Position();maximum_step=std::max(maximum_step,Distance(old,next));maximum_horizontal=std::max(maximum_horizontal,std::hypot(next[0]-old[0],next[2]-old[2]));if(steps++)out<<',';Json(out,next);}
        out<<"],\"final\":";Json(out,motion.Position());out<<",\"active\":"<<(motion.Active()?"true":"false")<<",\"status\":"<<motion.Status()<<",\"maximum_step\":"<<maximum_step<<",\"maximum_horizontal_step\":"<<maximum_horizontal<<",\"clearance_queries\":"<<queries<<'}';
        // Original height projection occurs after velocity limiting, so the
        // resulting vertical displacement can exceed the horizontal step.
        Check(!motion.Active() && Distance(motion.Position(),to)<.01f && steps>60,"real movement did not naturally reach target");Check(maximum_horizontal<.751f,"movement skipped or exceeded original speed");std::cout<<"real Q01 route points="<<route.size()<<" frames="<<steps<<" linked_edges="<<surface.LinkedEdges()<<'\n';return 0;
    }
    auto floor=Fixture();CollisionSurface surface(floor);surface.EnableNavigation();Check(surface.LinkedEdges()==2,"reciprocal adjacency missing");
    Point3 target{9,20,9};const auto face=surface.Snap(target);Check(face==2 && target[1]==0,"original target projection");
    std::vector<SurfacePathPoint> route;Check(surface.TracePath(1,{1,0,1},face,target,route) && route.size()>=3,"cross face path missing");
    SurfaceMotion motion(surface,{1,0,1},1,[](auto,auto){return false;});Check(motion.Start({1,0,1},target,face),"motion path start");motion.SetSpeed(5);
    unsigned frames{};float maximum{};
    while(motion.Active() && frames++<200){const auto from=motion.Position();motion.Advance(.05f);maximum=std::max(maximum,Distance(from,motion.Position()));}
    Check(!motion.Active() && frames>20 && Distance(motion.Position(),target)<.001f && maximum<.26f,"original movement completion or speed");
    Check(motion.Start(target,{1,0,1},1),"movement restart");motion.Advance(.1f);motion.Stop();const auto stopped=motion.Position();motion.Advance(.1f);Check(!motion.Active() && motion.Position()==stopped,"movement stop");
    floor.push_back({{Point3{20,0,0},Point3{30,0,0},Point3{20,0,10}},0,0,2});CollisionSurface islands(floor);
    Check(!islands.TracePath(1,{1,0,1},3,{21,0,1},route) && route.empty(),"disconnected floor must fail");
    Check(islands.TracePath(3,{21,0,1},3,{22,0,1},route) && route.size()==2,"same face route");
    CollisionSurface empty({});Point3 point{};Check(empty.Snap(point)==0,"empty surface target");
    Scene world;for(auto p:{Point3{5,0,0},Point3{5,5,0},Point3{5,0,5}})world.vertices.push_back({{p[0],p[1],p[2]},{},{},{}});
    Check(WorldSegmentBlocked(world,{4,1,1},{6,1,1}) || WorldSegmentBlocked(world,{6,1,1},{4,1,1}),"render world collision missing");
    Check(!WorldSegmentBlocked(world,{4,1,1},{4,2,1}),"clear world segment blocked");
    std::cout<<"original face adjacency, route, projection, bounded movement, stop and world clearance passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
