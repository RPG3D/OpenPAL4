#pragma once
#include "pal4/comparison_clump_query_trace.h"
#include <utility>

namespace pal4 {
// Observe the existing 5FF680 boundary, including mutations to its displacement.
// This is independent of the camera update observer and defaults to disabled.
class ComparisonMovementCollisionScope final {
    struct Row {
        std::array<std::uint32_t,4> from{},to{};
        std::array<std::uint32_t,3> before{},after{};
        std::uint64_t completed{},callbacks{};
        std::uint32_t blocked{},owner{};
    };
    struct Storage {unsigned count{},dropped{};std::array<Row,128> rows{};};
    static Storage& Data(){static Storage value;return value;}
public:
    static void RecordSegment([[maybe_unused]] const std::array<float,3>& point,[[maybe_unused]] const std::array<float,3>& start,
                              [[maybe_unused]] const std::array<float,3>& end,[[maybe_unused]] const std::array<float,3>& output)noexcept {
#ifdef _WIN32
        if(!GetComparisonClumpQueryTrace().Contains(point.data()))return;
        try {
            static unsigned count{};if(count>=256u)return;
            const auto* root=std::getenv("PAL4_COMPARE_STATE_DIR");if(!root)return;
            const auto path=std::filesystem::path(root)/("compare-segment-projection-"+std::to_string(_getpid())+".jsonl");
            std::ofstream out(path,count?std::ios::app:std::ios::trunc);
            out<<"{\"index\":"<<count++<<",\"completed\":"<<GetComparisonFrameGate().Observe().completed;
            for(const auto& entry:{std::make_pair("point",&point),std::make_pair("start",&start),std::make_pair("end",&end),std::make_pair("output",&output)}){
                std::array<std::uint32_t,3> bits{};std::memcpy(bits.data(),entry.second->data(),12u);
                out<<",\""<<entry.first<<"\":";Words(out,bits);
            }
            out<<"}\n";
        }catch(...){}
#endif
    }
    ComparisonMovementCollisionScope(const float* from,const float* to,const float* movement,
                                     const std::uint32_t* blocked,std::uintptr_t owner)
        :movement_(movement),blocked_(blocked) {
        if(!from || !to || !movement || !GetComparisonClumpQueryTrace().Contains(from))return;
        auto& data=Data();if(data.count==data.rows.size()){++data.dropped;return;}
        index_=data.count++;auto& row=data.rows[index_];
        std::memcpy(row.from.data(),from,16u);std::memcpy(row.to.data(),to,16u);
        std::memcpy(row.before.data(),movement,12u);row.owner=static_cast<std::uint32_t>(owner);
        const auto gate=GetComparisonFrameGate().Observe();row.completed=gate.completed;row.callbacks=gate.callbacks;
    }
    ComparisonMovementCollisionScope(const ComparisonMovementCollisionScope&)=delete;
    ~ComparisonMovementCollisionScope(){
        if(index_==~0u)return;
        auto& row=Data().rows[index_];std::memcpy(row.after.data(),movement_,12u);
        if(blocked_)row.blocked=*blocked_;
        Publish();
    }
private:
    template<class T>static void Words(std::ostream& out,const T& words){out<<'[';bool first=true;for(auto w:words){if(!first)out<<',';first=false;out<<w;}out<<']';}
    static void Publish()noexcept {
#ifdef _WIN32
        try {
            const auto* root=std::getenv("PAL4_COMPARE_STATE_DIR");if(!root)return;
            const auto path=std::filesystem::path(root)/("compare-movement-collision-"+std::to_string(_getpid())+".json");
            auto temp=path;temp+=".tmp";std::ofstream out(temp);const auto& data=Data();
            out<<"{\"count\":"<<data.count<<",\"dropped\":"<<data.dropped<<",\"calls\":[";
            for(unsigned i=0;i<data.count;++i){if(i)out<<',';const auto& r=data.rows[i];
                out<<"{\"completed\":"<<r.completed<<",\"callbacks\":"<<r.callbacks<<",\"owner\":"<<r.owner<<",\"blocked\":"<<r.blocked<<",\"from\":";Words(out,r.from);
                out<<",\"to\":";Words(out,r.to);out<<",\"before\":";Words(out,r.before);out<<",\"after\":";Words(out,r.after);out<<'}';}
            out<<"]}";out.flush();const bool good=bool(out);out.close();if(good)std::filesystem::rename(temp,path);
        }catch(...){}
#endif
    }
    unsigned index_=~0u;const float* movement_;const std::uint32_t* blocked_;
};
} // namespace pal4
