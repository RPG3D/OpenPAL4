#pragma once
#include "pal4/rw_dict_schema.h"
namespace pal4 {
// Implements the reviewed native list boundary. Dictionary loading, traversal
// and UV-animation services remain pure virtual until their integration is ready.
class NativeRwDictSchemaListRuntime : public IRwDictSchemaRuntime {
public:
    void DestroyList(std::uint32_t list) final;
    std::uint32_t CreateList(std::uint32_t entry_size,std::uint32_t hint) final;
    std::uint32_t ListBegin(std::uint32_t list) final;
    std::uint32_t ListEnd(std::uint32_t list) final;
    std::uint32_t ListAppend(std::uint32_t list) final;
    std::uint32_t ReadWord(std::uint32_t address) final;
    void WriteWord(std::uint32_t address,std::uint32_t value) final;
};
}
