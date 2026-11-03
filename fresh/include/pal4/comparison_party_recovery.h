#pragma once
#include "pal4/comparison_frame_gate.h"
#include <windows.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>

namespace pal4 {
// Explicit difficulty preparation, never part of the equivalence baseline.
// Runs synchronously at a held Scene boundary; no queued operation can heal
// actors after entering combat. Hosts supply the existing giSetFullHP services.
class ComparisonPartyRecovery final {
public:
    static constexpr std::uint32_t kMessage=0x851cu;
    explicit ComparisonPartyRecovery(std::string side):side_(std::move(side)){}
    static bool Held(const ComparisonFrameGate::Observation& s) noexcept {
        return s.phase==ComparisonFrameGate::Phase::held && !s.depth && !s.remaining && !s.arm_pending;
    }
    void Apply(std::uint32_t id,std::uint32_t expected_story,std::uint32_t actual_story,
               const ComparisonFrameGate::Observation& gate,bool scene_ready,
               const std::function<void()>& restore_hp){
        if(!id || id>0x7fffffffu)throw std::invalid_argument("invalid party recovery request id");
        if(!seen_.insert(id).second)return;
        if(busy_){Publish(id,expected_story,actual_story,"busy");return;}
        if(!expected_story || expected_story!=actual_story || !Held(gate) || !scene_ready || !restore_hp){
            Publish(id,expected_story,actual_story,"rejected");return;
        }
        struct Scope{bool& b;explicit Scope(bool& v):b(v){b=true;}~Scope(){b=false;}} scope(busy_);
        // Record consumption before calling game code. Failed/partial calls
        // cannot be replayed using the same id, including after later requests.
        Publish(id,expected_story,actual_story,"applying");
        try{restore_hp();}
        catch(...){Publish(id,expected_story,actual_story,"failed");throw;}
        Publish(id,expected_story,actual_story,"applied");
    }
private:
    void Publish(std::uint32_t id,std::uint32_t expected,std::uint32_t actual,const char* status)const{
        const auto* root=std::getenv("PAL4_COMPARE_STATE_DIR");
        if(!root || !*root)throw std::logic_error("comparison output directory unavailable");
        const auto path=std::filesystem::path(root)/("compare-"+side_+"-party-recovery-"+
            std::to_string(GetCurrentProcessId())+"-"+std::to_string(id)+".json");
        auto temporary=path;temporary+=L".tmp";
        std::ofstream out(temporary);out<<"{\"request\":"<<id<<",\"expected_story\":"<<expected
            <<",\"actual_story\":"<<actual<<",\"operation\":\"giSetFullHP\",\"status\":\""<<status<<"\"}\n";
        out.flush();const bool good=bool(out);out.close();
        if(!good || !MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING))
            throw std::runtime_error("party recovery acknowledgement publication failed");
    }
    std::string side_;std::set<std::uint32_t> seen_;bool busy_{};
};
}
