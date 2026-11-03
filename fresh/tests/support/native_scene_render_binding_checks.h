#pragma once
#include "pal4/native_world_asset.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/rw_plugin_8449_native.h"
#include <rpworld.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
// Real N01 clipNA uses13 alpha-textured atomics. They must all defer to
// arena+284; the old native resource stage left the default draw callback.
inline void VerifyNativeN01AlphaQueueBindings(NativeWorldAsset& world,std::uint32_t arena) {
    auto* clump=world.ClumpAt(2u);
    if (!clump) throw std::runtime_error("N01 clipNA resource is absent");
    struct Context {std::uint32_t arena;unsigned count{};bool valid=true;};
    Context context{arena};
    RpClumpForAllAtomics(clump,[](RpAtomic* a,void* data)->RpAtomic* {
        auto& c=*static_cast<Context*>(data);
        ++c.count;
        const auto offset=NativeRwPlugin8449Lifecycle::State().geometry_offset;
        if(offset<=0) {c.valid=false;return a;}
        std::uint32_t words[3]{};
        std::memcpy(words,reinterpret_cast<const char*>(a)+offset,sizeof words);
        c.valid &= words[0]!=0u && words[1]==1u && words[2]==c.arena+284u &&
            reinterpret_cast<std::uint32_t>(RpAtomicGetRenderCallBack(a))==
                NativeRwGeometryProcessor::RenderCallbackAddress();
        return a;
    },&context);
    if (!context.valid || context.count!=13u)
        throw std::runtime_error("N01 clipNA alpha queue/callback binding differs from518E81");
    std::cout << "N01 clipNA13 real alpha atomics bind arena+284; no8450 wrapper\n";
}
}
