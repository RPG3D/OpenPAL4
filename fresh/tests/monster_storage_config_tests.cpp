#include "pal4/monster_storage_config.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterStorageConfigRuntime {
    std::vector<std::string> calls;
    const pal4::MonsterStorageRecordState* record{};
    std::uint32_t found_name{0x1100u};
    std::uint32_t found_visible{0x1200u};
    std::string key;
    bool throw_on_add{};
    bool throw_on_write{};
    bool reading{};
    bool version_ok{true};
    bool throw_on_assign{};
    std::uint8_t reference{0u};

    void InitializeDocument(pal4::MonsterConfigDocument& document,
                            std::string_view root, std::uint32_t flag) override {
        Check(root == "soMonster" && flag == 0u && document.words[0] == 0u,
              "document initialization");
        calls.emplace_back("document");
    }
    void SetErrorFlag(pal4::MonsterConfigAttribute& attribute,
                      bool flag) override {
        Check((attribute.words[0] & 0xffu) == (reading ? 0u : 0xcdu),
              "query seed is argument low byte");
        calls.emplace_back(flag ? "flag+" : "flag-");
    }
    void SetExceptionMessage(pal4::MonsterConfigAttribute&,
                             std::string_view value) override {
        key = value;
        Check(key == "valName" || key == "valVisible", "attribute key");
        calls.emplace_back(key);
    }
    std::uint32_t FindAttribute(pal4::MonsterConfigLookup& lookup,
                                const pal4::MonsterConfigAttribute&) override {
        lookup.words[1] = 0x1300u;
        calls.emplace_back("find");
        return key == "valName" ? found_name : found_visible;
    }
    const char* StringAt(std::uint32_t address) override {
        Check(address == record->text_data, "record string pointer");
        calls.emplace_back("string");
        return "Monster A";
    }
    void SetAttributeString(std::uint32_t field,
                            const char* value) override {
        Check(field == found_name + 0x1cu &&
                  std::string(value) == "Monster A", "existing name update");
        calls.emplace_back("set-string");
    }
    void SetAttributeInt(std::uint32_t field,
                         const std::uint32_t* value) override {
        Check(field == found_visible + 0x1cu &&
                  value == &record->property, "existing visible update");
        calls.emplace_back("set-int");
    }
    std::uint32_t InitializeStringAttribute(
        pal4::MonsterConfigAttribute&, std::string_view name,
        const char* value) override {
        Check(name == "valName" && std::string(value) ==
              (record->text_data == 0u ? "" : "Monster A"),
              "new name attribute");
        calls.emplace_back("new-string");
        return 0xa100u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::MonsterConfigAttribute&, std::string_view name,
        const std::uint32_t* value) override {
        Check(name == "valVisible" && value == &record->property,
              "new visible attribute");
        calls.emplace_back("new-int");
        return 0xa200u;
    }
    void AddAttribute(pal4::MonsterConfigDocument&,
                      std::uint32_t address) override {
        Check(address == (key == "valName" ? 0xa100u : 0xa200u),
              "attribute handle");
        calls.emplace_back("add");
        if (throw_on_add) throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::MonsterConfigAttribute&) override {
        calls.emplace_back("destroy-attribute");
    }
    void InitializeWriteFacet(pal4::MonsterConfigFacet& facet,
                              std::uint32_t argument) override {
        Check(argument == 0x1234abcdu, "full facet argument");
        facet.words[0] = 0xfeedu;
        calls.emplace_back("facet");
    }
    void WriteConfig(pal4::MonsterConfigDocument&,
                     pal4::MonsterConfigFacet& facet) override {
        Check(facet.words[0] == 0xfeedu, "facet before reset");
        calls.emplace_back("write");
        if (throw_on_write) throw std::runtime_error("write");
    }
    void ResetWriteFacet(pal4::MonsterConfigFacet& facet) override {
        pal4::IMonsterStorageConfigRuntime::ResetWriteFacet(facet);
        Check(facet.words[0] == 0x8426acu, "facet vtable reset");
        calls.emplace_back("reset-facet");
    }
    bool CheckVersionAndType(pal4::MonsterConfigFacet& facet,
                             pal4::MonsterConfigDocument&) override {
        Check(facet.words[0] == 0xfeedu, "version facet initialization");
        calls.emplace_back("version");
        return version_ok;
    }
    void ReadName(pal4::MonsterConfigDocument&,
                  pal4::MonsterConfigTemporaryString& output,
                  std::string_view name) override {
        Check(name == "valName", "read name key");
        output.data = 0x8001u;
        output.length = 9u;
        output.capacity = 31u;
        calls.emplace_back("read-name");
    }
    void AssignName(pal4::MonsterStorageRecordState& output,
                    const pal4::MonsterConfigTemporaryString& input) override {
        Check(input.data == 0x8001u && input.length == 9u,
              "temporary string forwarded");
        calls.emplace_back("assign-name");
        if (throw_on_assign) throw std::runtime_error("assign");
        output.text_data = input.data;
        output.text_length = input.length;
        output.text_capacity = input.capacity;
    }
    std::uint8_t ReadReferenceByte(std::uint32_t address) override {
        Check(address == 0x8000u, "temporary reference header");
        calls.emplace_back("read-reference");
        return reference;
    }
    void WriteReferenceByte(std::uint32_t address,
                            std::uint8_t value) override {
        Check(address == 0x8000u && value == reference - 1u,
              "decrement temporary reference");
        calls.emplace_back("write-reference");
    }
    void FreeString(std::uint32_t address) override {
        Check(address == 0x8000u, "free temporary header");
        calls.emplace_back("free-string");
    }
    std::uint32_t AttributeToInt(std::uint32_t field,
                                 std::uint32_t* fallback) override {
        Check(field == found_visible + 0x1cu && *fallback == 0u,
              "visible parse with zero fallback");
        calls.emplace_back("to-int");
        return 0x42u;
    }
    std::uint32_t DestroyDocument(pal4::MonsterConfigDocument&) override {
        calls.emplace_back("destroy-document");
        return 0xfaceu;
    }
};

