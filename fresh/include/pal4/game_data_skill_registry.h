#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <vector>

namespace pal4 {

using GameDataSkillFieldKind = GameDataFieldKind;
using GameDataSkillField = GameDataFieldDescriptor;

class IGameDataSkillRegistryRuntime {
public:
    virtual ~IGameDataSkillRegistryRuntime() = default;
    virtual void InitializeGlobalStorage() = 0;
    virtual void RegisterGlobalCleanup() = 0;
    virtual void CleanupGlobalStorage() = 0;
    [[nodiscard]] virtual std::string SourceName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void AppendField(std::vector<GameDataSkillField>& fields,
                             GameDataSkillField field) = 0;
    virtual void Sort(std::vector<GameDataSkillField>& fields) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCollection(
        std::uint32_t object, const std::vector<GameDataSkillField>& fields) = 0;
};

class GameDataSkillRegistry final {
public:
    // IDA 0x4d8bb0: one-time Skill field registration.
    [[nodiscard]] const std::vector<GameDataSkillField>& Initialize(
        IGameDataSkillRegistryRuntime& runtime);
    // IDA 0x4d9bb0: CRT collection cleanup.
    void Cleanup(IGameDataSkillRegistryRuntime& runtime);
    // IDA 0x4d9bc0: initialize and process the collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t object, IGameDataSkillRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataSkillField> fields_;
};

}  // namespace pal4
