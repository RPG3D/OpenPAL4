#pragma once

#include "pal4/comparison_frame_gate.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <ostream>
#include <stdexcept>
#ifdef _WIN32
#include <process.h>
#endif

namespace pal4 {
// Opt-in observation of vertical 661A50 queries outside camera updates.
// Captures actual SDK outputs; never evaluates or substitutes an intersection.
class ComparisonClumpQueryTrace final {
public:
    static constexpr unsigned kCapacity=256u, kNone=~0u;
    struct Row {
        std::array<std::uint32_t,6> ray{};
        std::array<std::uint32_t,4> hit{};
        std::uint64_t completed{},callbacks{};
        std::uint32_t owner{},result{},atomic{},triangle{},cw_before{},cw_after{},sse_before{},sse_after{};
        bool finished{};
    };
    explicit ComparisonClumpQueryTrace(bool enabled=false,std::array<float,6> box={})
        :enabled_(enabled),box_(box) {
        if(enabled)for(unsigned i=0;i<3;++i)
            if(!std::isfinite(box[2*i]) || !std::isfinite(box[2*i+1]) || box[2*i]>box[2*i+1])
                throw std::invalid_argument("clump query trace bounds");
    }
    bool Contains(const float* point)const noexcept {
        if(!enabled_ || !point)return false;
        for(unsigned i=0;i<3;++i)if(!(point[i]>=box_[2*i] && point[i]<=box_[2*i+1]))return false;
        return true;
    }
    unsigned Begin(const void* ray,std::uintptr_t owner,unsigned cw,unsigned sse) noexcept {
        if(!enabled_ || !ray)return kNone;
        std::array<float,6> value{};std::memcpy(value.data(),ray,24u);
        if(value[0]!=value[3] || value[2]!=value[5])return kNone;
        for(unsigned i=0;i<3;++i)
            if(!(value[i]>=box_[2*i] && value[i]<=box_[2*i+1]))return kNone;
        if(count_==kCapacity){++dropped_;return kNone;}
        auto& row=rows_[count_];std::memcpy(row.ray.data(),ray,24u);
        const auto gate=GetComparisonFrameGate().Observe();
        row.completed=gate.completed;row.callbacks=gate.callbacks;
        row.owner=static_cast<std::uint32_t>(owner);row.cw_before=cw;row.sse_before=sse;
        return count_++;
    }
    void End(unsigned ticket,unsigned result,const float* hit,unsigned atomic,unsigned triangle,
             unsigned cw,unsigned sse) noexcept {
        if(ticket==kNone || ticket>=count_ || rows_[ticket].finished)return;
        auto& row=rows_[ticket];row.result=result;row.atomic=atomic;row.triangle=triangle;
        row.cw_after=cw;row.sse_after=sse;row.finished=true;
        if(result && hit)std::memcpy(row.hit.data(),hit,16u);
        Publish();
    }
    void Write(std::ostream& out)const {
        out<<"{\"count\":"<<count_<<",\"dropped\":"<<dropped_<<",\"queries\":[";
        for(unsigned i=0;i<count_;++i){if(i)out<<',';const auto& r=rows_[i];
            out<<"{\"completed\":"<<r.completed<<",\"callbacks\":"<<r.callbacks
               <<",\"owner\":"<<r.owner<<",\"result\":"<<r.result<<",\"atomic\":"<<r.atomic
               <<",\"triangle\":"<<r.triangle<<",\"finished\":"<<(r.finished?"true":"false")
               <<",\"cw_before\":"<<r.cw_before<<",\"cw_after\":"<<r.cw_after
               <<",\"sse_before\":"<<r.sse_before<<",\"sse_after\":"<<r.sse_after<<",\"ray\":";
            Words(out,r.ray);out<<",\"hit\":";Words(out,r.hit);out<<'}';}
        out<<"]}";
    }
private:
    template<class T>static void Words(std::ostream& out,const T& values){out<<'[';bool first=true;for(auto v:values){if(!first)out<<',';first=false;out<<v;}out<<']';}
    void Publish()const noexcept {
#ifdef _WIN32
        try {
            const auto* root=std::getenv("PAL4_COMPARE_STATE_DIR");
            if(!root)return;
            const auto path=std::filesystem::path(root)/("compare-clump-queries-"+std::to_string(_getpid())+".json");
            auto temp=path;temp+=".tmp";std::ofstream out(temp);Write(out);out.flush();
            const bool good=bool(out);out.close();if(good)std::filesystem::rename(temp,path);
        }catch(...){}
#endif
    }
    bool enabled_{};std::array<float,6> box_{};unsigned count_{},dropped_{};
    std::array<Row,kCapacity> rows_{};
};
inline ComparisonClumpQueryTrace& GetComparisonClumpQueryTrace(){
    static ComparisonClumpQueryTrace trace=[] {
        const auto* value=std::getenv("PAL4_COMPARE_CLUMP_QUERY_BOX");
        if(!std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") || !value)return ComparisonClumpQueryTrace();
        std::array<float,6> box{};
        for(unsigned i=0;i<6;++i){char* end{};box[i]=std::strtof(value,&end);
            if(end==value || (i<5 ? *end!=',' : *end!='\0'))throw std::invalid_argument("clump query trace six-number box");
            value=end+(i<5?1:0);}
        return ComparisonClumpQueryTrace(true,box);
    }();return trace;
}
} // namespace pal4
