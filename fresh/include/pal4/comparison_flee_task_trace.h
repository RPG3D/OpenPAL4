#pragma once
#include "pal4/comparison_frame_gate.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <ostream>
#include <string_view>

namespace pal4 {
struct ComparisonFleeTaskSample {
    std::uint32_t count{},ready{},sampled{};
    struct Task{std::uint32_t address{};std::array<std::uint32_t,15> words{};
        bool run_sampled{};std::array<std::uint32_t,7> run_words{};};
    std::array<Task,8> tasks{};
};
class ComparisonFleeTaskTrace final {
public:
    struct Record{std::uint32_t container{},elapsed_bits{};std::uint64_t begin_ns{},end_ns{},outer_completed{},outer_callbacks{};std::uint32_t outer_depth{};ComparisonFleeTaskSample before,after;bool completed{};};
    explicit ComparisonFleeTaskTrace(bool enabled,bool approach=false):enabled_(enabled),approach_(approach),capacity_(approach?16384u:2048u),records_(enabled?new Record[capacity_]{}:nullptr){}
    [[nodiscard]] bool Enabled()const noexcept{return enabled_;}
    // Native dispatch milestones:1 raw Win32,2 UI action,3/4 factory,
    //5/6 recipe preparation,7/8 BeginWaiting. Not gameplay events.
    // Scenario diagnostics19 mouse-up(button,owner),20 isTypeEnd,21 page
    // predicate(value,edit),22 changePage,23 changeLine,24 hide,25 advance,
    //26 successfully shown UTF-8 text(FNV-1a32,display value).
    // Skill diagnostics28 actor AMF(name hash,task),29 effect message(name
    // hash,task),30 effect command(hash,task),31 created(task,handle id),
    //32 received(task,handle id),33 cbcast(handle id,task),34 cbattachcast,
    //35 retired effect callback whose raw address aliases a current task.
    // Report diagnostics36 total/rank;37 score,38 count,39 coefficient,
    //40 limit(index,value);41 initial score/rank thresholds(index,value).
    void Mark(std::uint32_t kind,std::uint32_t first=0u,std::uint32_t second=0u) noexcept {
        if(!enabled_)return;
        if(mark_count_==marks_.size())++marks_dropped_;else ++mark_count_;
        marks_[mark_next_]={Now(),kind,first,second};
        mark_next_=(mark_next_+1u)%marks_.size();
    }
    void MarkText(std::uint32_t kind,std::string_view text,std::uint32_t value) noexcept {
        if(!enabled_)return;
        std::uint32_t hash=2166136261u;
        for(const unsigned char byte:text){hash^=byte;hash*=16777619u;}
        Mark(kind,hash,value);
    }
    template<class Read> Record* Begin(std::uint32_t container,float delta,Read read){
        if(!enabled_)return nullptr;
        const auto before=Sample(container,read);bool flee{};
        for(unsigned i=0;i<before.sampled;++i)flee=flee || before.tasks[i].words[0]==0x845d3cu || before.tasks[i].words[0]==0x845d58u ||
            (approach_ && (before.tasks[i].words[0]==0x845740u || before.tasks[i].words[0]==0x845760u));
        if(!flee)return nullptr;
        if(count_==capacity_){++dropped_;return nullptr;}
        auto& r=records_[count_++];r.container=container;r.before=before;r.begin_ns=Now();std::memcpy(&r.elapsed_bits,&delta,4u);
        const auto gate=GetComparisonFrameGate().Observe();r.outer_completed=gate.completed;r.outer_callbacks=gate.callbacks;r.outer_depth=gate.depth;return &r;
    }
    template<class Read> void End(Record* record,Read read){if(record){record->after=Sample(record->container,read);record->end_ns=Now();record->completed=true;}}
    void Write(std::ostream& out)const{
        out<<"{\"enabled\":"<<(enabled_?"true":"false")<<",\"capacity\":"<<capacity_<<",\"dropped\":"<<dropped_<<",\"records\":[";
        for(unsigned i=0;i<count_;++i){if(i)out<<',';const auto& r=records_[i];out<<"{\"container\":"<<r.container<<",\"elapsed_bits\":"<<r.elapsed_bits<<",\"outer_completed\":"<<r.outer_completed<<",\"outer_callbacks\":"<<r.outer_callbacks<<",\"outer_depth\":"<<r.outer_depth<<",\"begin_ns\":"<<r.begin_ns<<",\"end_ns\":"<<r.end_ns<<",\"completed\":"<<(r.completed?"true":"false")<<",\"before\":";WriteSample(out,r.before);out<<",\"after\":";WriteSample(out,r.after);out<<'}';}
        out<<"],\"marks_policy\":\"recent_ring\",\"marks_capacity\":"<<marks_.size()<<",\"marks_dropped\":"<<marks_dropped_<<",\"marks\":[";
        for(unsigned i=0;i<mark_count_;++i){if(i)out<<',';const auto& m=marks_[(mark_next_+marks_.size()-mark_count_+i)%marks_.size()];out<<"{\"ns\":"<<m.ns<<",\"kind\":"<<m.kind<<",\"first\":"<<m.first<<",\"second\":"<<m.second<<'}';}out<<"]}";
    }
private:
    static std::uint64_t Now(){return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    template<class Read> ComparisonFleeTaskSample Sample(std::uint32_t c,Read read) const {
        ComparisonFleeTaskSample s;s.count=read(c+8u);s.ready=read(c+12u)&255u;
        if(!s.count)return s;
        const auto head=read(c+4u);auto node=read(head);
        for(unsigned i=0;i<8u && i<s.count && node!=head;++i){auto& t=s.tasks[s.sampled++];t.address=read(node+8u);for(unsigned w=0;w<15u;++w)t.words[w]=read(t.address+4u*w);t.words[1]&=255u;if(approach_ && t.words[0]==0x845740u){t.run_sampled=true;for(unsigned w=0;w<7u;++w)t.run_words[w]=read(t.address+60u+4u*w);}node=read(node);}return s;
    }
    static void WriteSample(std::ostream& out,const ComparisonFleeTaskSample& s){
        out<<"{\"count\":"<<s.count<<",\"ready\":"<<s.ready<<",\"sampled\":"<<s.sampled<<",\"tasks\":[";
        for(unsigned i=0;i<s.sampled;++i){if(i)out<<',';const auto& t=s.tasks[i];out<<"{\"address\":"<<t.address<<",\"words\":[";for(unsigned w=0;w<15u;++w){if(w)out<<',';out<<t.words[w];}out<<"],\"run_sampled\":"<<(t.run_sampled?"true":"false")<<",\"run_words\":[";if(t.run_sampled)for(unsigned w=0;w<7u;++w){if(w)out<<',';out<<t.run_words[w];}out<<"]}";}out<<"]}";
    }
    bool enabled_{},approach_{};unsigned capacity_{},count_{};std::uint64_t dropped_{};std::unique_ptr<Record[]> records_;
    struct MarkValue{std::uint64_t ns{};std::uint32_t kind{},first{},second{};};
    std::array<MarkValue,512> marks_{};unsigned mark_count_{};std::size_t mark_next_{};std::uint64_t marks_dropped_{};
};
inline ComparisonFleeTaskTrace& GetComparisonFleeTaskTrace(){static ComparisonFleeTaskTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") && std::getenv("PAL4_COMPARE_ATB_TRACE"),std::getenv("PAL4_COMPARE_APPROACH_TRACE")!=nullptr);return trace;}
}
