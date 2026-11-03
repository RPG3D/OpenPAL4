#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <ostream>
#include <source_location>

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
    explicit ComparisonGameHeapTrace(bool enabled,std::uint32_t recent_capacity=8192u,
                                    std::uint32_t image_capacity=65536u)
        :enabled_(enabled),recent_(enabled?recent_capacity:0u),images_(enabled?image_capacity:0u){}
    bool Enabled()const noexcept{return enabled_;}
    void Add(Record record) {
        if(!enabled_)return;
        std::lock_guard lock(mutex_);
        record.sequence=total_++;
        recent_.Add(record);
        if(record.usable_before==172u || record.usable_after==172u ||
           ((record.kind==0u || record.kind==2u) && record.size==168u))images_.Add(record);
    }
    void Write(std::ostream& out)const {
        std::lock_guard lock(mutex_);
        out<<"{\"enabled\":"<<(enabled_?"true":"false")<<",\"total\":"<<total_
           <<",\"ordering\":\"outer API call completion; game execution is not serialized\","
             "\"image_filter\":\"usable size172 or malloc/realloc request168; not semantic object identity\","
             "\"recent\":";
        recent_.Write(out);out<<",\"image_candidates\":";images_.Write(out);out<<'}';
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
