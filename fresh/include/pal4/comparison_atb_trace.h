#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <ostream>

namespace pal4 {
inline constexpr std::uint32_t kComparisonAtbTraceMessage = 0x850cu;
struct ComparisonAtbActor {
    std::uint32_t facet{}, id{}, group{}, progress_bits{}, turn{}, ready{}, action{};
};
struct ComparisonAtbSample {
    static constexpr std::uint32_t kActorLimit = 16;
    std::uint32_t state{}, mode{}, active{}, elapsed_bits{}, random_index{}, count{}, error{};
    std::array<ComparisonAtbActor, kActorLimit> actors{};
};

// Read only the actual manager's bounded facet view. No virtual calls, rate
// evaluation, RNG draws or game writes. Read supplies owned/native or original
// in-process words; invalid spans/groups remain explicit incomplete evidence.
template<class Read>
ComparisonAtbSample ReadComparisonAtbSample(std::uint32_t manager,
    std::uint32_t combat, std::uint32_t random_index, Read read) {
    ComparisonAtbSample s;
    s.state=read(combat+508u); s.mode=read(combat+516u)&255u;
    s.elapsed_bits=read(combat+512u); s.active=read(manager+124u)&255u;
    s.random_index=random_index;
    const auto begin=read(manager+112u),end=read(manager+116u);
    if(end<begin || (begin&3u) || (end&3u) || (end-begin)/4u>s.kActorLimit ||
        (begin==0u && end!=0u)) {s.error=1u;return s;}
    s.count=(end-begin)/4u;
    for(std::uint32_t n=0;n<s.count;++n){
        auto& a=s.actors[n];a.facet=read(begin+4u*n);
        if(a.facet<4096u+716u || (a.facet&3u) || a.facet>0xfffff000u){s.error=2u;return s;}
        const auto actor=a.facet-716u;
        a.group=read(actor+804u);
        if(a.group!=1u && a.group!=2u){s.error=3u;return s;}
        a.id=read(actor+(a.group==1u?1196u:960u));
        a.progress_bits=read(a.facet+4u);a.turn=read(actor+832u);
        a.ready=read(actor+828u);a.action=read(actor+660u);
    }
    return s;
}

struct ComparisonAtbRecord {
    std::uint64_t sequence{};
    std::uint32_t manager{}, delta_bits{}, argument_before{}, argument_after{};
    bool aborted{};
    ComparisonAtbSample before, after;
};

// Prefix buffer, allocated once before simulation. Updates only copy bounded
// PODs. Overflow/abort/reentry are exported, never silently treated as coverage.
class ComparisonAtbTrace final {
public:
    explicit ComparisonAtbTrace(bool enabled, std::uint32_t capacity=8192u,bool preparation=false,const char* boundary=nullptr)
        : enabled_(enabled), preparation_(preparation), boundary_(boundary), capacity_(enabled?capacity:0u),
          records_(capacity_?std::make_unique<ComparisonAtbRecord[]>(capacity_):nullptr) {}
    bool Enabled()const noexcept{return enabled_;}
    ComparisonAtbRecord* Begin(std::uint32_t manager,std::uint32_t delta_bits,
                              const ComparisonAtbSample& sample) noexcept {
        if(!enabled_)return nullptr;
        if(pending_){++nested_;return nullptr;}
        const auto sequence=attempted_++;
        if(count_==capacity_){++dropped_;return nullptr;}
        auto& r=records_[count_];r.sequence=sequence;r.manager=manager;
        r.delta_bits=delta_bits;r.before=sample;r.aborted=false;
        pending_=true;return &r;
    }
    void End(ComparisonAtbRecord* record,const ComparisonAtbSample& sample) noexcept {
        if(!record)return;
        record->after=sample;++count_;pending_=false;
    }
    void Abort(ComparisonAtbRecord* record) noexcept {
        if(!record)return;
        record->aborted=true;++aborted_;++count_;pending_=false;
    }
    ComparisonAtbRecord* BeginPreparation(std::uint32_t manager,std::uint32_t argument,
        const ComparisonAtbSample& sample) noexcept {
        auto* record=Begin(manager,0u,sample);
        if(record)record->argument_before=argument;
        return record;
    }
    void EndPreparation(ComparisonAtbRecord* record,std::uint32_t argument,
        const ComparisonAtbSample& sample) noexcept {
        if(record)record->argument_after=argument;
        End(record,sample);
    }
    void Write(std::ostream& out)const {
        out<<"{\"boundary\":\""<<(boundary_?boundary_:preparation_?"combat_prepare":"manager_update")
           <<"\",\"enabled\":"<<(enabled_?"true":"false")<<",\"capacity\":"<<capacity_
           <<",\"count\":"<<count_<<",\"dropped\":"<<dropped_<<",\"aborted\":"<<aborted_
           <<",\"nested\":"<<nested_<<",\"pending\":"<<(pending_?"true":"false")<<",\"records\":[";
        for(std::uint32_t n=0;n<count_;++n){
            if(n)out<<',';const auto& r=records_[n];
            out<<"{\"sequence\":"<<r.sequence<<",\"manager\":"<<r.manager
               <<",\"delta_bits\":"<<r.delta_bits<<",\"aborted\":"<<(r.aborted?"true":"false")
               <<",\"argument_before\":"<<r.argument_before<<",\"argument_after\":"<<r.argument_after
               <<",\"before\":";WriteSample(out,r.before);out<<",\"after\":";WriteSample(out,r.after);out<<'}';
        }
        out<<"]}";
    }
private:
    static void WriteSample(std::ostream& out,const ComparisonAtbSample& s){
        out<<"{\"state\":"<<s.state<<",\"mode\":"<<s.mode<<",\"active\":"<<s.active
           <<",\"elapsed_bits\":"<<s.elapsed_bits<<",\"random_index\":"<<s.random_index
           <<",\"count\":"<<s.count<<",\"error\":"<<s.error<<",\"actors\":[";
        for(std::uint32_t n=0;n<s.count;++n){
            if(n)out<<',';const auto& a=s.actors[n];
            out<<"{\"facet\":"<<a.facet<<",\"id\":"<<a.id<<",\"group\":"<<a.group
               <<",\"progress_bits\":"<<a.progress_bits<<",\"turn\":"<<a.turn
               <<",\"ready\":"<<a.ready<<",\"action\":"<<a.action<<'}';
        }
        out<<"]}";
    }
    bool enabled_{},preparation_{},pending_{};
    const char* boundary_{}; // Internal static literal; never supplied by game data.
    std::uint32_t capacity_{},count_{};
    std::uint64_t attempted_{},dropped_{},aborted_{},nested_{};
    std::unique_ptr<ComparisonAtbRecord[]> records_;
};

inline ComparisonAtbTrace& GetComparisonAtbTrace(){
    static ComparisonAtbTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
                                    std::getenv("PAL4_COMPARE_ATB_TRACE"));
    return trace;
}
// Separate prefix buffer: preparation events never masquerade as frame updates.
// Its parameter is the actual full DWORD before/after the original callback.
inline ComparisonAtbTrace& GetComparisonPrepareTrace(){
    static ComparisonAtbTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
                                    std::getenv("PAL4_COMPARE_ATB_TRACE"),64u,true);
    return trace;
}
// Command/target selection can last minutes. Capture all simulation frames,
//including paused manager frames, without filling the prefix on idle menus.
inline bool ShouldTraceComparisonCombatFrame(std::uint32_t state) noexcept {
    return (state>=1u && state<=3u) || state==6u;
}
inline ComparisonAtbTrace& GetComparisonCombatFrameTrace(){
    static ComparisonAtbTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
        std::getenv("PAL4_COMPARE_ATB_TRACE"),16384u,false,"combat_update");
    return trace;
}
}
