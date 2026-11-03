#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <ostream>
#include <optional>
#include <charconv>
#include <stdexcept>

namespace pal4 {
// Stages1/2: two verified65F840 calls in65F1E0. Stage3: preceding65F100
// constraint (factor_bits holds current speed). POD bit copies only;
// Stage4: constraint-entry elapsed/position/acceleration/first-axis inputs.
// no frame/clock/RNG calls or gameplay writes. Unknown callers are not owners.
class ComparisonHelperInterpolationTrace final {
public:
    explicit ComparisonHelperInterpolationTrace(bool enabled,
        std::uint32_t capacity=8192u,std::optional<std::uint32_t> filter_id={}):
        enabled_(enabled),filter_id_(filter_id),capacity_(enabled?capacity:0u),
        entries_(capacity_?std::make_unique<Entry[]>(capacity_):nullptr){}
    bool Enabled()const noexcept{return enabled_;}
    void Add(std::uint32_t stage,std::uint32_t id,float factor,
             const std::array<float,3>& first,const std::array<float,3>& second,
             const std::array<float,3>& result) {
        if(!enabled_ || (filter_id_ && id!=*filter_id_))return;
        std::lock_guard<std::mutex> guard(mutex_);
        if(count_==capacity_){++dropped_;return;}
        auto& e=entries_[count_++];e.stage=stage;e.id=id;
        std::memcpy(&e.factor,&factor,4);
        std::memcpy(e.first.data(),first.data(),12);
        std::memcpy(e.second.data(),second.data(),12);
        std::memcpy(e.result.data(),result.data(),12);
    }
    void Write(std::ostream& out)const {
        std::lock_guard<std::mutex> guard(mutex_);
        out<<"{\"enabled\":"<<(enabled_?"true":"false")<<",\"capacity\":"<<capacity_
           <<",\"count\":"<<count_<<",\"dropped\":"<<dropped_<<",\"filter_id\":";
        if(filter_id_)out<<*filter_id_;else out<<"null";
        out<<",\"records\":[";
        for(std::uint32_t n=0;n<count_;++n){
            if(n)out<<',';const auto& e=entries_[n];
            out<<"{\"stage\":"<<e.stage<<",\"id\":"<<e.id<<",\"factor_bits\":"<<e.factor;
            Vector(out,"first_bits",e.first);Vector(out,"second_bits",e.second);
            Vector(out,"result_bits",e.result);out<<'}';
        }
        out<<"]}";
    }
private:
    static void Vector(std::ostream& out,const char* name,const std::array<std::uint32_t,3>& v){
        out<<",\""<<name<<"\":["<<v[0]<<','<<v[1]<<','<<v[2]<<']';
    }
    struct Entry{std::uint32_t stage{},id{},factor{};
        std::array<std::uint32_t,3> first{},second{},result{};};
    bool enabled_{};std::optional<std::uint32_t> filter_id_;
    std::uint32_t capacity_{},count_{};std::uint64_t dropped_{};
    std::unique_ptr<Entry[]> entries_;mutable std::mutex mutex_;
};
// getenv is read-only here; keep the standard API on every host without
// disabling secure-CRT diagnostics for unrelated code in including modules.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
inline ComparisonHelperInterpolationTrace& GetComparisonHelperInterpolationTrace(){
    static const auto filter=[]()->std::optional<std::uint32_t>{
        const auto* text=std::getenv("PAL4_COMPARE_HELPER_INTERPOLATION_ID");
        if(!text)return {};
        std::uint32_t value{};const auto* end=text+std::strlen(text);
        const auto parsed=std::from_chars(text,end,value);
        if(parsed.ec!=std::errc{} || parsed.ptr!=end)
            throw std::invalid_argument("invalid PAL4_COMPARE_HELPER_INTERPOLATION_ID");
        return value;
    }();
    static ComparisonHelperInterpolationTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
        std::getenv("PAL4_COMPARE_HELPER_INTERPOLATION"),8192u,filter);return trace;
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
} // namespace pal4
