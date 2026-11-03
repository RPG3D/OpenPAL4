#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/skill_data_lookup.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Field { const char* name; unsigned offset, type; };
// Independent IDA4F2DB0 record contract, including all four text headers.
constexpr Field fields[]{
    {"ID",0,1},{"TYPE",4,1},{"TRIGER",8,1},{"EXIST_TIME",12,1},{"FUNCTION",16,1},
    {"SKILL_PARAM_1",20,3},{"SKILL_PARAM_2",24,3},{"SKILL_PARAM_3",28,3},
    {"SKILL_PARAM_4",32,3},{"SKILL_PARAM_5",36,3},{"SKILL_PARAM_6",40,3},
    {"SKILL_PARAM_7",44,3},{"SKILL_PARAM_8",48,3},{"SKILL_PARAM_9",52,3},
    {"ATTACHED_SKILL",56,1},{"MUTEX_ID",60,1},{"STATUS_ID",64,1},
    {"EF",68,2},{"BIND",84,2},{"EF1",100,2},{"BIND1",116,2}};
unsigned Word(unsigned address) { unsigned result; std::memcpy(&result,reinterpret_cast<const void*>(address),4u); return result; }
struct Lookup : pal4::ISkillDataLookupRuntime { unsigned ReadWord(unsigned address) override { return Word(address); } };
void Verify(const pal4::NativeGameDbTable& table, const pal4::NativeSkillRecordCatalog& catalog) {
    Check(catalog.Size() == table.rows.size(), "catalog retains physical record count");
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
        const auto& row = table.rows[i];
        const auto& record = catalog.At(i);
        const auto address = catalog.Address(record[0]);
        Check(address == reinterpret_cast<std::uintptr_t>(record.data()) && catalog.OwnsRecord(address) &&
            !catalog.OwnsRecord(address + 1u), "shared signed lookup returns exact physical record boundary");
        if (i) Check(address == reinterpret_cast<std::uintptr_t>(catalog.At(0).data()) + i * 132u,
            "physical native skill records are contiguous with132-byte stride");
        for (const auto& field : fields) {
            const auto it = std::find_if(table.fields.begin(),table.fields.end(),
                [&](const auto& value) { return value.name == field.name; });
            Check(it != table.fields.end(), "source has required skill field");
            const auto& value = row[static_cast<std::size_t>(it - table.fields.begin())];
            const auto word = field.offset / 4u;
            if (field.type == 1u) Check(record[word] == std::get<unsigned>(value), "skill numeric field at original offset");
            else if (field.type == 3u) Check(record[word] == std::bit_cast<unsigned>(std::get<float>(value)), "all9 float parameters retain exact bit patterns");
            else {
                const auto& text = std::get<std::string>(value);
                Check(record[word + 2u] == text.size() && record[word + 3u] == text.size(), "skill text header retains byte length");
                if (text.empty()) Check(record[word+1u] == 0u, "empty skill text has no allocation");
                else {
                    const auto* bytes = reinterpret_cast<const unsigned char*>(record[word+1u]);
                    Check(bytes[-1] == 0xfeu && bytes[text.size()] == 0u &&
                        std::memcmp(bytes,text.data(),text.size()) == 0, "skill owns exact counted text and terminator");
                }
            }
        }
    }
}
pal4::NativeGameDbTable GeneratedTable() {
    pal4::NativeGameDbTable table;
    table.name = "Skill";
    for (const auto& field : fields) table.fields.push_back({field.name,field.type});
    // Unsorted physical order and signed edge keys exercise the native index.
    for (auto id : {7u, 0xffffffffu, 0u, 0x80000000u}) {
        std::vector<pal4::NativeGameDbValue> row;
        for (const auto& field : fields) {
            if (field.type == 1u) row.emplace_back(field.offset == 0u ? id : 100u + field.offset);
            else if (field.type == 3u) row.emplace_back(std::bit_cast<float>(0x80000000u + field.offset));
            else row.emplace_back(field.offset == 84u ? std::string{} : std::string("x\0y",3));
        }
        table.rows.push_back(std::move(row));
    }
    return table;
}
void Generated() {
    auto table = GeneratedTable();
    pal4::NativeSkillRecordCatalog catalog(table);
    Verify(table,catalog);
    Lookup lookup;
    Check(catalog.Address(8u) == 0u && !catalog.OwnsRecord(0u) &&
        pal4::SkillDataLookup::HasSkill(catalog.CatalogAddress(),-1,lookup), "native skill lookup distinguishes misses and negative keys");
    const auto retained = catalog.Address(7u);
    table.rows.clear();
    Check(Word(retained) == 7u && catalog.OwnsRecord(retained), "catalog owns records after source mutation");
    const auto fallback = catalog.ResolveRecord(0u);
    Check(catalog.ResolveRecord(retained) == retained && catalog.OwnsRecord(fallback) &&
        Word(fallback) == 0xffffffffu && Word(fallback+4u) == 4u &&
        Word(fallback+8u) == 15u && Word(fallback+12u) == 0u &&
        Word(fallback+16u) == 0u && Word(fallback+56u) == 0u &&
        Word(fallback+60u) == 0xffffffffu && Word(fallback+64u) == 0xffffffffu &&
        Word(fallback+72u) == 0u && catalog.ResolveRecord(0u) == fallback,
        "null skill entries reuse the original owned fallback fields without record allocation");
    bool foreign = false;
    try { (void)catalog.ResolveRecord(retained+4u); } catch(const std::invalid_argument&) { foreign=true; }
    Check(foreign, "skill resolution rejects foreign and interior addresses");
    pal4::NativeSkillRecordCatalog empty(table);
    Check(empty.Size() == 0u && empty.Address(1u) == 0u, "empty native skill index is queryable");
    auto Reject = [](const pal4::NativeGameDbTable& value) {
        bool threw = false;
        try { pal4::NativeSkillRecordCatalog invalid(value); } catch (const std::exception&) { threw = true; }
        Check(threw, "incompatible skill source is rejected");
    };
    table = GeneratedTable(); table.fields[0].type = 2u; Reject(table);
    table = GeneratedTable(); table.fields.erase(table.fields.begin()); Reject(table);
    table = GeneratedTable(); table.rows[0].pop_back(); Reject(table);
    table = GeneratedTable(); table.rows[0][5] = std::string("wrong type"); Reject(table);
    table = GeneratedTable(); table.rows.push_back(table.rows.front()); Reject(table);
}
void Original(const char* root) {
    pal4::NativeGameDatabase database;
    Check(database.Load(root), "load original skill database");
    pal4::NativeSkillRecordCatalog catalog(database);
    Verify(database.Tables().at("Skill"),catalog);
    std::map<unsigned,unsigned> functions;
    unsigned links{};
    for (std::size_t i=0;i<catalog.Size();++i) {
        const auto& record = catalog.At(i);
        if (record[3] == 0xffffffffu && record[2] == 1u) ++functions[record[4]];
        if (record[14] != 0u && catalog.Address(record[14]) != 0u) ++links;
    }
    Check(!functions.empty() && links > 0u, "original skill data has passive effects and linked skills");
    std::cout << "Original complete Skill records=" << catalog.Size() << " resolved links=" << links << '\n';
    for (const auto& [function,count] : functions) std::cout << "Passive function=" << function << " count=" << count << '\n';
}
}
int main(int argc,char** argv) {
    try { Generated(); if(argc>1) Original(argv[1]); std::cout << "PASS: native complete Skill records\n"; }
    catch(const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
