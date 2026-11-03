#include "pal4/monster_storage_config.h"

namespace pal4 {

void IMonsterStorageConfigRuntime::InitializeDocument(
    MonsterConfigDocument&, std::string_view, std::uint32_t) {}
void IMonsterStorageConfigRuntime::SetErrorFlag(
    MonsterConfigAttribute&, bool) {}
void IMonsterStorageConfigRuntime::SetExceptionMessage(
    MonsterConfigAttribute&, std::string_view) {}
std::uint32_t IMonsterStorageConfigRuntime::FindAttribute(
    MonsterConfigLookup& lookup, const MonsterConfigAttribute&) {
    return lookup.words[1];
}
const char* IMonsterStorageConfigRuntime::StringAt(std::uint32_t) {
    return "";
}
void IMonsterStorageConfigRuntime::SetAttributeString(
    std::uint32_t, const char*) {}
void IMonsterStorageConfigRuntime::SetAttributeInt(
    std::uint32_t, const std::uint32_t*) {}
std::uint32_t IMonsterStorageConfigRuntime::InitializeStringAttribute(
    MonsterConfigAttribute&, std::string_view, const char*) { return 0u; }
std::uint32_t IMonsterStorageConfigRuntime::InitializeIntAttribute(
    MonsterConfigAttribute&, std::string_view, const std::uint32_t*) {
    return 0u;
}
void IMonsterStorageConfigRuntime::AddAttribute(
    MonsterConfigDocument&, std::uint32_t) {}
void IMonsterStorageConfigRuntime::DestroyAttribute(
    MonsterConfigAttribute&) {}
void IMonsterStorageConfigRuntime::InitializeWriteFacet(
    MonsterConfigFacet&, std::uint32_t) {}
void IMonsterStorageConfigRuntime::WriteConfig(
    MonsterConfigDocument&, MonsterConfigFacet&) {}
void IMonsterStorageConfigRuntime::ResetWriteFacet(MonsterConfigFacet& facet) {
    facet.words[0] = 0x8426acu;
}
bool IMonsterStorageConfigRuntime::CheckVersionAndType(
    MonsterConfigFacet&, MonsterConfigDocument&) { return false; }
void IMonsterStorageConfigRuntime::ReadName(
    MonsterConfigDocument&, MonsterConfigTemporaryString&, std::string_view) {}
void IMonsterStorageConfigRuntime::AssignName(
    MonsterStorageRecordState&, const MonsterConfigTemporaryString&) {}
std::uint8_t IMonsterStorageConfigRuntime::ReadReferenceByte(
    std::uint32_t) { return 0u; }
void IMonsterStorageConfigRuntime::WriteReferenceByte(
    std::uint32_t, std::uint8_t) {}
void IMonsterStorageConfigRuntime::FreeString(std::uint32_t) {}
std::uint32_t IMonsterStorageConfigRuntime::AttributeToInt(
    std::uint32_t, std::uint32_t* fallback) { return *fallback; }
std::uint32_t IMonsterStorageConfigRuntime::DestroyDocument(
    MonsterConfigDocument&) { return 0u; }

std::uint32_t MonsterStorageConfig::Write(
    const MonsterStorageRecordState& record, const std::uint32_t argument,
    IMonsterStorageConfigRuntime& runtime) {
    MonsterConfigDocument document{};
    MonsterConfigAttribute attribute{};
    MonsterConfigLookup lookup{};
    MonsterConfigFacet facet{};
    bool document_ready = false;
    bool query_ready = false;
    bool attribute_ready = false;
    bool facet_ready = false;

    try {
        runtime.InitializeDocument(document, "soMonster", 0u);
        document_ready = true;
        const char* name = record.text_data == 0u
            ? "" : runtime.StringAt(record.text_data);

        auto find = [&](std::string_view key) {
            attribute.words[0] = (attribute.words[0] & 0xffffff00u)
                | (argument & 0xffu);
            runtime.SetErrorFlag(attribute, false);
            query_ready = true;
            runtime.SetExceptionMessage(attribute, key);
            const auto found = runtime.FindAttribute(lookup, attribute);
            runtime.SetErrorFlag(attribute, true);
            query_ready = false;
            return found;
        };
        auto missing = [&](std::uint32_t found) {
            return found == lookup.words[1] || found + 0x1cu == 0u;
        };

        const auto name_attribute = find("valName");
        if (missing(name_attribute)) {
            const auto value = runtime.InitializeStringAttribute(
                attribute, "valName", name);
            attribute_ready = true;
            runtime.AddAttribute(document, value);
            runtime.DestroyAttribute(attribute);
            attribute_ready = false;
        } else {
            runtime.SetAttributeString(name_attribute + 0x1cu, name);
        }

        const auto visible_attribute = find("valVisible");
        if (missing(visible_attribute)) {
            const auto value = runtime.InitializeIntAttribute(
                attribute, "valVisible", &record.property);
            attribute_ready = true;
            runtime.AddAttribute(document, value);
            runtime.DestroyAttribute(attribute);
            attribute_ready = false;
        } else {
            runtime.SetAttributeInt(visible_attribute + 0x1cu,
                                    &record.property);
        }

        runtime.InitializeWriteFacet(facet, argument);
        facet_ready = true;
        runtime.WriteConfig(document, facet);
        runtime.ResetWriteFacet(facet);
        facet_ready = false;
        document_ready = false;
        return runtime.DestroyDocument(document);
    } catch (...) {
        if (facet_ready)
            runtime.ResetWriteFacet(facet);
        if (attribute_ready)
            runtime.DestroyAttribute(attribute);
        else if (query_ready)
            runtime.SetErrorFlag(attribute, true);
        if (document_ready)
            (void)runtime.DestroyDocument(document);
        throw;
    }
}

std::uint32_t MonsterStorageConfig::Read(
    MonsterStorageRecordState& record, const std::uint32_t argument,
    IMonsterStorageConfigRuntime& runtime) {
    MonsterConfigDocument document{};
    MonsterConfigFacet facet{};
    MonsterConfigAttribute query{};
    MonsterConfigLookup lookup{};
    MonsterConfigTemporaryString temporary{};
    bool document_ready = false;
    bool facet_ready = false;
    bool query_ready = false;
    bool temporary_ready = false;

    try {
        runtime.InitializeDocument(document, "soMonster", 0u);
        document_ready = true;
        runtime.InitializeWriteFacet(facet, argument);
        facet_ready = true;
        if (runtime.CheckVersionAndType(facet, document)) {
            // sub_432960 supplies a temporary string, whose reference is
            // released after StringBuffer_Assign has consumed it.
            runtime.ReadName(document, temporary, "valName");
            temporary_ready = true;
            runtime.AssignName(record, temporary);
            temporary_ready = false;
            if (temporary.data != 0u) {
                const auto header = temporary.data - 1u;
                const auto reference = runtime.ReadReferenceByte(header);
                if (reference == 0u || reference == 0xffu)
                    runtime.FreeString(header);
                else
                    runtime.WriteReferenceByte(header,
                        static_cast<std::uint8_t>(reference - 1u));
            }

            query.words[0] = 0u;
            runtime.SetErrorFlag(query, false);
            query_ready = true;
            runtime.SetExceptionMessage(query, "valVisible");
            const auto found = runtime.FindAttribute(lookup, query);
            runtime.SetErrorFlag(query, true);
            query_ready = false;
            std::uint32_t visible = 0u;
            if (found != lookup.words[1] && found + 0x1cu != 0u)
                visible = runtime.AttributeToInt(found + 0x1cu, &visible);
            record.property = visible;
        }
        runtime.ResetWriteFacet(facet);
        facet_ready = false;
        document_ready = false;
        return runtime.DestroyDocument(document);
    } catch (...) {
        if (temporary_ready && temporary.data != 0u) {
            const auto header = temporary.data - 1u;
            const auto reference = runtime.ReadReferenceByte(header);
            if (reference == 0u || reference == 0xffu)
                runtime.FreeString(header);
            else
                runtime.WriteReferenceByte(header,
                    static_cast<std::uint8_t>(reference - 1u));
        }
        if (query_ready)
            runtime.SetErrorFlag(query, true);
        if (facet_ready)
            runtime.ResetWriteFacet(facet);
        if (document_ready)
            (void)runtime.DestroyDocument(document);
        throw;
    }
}

}  // namespace pal4
