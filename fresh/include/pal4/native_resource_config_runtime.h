#pragma once
#include "pal4/game_resource_manager.h"
#include "pal4/native_base_object_memory.h"
#include <filesystem>

namespace pal4 {
class NativeRwDictSchemaRuntime;
class NativePkFileManagerRuntime;
// Owns buffers for shared resource effect/size loading. Other material modes
// fail explicitly until their native adapters are composed.
class NativeResourceConfigRuntime final : private GameResourceManagerRuntimeStub {
public:
    explicit NativeResourceConfigRuntime(NativeRwDictSchemaRuntime& streams) : streams_(streams) {}
    NativeResourceConfigRuntime(NativeRwDictSchemaRuntime& streams,NativeBaseObjectMemory& memory)
        : streams_(streams),memory_(memory) {}
    void LoadSize(GameResourceManager& resource);
    void CleanupTextures(GameResourceManager&);
    void ClearTexture(GameResourceManager&);
    void LoadEffects(GameResourceManager& resource,NativePkFileManagerRuntime& packages,const std::filesystem::path& root);
    // resource.words[4] must be null on entry. Returns the new metadata as an
    // owning host string; no runtime allocation escapes through words[4].
    std::string ReplaceTexture(GameResourceManager& resource,std::string_view name,std::uint32_t texture,
        NativePkFileManagerRuntime& packages,const std::filesystem::path& root);
private:
    std::uint32_t Allocate(std::uint32_t size) override;
    std::uint32_t Free(std::uint32_t address) override;
    void StoreString(std::uint32_t address,std::string_view text) override;
    std::string ReadString(std::uint32_t address) override;
    std::uint32_t OpenDataStream(std::uint32_t type,std::uint32_t mode,std::string_view path) override;
    void ReadStreamWords(std::uint32_t stream,std::uint32_t* destination,std::uint32_t count) override;
    std::uint32_t CloseDataStream(std::uint32_t stream) override;
    void ReadStreamWord(std::uint32_t stream,std::uint32_t* destination) override;
    void ReadStreamBuffer(std::uint32_t stream,std::uint32_t destination,std::uint32_t bytes) override;
    void TerminateBuffer(std::uint32_t address,std::uint32_t index) override;
    void AddAndProcessScript8() override;
    void CleanupAllScripts() override;
    void PrepareTextureErrorString() override;
    std::uint32_t ReadTexture(std::string_view name) override;
    void SetTextureBytes(std::uint32_t texture,std::uint8_t filter,std::uint8_t addressing) override;
    void DestroyTexture(std::uint32_t texture) override;
    void ApplyMaterialPipeline4(std::uint32_t clump,std::array<std::uint32_t,3>& args) override;
    void ApplyMaterialPipeline6(std::uint32_t clump) override;
    std::uint32_t CheckTexturePathTarget(std::uint32_t clump,std::uint32_t texture) override;
    void ApplyMaterialPipeline(std::uint32_t,std::array<std::uint32_t,3>&) override;
    void ApplyMaterialPipeline2(std::uint32_t,std::array<std::uint32_t,8>&) override;
    std::uint32_t CreateIm3D() override;
    NativePkFileManagerRuntime* packages_{};
    std::filesystem::path root_;
    bool package_scope_{};
    NativeRwDictSchemaRuntime& streams_;
    NativeBaseObjectMemory owned_memory_;
    NativeBaseObjectMemory& memory_=owned_memory_;
};
}  // namespace pal4
