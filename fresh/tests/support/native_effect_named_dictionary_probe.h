#pragma once
#include "pal4/native_effect_named_dictionary_tree.h"
#include "pal4/native_base_object_memory.h"
#include <rwcore.h>
#include <cstring>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeEffectNamedDictionaryTree(NativeBaseObjectMemory& memory,
    NativeEffectNamedDictionaryTree& owner,RwTexDictionaryTreeState& state,
    NativeEffectNamedDictionarySharedState& shared) {
    const auto check=[](bool ok) {
        if(!ok) throw std::runtime_error("native named dictionary lifecycle assertion");
    };
    const auto word=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto baseline=memory.OutstandingAllocations();
    const auto nil=shared.nil,head=state.head;
    check(nil && head && shared.users==1u && !state.count);
    bool duplicate{};
    try {NativeEffectNamedDictionaryTree rejected(memory,state,shared);}
    catch(const std::invalid_argument&){duplicate=true;}
    check(duplicate && state.head==head && shared.users==1u &&
        memory.OutstandingAllocations()==baseline);
    struct RestoreCurrent {
        RwTexDictionary* previous=RwTexDictionaryGetCurrent();
        ~RestoreCurrent(){RwTexDictionarySetCurrent(previous);}
    } restore;
    // Seed actual original-layout nodes to exercise native payload and name
    // ownership. Public named insertion/loading is a separate service.
    const auto populate=[&](RwTexDictionaryTreeState& tree) {
        const auto name=memory.Allocate(6u);
        memory.WriteByte(name,1u); // Two names share this buffer.
        std::memcpy(reinterpret_cast<void*>(name+1u),"test",5u);
        const auto root=memory.Allocate(40u),right=memory.Allocate(40u);
        for(const auto node:{root,right}) {
            std::memset(reinterpret_cast<void*>(node),0,40u);
            memory.WriteDword(node,nil);
            memory.WriteDword(node+4u,node==root?tree.head:root);
            memory.WriteDword(node+8u,node==root?right:nil);
            memory.WriteDword(node+16u,name+1u);
            memory.WriteDword(node+20u,4u);
            memory.WriteDword(node+24u,4u);
            auto* dictionary=RwTexDictionaryCreate();check(dictionary!=nullptr);
            auto* raster=RwRasterCreate(4,4,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
            check(raster!=nullptr);
            auto* texture=RwTextureCreate(raster);check(texture!=nullptr);
            check(RwTexDictionaryAddTexture(dictionary,texture)==texture);
            memory.WriteDword(node+28u,word(dictionary));
            memory.WriteDword(node+32u,1u);
            memory.WriteDword(node+36u,node==root?1u:0u);
            RwTexDictionarySetCurrent(dictionary);
        }
        memory.WriteDword(tree.head,root);
        memory.WriteDword(tree.head+4u,root);
        memory.WriteDword(tree.head+8u,right);
        tree.count=2u;
    };
    RwTexDictionaryTreeState other;
    {
        NativeEffectNamedDictionaryTree second(memory,other,shared);
        check(shared.nil==nil && shared.users==2u && other.head!=head &&
            memory.ReadDword(other.head+4u)==nil &&
            memory.OutstandingAllocations()==baseline+1u);
        populate(state);
        owner.Cleanup();owner.Cleanup();
        check(state.head==head && !state.count && shared.nil==nil && shared.users==2u &&
            memory.ReadDword(head)==head && memory.ReadDword(head+4u)==nil &&
            memory.ReadDword(head+8u)==head && RwTexDictionaryGetCurrent()==nullptr &&
            memory.OutstandingAllocations()==baseline+1u);
        populate(other); // Destruction must also release live SDK payloads.
    }
    check(!other.head && !other.count && shared.nil==nil && shared.users==1u &&
        RwTexDictionaryGetCurrent()==nullptr && memory.OutstandingAllocations()==baseline);
}
}  // namespace pal4::test
