#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <locale>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace pal4 {
// Original4317F0/native MonsterBehaviorManager::Update. These are raw words,
// including float bits; capturing them never advances a frame or a game RNG.
struct ComparisonSceneMonsterSample {
    std::array<std::uint32_t,3> position{},direction{},target_position{};
    std::array<std::uint32_t,6> timers{}; // +6F8..70C
    std::uint32_t active{},pursuit{},movement_state{},target{},target_active{},speed_bits{};
    bool operator==(const ComparisonSceneMonsterSample&)const=default;
};
inline ComparisonSceneMonsterSample ReadComparisonSceneMonsterSample(const void* object){
    static_assert(sizeof(void*)==4u,"PAL4 diagnostic images are x86");
    ComparisonSceneMonsterSample s;
    if(!object)return s;
    const auto* bytes=static_cast<const unsigned char*>(object);
    const auto word=[&](unsigned offset){std::uint32_t v;std::memcpy(&v,bytes+offset,4u);return v;};
    std::memcpy(s.position.data(),bytes+0xacu,12u);
    std::memcpy(s.direction.data(),bytes+0xa0u,12u);
    std::memcpy(s.timers.data(),bytes+0x6f8u,24u);
    s.active=word(0x1f0u);s.pursuit=word(0x6e0u);s.movement_state=word(0x5f4u);
    s.target=word(0x6e4u);s.speed_bits=word(0x564u+9u*4u);
    if(s.target){const auto* target=reinterpret_cast<const unsigned char*>(static_cast<std::uintptr_t>(s.target));
        std::memcpy(&s.target_active,target+0x1f0u,4u);
        std::memcpy(s.target_position.data(),target+0xacu,12u);}
    return s;
}
class ComparisonSceneMonsterTrace final {
public:
    // Three real patrols overflowed the old 16K prefix before live111's first
    // battle. Keep a bounded full prefix for the longer multi-monster route.
    static constexpr unsigned kDefaultCapacity=131072u;
    static constexpr unsigned kMaximumCapacity=262144u;
    struct Ticket {
        bool active{};unsigned slot{};std::uint32_t object{},delta_bits{},mode{};
        ComparisonSceneMonsterSample before{};
    };
    explicit ComparisonSceneMonsterTrace(bool enabled,std::string_view names={},unsigned capacity=kDefaultCapacity)
        :enabled_(enabled),capacity_(capacity){
        if(!enabled)return;
        if(!capacity || capacity>kMaximumCapacity)throw std::invalid_argument("scene Monster trace capacity");
        while(!names.empty()){
            const auto end=names.find(';');const auto name=names.substr(0,end);
            if(name.empty() || name.size()>=32u || name_count_==names_.size())
                throw std::invalid_argument("scene Monster trace names");
            for(const auto c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))
                throw std::invalid_argument("scene Monster trace name character");
            for(unsigned i=0;i<name_count_;++i)if(name==names_[i].data())throw std::invalid_argument("duplicate scene Monster trace name");
            std::memcpy(names_[name_count_++].data(),name.data(),name.size());
            if(end==std::string_view::npos)break;
            names.remove_prefix(end+1u);if(names.empty())throw std::invalid_argument("empty scene Monster trace name");
        }
        if(!name_count_)throw std::invalid_argument("scene Monster trace requires selected names");
        records_.reset(new Record[capacity]{});
    }
    bool Enabled()const noexcept{return enabled_;}
    Ticket Begin(void* object,float delta,std::uint32_t mode){
        if(!enabled_ || !object)return {};
        const char* name{};std::memcpy(&name,static_cast<const unsigned char*>(object)+4u,sizeof(name));
        if(!name)return {};
        for(unsigned i=0;i<name_count_;++i)if(std::strcmp(name,names_[i].data())==0){
            Ticket t;t.active=true;t.slot=i;t.object=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(object));
            std::memcpy(&t.delta_bits,&delta,4u);t.mode=mode;t.before=ReadComparisonSceneMonsterSample(object);
            if(pending_)++nested_;++pending_;return t;}
        return {};
    }
    void End(Ticket& ticket,std::uint32_t result,bool aborted=false)noexcept{
        if(!ticket.active)return;
        ticket.active=false;--pending_;if(aborted)++aborted_;
        const auto after=ReadComparisonSceneMonsterSample(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(ticket.object)));
        ++frames_[ticket.slot];float delta;std::memcpy(&delta,&ticket.delta_bits,4u);seconds_[ticket.slot]+=delta;
        // Retain initial idle evidence; later unchanged idle frames contribute
        // to counters only. Every pursuit/return frame and idle move is kept.
        if(!aborted && seen_[ticket.slot] && !ticket.before.pursuit && !after.pursuit && ticket.before==after){++idle_skipped_;return;}
        seen_[ticket.slot]=true;
        if(count_==capacity_){++dropped_;return;}
        auto& r=records_[count_++];r.ticket=ticket;r.after=after;r.result=result;r.aborted=aborted;
        r.frame=frames_[ticket.slot];r.seconds=seconds_[ticket.slot];
    }
    void Write(std::ostream& out)const{
        out.imbue(std::locale::classic());
        out<<"{\"boundary\":\"4317F0\",\"enabled\":"<<(enabled_?"true":"false")<<",\"capacity\":"<<capacity_
           <<",\"count\":"<<count_<<",\"dropped\":"<<dropped_<<",\"aborted\":"<<aborted_<<",\"nested\":"<<nested_
           <<",\"pending\":"<<pending_<<",\"idle_skipped\":"<<idle_skipped_<<",\"actors\":[";
        for(unsigned i=0;i<name_count_;++i){if(i)out<<',';out<<"{\"name\":"<<std::quoted(names_[i].data())<<",\"frames\":"<<frames_[i]<<",\"seconds\":"<<std::setprecision(17)<<seconds_[i]<<'}';}
        out<<"],\"records\":[";
        for(unsigned i=0;i<count_;++i){const auto& r=records_[i];if(i)out<<',';
            out<<"{\"sequence\":"<<i<<",\"slot\":"<<r.ticket.slot<<",\"object\":"<<r.ticket.object<<",\"delta_bits\":"<<r.ticket.delta_bits
               <<",\"mode\":"<<r.ticket.mode<<",\"result\":"<<r.result<<",\"frame\":"<<r.frame<<",\"seconds\":"<<std::setprecision(17)<<r.seconds
               <<",\"aborted\":"<<(r.aborted?"true":"false")<<",\"before\":";Sample(out,r.ticket.before);out<<",\"after\":";Sample(out,r.after);out<<'}';}
        out<<"]}";
    }
