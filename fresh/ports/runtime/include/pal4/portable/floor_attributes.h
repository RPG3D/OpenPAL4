#pragma once
#include "pal4/portable/world.h"
#include <functional>
#include <iosfwd>
#include <list>
#include <string>

namespace pal4::portable {
// Data encoding adapter for original RGBA bytes and D3D9 8888 identifiers.
// No graphics-device calls or native pointers enter the game policy.
std::uint32_t FloorPixel8888(std::uint32_t rgba);
class FloorAttributes {
public:
    using Callback=std::function<void(std::uint32_t)>;
    void AddCallback(std::uint32_t pixel,Callback callback);
    void Update(const CollisionTriangle& triangle);
    void WriteReport(std::ostream& output) const;
    std::uint32_t CurrentColor() const{return current_color_;}
    std::uint32_t CurrentAlpha() const{return current_alpha_;}
    std::uint32_t FallbackAlpha() const{return fallback_alpha_;}
    const std::string& SoundPath() const{return sound_;}
private:
    struct Entry {std::uint32_t pixel;Callback callback;};
    void Apply(std::uint32_t pixel);
    std::list<Entry> callbacks_;
    std::uint32_t current_color_{0xff000000},previous_color_{},current_alpha_{},fallback_alpha_{};
    std::uint32_t sampled_color_{},updates_{},callback_calls_{};
    bool has_colors_{};
    std::string sound_;
};
} // namespace pal4::portable
