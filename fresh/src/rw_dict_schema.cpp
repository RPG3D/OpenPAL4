#include "pal4/rw_dict_schema.h"

#include <bit>

namespace pal4 {

RwDictSchema& RwDictSchema::Initialize() {
    dict = 0;
    list = 0;
    time_scale = 1.0f;
    return *this;
}

std::uint32_t RwDictSchema::CleanupWrapper(IRwDictSchemaRuntime& runtime) {
    return Cleanup(runtime);
}

std::uint32_t RwDictSchema::Cleanup(IRwDictSchemaRuntime& runtime) {
    if (list != 0) {
        runtime.DestroyList(list);
        list = 0;
    }
    if (dict != 0) {
        runtime.DestroyDict(dict);
        dict = 0;
    }
    return runtime.SetCurrentDict(0);
}

bool RwDictSchema::LoadDict(const char* source, IRwDictSchemaRuntime& runtime) {
    (void)Cleanup(runtime);
    const auto stream = runtime.OpenStream(2, 1, source);
    if (stream != 0) {
        if (runtime.FindChunk(stream, 43)) {
            dict = runtime.ReadDict(stream);
        }
        runtime.CloseStream(stream);
    }
    (void)runtime.SetCurrentDict(dict);
    return dict != 0;
}

bool RwDictSchema::SetClump(std::uint32_t clump, IRwDictSchemaRuntime& runtime) {
    if (dict == 0) return false;
    (void)CreateClumpList(clump, runtime);
    return true;
}

bool RwDictSchema::SetWorld(std::uint32_t world, IRwDictSchemaRuntime& runtime) {
    if (dict == 0) return false;
    (void)CreateWorldList(world, runtime);
    return true;
}

std::uint32_t RwDictSchema::CreateClumpList(std::uint32_t clump,
                                           IRwDictSchemaRuntime& runtime) {
    if (list != 0) runtime.DestroyList(list);
    list = runtime.CreateList(4, 0);
    if (list == 0) return 0;
    return runtime.ForAllAtomics(clump, &ProcessGeometryMaterials, list);
}

std::uint32_t RwDictSchema::CreateWorldList(std::uint32_t world,
                                           IRwDictSchemaRuntime& runtime) {
    if (list != 0) runtime.DestroyList(list);
    list = runtime.CreateList(4, 0);
    if (list == 0) return 0;
    return runtime.ForAllWorldMaterials(world, &AddUVAnimToList, list);
}

std::uint32_t RwDictSchema::ProcessGeometryMaterials(
    std::uint32_t atomic, std::uint32_t list, IRwDictSchemaRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24);
    (void)runtime.ForAllGeometryMaterials(geometry, &AddUVAnimToList, list);
    return atomic;
}

std::uint32_t RwDictSchema::AddUVAnimToList(
    std::uint32_t material, std::uint32_t list, IRwDictSchemaRuntime& runtime) {
    if (!runtime.HasUVAnim(material)) return material;
    for (auto cursor = runtime.ListBegin(list), end = runtime.ListEnd(list);
         cursor != end; cursor += 4) {
        if (runtime.ReadWord(cursor) == material) return material;
        end = runtime.ListEnd(list);
    }
    runtime.WriteWord(runtime.ListAppend(list), material);
    return material;
}

std::uint32_t RwDictSchema::Update(float elapsed,
                                   IRwDictSchemaUpdateRuntime& runtime) const {
    if (list == 0) return 0;
    auto cursor = runtime.ListBegin(list);
    auto end = runtime.ListEnd(list);
    while (cursor != end) {
        const auto material = runtime.ReadWord(cursor);
        runtime.AdvanceUVAnim(material, elapsed * time_scale);
        runtime.ApplyUVAnim(material);
        cursor += 4;
        end = runtime.ListEnd(list);
    }
    return end;
}

void RwDictSchema::InitConstantsWrapper(float& output) {
    InitConstants(output);
}

void RwDictSchema::InitConstants(float& output) {
    output = std::bit_cast<float>(0x3fc90fdbu) +
             std::bit_cast<float>(0x40490fdbu);
}

}  // namespace pal4
