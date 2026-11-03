#include "pal4/game_data_question_registry.h"

#include <array>

namespace pal4 {
namespace {

struct Field {
    GameDataFieldKind kind;
    std::string_view name;
    std::uint32_t offset;
};

constexpr std::array<Field, 7> kFields = {{
    {GameDataFieldKind::Integer, "ID", 0},
    {GameDataFieldKind::Text, "QUESTION", 4},
    {GameDataFieldKind::Text, "ANSWER_1", 20},
    {GameDataFieldKind::Text, "ANSWER_2", 36},
    {GameDataFieldKind::Text, "ANSWER_3", 52},
    {GameDataFieldKind::Integer, "RIGHT_ANSWER", 68},
    {GameDataFieldKind::Integer, "WINDOW", 72},
}};

}  // namespace

std::uint32_t GameDataQuestionRegistry::Initialize(
    IGameDataFieldRegistryRuntime& runtime) {
    if (!collection_initialized_) {
        collection_initialized_ = true;
        runtime.InitializeCollection(kCollectionAddress);
        runtime.RegisterCleanup(kCleanupAddress);
    }

    runtime.BeginTemporaryText();
    try {
        if (!properties_initialized_) {
            properties_initialized_ = true;
            for (const auto& field : kFields)
                runtime.AddProperty(field.kind, field.name, field.offset);
            runtime.Sort(kCollectionAddress);
        }
    } catch (...) {
        runtime.ReleaseTemporaryText();
        throw;
    }
    runtime.ReleaseTemporaryText();
    return kCollectionAddress;
}

void GameDataQuestionRegistry::Cleanup(
    IGameDataFieldRegistryRuntime& runtime) const {
    runtime.CleanupCollection(kCollectionAddress);
}

std::uint8_t GameDataQuestionRegistry::ProcessEvent(
    std::uint32_t input, IGameDataFieldRegistryRuntime& runtime) {
    return runtime.ProcessEvent(input, Initialize(runtime));
}

}  // namespace pal4
