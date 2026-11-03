#include "pal4/portable/floor_attributes.h"
#include "pal4/temperature_floor_selection.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("floor attributes contract failed");}
}
int main(int argc,char** argv) {
    try {
        if(argc==3) {
            // Independent host sampling of the actual initial scene triangle.
            std::ifstream input(argv[1],std::ios::binary);
            const auto mesh=ReadCollision(input);FloorAttributes floor;
            floor.Update(mesh.floor.at(82));std::ofstream report(argv[2]);floor.WriteReport(report);return 0;
        }
        Check(FloorPixel8888(0x44332211)==0x44112233);
        FloorAttributes floor;CollisionTriangle triangle{};
        bool rejected=false;try{floor.Update(triangle);}catch(const std::logic_error&){rejected=true;}Check(rejected);
        triangle.attributes_known=true;floor.Update(triangle);Check(floor.CurrentAlpha()==0 && floor.SoundPath().empty());
        std::vector<unsigned> calls;
        const auto brown=FloorPixel8888(0xff003399);
        floor.AddCallback(brown,[&](auto pixel){Check(pixel==brown && floor.CurrentAlpha()==0);calls.push_back(1);});
        floor.AddCallback(brown,[&](auto){calls.push_back(2);});
        floor.AddCallback(floor.CurrentColor(),[&](auto pixel){Check(pixel==0xff000000);calls.push_back(3);});
        triangle.rgba=std::array<std::uint32_t,3>{0x11003399,0x22003399,0x22003399};floor.Update(triangle);
        Check(calls==std::vector<unsigned>{1,2} && floor.CurrentAlpha()==0x22);
        Check(floor.SoundPath()=="gamedata\\palSound\\WF013.wav" && floor.CurrentColor()==0xff000000);
        calls.clear();triangle.rgba=std::array<std::uint32_t,3>{0x03000001,0x04000002,0x05000003};floor.Update(triangle);
        Check(calls==std::vector<unsigned>{3} && floor.CurrentAlpha()==0 && floor.SoundPath().empty());
        // A known color without a direct callback falls back to current_color.
        calls.clear();triangle.rgba=std::array<std::uint32_t,3>{0xffffffff,0xffffffff,0xffffffff};floor.Update(triangle);
        Check(calls==std::vector<unsigned>{3} && floor.CurrentAlpha()==255);
        triangle.rgba.reset();floor.Update(triangle);Check(calls.size()==1 && floor.CurrentAlpha()==255);
        rejected=false;try{floor.AddCallback(0,{});}catch(const std::invalid_argument&){rejected=true;}Check(rejected);
        std::array<std::uint32_t,5> colors{};for(unsigned i=0;i<4;++i)colors[i]=FloorPixel8888(pal4::kTemperatureFloorRgba[i]);colors[4]=0xff000000;
        unsigned selected=4;for(unsigned i=0;i<5;++i)Check(pal4::SelectTemperatureFloorColor(colors,colors[i],selected)==i && selected==i);
        Check(pal4::SelectTemperatureFloorColor(colors,123,selected)==5 && selected==4);
        colors[3]=colors[0];Check(pal4::SelectTemperatureFloorColor(colors,colors[0],selected)==0 && selected==0);
        std::cout<<"floor raw color, alpha, sound, callback ordering/fallback and temperature selection contracts passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
