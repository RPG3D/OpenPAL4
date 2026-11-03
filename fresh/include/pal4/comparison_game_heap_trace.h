#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <ostream>
#include <source_location>
#include <array>

namespace pal4 {
struct ComparisonGameHeapOrigin {
    const char* source{};
    std::uint32_t line{};
};
inline thread_local ComparisonGameHeapOrigin comparison_game_heap_origin;
class ComparisonGameHeapOriginScope final {
public:
    explicit ComparisonGameHeapOriginScope(std::source_location source)
        :prior_(comparison_game_heap_origin) {
        comparison_game_heap_origin={source.file_name(),source.line()};
    }
    ~ComparisonGameHeapOriginScope(){comparison_game_heap_origin=prior_;}
    ComparisonGameHeapOriginScope(const ComparisonGameHeapOriginScope&)=delete;
private:
    ComparisonGameHeapOrigin prior_;
};

// Records completed outer API calls. Nested calloc/realloc implementation calls
// are excluded on both sides. This observer never owns gameplay allocations.
class ComparisonGameHeapTrace final {
public:
    struct Record {
        std::uint32_t kind{},size{},secondary{},before{},after{},usable_before{},usable_after{};
        std::uint32_t caller{},thread{},source_line{};
        const char* source{};
        std::uint64_t sequence{};
    };
    // kind4=sbrk, kind5=mmap. Size is the original signed-long bit pattern.
    // heap_completed anchors backend completion before its outer API record.
    struct BackendRecord {
        std::uint32_t kind{},size_bits{},requested{},result{},top{},properties{},caller{},thread{};
        std::uint64_t sequence{},heap_completed{};
        std::uint32_t windows_ui{},core_messaging{};
    };
    struct FailureRecord {
        std::uint64_t backend_sequence{},heap_completed{};
        std::uint32_t stack_count{},query_valid{};
        std::array<std::uint32_t,16> stack{};
        // BaseAddress, AllocationBase, RegionSize, State, Protect,
        // AllocationProtect, Type; sampled after the failed backend call.
        std::array<std::uint32_t,7> top_region{},after_reservation{};
    };
    explicit ComparisonGameHeapTrace(bool enabled,std::uint32_t recent_capacity=8192u,
                                    std::uint32_t image_capacity=65536u,
                                    std::uint32_t backend_capacity=32768u)
        :enabled_(enabled),recent_(enabled?recent_capacity:0u),images_(enabled?image_capacity:0u),
         backend_capacity_(enabled?backend_capacity:0u),
         backend_(backend_capacity_?std::make_unique<BackendRecord[]>(backend_capacity_):nullptr),
         failures_(enabled?std::make_unique<FailureRecord[]>(failure_capacity):nullptr){}
    bool Enabled()const noexcept{return enabled_;}
    void Add(Record record) {
        if(!enabled_)return;
        std::lock_guard lock(mutex_);
        record.sequence=total_++;
        recent_.Add(record);
        if(record.usable_before==172u || record.usable_after==172u ||
           ((record.kind==0u || record.kind==2u) && record.size==168u))images_.Add(record);
    }
    void AddBackend(BackendRecord record,const FailureRecord* failure=nullptr) {
        if(!enabled_ || !backend_capacity_)return;
        std::lock_guard lock(mutex_);
        record.sequence=backend_total_++;
        record.heap_completed=total_;
        backend_[static_cast<std::uint32_t>(record.sequence%backend_capacity_)]=record;
        if(failure) {
            auto& copy=failures_[static_cast<std::uint32_t>(failure_total_++%failure_capacity)];
            copy=*failure;copy.backend_sequence=record.sequence;copy.heap_completed=record.heap_completed;
        }
    }
    void Write(std::ostream& out)const {
        std::lock_guard lock(mutex_);
        out<<"{\"enabled\":"<<(enabled_?"true":"false")<<",\"total\":"<<total_
           <<",\"ordering\":\"outer API call completion; game execution is not serialized\","
             "\"image_filter\":\"usable size172 or malloc/realloc request168; not semantic object identity\","
             "\"recent\":";
        recent_.Write(out);out<<",\"image_candidates\":";images_.Write(out);
        const auto count=backend_total_<backend_capacity_?backend_total_:backend_capacity_;
        const auto dropped=backend_total_-count;
        out<<",\"backend\":{\"capacity\":"<<backend_capacity_<<",\"count\":"<<count
           <<",\"dropped\":"<<dropped<<",\"ordering\":\"backend completion; top/properties sampled before call; heap_completed counts completed outer calls\",\"records\":[";
        for(std::uint64_t n=0;n<count;++n) {
            if(n)out<<',';
            const auto& r=backend_[static_cast<std::uint32_t>((dropped+n)%backend_capacity_)];
            out<<"{\"sequence\":"<<r.sequence<<",\"kind\":"<<r.kind<<",\"size_bits\":"<<r.size_bits
               <<",\"requested\":"<<r.requested<<",\"result\":"<<r.result<<",\"top\":"<<r.top
               <<",\"properties\":"<<r.properties<<",\"caller\":"<<r.caller<<",\"thread\":"<<r.thread
               <<",\"heap_completed\":"<<r.heap_completed
               <<",\"windows_ui\":"<<r.windows_ui<<",\"core_messaging\":"<<r.core_messaging<<'}';
        }
        out<<"]},\"backend_failures\":{\"capacity\":"<<(enabled_?failure_capacity:0)
           <<",\"count\":"<<(failure_total_<failure_capacity?failure_total_:failure_capacity)
           <<",\"dropped\":"<<(failure_total_>failure_capacity?failure_total_-failure_capacity:0)
           <<",\"records\":[";
        const auto failure_count=failure_total_<failure_capacity?failure_total_:failure_capacity;
        for(std::uint64_t n=0;n<failure_count;++n) {
            if(n)out<<',';
            const auto& r=failures_[static_cast<std::uint32_t>((failure_total_-failure_count+n)%failure_capacity)];
            out<<"{\"backend_sequence\":"<<r.backend_sequence<<",\"heap_completed\":"<<r.heap_completed
               <<",\"query_valid\":"<<r.query_valid<<",\"stack\":[";
            for(std::uint32_t i=0;i<r.stack_count && i<r.stack.size();++i){if(i)out<<',';out<<r.stack[i];}
            out<<"],\"top_region\":[";
            for(std::size_t i=0;i<r.top_region.size();++i){if(i)out<<',';out<<r.top_region[i];}
            out<<"],\"after_reservation\":[";
            for(std::size_t i=0;i<r.after_reservation.size();++i){if(i)out<<',';out<<r.after_reservation[i];}
            out<<"]}";
        }
        out<<"]}}";
    }
private:
    class Ring {
    public:
        explicit Ring(std::uint32_t capacity):capacity_(capacity),
            records_(capacity?std::make_unique<Record[]>(capacity):nullptr){}
        void Add(const Record& record) {
            if(!capacity_)return;
            records_[static_cast<std::uint32_t>(total_%capacity_)]=record;++total_;
        }
        void Write(std::ostream& out)const {
            const auto count=total_<capacity_?total_:capacity_;
            const auto dropped=total_-count;
            out<<"{\"capacity\":"<<capacity_<<",\"count\":"<<count<<",\"dropped\":"<<dropped
               <<",\"records\":[";
            for(std::uint64_t n=0;n<count;++n) {
                if(n)out<<',';
                const auto& r=records_[static_cast<std::uint32_t>((dropped+n)%capacity_)];
                out<<"{\"sequence\":"<<r.sequence<<",\"kind\":"<<r.kind<<",\"size\":"<<r.size
                   <<",\"secondary\":"<<r.secondary<<",\"before\":"<<r.before<<",\"after\":"<<r.after
                   <<",\"usable_before\":"<<r.usable_before<<",\"usable_after\":"<<r.usable_after
                   <<",\"caller\":"<<r.caller<<",\"thread\":"<<r.thread<<",\"source_line\":"<<r.source_line
                   <<",\"source\":";
                if(!r.source)out<<"null";
                else {
                    out<<'"';for(const char* p=r.source;*p;++p){if(*p=='\\' || *p=='"')out<<'\\';out<<*p;}out<<'"';
                }
                out<<'}';
            }
            out<<"]}";
        }
    private:
        std::uint32_t capacity_{};
        std::uint64_t total_{};
        std::unique_ptr<Record[]> records_;
    };
    bool enabled_{};
    std::uint64_t total_{};
    Ring recent_,images_;
    std::uint32_t backend_capacity_{};
    std::unique_ptr<BackendRecord[]> backend_;
    std::uint64_t backend_total_{};
    static constexpr std::uint32_t failure_capacity=128;
    std::unique_ptr<FailureRecord[]> failures_;
    std::uint64_t failure_total_{};
    mutable std::mutex mutex_;
};
inline ComparisonGameHeapTrace& GetComparisonGameHeapTrace() {
    static ComparisonGameHeapTrace trace([] {
        const auto* scripted=std::getenv("PAL4_COMPARE_SCRIPTED_INPUT");
        const auto* heap=std::getenv("PAL4_COMPARE_GAME_HEAP_TRACE");
        return scripted && std::strcmp(scripted,"1")==0 && heap && std::strcmp(heap,"1")==0;
    }());
    return trace;
}
}