private:
    static void Sample(std::ostream& out,const ComparisonSceneMonsterSample& s){
        out<<"{\"active\":"<<s.active<<",\"pursuit\":"<<s.pursuit<<",\"movement_state\":"<<s.movement_state<<",\"target\":"<<s.target<<",\"target_active\":"<<s.target_active<<",\"speed_bits\":"<<s.speed_bits;
        const auto words=[&](const char* name,const auto& values){out<<",\""<<name<<"\":[";bool first=true;for(auto v:values){if(!first)out<<',';first=false;out<<v;}out<<']';};
        words("position_bits",s.position);words("direction_bits",s.direction);words("target_position_bits",s.target_position);words("timers",s.timers);out<<'}';
    }
    struct Record{Ticket ticket;ComparisonSceneMonsterSample after;std::uint32_t result{};bool aborted{};std::uint64_t frame{};double seconds{};};
    bool enabled_{};unsigned capacity_{},name_count_{},count_{},pending_{};
    std::uint64_t dropped_{},aborted_{},nested_{},idle_skipped_{};
    std::array<std::array<char,32>,8> names_{};std::array<std::uint64_t,8> frames_{};
    std::array<double,8> seconds_{};std::array<bool,8> seen_{};std::unique_ptr<Record[]> records_;
};
inline ComparisonSceneMonsterTrace& GetComparisonSceneMonsterTrace(){
    static ComparisonSceneMonsterTrace trace=[] {const auto* names=std::getenv("PAL4_COMPARE_SCENE_MONSTERS");
        return ComparisonSceneMonsterTrace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") && names,names?names:"");}();return trace;
}
class ComparisonSceneMonsterScope final {
public:
    ComparisonSceneMonsterScope(ComparisonSceneMonsterTrace& trace,void* actor,float elapsed,std::uint32_t mode)
        :trace_(trace),ticket_(trace.Begin(actor,elapsed,mode)){}
    ComparisonSceneMonsterScope(const ComparisonSceneMonsterScope&)=delete;
    ComparisonSceneMonsterScope& operator=(const ComparisonSceneMonsterScope&)=delete;
    ~ComparisonSceneMonsterScope(){trace_.End(ticket_,0u,true);}
    void Complete(std::uint32_t result)noexcept{trace_.End(ticket_,result);}
private:
    ComparisonSceneMonsterTrace& trace_;ComparisonSceneMonsterTrace::Ticket ticket_;
};
template<class Invoke>
std::uint32_t ObserveComparisonSceneMonsterUpdate(ComparisonSceneMonsterTrace& trace,void* actor,float elapsed,std::uint32_t mode,Invoke&& invoke){
    if(!trace.Enabled())return invoke();
    ComparisonSceneMonsterScope observation(trace,actor,elapsed,mode);
    const auto result=invoke();observation.Complete(result);return result;
}
}
