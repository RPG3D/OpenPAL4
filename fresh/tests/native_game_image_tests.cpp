#include "pal4/native_game_image.h"
#include "pal4/native_game_allocator.h"
#include "pal4/effect_object.h"
#include <array>
#include <iostream>
#include <stdexcept>
struct Throws { Throws(){throw std::runtime_error("image constructor fixture");} };
int main() {
    const auto check=[](bool ok){if(!ok)throw std::runtime_error("native game image ownership regression");};
    try {
        pal4::NativeBaseObjectMemory memory;const auto baseline=pal4_game_accounted_bytes();
        {
            pal4::NativeGameImage<pal4::EffectObjectState> parent(memory),child(memory),grandchild(memory);
            const auto a=reinterpret_cast<std::uint32_t>(&parent.Get()),b=reinterpret_cast<std::uint32_t>(&child.Get()),c=reinterpret_cast<std::uint32_t>(&grandchild.Get());
            check(a<b && b<c && memory.OutstandingAllocations()==3u);
            check(pal4_game_usable_size(&parent.Get())>=sizeof(pal4::EffectObjectState));
            parent.Get().words[26]=0x12345678u;check(memory.ReadDword(a+104u)==0x12345678u && !child.Get().words[26]);
        }
        check(memory.OutstandingAllocations()==0u && pal4_game_accounted_bytes()==baseline);
        bool failed{};try {pal4::NativeGameImage<Throws> value(memory);}catch(const std::runtime_error&){failed=true;}
        check(failed && !memory.OutstandingAllocations() && pal4_game_accounted_bytes()==baseline);
        std::cout<<"PASS: dlmalloc game image identity, separate A8 prefix, lifetime and constructor rollback\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
