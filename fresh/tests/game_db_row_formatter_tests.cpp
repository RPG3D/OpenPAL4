#include "pal4/game_db_row_formatter.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IGameDbRowFormatRuntime {
    struct Field {
        std::uint32_t type;
        const char* name;
    };
    std::map<std::uint32_t, Field> fields;
    std::map<std::uint32_t, std::int32_t> integers;
    std::map<std::uint32_t, const char*> strings;
    std::map<std::uint32_t, double> floats;
    std::vector<std::string> calls;

    std::uint32_t FieldType(std::uint32_t address) override {
        calls.push_back("type:" + std::to_string(address));
        return fields.at(address).type;
    }
    const char* FieldName(std::uint32_t address) override {
        calls.push_back("name:" + std::to_string(address));
        return fields.at(address).name;
    }
    std::int32_t IntegerAt(const pal4::GameDbValueArrayState&,
                           std::uint32_t index) override {
        calls.push_back("int:" + std::to_string(index));
        return integers.at(index);
    }
    const char* StringAt(const pal4::GameDbValueArrayState&,
                         std::uint32_t index) override {
        calls.push_back("string:" + std::to_string(index));
        return strings.at(index);
    }
    double FloatAt(const pal4::GameDbValueArrayState&,
                   std::uint32_t index) override {
        calls.push_back("float:" + std::to_string(index));
        return floats.at(index);
    }
};

void CountMismatchReplacesOutput() {
    Runtime runtime;
    pal4::GameDbSchemaArrayState schema{};
    schema.words[1] = 0x1000u;
    schema.words[2] = 0x103cu;
    pal4::GameDbValueArrayState row{0u, 0x2000u, 0x2010u, 0x2020u};
    std::string output = "old text";
    Check(pal4::GameDbRowFormatter::Dump(schema, row, output, runtime) == 1u &&
              output == "ERROR, the row doesn't belong to the table!! " &&
              runtime.calls.empty(),
          "field-count mismatch replaces text before field access");
    schema.words[1] = schema.words[2] = 0u;
    row.begin = row.end = 0u;
    output = "prefix";
    Check(pal4::GameDbRowFormatter::Dump(schema, row, output, runtime) == 0u &&
              output == "prefix\ndump dbRow info :\n ID\tFieldInfo\tValue\n",
          "matching empty schema and row append only the exact header");
}

void AllTagsAndUnrecognizedTag() {
    Runtime runtime;
    runtime.fields = {{0x1000u, {1u, "HP"}},
                      {0x103cu, {2u, "Name"}},
                      {0x1078u, {3u, "Rate"}},
                      {0x10b4u, {9u, "ignored"}}};
    runtime.integers[0u] = -17;
    runtime.strings[1u] = "Yun";
    runtime.floats[2u] = -2.5;
    pal4::GameDbSchemaArrayState schema{};
    schema.words[1] = 0x1000u;
    schema.words[2] = 0x10f0u;
    const pal4::GameDbValueArrayState row{0u, 0x2000u, 0x2020u, 0x2030u};
    std::string output;
    Check(pal4::GameDbRowFormatter::Dump(schema, row, output, runtime) == 0u &&
              output == "\ndump dbRow info :\n ID\tFieldInfo\tValue\n"
                        " 0\tHP\t-17 \n"
                        " 1\tName\tYun \n"
                        " 2\tRate\t-2.500000 \n"
                        " 2\tRate\t-2.500000 \n" &&
              runtime.calls == std::vector<std::string>{
                  "type:4096", "name:4096", "int:0",
                  "type:4156", "name:4156", "string:1",
                  "type:4216", "name:4216", "float:2",
                  "type:4276"},
          "all three types format exactly and unknown type reuses stack line");
    runtime.fields[0x1000u] = {8u, nullptr};
    runtime.calls.clear();
    schema.words[2] = 0x103cu;
    const pal4::GameDbValueArrayState one_row{0u, 0x2000u, 0x2008u, 0x2010u};
    output.clear();
    Check(pal4::GameDbRowFormatter::Dump(
              schema, one_row, output, runtime) == 0u &&
              output == "\ndump dbRow info :\n ID\tFieldInfo\tValue\n" &&
              runtime.calls == std::vector<std::string>{"type:4096"},
          "unknown first type appends no stale text");
}

void NullNamesAndValues() {
    Runtime runtime;
    runtime.fields = {{0x3000u, {1u, nullptr}},
                      {0x303cu, {2u, nullptr}}};
    runtime.integers[0u] = 0;
    runtime.strings[1u] = nullptr;
    pal4::GameDbSchemaArrayState schema{};
    schema.words[1] = 0x3000u;
    schema.words[2] = 0x3078u;
    const pal4::GameDbValueArrayState row{0u, 0x4000u, 0x4010u, 0x4020u};
    std::string output;
    (void)pal4::GameDbRowFormatter::Dump(schema, row, output, runtime);
    Check(output == "\ndump dbRow info :\n ID\tFieldInfo\tValue\n"
                    " 0\t\t0 \n"
                    " 1\t\t \n",
          "null schema and row strings use the native empty text fallback");
}

}  // namespace

int main() {
    CountMismatchReplacesOutput();
    AllTagsAndUnrecognizedTag();
    NullNamesAndValues();
    std::cout << "GameDB row formatter tests passed\n";
}
