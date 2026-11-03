#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/game_data_equip_record.h"
#include "pal4/game_data_monster_record.h"
#include "pal4/game_data_parameter_record.h"

namespace pal4 {

class IGameDataAiConfigObjectRuntime {
public:
    virtual ~IGameDataAiConfigObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void EffectConstruct(std::uint32_t object,
                                 std::uint32_t name) = 0;
    virtual void ClearEffectData(std::uint32_t object) = 0;
    virtual void ProcessElements(std::uint32_t array) = 0;
    virtual void DestroyElement(std::uint32_t element,
                                std::uint8_t flags) = 0;
    virtual void ResetRecordPointers(std::uint32_t element) = 0;
    virtual void SetErrorFlag(std::uint32_t text, std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t EffectDestruct(
        std::uint32_t object) = 0;
};

class GameDataAiConfigObject final {
public:
    // IDA 0x4cc040: allocate Effect-derived object and three text records.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4cc110: release members and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4cc310: clear effects and reset the first array end.
    [[nodiscard]] static std::uint32_t ResetEffects(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
};

class GameDataAiConfigStorage final {
public:
    // IDA 0x4cc210: destroy 60-byte records and release their array.
    [[nodiscard]] static std::uint32_t ClearRecords(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4cc250: reset flags inside 60-byte records and release arrays.
    [[nodiscard]] static std::uint32_t ClearFlags(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
};

class GameDataAiPropertyObject final {
public:
    // IDA 0x4ccb60: allocate the AiProperty Effect-derived object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4ccc30: release its three arrays and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4ccd30: clear effects and reset the first array end.
    [[nodiscard]] static std::uint32_t ResetEffects(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
};

class IGameDataUpgradeEffectNameRuntime {
public:
    virtual ~IGameDataUpgradeEffectNameRuntime() = default;
    [[nodiscard]] virtual bool PrepareTemporaryName(
        std::uint8_t seed, std::string_view name) = 0;
    virtual void CreateEffect(std::uint32_t object,
                              std::string_view temporary_name,
                              std::uint32_t flags) = 0;
    virtual void ReleaseTemporaryName() = 0;
};

class GameDataUpgradeEffectObject final {
public:
    // IDA 0x4cfe00: create the ITEM_ID effect from a temporary name buffer.
    [[nodiscard]] static std::uint8_t InitializeItemId(
        std::uint32_t object, IGameDataUpgradeEffectNameRuntime& runtime);
    // IDA 0x4cfee0: allocate the 140-byte Effect-derived object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4cffb0: release three arrays and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4d00b0: clear effects and reset the 92-byte record array end.
    [[nodiscard]] static std::uint32_t ResetEffects(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
};

class GameDataDropEffectObject final {
public:
    // IDA 0x4d0910: allocate the 140-byte DropProperty Effect object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4d09e0: release three arrays and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4d0ad0: clear effects and reset the scalar record array end.
    [[nodiscard]] static std::uint32_t ResetEffects(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
};

class GameDataEquipRelationEffectObject final {
public:
    // IDA 0x4d5640: allocate the EquipRelation Effect object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4d5710: release its three arrays and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4d5810: clear effects and reset the relation-record array end.
    [[nodiscard]] static std::uint32_t ResetEffects(
        std::uint32_t object, IGameDataAiConfigObjectRuntime& runtime);
};

class IGameDataEquipEffectObjectRuntime
    : public IGameDataAiConfigObjectRuntime,
      public IGameDataEquipRecordRuntime {};

class GameDataEquipEffectObject final {
public:
    // IDA 0x4d6040: allocate the 140-byte Equip Effect object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataEquipEffectObjectRuntime& runtime);
    // IDA 0x4d6110: release 400-byte records, field arrays and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataEquipEffectObjectRuntime& runtime);
    // IDA 0x4d62a0: clear effects and destroy live Equip records.
    static void ResetEffects(std::uint32_t object,
                             IGameDataEquipEffectObjectRuntime& runtime);
};

class GameDataEquipRecordStorage final {
public:
    // IDA 0x4d6260: destroy and free an array of 400-byte records.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t array, IGameDataEquipEffectObjectRuntime& runtime);
};

class IGameDataMissionEffectRuntime : public IGameDataAiConfigObjectRuntime {
public:
    virtual void DestroyTextRecord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyMissionRecord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffectDataForReset(
        std::uint32_t object) = 0;
};

class GameDataMissionEffectObject final {
public:
    // IDA 0x4db740: allocate a 140-byte Mission Effect object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataMissionEffectRuntime& runtime);
    // IDA 0x4db810: release Mission records, arrays and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataMissionEffectRuntime& runtime);
    // IDA 0x4db990: clear effects and destroy live Mission records.
    [[nodiscard]] static std::uint32_t ResetEffects(
        std::uint32_t object, IGameDataMissionEffectRuntime& runtime);
};

class GameDataMonsterEffectObject final {
public:
    // IDA 0x4debd0: allocate the 140-byte Monster Effect object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4deca0: release Monster records, fields and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, class IGameDataMonsterEffectRuntime& runtime);
    // IDA 0x4deed0: clear effects and destroy live Monster records.
    static void ResetEffects(
        std::uint32_t object, class IGameDataMonsterEffectRuntime& runtime);
};

class IGameDataParameterEffectRuntime
    : public IGameDataAiConfigObjectRuntime,
      public IGameDataParameterRecordDestroyRuntime {};

class GameDataParameterEffectObject final {
public:
    // IDA 0x4e2ad0: allocate the 140-byte Prescription Effect object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4e2ba0: destroy records, fields and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataParameterEffectRuntime& runtime);
    // IDA 0x4e2d30: clear effects and destroy live 328-byte records.
    static void ResetEffects(std::uint32_t object,
                             IGameDataParameterEffectRuntime& runtime);
};

class GameDataParameterRecordStorage final {
public:
    // IDA 0x4e2cf0: destroy and free an array of 328-byte records.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t array, IGameDataParameterEffectRuntime& runtime);
};

class IGameDataMonsterEffectRuntime
    : public IGameDataAiConfigObjectRuntime,
      public IGameDataMonsterRecordDestroyRuntime {};

class GameDataMonsterRecordStorage final {
public:
    // IDA 0x4dee90: destroy and free an array of 596-byte records.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t array, IGameDataMonsterEffectRuntime& runtime);
};

class GameDataMissionRecordStorage final {
public:
    // IDA 0x4db950: destroy/free an array of 68-byte records.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t array, IGameDataMissionEffectRuntime& runtime);
};

}  // namespace pal4
