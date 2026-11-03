#include "pal4/native_crt_random.h"
#include <array>
#include <cassert>
#include <sstream>
#include <thread>
#include <vector>
#include <iostream>

namespace {
void TestScopedSources() {
    std::array<int,5> expected{};
    std::srand(67531u);
    for(auto& value:expected)value=std::rand();
    pal4::NativeCrtSeedRandom(67531u,"scoped source fixture");
    {
        const pal4::NativeCrtObservationOwnerScope owner(0x1234u);
        const pal4::NativeCrtObservationScope outer("fixture outer site");
        assert(pal4::NativeCrtRandom("fallback site")==expected[0]);
        try {
            const pal4::NativeCrtObservationOwnerScope child(0x5678u);
            const pal4::NativeCrtObservationScope inner("fixture inner site");
            assert(pal4::NativeCrtRandom("fallback site")==expected[1]);
            throw 1;
        } catch(int) {}
        // A helper thread must neither inherit nor overwrite the parent's site.
        std::thread other([] {
            assert(pal4::native_crt_observation_site==nullptr);
            assert(pal4::native_crt_observation_owner==0u);
            { const pal4::NativeCrtObservationOwnerScope local(0x789au); }
            assert(pal4::native_crt_observation_owner==0u);
            { const pal4::NativeCrtObservationScope local("fixture worker site"); }
            assert(pal4::native_crt_observation_site==nullptr);
        });
        other.join();
        assert(pal4::native_crt_observation_owner==0x1234u);
        { const pal4::NativeCrtObservationOwnerScope clear(0u);
          assert(pal4::native_crt_observation_owner==0u); }
        assert(pal4::NativeCrtRandom("fallback site")==expected[2]);
    }
    assert(pal4::native_crt_observation_site==nullptr);
    assert(pal4::native_crt_observation_owner==0u);
    assert(pal4::NativeCrtRandom("fixture restored fallback")==expected[3]);
    std::ostringstream out;
    pal4::GetComparisonCrtTrace().Write(out);
    auto occurrences=[&](const char* label) {
        std::size_t count=0,pos=0;
        const std::string needle=std::string("\"source\":\"")+label+'"';
        while((pos=out.str().find(needle,pos))!=std::string::npos){++count;pos+=needle.size();}
        return count;
    };
    assert(occurrences("fixture outer site")==2u);
    assert(occurrences("fixture inner site")==1u);
    assert(occurrences("fixture restored fallback")==1u);
    assert(occurrences("fallback site")==0u);
    const auto owner_count=[&](std::uintptr_t owner) {
        const auto text=out.str();const auto needle="\"owner\":"+std::to_string(owner)+",";
        std::size_t count=0,pos=0;
        while((pos=text.find(needle,pos))!=std::string::npos){++count;pos+=needle.size();}
        return count;
    };
    assert(owner_count(0x1234u)==2u && owner_count(0x5678u)==1u);
    assert(std::rand()==expected[4]);
}

void TestRecentRetention() {
    // One fixed allocation; overwrite observation slots without changing RNG,
    // the global ordinal, seed attribution or the independent script bucket.
    pal4::ComparisonCrtTrace recent(true,3u,true);
    recent.Add({1u,17u,7u});
    for(unsigned i=1;i<=8u;++i)recent.Add({0u,i,7u,0x646b63u});
    std::ostringstream out;recent.Write(out);
    const auto text=out.str();
    assert(text.find("\"count\":3,\"dropped\":6")!=std::string::npos);
    assert(text.find("\"retention\":\"recent\",\"start_sequence\":6,\"next_sequence\":9")!=std::string::npos);
    auto previous=std::size_t{};
    for(unsigned i=6;i<=8;++i) {
        const auto pos=text.find("\"sequence\":"+std::to_string(i)+",\"kind\":0,\"value\":"+std::to_string(i));
        assert(pos!=std::string::npos && pos>=previous);previous=pos;
        assert(text.find("\"full_sequence\":"+std::to_string(i))!=std::string::npos);
    }
    assert(text.find("\"value\":5,")==std::string::npos);
    std::ostringstream focused;recent.WriteFocused(focused);
    assert(focused.str().find("\"seed\":{\"sequence\":0,\"value\":17,\"thread\":7}")!=std::string::npos);
    assert(focused.str().find("\"calls\":8,\"unclassified\":0")!=std::string::npos);
    recent.Add({0u,29u,7u,0x5d400du});
    std::ostringstream script;recent.WriteFocused(script);
    assert(script.str().find("\"script_sequence\":9,\"script_value\":29")!=std::string::npos);
    pal4::ComparisonCrtTrace empty(true,0u,true);
    empty.Add({0u,1u});std::ostringstream zero;empty.Write(zero);
    assert(zero.str().find("\"start_sequence\":1,\"next_sequence\":1")!=std::string::npos);
}
}

