#include "pal4/portable/scene.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
using namespace pal4::portable;
void Check(bool condition) { if (!condition) throw std::runtime_error("contract failed"); }
template<class T> void Append(std::string& bytes,const T& value) {
    bytes.append(reinterpret_cast<const char*>(&value),sizeof value);
}
std::string Fixture() {
    std::string bytes="P4SC";
    for (std::uint32_t value:{1u,1u,3u}) Append(bytes,value);
    for (float value:{0.f,0.f,0.f,1.f,1.f,0.f,1.f,1.f,1.f,0.f}) Append(bytes,value);
    for (std::uint32_t value:{0u,3u}) Append(bytes,value);
    for (std::int32_t value:{0,-1}) Append(bytes,value);
    for (float value:{1.f,1.f,1.f,1.f}) Append(bytes,value);
    for (auto position: {std::array<float,3>{0,0,0},{1,0,0},{0,1,0}}) {
        Vertex vertex{{position[0],position[1],position[2]},{0,0},{0,0},{1,1,1,1}};
        Append(bytes,vertex);
    }
    for (std::uint32_t value:{1u,1u,1u,4u}) Append(bytes,value);
    bytes.append("\xff\x80\x40\xff",4);
    return bytes;
}
Scene Parse(const std::string& bytes) { std::istringstream stream(bytes); return ReadScene(stream); }
void Rejected(const std::string& bytes) {
    bool rejected=false;
    try { (void)Parse(bytes); } catch (const std::runtime_error&) { rejected=true; }
    Check(rejected);
}
template<class T> void Patch(std::string& bytes,std::size_t offset,T value) {
    std::memcpy(bytes.data()+offset,&value,sizeof value);
}
}
int main(int argc,char** argv) {
    try {
        if (argc==2) {
            std::ifstream file(argv[1],std::ios::binary); const auto scene=ReadScene(file);
            std::cout<<"draws="<<scene.draws.size()<<" triangles="<<scene.vertices.size()/3
                     <<" textures="<<scene.textures.size()<<'\n'; return 0;
        }
        const auto bytes=Fixture(); const auto scene=Parse(bytes);
        Check(scene.vertices.size()==3 && scene.textures[0].rgba[1]==128 && !scene.textures[0].alpha);
        Check(scene.draws[0].uv_animations[0].empty());
        // Every truncation boundary must reject, including headers and pixel payload.
        for (std::size_t length=0; length<bytes.size(); ++length) Rejected(bytes.substr(0,length));
        auto bad=bytes; bad[0]='X'; Rejected(bad);
        bad=bytes; Patch(bad,4,2u); Rejected(bad);
        bad=bytes; Patch(bad,12,3000001u); Rejected(bad);
        bad=bytes; Patch(bad,56,0xffffffffu); Rejected(bad);
        bad=bytes; Patch(bad,60,4u); Rejected(bad);
        bad=bytes; Patch(bad,64,1); Rejected(bad);
        bad=bytes; Patch(bad,68,-2); Rejected(bad);
        bad=bytes; Patch(bad,88,std::numeric_limits<float>::quiet_NaN()); Rejected(bad);
        bad=bytes; Patch(bad,224,0xffffffffu); Rejected(bad);
        bad=bytes; bad+='x'; Rejected(bad);
        OrbitCamera camera; camera.Fit(scene);
        camera.Orbit(1e8f,1e8f); camera.Zoom(-1e8f);
        Check(camera.pitch<=1.45f && camera.distance>=camera.scale*.01f);
        const auto matrix=camera.Matrix(16.f/9.f);
        for (float value:matrix) Check(std::isfinite(value));
        const auto& p=camera.target;
        const float x=matrix[0]*p[0]+matrix[4]*p[1]+matrix[8]*p[2]+matrix[12];
        const float y=matrix[1]*p[0]+matrix[5]*p[1]+matrix[9]*p[2]+matrix[13];
        Check(std::abs(x)<.001f && std::abs(y)<.001f);
        std::cout<<"scene corruption, texture, range and camera contracts passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
