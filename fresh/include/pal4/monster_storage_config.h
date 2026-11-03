#pragma once

#include "pal4/monster_storage_object.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct MonsterConfigDocument { std::array<std::uint32_t, 5> words{}; };
struct MonsterConfigAttribute { std::array<std::uint32_t, 7> words{}; };
struct MonsterConfigLookup { std::array<std::uint32_t, 10> words{}; };
struct MonsterConfigFacet { std::array<std::uint32_t, 2> words{}; };
struct MonsterConfigTemporaryString {
    std::uint32_t flags{};
    std::uint32_t data{};
    std::uint32_t length{};
    std::uint32_t capacity{};
};
static_assert(sizeof(MonsterConfigDocument) == 20u);
static_assert(sizeof(MonsterConfigAttribute) == 28u);
static_assert(sizeof(MonsterConfigLookup) == 40u);
static_assert(sizeof(MonsterConfigFacet) == 8u);
static_assert(sizeof(MonsterConfigTemporaryString) == 16u);

class IMonsterStorageConfigRuntime {
public:
    virtual ~IMonsterStorageConfigRuntime() = default;
    virtual void InitializeDocument(MonsterConfigDocument& document,
                                    std::string_view root, std::uint32_t flag);
    virtual void SetErrorFlag(MonsterConfigAttribute& attribute, bool flag);
    virtual void SetExceptionMessage(MonsterConfigAttribute& attribute,
                                     std::string_view name);
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        MonsterConfigLookup& lookup, const MonsterConfigAttribute& attribute);
    [[nodiscard]] virtual const char* StringAt(std::uint32_t address);
    virtual void SetAttributeString(std::uint32_t value_field,
                                    const char* value);
    virtual void SetAttributeInt(std::uint32_t value_field,
                                 const std::uint32_t* value);
    [[nodiscard]] virtual std::uint32_t InitializeStringAttribute(
        MonsterConfigAttribute& attribute, std::string_view name,
        const char* value);
    [[nodiscard]] virtual std::uint32_t InitializeIntAttribute(
        MonsterConfigAttribute& attribute, std::string_view name,
        const std::uint32_t* value);
    virtual void AddAttribute(MonsterConfigDocument& document,
                              std::uint32_t attribute);
    virtual void DestroyAttribute(MonsterConfigAttribute& attribute);
    virtual void InitializeWriteFacet(MonsterConfigFacet& facet,
                                      std::uint32_t argument);
    virtual void WriteConfig(MonsterConfigDocument& document,
                             MonsterConfigFacet& facet);
    virtual void ResetWriteFacet(MonsterConfigFacet& facet);
    [[nodiscard]] virtual bool CheckVersionAndType(
        MonsterConfigFacet& facet, MonsterConfigDocument& document);
    virtual void ReadName(MonsterConfigDocument& document,
                          MonsterConfigTemporaryString& result,
                          std::string_view key);
    virtual void AssignName(MonsterStorageRecordState& record,
                            const MonsterConfigTemporaryString& source);
    [[nodiscard]] virtual std::uint8_t ReadReferenceByte(
        std::uint32_t address);
    virtual void WriteReferenceByte(std::uint32_t address,
                                    std::uint8_t value);
    virtual void FreeString(std::uint32_t address);
    [[nodiscard]] virtual std::uint32_t AttributeToInt(
        std::uint32_t value_field, std::uint32_t* fallback);
    [[nodiscard]] virtual std::uint32_t DestroyDocument(
        MonsterConfigDocument& document);
};

class MonsterStorageConfig final {
public:
    // IDA 0x432270. Returns the document destructor's raw EAX value.
    [[nodiscard]] static std::uint32_t Write(
        const MonsterStorageRecordState& record, std::uint32_t argument,
        IMonsterStorageConfigRuntime& runtime);
    // IDA 0x432460. Failed version checks leave the record untouched.
    [[nodiscard]] static std::uint32_t Read(
        MonsterStorageRecordState& record, std::uint32_t argument,
        IMonsterStorageConfigRuntime& runtime);
};

}  // namespace pal4
