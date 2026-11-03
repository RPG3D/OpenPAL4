#pragma once
#include "pal4/comparison_frame_gate.h"
#include <array>
#include <cstdint>
#include <ostream>
#include <type_traits>
namespace pal4 {
// Comparison-only integer observations. No clock, floating-point state, game
// callbacks, allocation or publication occurs at the observed mode boundary.
class ComparisonCameraModeTrace final {
public:
    static constexpr std::uint32_t kCapacity=64u;
    struct Event {
        std::uint32_t sequence{},mode{},after{},outer_depth{};
        std::uint64_t outer_completed{},outer_callbacks{};
    };
    static_assert(std::is_trivially_copyable_v<Event> && std::is_standard_layout_v<Event>);
    void Arm(std::uint32_t request,bool enabled) noexcept {
        events_={};request_=request;enabled_=enabled && request!=0u;
        count_=dropped_=sequence_=0u;
    }
    void Record(std::uint32_t mode,bool after) noexcept {
        if(!enabled_)return;
        Record(mode,after,GetComparisonFrameGate().Observe());
    }
    // Explicit observation overload also permits deterministic contract tests
    // without changing the process's real comparison frame gate.
    void Record(std::uint32_t mode,bool after,
                const ComparisonFrameGate::Observation& outer) noexcept {
        if(!enabled_)return;
        ++sequence_;
        if(count_==kCapacity){++dropped_;return;}
        events_[count_++]={sequence_,mode,after?1u:0u,outer.depth,
                          outer.completed,outer.callbacks};
    }
    bool Enabled() const noexcept{return enabled_;}
    std::uint32_t Count() const noexcept{return count_;}
    std::uint32_t Dropped() const noexcept{return dropped_;}
    const Event& At(std::uint32_t index) const noexcept{return events_[index];}
    void WriteJSON(std::ostream& out) const {
        out<<"{\"schema\":1,\"enabled\":"<<(enabled_?"true":"false")
           <<",\"request\":"<<request_<<",\"capacity\":"<<kCapacity
           <<",\"count\":"<<count_<<",\"dropped\":"<<dropped_<<",\"entries\":[";
        for(std::uint32_t i=0u;i<count_;++i){
            if(i)out<<',';const auto& e=events_[i];
            out<<"{\"sequence\":"<<e.sequence<<",\"mode\":"<<e.mode
               <<",\"after\":"<<(e.after?"true":"false")<<",\"outer_completed\":"<<e.outer_completed
               <<",\"outer_depth\":"<<e.outer_depth<<",\"outer_callbacks\":"<<e.outer_callbacks<<'}';
        }
        out<<"]}";
    }
private:
    std::array<Event,kCapacity> events_{};
    std::uint32_t request_{},count_{},dropped_{},sequence_{};
    bool enabled_{};
};
inline ComparisonCameraModeTrace& GetComparisonCameraModeTrace(){
    static ComparisonCameraModeTrace trace;return trace;
}
}
