#include "pal4/native_gob_config_document.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/binary_xml_config.h"

#include <array>
#include <cstdio>
#include <new>
#include <stdexcept>

namespace {
void Check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
class Storage final : public pal4::IStringBufferStorageRuntime {
public:
    pal4::NativeBaseObjectMemory memory;
    unsigned calls{},fail_at{};
    std::uint32_t Allocate(std::uint32_t n) override {
        if(++calls==fail_at)throw std::bad_alloc{};
        return memory.Allocate(n);
    }
    std::uint32_t Free(std::uint32_t p) override {return memory.Free(p);}
    std::uint32_t ReadDword(std::uint32_t p) override {return memory.ReadDword(p);}
    void WriteDword(std::uint32_t p,std::uint32_t v) override {memory.WriteDword(p,v);}
    void WriteByte(std::uint32_t p,std::uint8_t v) override {memory.WriteByte(p,v);}
};
pal4::BinaryXmlConfig Source() {
    const pal4::BinaryXmlNode condition{"PAL4_GameObject-machine-condition",{{"test",1u,std::int32_t{7}}},{}};
    const pal4::BinaryXmlNode machine{"PAL4_GameObject-machine",{{"PAL4_GameObject-machine-repeat",1u,std::int32_t{3}}},{condition}};
    const pal4::BinaryXmlNode parameters{"PAL4-GameObject-Parameters",{},{machine}};
    const pal4::BinaryXmlNode root{"PAL4-GameObject",{{"PAL4-GameObject-object-scale",2u,1.0f}},{parameters}};
    const auto bytes=pal4::BinaryXmlConfig::Serialize(root);
    pal4::BinaryXmlConfig source;
    Check(bytes && source.Parse(*bytes),"valid retained GOB source");return source;
}
}
int main() try {
    const auto source=Source();
    bool completed{};
    for(unsigned failure=1u;failure<160u;++failure) {
        Storage storage;storage.fail_at=failure;
        std::array<std::uint32_t,113> object{};
        bool failed{};
        try {
            pal4::NativeGobConfigDocument config(storage,reinterpret_cast<std::uint32_t>(object.data()),source);
            Check(config.Address()==object[112] && config.Address()!=0u,"GOB+1C0 owns actual native document");
            auto& docs=config.Documents();
            Check(docs.NodeName(config.Address())=="PAL4-GameObject" && docs.DocumentCount()==4u,
                "native GOB source retains all four document nodes");
            const auto parameters=docs.FindChild(config.Address(),"PAL4-GameObject-Parameters");
            const auto machine=docs.FindChild(parameters,"PAL4_GameObject-machine");
            Check(docs.AttributeToInt(docs.FindNumericAttribute(machine,"PAL4_GameObject-machine-repeat"),0)==3,
                "retained machine field is readable");
            config.Reset();config.Reset();
            Check(object[112]==0u && docs.DocumentCount()==0u,"GOB configuration reset clears its slot and references");
        } catch(const std::bad_alloc&){failed=true;}
        if(object[112] || storage.memory.OutstandingAllocations()!=0u)
            std::fprintf(stderr,"failure=%u calls=%u slot=%08x outstanding=%zu\n",failure,storage.calls,object[112],storage.memory.OutstandingAllocations());
        Check(!object[112] && storage.memory.OutstandingAllocations()==0u,
            "GOB creation/parse failure releases every allocation and clears slot");
        if(!failed){completed=true;break;}
    }
    Check(completed,"bounded GOB configuration fault sweep reaches success");
    {
        Storage storage;std::array<std::uint32_t,113> object{};
        {
            pal4::NativeGobConfigDocument config(storage,reinterpret_cast<std::uint32_t>(object.data()),source);
            const auto document=config.Address();
            config.Documents().DestroyDocument(document);(void)storage.Free(document);object[112]=0u;
        }
        Check(storage.memory.OutstandingAllocations()==0u,"ordinary GOB cleanup followed by owner destruction is not a double free");
    }
    std::puts("Native retained GOB XML construction/parse rollback and cleanup PASS");return 0;
} catch(const std::exception& e) {std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
