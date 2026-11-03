#pragma once
#include "pal4/comparison_random_call_trace.h"
#include <array>
#include <string_view>

namespace pal4 {
// Called under ComparisonCrtTrace's observation lock, after the public CRT call.
// These are bounded completion-order observations, never an RNG state owner.
class ComparisonCrtAttribution final {
public:
    using Record = ComparisonRandomCallTrace::Record;
    struct Counter {
        const char* source{};
        std::uint32_t caller{}, thread{}, source_line{};
        std::uint64_t count{};
        std::uint32_t last_value{};
        std::uint64_t last_sequence{};
    };
    struct Interval {
        Record seed{}, script{};
        bool has_seed{};
        std::array<Counter, 128> counters{};
        std::uint32_t used{};
        std::uint64_t calls{}, unclassified{};
        std::array<Record,256> prefix{};
        std::uint32_t prefix_used{};
    };
    void Add(const Record& record, bool script) {
        if (record.kind == 1u) {
            current_ = {};
            current_.seed = record;
            current_.has_seed = true;
            return;
        }
        if (script) {
            if (count_ == intervals_.size()) ++dropped_;
            else {
                current_.script = record;
                intervals_[count_++] = current_;
            }
        }
        ++current_.calls;
        if(current_.prefix_used<current_.prefix.size())
            current_.prefix[current_.prefix_used++]=record;
        for (std::uint32_t i = 0; i < current_.used; ++i) {
            auto& counter = current_.counters[i];
            if (counter.caller == record.caller && counter.thread == record.thread &&
                counter.source_line == record.source_line &&
                (counter.source == record.source || (counter.source && record.source &&
                 std::string_view(counter.source) == record.source))) {
                ++counter.count;
                counter.last_value = record.value;
                counter.last_sequence = record.full_sequence;
                return;
            }
        }
        if (current_.used == current_.counters.size()) ++current_.unclassified;
        else current_.counters[current_.used++] = {
            record.source, record.caller, record.thread, record.source_line,
            1u, record.value, record.full_sequence};
    }
    void Write(std::ostream& out) const {
        out << "{\"scope\":\"last observed seed to each script call; completion order, not atomic RNG execution\","
               "\"capacity\":64,\"dropped\":" << dropped_ << ",\"intervals\":[";
        for (std::uint32_t i = 0; i < count_; ++i) {
            if (i) out << ',';
            const auto& interval = intervals_[i];
            out << "{\"script_sequence\":" << interval.script.full_sequence
                << ",\"script_value\":" << interval.script.value
                << ",\"script_thread\":" << interval.script.thread << ',';
            WriteState(out, interval);
            out << '}';
        }
        // Held-frame inspection before giGetRandnum must not require advancing
        // gameplay until a script call happens merely to close an interval.
        out << "],\"current\":{";
        WriteState(out, current_);
        out << "}}";
    }
private:
    static void WriteState(std::ostream& out, const Interval& interval) {
        out << "\"seed\":";
        if (!interval.has_seed) out << "null";
        else out << "{\"sequence\":" << interval.seed.full_sequence
                 << ",\"value\":" << interval.seed.value
                 << ",\"thread\":" << interval.seed.thread << '}';
        out << ",\"calls\":" << interval.calls
            << ",\"unclassified\":" << interval.unclassified << ",\"counters\":[";
        for (std::uint32_t j = 0; j < interval.used; ++j) {
            if (j) out << ',';
            const auto& counter = interval.counters[j];
            out << "{\"caller\":" << counter.caller << ",\"thread\":" << counter.thread
                << ",\"source_line\":" << counter.source_line << ",\"count\":" << counter.count
                << ",\"source\":";
            WriteSource(out,counter.source);
            out << ",\"last_value\":" << counter.last_value
                << ",\"last_sequence\":" << counter.last_sequence << '}';
        }
        out << "],\"prefix_capacity\":" << interval.prefix.size()
            << ",\"prefix_dropped\":" << interval.calls-interval.prefix_used
            << ",\"prefix\":[";
        for(std::uint32_t i=0;i<interval.prefix_used;++i) {
            if(i)out << ',';
            const auto& r=interval.prefix[i];
            out << "{\"full_sequence\":" << r.full_sequence << ",\"value\":" << r.value
                << ",\"caller\":" << r.caller << ",\"thread\":" << r.thread
                << ",\"source_line\":" << r.source_line << ",\"source\":";
            WriteSource(out,r.source);
            out << '}';
        }
        out << ']';
    }
    static void WriteSource(std::ostream& out,const char* source) {
        if(!source){out << "null";return;}
        out << '"';
        for(auto* c=source;*c;++c){if(*c=='\\' || *c=='"')out << '\\';out << *c;}
        out << '"';
    }
    Interval current_{};
    std::array<Interval, 64> intervals_{};
    std::uint32_t count_{};
    std::uint64_t dropped_{};
};
} // namespace pal4
