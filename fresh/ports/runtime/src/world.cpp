#include "pal4/portable/world.h"
#include "pal4/portable/scene.h"
#include "pal4/collision_triangle_containment.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <istream>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
template<class T> T Read(std::istream& s) {
    T value{}; Require(bool(s.read(reinterpret_cast<char*>(&value),sizeof value)),"truncated world resource"); return value;
}
void Finite(float x) { Require(std::isfinite(x),"non-finite world resource"); }
void Finite(Point3 p) { for (float x:p) Finite(x); }
std::uint32_t Header(std::istream& s,const char* expected,std::uint32_t maximum=1) {
    static_assert(std::endian::native==std::endian::little);
    char magic[4]{}; Require(bool(s.read(magic,4)),"truncated world header");
    const auto version=Read<std::uint32_t>(s);
    Require(!std::memcmp(magic,expected,4) && version>=1 && version<=maximum,"unsupported world resource");
    return version;
}
void End(std::istream& s) { Require(s.peek()==std::char_traits<char>::eof(),"world resource has trailing bytes"); }
std::string Name(std::istream& s) {
    const auto length=Read<std::uint32_t>(s); Require(length>0 && length<=64,"invalid resource name length");
    std::string name(length,'\0'); Require(bool(s.read(name.data(),length)),"truncated resource name");
    Require(name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos,"invalid resource name");
    return name;
}
}
CollisionMesh ReadCollision(std::istream& stream) {
    const auto version=Header(stream,"P4CL",2); const auto floor=Read<std::uint32_t>(stream),wall=Read<std::uint32_t>(stream);
    Require(floor>0 && std::uint64_t(floor)+wall<=1000000,"invalid collision triangle count");
    CollisionMesh mesh; mesh.floor.resize(floor); mesh.wall.resize(wall);
    for (auto* triangles:{&mesh.floor,&mesh.wall}) for (auto& triangle:*triangles) {
        for (auto& p:triangle.points) { p=Read<Point3>(stream); Finite(p); }
        triangle.atomic=Read<std::uint32_t>(stream); triangle.material=Read<std::uint32_t>(stream);
        triangle.triangle=Read<std::uint32_t>(stream);
        if(version==2) {
            const auto present=Read<std::uint32_t>(stream);
            const auto rgba=Read<std::array<std::uint32_t,3>>(stream);
            Require(present<=1,"invalid collision attribute flag");
            Require(present || rgba==std::array<std::uint32_t,3>{},"colors on uncolored collision triangle");
            triangle.attributes_known=true;
            if(present)triangle.rgba=rgba;
        }
    }
    End(stream); return mesh;
}
std::optional<FloorHit> FindFloor(const CollisionMesh& mesh,Point3 position,float above,float below) {
    Finite(position); Finite(above); Finite(below); Require(above>=0 && below>=0,"negative floor query range");
    std::optional<FloorHit> result; float distance=0;
    for (std::size_t i=0;i<mesh.floor.size();++i) {
        Point3 hit{};
        if (!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(position,{0,-1,0},mesh.floor[i].points,&hit)) continue;
        const float difference=hit[1]-position[1];
        if (difference>above || -difference>below) continue;
        if (!result || std::abs(difference)<distance) {
            result=FloorHit{hit,static_cast<std::uint32_t>(i)}; distance=std::abs(difference);
        }
    }
    return result;
}
bool SegmentBlocked(const CollisionMesh& mesh,Point3 from,Point3 to) {
    Finite(from); Finite(to);
    const Point3 direction{to[0]-from[0],to[1]-from[1],to[2]-from[2]};
    double length{}; for (float x:direction) length+=double(x)*x;
    if (length==0) return false;
    for (const auto& triangle:mesh.wall) {
        Point3 hit{};
        if (!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(from,direction,triangle.points,&hit)) continue;
        double projection{}; for (int i=0;i<3;++i) projection+=double(hit[i]-from[i])*direction[i];
        if (projection>=0 && projection<=length) return true;
    }
    return false;
}
bool WorldSegmentBlocked(const Scene& scene,Point3 from,Point3 to) {
    Finite(from);Finite(to);const Point3 direction{to[0]-from[0],to[1]-from[1],to[2]-from[2]};
    double length{};for(float x:direction)length+=double(x)*x;if(length==0)return false;
    for(std::size_t i=0;i+2<scene.vertices.size();i+=3) {
        std::array<Point3,3> points{};for(unsigned j=0;j<3;++j)for(unsigned axis=0;axis<3;++axis)points[j][axis]=scene.vertices[i+j].position[axis];
        Point3 hit{};if(!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(from,direction,points,&hit))continue;
        double projection{};for(unsigned axis=0;axis<3;++axis)projection+=double(hit[axis]-from[axis])*direction[axis];
        if(projection>=0 && projection<=length)return true;
    }
    return false;
}
std::optional<FloorHit> FindFloorFromAbove(std::span<const CollisionTriangle> triangles,Point3 position,float above,float below) {
    Finite(position);Finite(above);Finite(below);Require(above>=0 && below>=0,"negative floor query range");
    Finite(position[1]+above);Finite(position[1]-below);
    if(above==0 && below==0)return {};
    std::optional<FloorHit> result;
    for(std::size_t i=0;i<triangles.size();++i) {
        Point3 hit{};
        if(!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(position,{0,-1,0},triangles[i].points,&hit))continue;
        const float difference=hit[1]-position[1];
        if(difference>above || -difference>below)continue;
        if(!result || hit[1]>result->position[1])result=FloorHit{hit,static_cast<std::uint32_t>(i)};
    }
    return result;
}
LevelDefinition ReadLevel(std::istream& stream) {
    Header(stream,"P4LV"); LevelDefinition level; level.world=Name(stream); level.actor=Name(stream);
    Require(level.actor.starts_with("actor_"),"level actor must reference an actor resource");
    level.position=Read<Point3>(stream); Finite(level.position); level.yaw=Read<float>(stream); Finite(level.yaw);
    End(stream); return level;
}
std::array<float,16> InstanceMatrix(Point3 position,float yaw_degrees) {
    Finite(position); Finite(yaw_degrees);
    const float yaw=std::remainder(yaw_degrees,360.f)*.017453292519943295f;
    const float c=std::cos(yaw),s=std::sin(yaw);
    return {c,0,-s,0,0,1,0,0,s,0,c,0,position[0],position[1],position[2],1};
}
std::array<float,16> InstanceMatrix(Point3 position,Point3 rotation) {
    Finite(position);Finite(rotation);
    if(rotation[0]==0 && rotation[2]==0)return InstanceMatrix(position,rotation[1]);
    std::array<float,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,position[0],position[1],position[2],1};
    // GameResourceManager 0x6668D4/8E8/8FC: X, Y, Z POSTCONCAT.
    for(unsigned axis=0;axis<3;++axis) {
        const float radians=std::remainder(rotation[axis],360.f)*.017453292519943295f;
        const float c=std::cos(radians),s=std::sin(radians);
        for(unsigned column=0;column<3;++column) {
            const unsigned a=(axis+1)%3,b=(axis+2)%3;
            const float x=matrix[column*4+a],y=matrix[column*4+b];
            matrix[column*4+a]=c*x-s*y;matrix[column*4+b]=s*x+c*y;
        }
    }
    return matrix;
}
}  // namespace pal4::portable
