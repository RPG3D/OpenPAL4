#include "pal4/portable/floor_attributes.h"
#include "pal4/floor_color_sample.h"
#include "pal4/floor_color_sounds.h"
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
std::uint32_t FloorPixel8888(std::uint32_t rgba) {
    return (rgba&0xff00ff00u)|((rgba&0xffu)<<16)|((rgba>>16)&0xffu);
}
void FloorAttributes::AddCallback(std::uint32_t pixel,Callback callback) {
    if(!callback)throw std::invalid_argument("empty floor callback");
    callbacks_.push_back({pixel,std::move(callback)});
}
void FloorAttributes::Apply(std::uint32_t pixel) {
    sound_.clear();
    for(const auto& entry:pal4::kColorSounds)
        if(FloorPixel8888(entry.rgb|0xff000000u)==pixel) {sound_=entry.path;break;}
    bool matched=false;
    for(auto& entry:callbacks_)if(entry.pixel==pixel) {
        entry.callback(pixel);++callback_calls_;matched=true;
    }
    if(!matched)for(auto& entry:callbacks_)if(entry.pixel==current_color_) {
        entry.callback(current_color_);++callback_calls_;
    }
    // 0x420D90 does not write current_color or previous_color.
}
void FloorAttributes::Update(const CollisionTriangle& triangle) {
    if(!triangle.attributes_known)throw std::logic_error("original floor colors are missing from cache");
    has_colors_=triangle.rgba.has_value();
    if(!has_colors_)return; // Original null prelight array: no callbacks or alpha change.
    (void)pal4::UpdateFloorColorSample(*triangle.rgba,current_color_,fallback_alpha_,current_alpha_,
        FloorPixel8888,[&](std::uint32_t pixel){sampled_color_=pixel;Apply(pixel);});
    ++updates_;
}
void FloorAttributes::WriteReport(std::ostream& output) const {
    output<<"{\"has_colors\":"<<(has_colors_?"true":"false")<<",\"updates\":"<<updates_
        <<",\"sampled_color\":"<<sampled_color_<<",\"current_color\":"<<current_color_
        <<",\"previous_color\":"<<previous_color_<<",\"alpha\":"<<current_alpha_
        <<",\"callbacks\":"<<callback_calls_<<",\"sound_path\":"<<std::quoted(sound_)<<'}';
    if(!output)throw std::runtime_error("cannot write floor attribute report");
}
} // namespace pal4::portable
