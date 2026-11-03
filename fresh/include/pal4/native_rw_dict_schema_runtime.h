#pragma once

#include "pal4/rw_dict_schema_list_native.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pal4 {

class NativePackageArchive;

// Supplies the rewritten UV-animation dictionary with public RenderWare APIs.
// The archive and SDK engine must outlive this adapter and its open streams.
class NativeRwDictSchemaRuntime final : public NativeRwDictSchemaListRuntime {
public:
    // SDK lifecycle/traversal operations need no archive. Stream operations
    // reject an unbound archive instead of allocating an unrelated CPK owner.
    NativeRwDictSchemaRuntime() = default;
    explicit NativeRwDictSchemaRuntime(
        NativePackageArchive& archive, std::string source_prefix = {})
        : archive_(&archive), source_prefix_(std::move(source_prefix)) {}
    ~NativeRwDictSchemaRuntime() override;

    [[nodiscard]] std::uint32_t ReadDictionary(std::string_view source);

    void DestroyDict(std::uint32_t dict) override;
    std::uint32_t SetCurrentDict(std::uint32_t dict) override;
    std::uint32_t OpenStream(std::uint32_t type, std::uint32_t mode,
                             const char* source) override;
    bool FindChunk(std::uint32_t stream, std::uint32_t chunk) override;
    std::uint32_t ReadDict(std::uint32_t stream) override;
    void CloseStream(std::uint32_t stream) override;
    std::uint32_t ForAllAtomics(
        std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwDictSchemaRuntime&),
        std::uint32_t list) override;
    std::uint32_t ForAllWorldMaterials(
        std::uint32_t world,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwDictSchemaRuntime&),
        std::uint32_t list) override;
    std::uint32_t ForAllGeometryMaterials(
        std::uint32_t geometry,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwDictSchemaRuntime&),
        std::uint32_t list) override;
    bool HasUVAnim(std::uint32_t material) override;
    void AdvanceUVAnim(std::uint32_t material, float elapsed) override;
    void ApplyUVAnim(std::uint32_t material) override;

private:
    NativePackageArchive* archive_ = nullptr;
    std::string source_prefix_;
    std::unordered_map<std::uint32_t, std::vector<std::uint8_t>> streams_;
};

}  // namespace pal4
