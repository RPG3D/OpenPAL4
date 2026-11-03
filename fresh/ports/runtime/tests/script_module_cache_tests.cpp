#include "pal4/portable/script_module_cache.h"
#include <iostream>
#include <stdexcept>

using namespace pal4::portable;
namespace {
void Check(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
}
int main() {
    try {
        ScriptModuleCache cache;
        unsigned reads{};
        const auto reader=[&](std::string_view world) {
            ++reads;
            if(world=="A")return ScriptModuleCache::Bytes{1,2,3};
            if(world=="B")return ScriptModuleCache::Bytes{4,5};
            throw std::runtime_error("unexpected world");
        };
        Check(cache.Load("A",reader)==ScriptModuleCache::Bytes{1,2,3} && reads==1,
            "initial world CSB was not cached");
        cache.RetainOnly("A");
        Check(cache.Load("B",reader)==ScriptModuleCache::Bytes{4,5} && reads==2,
            "second world CSB was not cached");
        cache.RetainOnly("B");
        Check(cache.Size()==1 && cache.Find("A")==nullptr,"stale world bytes were retained");
        Check(cache.Load("A",reader)==ScriptModuleCache::Bytes{1,2,3} && reads==3,
            "returning to a loaded world did not reread its CSB bytes");
        cache.RetainOnly("A");
        const auto failed=[&] { (void)cache.Load("C",[](std::string_view)->ScriptModuleCache::Bytes {
            throw std::runtime_error("read failed");
        }); };
        bool rejected=false;try { failed(); } catch(const std::runtime_error&) { rejected=true; }
        Check(rejected && cache.Size()==1 && cache.Find("A"),"failed world read polluted the cache");
        cache.Clear();
        Check(cache.Size()==0,"clear retained world bytes");
        std::cout<<"world CSB byte cache and A-B-A lifecycle contract passed\n";
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}
