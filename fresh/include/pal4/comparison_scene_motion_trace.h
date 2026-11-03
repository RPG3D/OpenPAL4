#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <ostream>

namespace pal4 {
struct ComparisonSceneMotionSample {
    std::array<std::uint32_t,3> position{},direction{};
};
inline ComparisonSceneMotionSample ReadComparisonSceneMotionSample(const void* actor) {
    ComparisonSceneMotionSample s;
    if (actor) {
        const auto* bytes=static_cast<const unsigned char*>(actor);
        std::memcpy(s.position.data(),bytes+172u,12u);
        std::memcpy(s.direction.data(),bytes+160u,12u);
    }
    return s;
}
template<class Read>
std::uint32_t ComparisonSceneMotionMask(const std::uint32_t* control,Read read) {
    if (!control || !control[5]) return 0u;
    const auto active=[&](unsigned count,unsigned flags){return static_cast<std::int32_t>(read(count))>0 && (read(flags)&15u);};
    std::uint32_t mask=control[9]?16u:0u;
    if(active(0x1f44,0x1f38)||active(0x1e34,0x1e28))mask|=1u;
    else if(active(0x1f54,0x1f48)||active(0x1df4,0x1de8))mask|=2u;
    if(active(0x1f24,0x1f18)||active(0x1cd4,0x1cc8))mask|=4u;
    else if(active(0x1f34,0x1f28)||active(0x1d04,0x1cf8))mask|=8u;
    return mask;
}
// Same4283B0 boundary in both programs. Bounded POD copies only during input;
// export is explicit. Float bit patterns preserve blocked steps and tiny drift.
class ComparisonSceneMotionTrace final {
    static constexpr std::uint32_t kCapacity=16384u;
public:
    explicit ComparisonSceneMotionTrace(bool enabled):enabled_(enabled),records_(enabled?new Record[kCapacity]{}:nullptr){}
    bool Enabled()const noexcept{return enabled_;}
    void RecordStep(std::uint32_t mask,float elapsed,const ComparisonSceneMotionSample& before,
                    const ComparisonSceneMotionSample& after) noexcept {
        if(!enabled_ || !mask)return;
        ++attempted_;
        if(count_==kCapacity){++dropped_;return;}
        auto& r=records_[count_++];r.mask=mask;std::memcpy(&r.elapsed_bits,&elapsed,4u);r.before=before;r.after=after;
    }
    void Write(std::ostream& out)const{
        out<<"{\"enabled\":"<<(enabled_?"true":"false")<<",\"boundary\":\"4283B0\",\"capacity\":"<<kCapacity<<",\"attempted\":"<<attempted_<<",\"dropped\":"<<dropped_<<",\"records\":[";
        for(std::uint32_t i=0;i<count_;++i){if(i)out<<',';const auto& r=records_[i];out<<"{\"mask\":"<<r.mask<<",\"elapsed_bits\":"<<r.elapsed_bits<<",\"before\":";Sample(out,r.before);out<<",\"after\":";Sample(out,r.after);out<<'}';}out<<"]}";
    }
private:
    static void Sample(std::ostream& out,const ComparisonSceneMotionSample& s){
        out<<"{\"position_bits\":[";for(unsigned i=0;i<3;++i){if(i)out<<',';out<<s.position[i];}out<<"],\"direction_bits\":[";for(unsigned i=0;i<3;++i){if(i)out<<',';out<<s.direction[i];}out<<"]}";
    }
    struct Record{std::uint32_t mask{},elapsed_bits{};ComparisonSceneMotionSample before,after;};
    bool enabled_{};std::uint32_t count_{};std::uint64_t attempted_{},dropped_{};std::unique_ptr<Record[]> records_;
};
inline ComparisonSceneMotionTrace& GetComparisonSceneMotionTrace(){
    static ComparisonSceneMotionTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") && std::getenv("PAL4_COMPARE_ATB_TRACE"));return trace;
}
}
