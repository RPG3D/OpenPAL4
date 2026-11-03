#include "pal4/native_game_allocator.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
int main() {
    static_assert(sizeof(void*)==4u && sizeof(unsigned long)==4u);
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    try {
        const auto baseline=pal4_game_accounted_bytes();
        std::array<void*,3> images{};
        for(auto& p:images){p=pal4_game_malloc(0xa8u);check(p && pal4_game_usable_size(p)>=0xa8u,"real dlmalloc EffectObject image allocation");std::memset(p,0x5a,0xa8u);}
        check(reinterpret_cast<std::uintptr_t>(images[0])<reinterpret_cast<std::uintptr_t>(images[1]) &&
              reinterpret_cast<std::uintptr_t>(images[1])<reinterpret_cast<std::uintptr_t>(images[2]),"fresh library top-chunk fixture uses increasing addresses");
        unsigned long total=baseline;for(auto p:images)total+=static_cast<unsigned long>(pal4_game_usable_size(p));
        check(pal4_game_accounted_bytes()==total,"malloc success accounting");
        for(auto p:images)pal4_game_free(p);
        pal4_game_free(nullptr);check(pal4_game_accounted_bytes()==baseline,"free and null accounting");
        auto* large=pal4_game_malloc(512u*1024u);check(large!=nullptr,"mapped allocation");
        check((*reinterpret_cast<const std::uint32_t*>(static_cast<const char*>(large)-4u)&2u)!=0u,"mapped-path fixture actually uses mmap");
        const auto mapped_total=pal4_game_accounted_bytes();
        check(pal4_game_realloc(large,256u*1024u)==large && pal4_game_accounted_bytes()==mapped_total,"target mapped realloc does not add again");
        pal4_game_free(large);check(pal4_game_accounted_bytes()==baseline,"mapped free accounting");
        auto* p=pal4_game_malloc(0xa8u);auto* guard=pal4_game_malloc(0xa8u);check(p && guard,"realloc fixture allocations");
        const auto usable=static_cast<unsigned long>(pal4_game_usable_size(p));const auto before=pal4_game_accounted_bytes();
        check(pal4_game_realloc(p,0xa8u)==p && pal4_game_accounted_bytes()==before+usable,"target ordinary realloc adds usable size again");
        pal4_game_free(guard);pal4_game_free(p);
        check(pal4_game_accounted_bytes()==baseline+usable,"preserve original realloc statistic bias");
        const auto biased=pal4_game_accounted_bytes();
        auto* from_null=pal4_game_realloc(nullptr,32u);check(from_null!=nullptr,"null realloc delegates to allocation");
        check(pal4_game_accounted_bytes()==biased+pal4_game_usable_size(from_null),"null realloc adds once");
        pal4_game_free(from_null);check(pal4_game_accounted_bytes()==biased,"null realloc/free balance");
        auto* moved=pal4_game_malloc(64u);auto* blocker=pal4_game_malloc(64u);check(moved && blocker,"moved realloc fixture");
        std::memset(moved,0x6d,64u);const auto old_usable=pal4_game_usable_size(moved);
        const auto moved_before=pal4_game_accounted_bytes();
        auto* replacement=pal4_game_realloc(moved,1024u);
        check(replacement && replacement!=moved,"blocked growth actually moves");
        const auto new_usable=pal4_game_usable_size(replacement);
        for(unsigned i=0;i<64u;++i)check(static_cast<const unsigned char*>(replacement)[i]==0x6d,"moved realloc preserves payload");
        check(pal4_game_accounted_bytes()==moved_before+2u*new_usable-old_usable,"target moved realloc preserves internal allocate/free plus return-tail add");
        const auto overflow_before=pal4_game_accounted_bytes();
        check(!pal4_game_malloc(0xfffffff0u) && !pal4_game_realloc(replacement,0xfffffff0u) &&
              pal4_game_accounted_bytes()==overflow_before,"overflow failures preserve allocation/accounting");
        pal4_game_free(blocker);pal4_game_free(replacement);
        check(pal4_game_trim_threshold()==0x40000u && pal4_game_disable_auto_trim()==1 &&
              pal4_game_trim_threshold()==0xffffffffu,"public tuning API matches explicit comparison trim policy");
        std::cout<<"PASS: linked dlmalloc core, real image allocations, mapped/null/free and target realloc accounting\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
