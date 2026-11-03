#include "pal4/native_renderware_game_memory.h"
#include "pal4/native_game_allocator.h"
#include <rwcore.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

int main() {
    const auto check=[](bool ok,const char* why) {
        if(!ok)throw std::runtime_error(why);
    };
    bool initialized{};
    try {
        const auto* table=pal4::NativeGameRenderWareMemoryFunctions();
        const auto before=pal4_game_accounted_bytes();
        auto* block=table->rwmalloc(0xa8u,0xabcdef01u);
        check(block && pal4_game_accounted_bytes()==before+pal4_game_usable_size(block),
              "malloc callback uses actual game heap and ignores SDK hint");
        std::memset(block,0x5a,0xa8u);
        const auto ordinary=pal4_game_accounted_bytes();
        check(table->rwrealloc(block,0xa8u,0x12345678u)==block &&
              pal4_game_accounted_bytes()==ordinary+pal4_game_usable_size(block),
              "realloc callback preserves target accounting bias");
        table->rwfree(block);
        auto* zero=table->rwcalloc(7u,19u,0xfedcba98u);
        check(zero!=nullptr,"calloc game callback");
        for(unsigned i=0;i<133u;++i)
            check(static_cast<unsigned char*>(zero)[i]==0,"source calloc zeroes reused allocation");
        table->rwfree(zero);
        table->rwfree(nullptr);
        check(!table->rwcalloc(0xfffffff0u,16u,0u),"source calloc overflow rejection");

        initialized=RwEngineInit(table,0u,0x400000u)!=0;
        check(initialized,"SDK engine initializes with real game callbacks");
        const auto* active=RwOsGetMemoryInterface();
        check(active && active->rwmalloc==table->rwmalloc && active->rwfree==table->rwfree &&
              active->rwrealloc==table->rwrealloc && active->rwcalloc==table->rwcalloc,
              "SDK retains all four callable game callbacks");
        const auto sdk_before=pal4_game_accounted_bytes();
        auto* sdk=RwMalloc(0x144u,0u);
        check(sdk && pal4_game_accounted_bytes()==sdk_before+pal4_game_usable_size(sdk),
              "public SDK allocation reaches game source allocator");
        RwFree(sdk);
        check(RwEngineTerm()!=0,"SDK engine terminates with creating allocator");
        initialized=false;
        std::cout<<"PASS: callable game memory table, source calloc/realloc, actual SDK allocation and termination\n";
        return 0;
    }catch(const std::exception& error) {
        if(initialized)(void)RwEngineTerm();
        std::cerr<<"FAIL: "<<error.what()<<'\n';
        return 1;
    }
}
