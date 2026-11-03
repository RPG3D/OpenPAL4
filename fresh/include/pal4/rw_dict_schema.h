#pragma once

#include <cstdint>

namespace pal4 {

class IRwDictSchemaUpdateRuntime {
public:
    virtual ~IRwDictSchemaUpdateRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual std::uint32_t ListBegin(std::uint32_t list) = 0;
    virtual std::uint32_t ListEnd(std::uint32_t list) = 0;
    virtual void AdvanceUVAnim(std::uint32_t material, float elapsed) = 0;
    virtual void ApplyUVAnim(std::uint32_t material) = 0;
};

class IRwDictSchemaRuntime : public IRwDictSchemaUpdateRuntime {
public:
    virtual ~IRwDictSchemaRuntime() = default;
    virtual void DestroyList(std::uint32_t list) = 0;
    virtual void DestroyDict(std::uint32_t dict) = 0;
    virtual std::uint32_t SetCurrentDict(std::uint32_t dict) = 0;
    virtual std::uint32_t OpenStream(std::uint32_t type, std::uint32_t mode,
                                     const char* source) = 0;
    virtual bool FindChunk(std::uint32_t stream, std::uint32_t chunk) = 0;
    virtual std::uint32_t ReadDict(std::uint32_t stream) = 0;
    virtual void CloseStream(std::uint32_t stream) = 0;
    virtual std::uint32_t CreateList(std::uint32_t entry_size,
                                     std::uint32_t hint) = 0;
    virtual std::uint32_t ForAllAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwDictSchemaRuntime&),
        std::uint32_t list) = 0;
    virtual std::uint32_t ForAllWorldMaterials(std::uint32_t world,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwDictSchemaRuntime&),
        std::uint32_t list) = 0;
    virtual std::uint32_t ForAllGeometryMaterials(std::uint32_t geometry,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwDictSchemaRuntime&),
        std::uint32_t list) = 0;
    virtual bool HasUVAnim(std::uint32_t material) = 0;
    virtual std::uint32_t ListAppend(std::uint32_t list) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class RwDictSchema final {
public:
    // IDA 0x665770.
    RwDictSchema() = default;
    [[nodiscard]] RwDictSchema& Initialize();
    // IDA 0x665790 and 0x665870.
    [[nodiscard]] std::uint32_t CleanupWrapper(IRwDictSchemaRuntime& runtime);
    [[nodiscard]] std::uint32_t Cleanup(IRwDictSchemaRuntime& runtime);
    // IDA 0x6657a0.
    [[nodiscard]] bool LoadDict(const char* source, IRwDictSchemaRuntime& runtime);
    // IDA 0x665830 and 0x665850.
    [[nodiscard]] bool SetClump(std::uint32_t clump, IRwDictSchemaRuntime& runtime);
    [[nodiscard]] bool SetWorld(std::uint32_t world, IRwDictSchemaRuntime& runtime);
    // IDA 0x6658b0 and 0x665990.
    [[nodiscard]] std::uint32_t CreateClumpList(std::uint32_t clump,
                                                 IRwDictSchemaRuntime& runtime);
    [[nodiscard]] std::uint32_t CreateWorldList(std::uint32_t world,
                                                 IRwDictSchemaRuntime& runtime);
    // IDA 0x665910 and 0x665930.
    [[nodiscard]] static std::uint32_t ProcessGeometryMaterials(
        std::uint32_t atomic, std::uint32_t list,
        IRwDictSchemaRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddUVAnimToList(
        std::uint32_t material, std::uint32_t list,
        IRwDictSchemaRuntime& runtime);
    // IDA 0x6659d0.
    [[nodiscard]] std::uint32_t Update(float elapsed,
                                       IRwDictSchemaUpdateRuntime& runtime) const;
    // IDA 0x665a30 and 0x665a40.
    static void InitConstantsWrapper(float& output);
    static void InitConstants(float& output);

    std::uint32_t dict{};
    std::uint32_t list{};
    float time_scale{1.0f};
};

}  // namespace pal4
