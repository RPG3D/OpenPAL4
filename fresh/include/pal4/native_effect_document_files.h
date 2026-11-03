#pragma once
#include <cstdint>
#include <string_view>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativePkFileManagerRuntime;

// Shared EFT/HEF package scope and versioned XML boundary. Each instance permits
// one synchronous load; file and adapter cleanup precede EndScriptScope.
class NativeEffectDocumentFiles final {
public:
    NativeEffectDocumentFiles(NativeBaseObjectMemory& m,NativeEventDocumentRuntime& d,
        NativePkFileManagerRuntime& p):memory_(m),documents_(d),packages_(p){}
    void BeginScriptScope();
    void EndScriptScope();
    std::uint32_t OpenFile(std::string_view path);
    void CloseFile(std::uint32_t file);
    std::uint32_t CreateXmlDocument(std::uint32_t bytes=60u);
    void DestroyXmlDocument(std::uint32_t xml);
    std::uint32_t CreateFileAdapter(std::uint32_t file);
    void DestroyFileAdapter(std::uint32_t adapter);
    bool ParseXml(std::uint32_t adapter,std::uint32_t xml);
private:
    NativeBaseObjectMemory& memory_;
    NativeEventDocumentRuntime& documents_;
    NativePkFileManagerRuntime& packages_;
    bool scope_{};
    std::uint32_t file_{},adapter_{};
};
}
