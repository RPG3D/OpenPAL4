#include "pal4/game_resource_manager.h"
#include "pal4/portable/world.h"
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void Check(bool value){if(!value)throw std::runtime_error("resource rotation contract failed");}
struct Runtime final:pal4::GameResourceManagerRuntimeStub {
    unsigned updates{};std::vector<unsigned> axes;
    std::uint32_t ClumpFrame(std::uint32_t clump) override{Check(clump==1);return 2;}
    void FrameRotate(std::uint32_t frame,std::uint32_t axis,float,std::uint32_t mode) override{Check(frame==2 && mode==2);axes.push_back(axis);}
    void FrameUpdateObjects(std::uint32_t frame) override{Check(frame==2);++updates;}
};
}
int main() {
    try {
        pal4::GameResourceManager resource{};resource.words[5]=1;
        for(unsigned i=46;i<=48;++i)resource.words[i]=std::bit_cast<unsigned>(1.f);
        Runtime runtime;
        for(const auto [angle,expected]:{std::pair{360.f,360.f},{-360.f,360.f},{720.f,0.f},{370.f,10.f},{-370.f,350.f}}) {
            resource.UpdateClumpRotation({12,angle,34},false,runtime);
            Check(std::bit_cast<float>(resource.words[40])==12 && std::bit_cast<float>(resource.words[41])==expected && std::bit_cast<float>(resource.words[42])==34);
        }
        Check(runtime.updates==5 && runtime.axes.size()==15 && runtime.axes[0]==0x84b870 && runtime.axes[1]==0x84b87c && runtime.axes[2]==0x84b888);
        const auto xyz=pal4::portable::InstanceMatrix({7,8,9},pal4::portable::Point3{90,90,0});
        Check(std::abs(xyz[4]-1)<1e-5f && std::abs(xyz[5])<1e-5f && std::abs(xyz[6])<1e-5f && xyz[12]==7 && xyz[13]==8 && xyz[14]==9);
        const auto z=pal4::portable::InstanceMatrix({0,0,0},pal4::portable::Point3{0,0,90});Check(std::abs(z[0])<1e-5f && std::abs(z[1]-1)<1e-5f);
        Check(pal4::portable::InstanceMatrix({7,8,9},180)==pal4::portable::InstanceMatrix({7,8,9},pal4::portable::Point3{0,180,0}));
        std::cout<<"original resource angle boundaries, frame axis order and XYZ instance matrices passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
