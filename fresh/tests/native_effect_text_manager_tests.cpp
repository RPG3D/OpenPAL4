#include "pal4/native_effect_text_manager.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_effect_load_text.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
std::uint32_t Nil(pal4::NativeBaseObjectMemory& memory, std::uint32_t manager) {
    const auto head=memory.ReadDword(manager+4u);
    auto node=memory.ReadDword(head+4u);
    for (unsigned i=0;i<1000u;++i) {
        if (!memory.ReadDword(node) && !memory.ReadDword(node+8u)) {
            Check(memory.ReadDword(node+32u)==1u,"template nil flag");
            return node;
        }
        node=memory.ReadDword(node);
    }
    throw std::runtime_error("template nil traversal exceeded bound");
}
}

int main() try {
    pal4::NativeBaseObjectMemory survivor_memory;
    auto& shared=dynamic_cast<pal4::NativeBaseObjectMemory&>(survivor_memory.SharedRecordStorage());
    const auto baseline=shared.OutstandingAllocations();
    {
        pal4::NativeEventDocumentRuntime survivor_xml(survivor_memory);
        std::unique_ptr<pal4::NativeEffectTextManager> survivor;
        std::uint32_t nil{};
        {
            pal4::NativeBaseObjectMemory creator_memory;
            pal4::NativeEventDocumentRuntime creator_xml(creator_memory);
            auto first=std::make_unique<pal4::NativeEffectTextManager>(creator_memory,creator_xml);
            survivor=std::make_unique<pal4::NativeEffectTextManager>(survivor_memory,survivor_xml);
            nil=Nil(creator_memory,first->Address());
            Check(nil==Nil(survivor_memory,survivor->Address()),
                "separate native template managers share original nil");
            const auto source=creator_xml.Allocate(60u);
            (void)creator_xml.InitializeDocument(source,"template",0u);
            Check(survivor->Add("entry",source),"survivor clones template from creator");
            creator_xml.DestroyDocument(source);(void)creator_xml.Free(source);
            first.reset();
            Check(creator_memory.OutstandingAllocations()==0u,"creator releases local template allocations");
        }
        Check(Nil(survivor_memory,survivor->Address())==nil && survivor->Find("entry")!=0u,
            "template and shared nil survive creating facade lifetime");
        std::uint32_t retained_data{};
        {
            pal4::NativeEffectLoadText name(survivor_memory,"retained");
            const auto key=name.NameRecordAddress();
            retained_data=survivor_memory.ReadDword(key+4u);
            const auto baseline=survivor_memory.OutstandingAllocations();
            Check(survivor->FindNodeByKeyRecord(key)==survivor_memory.ReadDword(survivor->Address()+4u) &&
                survivor_memory.OutstandingAllocations()==baseline,"borrowed key lookup allocates no duplicate string");
            const auto source=survivor_xml.Allocate(60u);
            (void)survivor_xml.InitializeEmptyDocument(source);
            const auto before_insert=survivor_memory.OutstandingAllocations();
            Check(survivor->AdoptKeyRecord(key,source) &&
                survivor_memory.OutstandingAllocations()==before_insert+1u,
                "cache insertion allocates only the tree node and shares its name");
            const auto node=survivor->FindNodeByKeyRecord(key);
            Check(survivor_memory.ReadDword(node+16u)==retained_data &&
                survivor_memory.ReadByte(retained_data-1u)==1u,
                "two stack cache pairs release references before retaining name and node");
            Check(!survivor->AdoptKeyRecord(key,source) &&
                survivor_memory.ReadByte(retained_data-1u)==1u,
                "duplicate cache insertion balances temporary COW references");
        }
        Check(survivor->Find("retained")!=0u && survivor_memory.ReadByte(retained_data-1u)==0u,
            "cache name survives release of the load name scope");
        for (const auto length : {5u,31u,32u,63u,64u}) {
            const std::string name(length,'x');
            const auto source=survivor_xml.Allocate(60u);
            (void)survivor_xml.InitializeEmptyDocument(source);
            Check(survivor->Adopt(name,source),"cache adopts boundary-length name");
            const auto node=survivor->FindNode(name);
            const auto data=survivor_memory.ReadDword(node+16u);
            Check(survivor_memory.ReadDword(node+20u)==length &&
                survivor_memory.ReadDword(node+24u)==(length|31u) &&
                survivor_memory.ReadByte(data-1u)==0u,
                "cached name uses original VC6 capacity and final reference header");
        }
        {
            const auto source=survivor_xml.Allocate(60u);
            (void)survivor_xml.InitializeEmptyDocument(source);
            Check(survivor->Adopt("",source),"cache adopts empty name");
            const auto node=survivor->FindNode("");
            Check(!survivor_memory.ReadDword(node+16u) &&
                !survivor_memory.ReadDword(node+20u) && !survivor_memory.ReadDword(node+24u),
                "empty cache name has no string allocation");
        }
        survivor->Clear();
        Check(!survivor->Find("entry") && Nil(survivor_memory,survivor->Address())==nil,
            "clear retains shared nil and empty tree supports lookup");
        survivor.reset();
        Check(survivor_memory.OutstandingAllocations()==0u,"survivor releases template allocations");
    }
    Check(shared.OutstandingAllocations()==baseline,"last template/XML reference releases shared allocations");
    std::puts("Native effect template shared nil lifetime PASS");
    return 0;
} catch (const std::exception& e) {
    std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;
}