int main() {
    _putenv_s("PAL4_COMPARE_SCRIPTED_INPUT","1");
    _putenv_s("PAL4_COMPARE_CRT_TRACE","1");
    assert(pal4::GetComparisonCrtTrace().Enabled());
    for(unsigned seed: {0u,1u,4502u,0xffffffffu}) {
        std::array<int,65> expected{};
        std::srand(seed);for(auto& value:expected)value=std::rand();
        pal4::NativeCrtSeedRandom(seed,"public CRT equivalence");
        for(unsigned n=0;n<64;++n)assert(pal4::NativeCrtRandom("public CRT equivalence")==expected[n]);
        // The observer and exporter must consume no additional random draws.
        std::ostringstream snapshot;pal4::GetComparisonCrtTrace().Write(snapshot);
        std::ostringstream focused_snapshot;pal4::GetComparisonCrtTrace().WriteFocused(focused_snapshot);
        assert(std::rand()==expected[64]);
    }
    TestScopedSources();
    TestRecentRetention();
    pal4::ComparisonCrtTrace disabled(false,1);
    disabled.Add({0,1});std::ostringstream off;disabled.Write(off);
    assert(off.str().find("\"count\":0")!=std::string::npos);
    pal4::ComparisonCrtTrace bounded(true,1);
    bounded.Add({1,42,3,4,5,6,"escaped\\\"source"});bounded.Add({0,99});
    std::ostringstream overflow;bounded.Write(overflow);
    assert(overflow.str().find("\"dropped\":1")!=std::string::npos);
    assert(overflow.str().find("escaped\\\\\\\"source")!=std::string::npos);
    // A busy particle prefix must not conceal the later combat choices.
    pal4::ComparisonCrtTrace focused(true,1);
    focused.Add({0,99,1,0x644ed0u});
    focused.Add({0,17,1,0x53d010u});
    focused.Add({0,22,1,0,0,0,"H:/source/combat_manager_events_native.cpp"});
    std::ostringstream selected;focused.WritePreparation(selected);
    assert(selected.str().find("\"count\":2,\"dropped\":0")!=std::string::npos);
    assert(selected.str().find("\"value\":17")!=std::string::npos);
    assert(selected.str().find("\"value\":22")!=std::string::npos);
    assert(selected.str().find("\"value\":99")==std::string::npos);
    for(unsigned i=0;i<64;++i)focused.Add({0,i,1,0x53d010u});
    std::ostringstream limited;focused.WritePreparation(limited);
    assert(limited.str().find("\"count\":64,\"dropped\":2")!=std::string::npos);
    // Script results and seeds survive a full particle prefix, with their
    // original observation ordinal retained across the independent buckets.
    pal4::ComparisonCrtTrace script(true,1u);
    script.Add({0u,99u,1u,0x644ed0u});
    script.Add({1u,123u,1u,0x644ed0u});
    script.Add({0u,29u,1u,0x5d400du});
    script.Add({0u,31u,2u,0u,0u,0u,"H:/source/native_game_script_callbacks.cpp"});
    std::ostringstream focus;script.WriteFocused(focus);
    assert(focus.str().find("\"value\":99")==std::string::npos);
    assert(focus.str().find("\"value\":29")!=std::string::npos);
    assert(focus.str().find("\"value\":31")!=std::string::npos);
    assert(focus.str().find("\"value\":123")!=std::string::npos);
    for(auto ordinal:{1u,2u,3u})
        assert(focus.str().find("\"full_sequence\":"+std::to_string(ordinal))!=std::string::npos);
    std::ostringstream full;script.Write(full);
    assert(full.str().find("\"count\":1,\"dropped\":3")!=std::string::npos);
    assert(focus.str().find("\"calls\":0,\"unclassified\":0")!=std::string::npos);
    assert(focus.str().find("\"calls\":1,\"unclassified\":0")!=std::string::npos);
    pal4::ComparisonCrtTrace attribution(true,1u);
    attribution.Add({0u,900u,7u,0x123u}); // Excluded by the later seed boundary.
    attribution.Add({1u,321u,7u,0x456u});
    for(unsigned i=0;i<3;++i)attribution.Add({0u,i,7u,0x646b63u});
    attribution.Add({0u,42u,8u,0u,0u,0u,"native\\particle.cpp"});
    attribution.Add({0u,19u,7u,0x5d4006u});
    std::ostringstream attributed;attribution.WriteFocused(attributed);
    assert(attributed.str().find("\"script_sequence\":6,\"script_value\":19")!=std::string::npos);
    assert(attributed.str().find("\"calls\":4,\"unclassified\":0")!=std::string::npos);
    assert(attributed.str().find("\"count\":3,\"source\":null")!=std::string::npos);
    pal4::ComparisonCrtTrace pending(true,1u);
    pending.Add({1u,123u,7u});
    for(unsigned i=0;i<3u;++i)pending.Add({0u,i,7u,0x42ab63u});
    std::ostringstream active;pending.WriteFocused(active);
    assert(active.str().find("\"intervals\":[],\"current\":{\"seed\":{\"sequence\":0,\"value\":123,\"thread\":7}")!=std::string::npos);
    assert(active.str().find("\"calls\":3,\"unclassified\":0")!=std::string::npos);
    assert(active.str().find("\"last_value\":2,\"last_sequence\":3")!=std::string::npos);
    assert(active.str().find("\"prefix_capacity\":256,\"prefix_dropped\":0,\"prefix\":[{\"full_sequence\":1,\"value\":0")!=std::string::npos);
    assert(active.str().find("\"full_sequence\":3,\"value\":2")!=std::string::npos);
    pending.Add({1u,456u,7u});
    std::ostringstream reset;pending.WriteFocused(reset);
    assert(reset.str().find("\"value\":456")!=std::string::npos);
    assert(reset.str().find("\"calls\":0,\"unclassified\":0,\"counters\":[]")!=std::string::npos);
    assert(reset.str().find("\"prefix_capacity\":256,\"prefix_dropped\":0,\"prefix\":[]")!=std::string::npos);
    pal4::ComparisonCrtTrace prefix_bounded(true,1u);
    prefix_bounded.Add({1u,17u,7u});
    for(unsigned n=0;n<300u;++n)prefix_bounded.Add({0u,n,7u,0x646b63u,0,0,"prefix\\source"});
    std::ostringstream prefix_json;prefix_bounded.WriteFocused(prefix_json);
    assert(prefix_json.str().find("\"prefix_capacity\":256,\"prefix_dropped\":44")!=std::string::npos);
    assert(prefix_json.str().find("\"full_sequence\":256,\"value\":255")!=std::string::npos);
    assert(prefix_json.str().find("\"value\":256,")==std::string::npos);
    auto bounded_attribution=std::make_unique<pal4::ComparisonCrtAttribution>();
    for(unsigned i=0;i<130;++i)bounded_attribution->Add({0u,i,1u,i},false);
    for(unsigned i=0;i<65;++i)bounded_attribution->Add({0u,1u,1u,0u},true);
    std::ostringstream capped;bounded_attribution->Write(capped);
    assert(capped.str().find("\"capacity\":64,\"dropped\":1")!=std::string::npos);
    assert(capped.str().find("\"calls\":130,\"unclassified\":2")!=std::string::npos);
    pal4::ComparisonCrtTrace concurrent(true,4000);
    std::vector<std::thread> writers;
    for(unsigned n=0;n<4;++n)writers.emplace_back([&,n]{for(unsigned k=0;k<1000;++k)concurrent.Add({0,k,n});});
    for(auto& writer:writers)writer.join();
    std::ostringstream all;concurrent.Write(all);
    assert(all.str().find("\"count\":4000,\"dropped\":0")!=std::string::npos);
    std::cout<<"PASS: public CRT sequence/next draw unchanged; export, disabled, bounded overflow, escaping and concurrent records\n";
}
