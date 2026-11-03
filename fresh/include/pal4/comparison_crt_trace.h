#pragma once
#include "pal4/comparison_random_call_trace.h"
#include "pal4/comparison_crt_attribution.h"
#include <cstdlib>
#include <string_view>

namespace pal4 {
// Keep the complete prefix and important call categories independently.
// Particle draws can fill the full prefix before a save has finished loading.
class ComparisonCrtTrace final {
public:
    using Record=ComparisonRandomCallTrace::Record;
    explicit ComparisonCrtTrace(bool enabled,std::uint32_t capacity=262144u)
        :full_(enabled,capacity),preparation_(enabled,64u),script_(enabled,4096u),seeds_(enabled,4096u),
         attribution_(enabled ? std::make_unique<ComparisonCrtAttribution>() : nullptr){}
    bool Enabled()const noexcept{return full_.Enabled();}
    void Add(Record record){
        if(!Enabled())return;
        std::lock_guard<std::mutex> lock(observation_mutex_);
        record.full_sequence=full_.Add(record);
        if((record.caller>=0x53cf20u && record.caller<0x53d3a0u) ||
           (record.source && std::string_view(record.source).ends_with("combat_manager_events_native.cpp")))
            preparation_.Add(record);
        if(record.kind==1u)seeds_.Add(record);
        const bool script=record.kind==0u && ((record.caller>=0x5d4000u && record.caller<0x5d4020u) ||
           (record.source && std::string_view(record.source).ends_with("native_game_script_callbacks.cpp")));
        if(script)
            script_.Add(record);
        attribution_->Add(record,script);
    }
    void Write(std::ostream& out)const{full_.Write(out);}
    void WritePreparation(std::ostream& out)const{
        out<<"{\"scope\":\"combat_prepare CRT calls only; not a complete RNG stream\",\"trace\":";
        preparation_.Write(out);out<<'}';
    }
    void WriteFocused(std::ostream& out)const{
        std::lock_guard<std::mutex> lock(observation_mutex_);
        out<<"{\"scope\":\"giGetRandnum and all observed srand calls; not a complete RNG stream\","
              "\"full_sequence_policy\":\"completion-order ordinal in full trace, including dropped entries\",\"script\":";
        script_.Write(out);out<<",\"seeds\":";seeds_.Write(out);
        out<<",\"attribution\":";
        if(attribution_)attribution_->Write(out);else out<<"null";
        out<<'}';
    }
private:
    ComparisonRandomCallTrace full_,preparation_,script_,seeds_;
    std::unique_ptr<ComparisonCrtAttribution> attribution_;
    mutable std::mutex observation_mutex_;
};
inline ComparisonCrtTrace& GetComparisonCrtTrace() {
    static ComparisonCrtTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
                                    std::getenv("PAL4_COMPARE_CRT_TRACE"));
    return trace;
}
} // namespace pal4
