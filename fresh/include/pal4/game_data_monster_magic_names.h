#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataMonsterMagicNamesRuntime {
public:
    virtual ~IGameDataMonsterMagicNamesRuntime() = default;
    [[nodiscard]] virtual std::uint8_t InitialSeed() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void AssignText(std::uint32_t address,
                            std::string_view text) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t address,
                                            std::string_view text) = 0;
    virtual void DestroyText(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterAtexit(
        std::uint32_t callback_address) = 0;
};

class GameDataMonsterMagicNames final {
public:
    // IDA 0x4dd580: initialize the five names and register their destructor.
    [[nodiscard]] static int InitializeAndRegister(
        IGameDataMonsterMagicNamesRuntime& runtime);
    // IDA 0x4dd590: initialize MONSTER_MAGIC_1 through MONSTER_MAGIC_5.
    static void Initialize(IGameDataMonsterMagicNamesRuntime& runtime);
    // IDA 0x4dd720: destroy the five contiguous 16-byte text records.
    static void Destroy(IGameDataMonsterMagicNamesRuntime& runtime);
    // IDA 0x4dd740: initialize and register six field-name texts.
    [[nodiscard]] static int InitializeFieldsAndRegister(
        IGameDataMonsterMagicNamesRuntime& runtime);
    // IDA 0x4dd750: initialize MA_NAME through EFF_FINAL_HIT.
    static void InitializeFields(IGameDataMonsterMagicNamesRuntime& runtime);
    // IDA 0x4dd970: destroy the six contiguous field-name texts.
    static void DestroyFields(IGameDataMonsterMagicNamesRuntime& runtime);
    // IDA 0x4dd990/0x4dd9a0/0x4ddb00: monster drop-field names.
    [[nodiscard]] static int InitializeDropFieldsAndRegister(
        IGameDataMonsterMagicNamesRuntime& runtime);
    static void InitializeDropFields(IGameDataMonsterMagicNamesRuntime& runtime);
    static void DestroyDropFields(IGameDataMonsterMagicNamesRuntime& runtime);
    // IDA 0x4ddb20/0x4ddb30/0x4ddc90: four MDT_1..4 names.
    [[nodiscard]] static int InitializeDataTypesAndRegister(
        IGameDataMonsterMagicNamesRuntime& runtime);
    static void InitializeDataTypes(IGameDataMonsterMagicNamesRuntime& runtime);
    static void DestroyDataTypes(IGameDataMonsterMagicNamesRuntime& runtime);
};

}  // namespace pal4
