#pragma once

#include <windows.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pal4 {
// Explicit test positioning, authorized separately from gameplay comparison.
// Both hosts consume the same request before input/world simulation. This does
// not change clocks, attributes, random state, story flags, or scene selection.
inline constexpr std::uint32_t kComparisonRelocateMessage = 0x850eu;
struct ComparisonSceneRelocationRequest {
    std::uint32_t id{};
    std::string world, area;
    std::array<float,3> position{};
};

class ComparisonSceneRelocation final {
public:
    explicit ComparisonSceneRelocation(std::string side):side_(std::move(side)){}
    static ComparisonSceneRelocationRequest Read(std::istream& input,std::uint32_t id) {
        ComparisonSceneRelocationRequest r;r.id=id;std::string magic,extra;
        if(!id || id>=0x80000000u || !(input>>magic>>r.world>>r.area>>r.position[0]>>r.position[1]>>r.position[2]) ||
           magic!="PAL4_SCENE_RELOCATE_1" || !Identifier(r.world) || !Identifier(r.area) || (input>>extra))
            throw std::invalid_argument("invalid scene relocation request");
        for(const auto v:r.position)if(!std::isfinite(v) || std::fabs(v)>1000000.f)
            throw std::invalid_argument("invalid scene relocation coordinate");
        return r;
    }
    void Accept(std::uint32_t id) {
        if(id==last_)return;
        if(pending_){Publish(id,"busy");return;}
        try {
            const auto root=Root();
            std::ifstream file(root/("compare-relocate-request-"+std::to_string(id)+".txt"));
            file.imbue(std::locale::classic());
            pending_=Read(file,id);last_=id;Publish(id,"queued");
        } catch(...) {Publish(id,"rejected");}
    }
    // Scene/menu transitions can defer a request, but may never apply it in a
    // different map or during combat/script control. The callback is synchronous
    // on the game's main thread and owns the real position services.
    void TryApply(bool available,std::string_view world,std::string_view area,
                  const std::function<void(const std::array<float,3>&)>& apply) {
        if(!pending_ || !available || pending_->world!=world || pending_->area!=area)return;
        const auto request=*pending_;pending_.reset();
        try {apply(request.position);Publish(request.id,"applied");}
        catch(...) {Publish(request.id,"failed");throw;}
    }
    bool Pending()const noexcept{return pending_.has_value();}
private:
    static bool Identifier(std::string_view s) {
        if(s.empty() || s.size()>31u)return false;
        for(const unsigned char c:s)if(!((c>='a' && c<='z') || (c>='A' && c<='Z') ||
            (c>='0' && c<='9') || c=='_'))return false;
        return true;
    }
    static std::filesystem::path Root() {
        const auto* directory=std::getenv("PAL4_COMPARE_STATE_DIR");
        if(!directory || !*directory)throw std::logic_error("comparison output directory unavailable");
        return std::filesystem::path(directory);
    }
    void Publish(std::uint32_t id,const char* status)const {
        const auto path=Root()/("compare-"+side_+"-relocate-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(id)+".json");
        auto temporary=path;temporary+=L".tmp";
        std::ofstream out(temporary);out<<"{\"request\":"<<id<<",\"status\":\""<<status<<"\"}\n";
        out.flush();const bool good=bool(out);out.close();
        if(!good || !MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING))
            throw std::runtime_error("scene relocation acknowledgement publication failed");
    }
    std::string side_;std::uint32_t last_{};
    std::optional<ComparisonSceneRelocationRequest> pending_;
};
} // namespace pal4
