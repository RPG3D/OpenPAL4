#include "pal4/cylinder_object_update.h"
#include <bit>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
namespace {
void Check(bool ok,const char* message) {
    if(!ok) {std::cerr << "FAIL: " << message << '\n';std::exit(EXIT_FAILURE);}
}
struct Runtime final : pal4::ICylinderObjectUpdateRuntime {
    std::array<std::uint8_t,4> color{0x11,0x22,0x33,20};
    std::vector<std::string> calls;
    double argument{};
    std::array<std::uint8_t,4> QueryTimedFacet(pal4::CylinderObjectState&,std::uint32_t offset) override {
        Check(offset==44,"read color facet");calls.push_back("get");return color;
    }
    std::int32_t Ftol(double value) override {argument=value;calls.push_back("ftol");return static_cast<std::int32_t>(value);}
    void ApplyTimedFacet(pal4::CylinderObjectState&,std::uint32_t offset,const std::array<std::uint8_t,4>& value) override {
        Check(offset==44,"write color facet");calls.push_back("set");color=value;
    }
};
}
int main() {
    pal4::CylinderObjectState state{};Runtime runtime;
    state.words[24]=std::bit_cast<std::uint32_t>(2.0f);
    pal4::CylinderObjectUpdater::Update(state,3.0,runtime);
    state.words[1]=1;
    pal4::CylinderObjectUpdater::Update(state,3.0,runtime);
    state.words[1]=0x10001;
    pal4::CylinderObjectUpdater::Update(state,3.0,runtime);
    Check(runtime.calls.empty(),"inactive, hidden and untimed updates have no color side effects");
    state.words[2]=0x1000000;
    pal4::CylinderObjectUpdater::Update(state,3.0,runtime);
    Check(runtime.argument==14 && runtime.color==std::array<std::uint8_t,4>{0x11,0x22,0x33,14} &&
          runtime.calls==std::vector<std::string>{"get","ftol","set"},"decay alpha and preserve distinct RGB");
    runtime.color[3]=2;
    pal4::CylinderObjectUpdater::Update(state,3.0,runtime);
    Check(runtime.argument==-4 && runtime.color==std::array<std::uint8_t,4>{0x11,0x22,0x33,0},"clamp alpha to zero without changing RGB");
    runtime.color[3]=10;
    pal4::CylinderObjectUpdater::Update(state,0.125,runtime);
    Check(runtime.argument==9.75 && runtime.color[3]==9,"truncate fractional alpha like original ftol");
}
