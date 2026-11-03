#pragma once
#include "pal4/rw_dict_schema.h"
namespace pal4::test {
// Test-only defaults for resource-manager fixtures. Never a production backend.
class RwDictSchemaFake : public IRwDictSchemaRuntime {
public:
    using Callback=std::uint32_t (*)(std::uint32_t,std::uint32_t,IRwDictSchemaRuntime&);
    void DestroyList(std::uint32_t) override {}
    void DestroyDict(std::uint32_t) override {}
    std::uint32_t SetCurrentDict(std::uint32_t) override {return 0;}
    std::uint32_t OpenStream(std::uint32_t,std::uint32_t,const char*) override {return 0;}
    bool FindChunk(std::uint32_t,std::uint32_t) override {return false;}
    std::uint32_t ReadDict(std::uint32_t) override {return 0;}
    void CloseStream(std::uint32_t) override {}
    std::uint32_t CreateList(std::uint32_t,std::uint32_t) override {return 0;}
    std::uint32_t ForAllAtomics(std::uint32_t,Callback,std::uint32_t) override {return 0;}
    std::uint32_t ForAllWorldMaterials(std::uint32_t,Callback,std::uint32_t) override {return 0;}
    std::uint32_t ForAllGeometryMaterials(std::uint32_t,Callback,std::uint32_t) override {return 0;}
    std::uint32_t ReadWord(std::uint32_t) override {return 0;}
    bool HasUVAnim(std::uint32_t) override {return false;}
    std::uint32_t ListBegin(std::uint32_t) override {return 0;}
    std::uint32_t ListEnd(std::uint32_t) override {return 0;}
    std::uint32_t ListAppend(std::uint32_t) override {return 0;}
    void WriteWord(std::uint32_t,std::uint32_t) override {}
    void AdvanceUVAnim(std::uint32_t,float) override {}
    void ApplyUVAnim(std::uint32_t) override {}
};
}