pal4::MonsterStorageRecordState Record() {
    pal4::MonsterStorageRecordState result{};
    result.text_data = 0x9000u;
    result.property = 0x76543210u;
    return result;
}

}  // namespace

int main() {
    auto record = Record();
    Runtime runtime;
    runtime.record = &record;
    Check(pal4::MonsterStorageConfig::Write(record, 0x1234abcdu, runtime)
              == 0xfaceu, "destructor result, not Config_Write result");
    Check(runtime.calls == std::vector<std::string>{
              "document", "string", "flag-", "valName", "find", "flag+",
              "set-string", "flag-", "valVisible", "find", "flag+",
              "set-int", "facet", "write", "reset-facet", "destroy-document"},
          "existing attributes and cleanup order");

    runtime.calls.clear();
    runtime.found_name = runtime.found_visible = 0x1300u;
    Check(pal4::MonsterStorageConfig::Write(record, 0x1234abcdu, runtime)
              == 0xfaceu, "inserted attributes result");
    Check(runtime.calls == std::vector<std::string>{
              "document", "string", "flag-", "valName", "find", "flag+",
              "new-string", "add", "destroy-attribute", "flag-",
              "valVisible", "find", "flag+", "new-int", "add",
              "destroy-attribute", "facet", "write", "reset-facet",
              "destroy-document"}, "two insertions and temporary cleanup");

    runtime.calls.clear();
    record.text_data = 0u;
    runtime.found_name = 0xffffffe4u;
    runtime.found_visible = 0x1200u;
    (void)pal4::MonsterStorageConfig::Write(record, 0x1234abcdu, runtime);
    Check(runtime.calls[1] == "flag-" &&
              runtime.calls[5] == "new-string", "empty name and wraparound node");

    runtime.calls.clear();
    runtime.throw_on_add = true;
    try {
        (void)pal4::MonsterStorageConfig::Write(record, 0x1234abcdu, runtime);
        Check(false, "add should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls[runtime.calls.size() - 2] == "destroy-attribute" &&
                  runtime.calls.back() == "destroy-document",
              "insertion failure releases attribute and document");
    }

    runtime.calls.clear();
    runtime.throw_on_add = false;
    runtime.throw_on_write = true;
    runtime.found_name = 0x1100u;
    record = Record();
    try {
        (void)pal4::MonsterStorageConfig::Write(record, 0x1234abcdu, runtime);
        Check(false, "write should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls[runtime.calls.size() - 2] == "reset-facet" &&
                  runtime.calls.back() == "destroy-document",
              "write failure resets facet then releases document");
    }

    runtime.calls.clear();
    runtime.reading = true;
    runtime.throw_on_write = false;
    runtime.version_ok = false;
    record = Record();
    Check(pal4::MonsterStorageConfig::Read(record, 0x1234abcdu, runtime)
              == 0xfaceu && record.text_data == 0x9000u &&
              record.property == 0x76543210u &&
              runtime.calls == std::vector<std::string>{
                  "document", "facet", "version", "reset-facet",
                  "destroy-document"},
          "version rejection leaves record unchanged");

    runtime.calls.clear();
    runtime.version_ok = true;
    runtime.found_visible = 0x1200u;
    runtime.reference = 2u;
    Check(pal4::MonsterStorageConfig::Read(record, 0x1234abcdu, runtime)
              == 0xfaceu && record.text_data == 0x8001u &&
              record.text_length == 9u && record.property == 0x42u &&
              runtime.calls == std::vector<std::string>{
                  "document", "facet", "version", "read-name",
                  "assign-name", "read-reference", "write-reference",
                  "flag-", "valVisible", "find", "flag+", "to-int",
                  "reset-facet", "destroy-document"},
          "read name, decrement reference and parse visible");

    runtime.calls.clear();
    runtime.found_visible = 0x1300u;
    runtime.reference = 0xffu;
    (void)pal4::MonsterStorageConfig::Read(record, 0x1234abcdu, runtime);
    Check(record.property == 0u &&
              runtime.calls[6] == "free-string" &&
              runtime.calls[10] == "flag+",
          "missing visible defaults to zero and sentinel reference frees");

    runtime.calls.clear();
    runtime.throw_on_assign = true;
    try {
        (void)pal4::MonsterStorageConfig::Read(record, 0x1234abcdu,
                                               runtime);
        Check(false, "assign should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls[runtime.calls.size() - 3] == "free-string" &&
                  runtime.calls[runtime.calls.size() - 2] == "reset-facet" &&
                  runtime.calls.back() == "destroy-document",
              "read exception releases temporary, facet and document");
    }
}
