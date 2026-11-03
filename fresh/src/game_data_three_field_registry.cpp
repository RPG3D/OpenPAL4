#include "pal4/game_data_three_field_registry.h"

namespace pal4 {

std::uint32_t GameDataThreeFieldRegistry::CollectionAddress() const {
    return schema_ == GameDataThreeFieldSchema::NameMemo ? 0x8ef344
                                                         : 0x8ef310;
}

std::uint32_t GameDataThreeFieldRegistry::CleanupAddress() const {
    return schema_ == GameDataThreeFieldSchema::NameMemo ? 0x4fa430
                                                         : 0x4fa760;
}

std::uint32_t GameDataThreeFieldRegistry::Initialize(
    IGameDataFieldRegistryRuntime& runtime) {
    const auto collection = CollectionAddress();
    if (!collection_initialized_) {
        collection_initialized_ = true;
        runtime.InitializeCollection(collection);
        runtime.RegisterCleanup(CleanupAddress());
    }

    runtime.BeginTemporaryText();
    try {
        if (!properties_initialized_) {
            properties_initialized_ = true;
            runtime.AddProperty(GameDataFieldKind::Integer, "ID", 0);
            runtime.AddProperty(GameDataFieldKind::Text,
                                schema_ == GameDataThreeFieldSchema::NameMemo
                                    ? "NAME" : "CHAPTER", 4);
            runtime.AddProperty(GameDataFieldKind::Text, "MEMO", 20);
            runtime.Sort(collection);
        }
    } catch (...) {
        runtime.ReleaseTemporaryText();
        throw;
    }
    runtime.ReleaseTemporaryText();
    return collection;
}

void GameDataThreeFieldRegistry::Cleanup(
    IGameDataFieldRegistryRuntime& runtime) const {
    runtime.CleanupCollection(CollectionAddress());
}

std::uint8_t GameDataThreeFieldRegistry::ProcessEvent(
    std::uint32_t input, IGameDataFieldRegistryRuntime& runtime) {
    const auto collection = Initialize(runtime);
    return runtime.ProcessEvent(input, collection);
}

}  // namespace pal4
