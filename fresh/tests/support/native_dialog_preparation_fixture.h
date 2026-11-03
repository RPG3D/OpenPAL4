#pragma once

#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/dialog_object.h"
#include "pal4/native_dialog_facet_cleanup.h"
#include <array>
#include <cstring>
#include <stdexcept>

namespace pal4::test {
// Owns actual native facet trees and XML documents under test Dialog images.
// The three facet payload pointers are borrowed stack data, never deletable.
class NativeDialogPreparationFixture final {
public:
    explicit NativeDialogPreparationFixture(NativeBaseObjectMemory& memory)
        : documents(memory), memory(memory) {
        nil=Allocate(36u); memory.WriteDword(nil+32u,1u);
    }
    ~NativeDialogPreparationFixture() { if (nil) (void)memory.Free(nil); }
    NativeEventDocumentRuntime documents;
    NativeBaseObjectMemory& memory;
    std::uint32_t nil{};
    std::array<std::uint32_t,3> payloads{11u,22u,33u};
    unsigned live{};
    std::uint32_t Allocate(std::uint32_t bytes) {
        const auto p=memory.Allocate(bytes); std::memset(reinterpret_cast<void*>(p),0,bytes); return p;
    }
    void Initialize(DialogObjectState& state) {
        const auto head=Allocate(36u);
        state.words[36]=head;
        memory.WriteDword(head,head); memory.WriteDword(head+4u,nil); memory.WriteDword(head+8u,head);
        const auto document=reinterpret_cast<std::uint32_t>(&state)+20u;
        (void)documents.InitializeDocument(document,"pool",0u);
        ++live;
        Dirty(state);
    }
    void Dirty(DialogObjectState& state) {
        if (state.words[38]) throw std::logic_error("facet fixture requires cleared tree");
        const auto head=state.words[36];
        std::array<std::uint32_t,3> nodes{};
        for (unsigned i=0;i<3u;++i) {
            const auto node=nodes[i]=Allocate(36u);
            const auto text=Allocate(3u);
            memory.WriteByte(text+1u,static_cast<std::uint8_t>('a'+i));
            memory.WriteDword(node+16u,text+1u); memory.WriteDword(node+20u,1u); memory.WriteDword(node+24u,1u);
            memory.WriteDword(node+28u,reinterpret_cast<std::uint32_t>(&payloads[i]));
            memory.WriteDword(node,nil); memory.WriteDword(node+8u,nil);
        }
        memory.WriteDword(nodes[0]+4u,nodes[1]); memory.WriteDword(nodes[2]+4u,nodes[1]);
        memory.WriteDword(nodes[1],nodes[0]); memory.WriteDword(nodes[1]+8u,nodes[2]);
        memory.WriteDword(nodes[1]+4u,head); memory.WriteDword(nodes[1]+32u,1u);
        memory.WriteDword(head,nodes[0]); memory.WriteDword(head+4u,nodes[1]); memory.WriteDword(head+8u,nodes[2]);
        state.words[38]=3u;
        documents.WriteIntAttribute(reinterpret_cast<std::uint32_t>(&state)+20u,"reuse_marker",123);
    }
    bool IsClean(const DialogObjectState& state) {
        const auto head=state.words[36];
        const auto document=reinterpret_cast<std::uint32_t>(&state)+20u;
        return state.words[38]==0u && memory.ReadDword(head)==head &&
            memory.ReadDword(head+4u)==nil && memory.ReadDword(head+8u)==head &&
            memory.ReadDword(document+52u)==0u && memory.ReadDword(document+8u)==0u &&
            payloads==std::array<std::uint32_t,3>{11u,22u,33u};
    }
    void Destroy(DialogObjectState& state) {
        if (!IsClean(state)) throw std::runtime_error("pool preparation leaves dirty Dialog");
        documents.DestroyDocument(reinterpret_cast<std::uint32_t>(&state)+20u);
        (void)memory.Free(state.words[36]); state.words[36]=0u;
        --live;
    }
    void Release() {
        if (live || documents.DocumentCount()) throw std::runtime_error("live preparation fixture payloads");
        (void)memory.Free(nil); nil=0u;
    }
    void ClearFacets(std::uint32_t effect) {
        NativeDialogFacetCleanup::Clear(memory,*reinterpret_cast<DialogObjectState*>(effect),nil);
    }
};
}  // namespace pal4::test
