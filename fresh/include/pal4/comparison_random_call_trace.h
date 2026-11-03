#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <ostream>

namespace pal4 {
enum class ComparisonRandomGenerator { crt, mt };
enum class ComparisonTraceRetention { Prefix, Recent };

// Completion-order observation only; tracing does not serialize RNG execution.
// source is a compile-time native literal, never borrowed gameplay text.
class ComparisonRandomCallTrace final {
public:
    struct Record {
        std::uint32_t kind{}, value{}, thread{}, caller{}, before{}, after{};
        const char* source{};
        std::uintptr_t owner{};
        std::uint32_t source_line{};
        std::uint64_t full_sequence{~std::uint64_t{0}};
    };
    explicit ComparisonRandomCallTrace(bool enabled, std::uint32_t capacity=262144u,
        ComparisonRandomGenerator generator=ComparisonRandomGenerator::crt,
        ComparisonTraceRetention retention=ComparisonTraceRetention::Prefix)
        : enabled_(enabled), capacity_(enabled?capacity:0u), generator_(generator),
          retention_(retention),
          records_(capacity_?std::make_unique<Record[]>(capacity_):nullptr) {}
    bool Enabled() const noexcept { return enabled_; }
    std::uint64_t Add(Record record) {
        if(!enabled_)return 0u;
        std::lock_guard<std::mutex> lock(mutex_);
        const auto sequence=static_cast<std::uint64_t>(count_)+dropped_;
        if(!capacity_){++dropped_;return sequence;}
        if(count_==capacity_ && retention_==ComparisonTraceRetention::Prefix){++dropped_;return sequence;}
        if(record.full_sequence==~std::uint64_t{0})record.full_sequence=sequence;
        const auto index=retention_==ComparisonTraceRetention::Recent
            ? static_cast<std::uint32_t>(sequence%capacity_) : count_;
        records_[index]=record;
        if(count_==capacity_)++dropped_;else ++count_;
        return sequence;
    }
    void Write(std::ostream& out) const {
        std::lock_guard<std::mutex> lock(mutex_);
        out<<"{\"generator\":\""<<(generator_==ComparisonRandomGenerator::mt?"mt":"crt")
           <<"\",\"enabled\":"<<(enabled_?"true":"false")<<",\"capacity\":"<<capacity_
           <<",\"count\":"<<count_<<",\"dropped\":"<<dropped_;
        const bool recent=retention_==ComparisonTraceRetention::Recent;
        if(recent)out<<",\"retention\":\"recent\",\"start_sequence\":"<<dropped_
                     <<",\"next_sequence\":"<<(dropped_+count_);
        out<<",\"ordering\":\"call completion; concurrent calls not atomic\",\"records\":[";
        for(std::uint32_t n=0;n<count_;++n){
            const auto sequence=recent?dropped_+n:n;
            const auto index=recent?static_cast<std::uint32_t>(sequence%capacity_):n;
            if(n)out<<',';const auto& r=records_[index];
            out<<"{\"sequence\":"<<sequence<<",\"kind\":"<<r.kind<<",\"value\":"<<r.value
               <<",\"thread\":"<<r.thread<<",\"caller\":"<<r.caller
               <<",\"state_before\":"<<r.before<<",\"state_after\":"<<r.after
               <<",\"owner\":"<<r.owner<<",\"source_line\":"<<r.source_line
               <<",\"full_sequence\":"<<r.full_sequence<<",\"source\":";
            if(!r.source)out<<"null";
            else {out<<'\"';for(auto* p=r.source;*p;++p){if(*p=='\\' || *p=='\"')out<<'\\';out<<*p;}out<<'\"';}
            out<<'}';
        }
        out<<"]}";
    }
private:
    bool enabled_{};
    std::uint32_t capacity_{},count_{};
    ComparisonRandomGenerator generator_;
    ComparisonTraceRetention retention_;
    std::uint64_t dropped_{};
    std::unique_ptr<Record[]> records_;
    mutable std::mutex mutex_;
};
} // namespace pal4
