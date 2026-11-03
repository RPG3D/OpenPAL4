#pragma once
#include "pal4/comparison_frame_gate.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace pal4 {
// Main-thread actual callback observer. Record stores opaque words only and
// never invokes game code, advances clocks, allocates, or alters FP control.
class ComparisonGobScriptTrace final {
public:
    static constexpr std::uint32_t kCapacity=512u;
    static constexpr std::uint32_t kMessage=0x851bu;
    struct Entry {
        std::uint64_t sequence;
        std::int32_t id;
        bool after;
        bool frame_gate_enabled;
        std::uint64_t outer_completed,outer_callbacks;
        std::uint32_t outer_depth;
        std::array<char,32> name;
        std::uint32_t name_length;
        bool name_truncated;
        std::array<std::uint32_t,5> args;
        std::uint32_t argc;
    };
    explicit ComparisonGobScriptTrace(bool enabled=false) noexcept:enabled_(enabled){}
    bool Enabled()const noexcept{return enabled_;}
    std::uint32_t Count()const noexcept{return count_;}
    std::uint64_t Sequence()const noexcept{return sequence_;}
    std::uint64_t Dropped()const noexcept{return dropped_;}
    const std::array<Entry,kCapacity>& Entries()const noexcept{return entries_;}
    void Record(std::int32_t id,bool after,std::string_view name,
        const std::array<std::uint32_t,5>& args,std::uint32_t argc) noexcept {
        if(!enabled_)return;
        const auto sequence=++sequence_;
        if(count_==kCapacity || argc>args.size()){++dropped_;return;}
        const auto& gate=GetComparisonFrameGate();
        const auto state=gate.Observe();
        auto& entry=entries_[count_++];
        entry.sequence=sequence;entry.id=id;entry.after=after;
        entry.frame_gate_enabled=gate.Enabled();
        entry.outer_completed=state.completed;entry.outer_callbacks=state.callbacks;entry.outer_depth=state.depth;
        entry.name_truncated=name.size()>=entry.name.size();
        entry.name_length=static_cast<std::uint32_t>(entry.name_truncated?entry.name.size()-1u:name.size());
        if(entry.name_length)std::memcpy(entry.name.data(),name.data(),entry.name_length);
        entry.name[entry.name_length]='\0';entry.argc=argc;
        if(argc)std::memcpy(entry.args.data(),args.data(),argc*sizeof(args[0]));
    }
    void Record(std::int32_t id,bool after,const char* name,
        const std::array<std::uint32_t,5>& args,std::uint32_t argc) noexcept {
        if(!enabled_)return; // Disabled callers need not provide readable text.
        std::size_t length{};
        if(name)while(length<32u && name[length]!='\0')++length;
        Record(id,after,std::string_view(name?name:"",length),args,argc);
    }
private:
    bool enabled_;
    std::uint32_t count_{};
    std::uint64_t sequence_{},dropped_{};
    std::array<Entry,kCapacity> entries_{};
};
static_assert(std::is_trivial_v<ComparisonGobScriptTrace::Entry> &&
    std::is_standard_layout_v<ComparisonGobScriptTrace::Entry>);
inline ComparisonGobScriptTrace& GetComparisonGobScriptTrace(){
    static ComparisonGobScriptTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
        std::getenv("PAL4_COMPARE_ATB_TRACE"));return trace;
}
}
