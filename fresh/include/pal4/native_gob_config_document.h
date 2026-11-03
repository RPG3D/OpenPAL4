#pragma once
#include "pal4/gob_object_base.h"
#include "pal4/native_event_document_runtime.h"

namespace pal4 {
class BinaryXmlConfig;

// Owns the real GOB+1C0 document. The object slot and its storage service must
// outlive this owner. Ordinary resource cleanup can clear the same slot first.
class NativeGobConfigDocument final : private IGobObjectDocumentRuntime {
public:
    NativeGobConfigDocument(IStringBufferStorageRuntime& memory,std::uint32_t object,
        const BinaryXmlConfig& source);
    ~NativeGobConfigDocument();
    NativeGobConfigDocument(const NativeGobConfigDocument&)=delete;
    NativeGobConfigDocument& operator=(const NativeGobConfigDocument&)=delete;
    [[nodiscard]] std::uint32_t Address() const noexcept;
    NativeEventDocumentRuntime& Documents() noexcept {return documents_;}
    void Reset();
private:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address,std::uint32_t value) override;
    void DestructXmlDocument(std::uint32_t document) override;
    std::uint32_t FreeMemory(std::uint32_t memory) override;
    std::uint32_t AllocateMemory(std::uint32_t size) override;
    std::uint32_t InitializeXmlDocument(std::uint32_t memory,std::string_view root,
        std::uint32_t mode) override;
    std::uint32_t CreateXmlNode(std::uint32_t document,std::string_view name) override;
    IStringBufferStorageRuntime& memory_;
    std::uint32_t object_;
    NativeEventDocumentRuntime documents_;
};
}
