#include "pal4/game_data_parameter_table.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataParameterRowRuntime,
                       pal4::IGameDataParameterAppendRuntime,
                       pal4::IGameDataParameterRefreshRuntime,
                       pal4::IGameDataParameterBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t collection_size = 2;
    std::uint32_t row_size = 2;
    std::uint32_t key = 7;
    std::uint32_t category = 2;
    std::uint32_t position = 0x3000;
    std::uint32_t copied_to = 0;
    std::uint32_t inserted_index = 0xffffffff;
    bool parse_success = true;
    bool throw_on_copy = false;
    std::uint32_t refresh_result = 0x7777;
    unsigned destroyed = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x1000 + 124, "collection size uses table +124");
        calls.emplace_back("size");
        return collection_size;
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t table_name) override {
        Check(table_name == 0x1000 + 52, "bind log names table +52");
        calls.push_back(std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x1000 + 108 && source == 0x1000 + 20,
              "Effect assignment uses table +108 and +20");
        calls.emplace_back("assign effects");
    }
    void InitializeStructures(std::uint32_t binding) override {
        Check(binding == 0x1000 + 108, "initialization uses binding +108");
        calls.emplace_back("initialize binding");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x1000 + 108, "binding uses +108");
        calls.emplace_back("bind effects");
    }
    void InitializeRecord(pal4::GameDataParameterRecord& record) override {
        record.fill(0);
        record[0] = key;
        record[25] = category;
        calls.emplace_back("construct record");
    }
    void DestroyTemporaryRecordTexts(
        pal4::GameDataParameterRecord&) override {
        ++destroyed;
        calls.emplace_back("destroy record texts");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::GameDataParameterRecord& record) override {
        Check(binding == 0x1000 + 108 && row == 0x9000,
              "parse gets binding and row");
        record[0] = key;
        record[25] = category;
        calls.emplace_back("parse");
        return parse_success;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t search_key) override {
        Check(begin == 0x3000 && end == 0x3008 && search_key == key,
              "lookup gets indexed key range and parsed key");
        calls.emplace_back("find key");
        return position;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataParameterRecord& record) override {
        Check(record[0] == key && record[25] == category,
              "replacement uses parsed record");
        copied_to = destination;
        calls.emplace_back("copy record");
        if (throw_on_copy) throw std::runtime_error("copy failed");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataParameterRecord& record) override {
        Check(array == 0x1000 + 92 && end == words[0x1000 + 100] &&
                  record[0] == key,
              "insert gets three-pointer array, old end and parsed record");
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t insert_key,
                   std::uint32_t index) override {
        Check(array == 0x1000 + 4 && at == position && insert_key == key,
              "key insertion uses array +4 and binary-search position");
        inserted_index = index;
        calls.emplace_back("insert key");
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x9000, "row size gets source row");
        calls.emplace_back("row size");
        return row_size;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x9000, "property lookup gets source row");
        calls.push_back("property " + std::to_string(index));
        return 0xa000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataParameterRecord& record,
                      std::uint32_t property) override {
        calls.push_back("convert " + std::to_string(field));
        Check(field == 0x6000 + (property - 0xa000) * 4,
              "converter gets bound field in row order");
        record[30 + (property - 0xa000)] = property;
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataParameterRecord& record) override {
        Check(array == 0x1000 + 92 && end == words[0x1000 + 100] &&
                  record[30] == 0xa000 && record[31] == 0xa001,
              "append gets converted record and current array end");
        calls.emplace_back("append record");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x9000,
              "error dump gets table and offending row");
        calls.emplace_back("dump row");
        return "row detail";
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1000 + 88 && value == 1,
              "refresh writes byte one at +88");
        calls.emplace_back("mark");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "refresh receives table");
        calls.emplace_back("refresh");
        return refresh_result;
    }
};

Runtime Base() {
    Runtime runtime;
    runtime.words[0x1000 + 24] = 0x2000;
    runtime.words[0x1000 + 28] = 0x2000 + 2 * 60;
    runtime.words[0x1000 + 8] = 0x3000;
    runtime.words[0x1000 + 12] = 0x3008;
    runtime.words[0x3000] = 7;
    runtime.words[0x3004] = 1;
    runtime.words[0x1000 + 96] = 0x4000;
    runtime.words[0x1000 + 100] = 0x4000 + 3 * 328;
    runtime.words[0x1000 + 128] = 0x5000;
    runtime.words[0x5000] = 0x6000;
    runtime.words[0x5004] = 0x6004;
    return runtime;
}

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected row operation missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckReplaceAndInsert() {
    auto replace = Base();
    Check(pal4::GameDataParameterTable::AddOrReplace(
              0x1000, 0x9000, replace) == 1 &&
              replace.copied_to == 0x4000 + 328 &&
              replace.inserted_index == 0xffffffff &&
              replace.destroyed == 1 &&
              replace.calls.back() == "destroy record texts",
          "equal signed key replaces indexed record and destroys temporary");
    Check(std::find(replace.calls.begin(), replace.calls.end(),
                    "assign effects") == replace.calls.end(),
          "matching field and bound counts skip rebinding");

    auto insert = Base();
    insert.position = 0x3008;
    insert.collection_size = 1;
    Check(pal4::GameDataParameterTable::AddOrReplace(
              0x1000, 0x9000, insert) == 1 &&
              insert.inserted_index == 3 && insert.destroyed == 1,
          "new key inserts record and records its 328-byte index");
    Check(At(insert, "beginBindField, table:") <
              At(insert, "assign effects") &&
              At(insert, "assign effects") <
                  At(insert, "initialize binding") &&
              At(insert, "initialize binding") <
                  At(insert, "bind effects") &&
              At(insert, "bind effects") <
                  At(insert, "endBindField, table:") &&
              At(insert, "insert record") < At(insert, "insert key") &&
              At(insert, "insert key") < At(insert, "destroy record texts"),
          "rebinding, insertion and cleanup follow IDA order");

    auto empty = Base();
    empty.words[0x1000 + 96] = 0;
    empty.words[0x1000 + 100] = 0;
    empty.position = 0x3008;
    Check(pal4::GameDataParameterTable::AddOrReplace(
              0x1000, 0x9000, empty) == 1 &&
              empty.inserted_index == 0,
          "empty record array inserts with index zero");
}

void CheckErrors() {
    for (const auto [parse, category] :
         {std::pair{false, 2u}, {true, 3u}, {true, 0xffffffffu}}) {
        auto runtime = Base();
        runtime.parse_success = parse;
        runtime.category = category;
        try {
            (void)pal4::GameDataParameterTable::AddOrReplace(
                0x1000, 0x9000, runtime);
            Check(false, "invalid row must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow >row detail" &&
                      runtime.destroyed == 1 &&
                      runtime.calls.back() == "destroy record texts",
                  "parse failure or invalid unsigned category dumps and cleans");
        }
    }
    auto thrown = Base();
    thrown.throw_on_copy = true;
    try {
        (void)pal4::GameDataParameterTable::AddOrReplace(
            0x1000, 0x9000, thrown);
        Check(false, "copy error should propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  thrown.destroyed == 1,
              "temporary record cleans up on replacement exception");
    }
}

void CheckAppendAndRefresh() {
    auto runtime = Base();
    pal4::GameDataParameterTable::Append(0x1000, 0x9000, runtime);
    Check(runtime.destroyed == 1 &&
              At(runtime, "convert 24576") <
                  At(runtime, "convert 24580") &&
              At(runtime, "convert 24580") <
                  At(runtime, "append record") &&
              runtime.calls.back() == "destroy record texts",
          "append converts bound properties in order then cleans temporary");

    auto mismatch = Base();
    mismatch.row_size = 1;
    try {
        pal4::GameDataParameterTable::Append(0x1000, 0x9000, mismatch);
        Check(false, "row size mismatch should throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()).find("row detail") !=
                  std::string::npos &&
                  mismatch.destroyed == 1 &&
                  std::find(mismatch.calls.begin(), mismatch.calls.end(),
                            "append record") == mismatch.calls.end(),
              "row size mismatch dumps row and releases temporary");
    }
    auto invalid = Base();
    invalid.category = 3;
    try {
        pal4::GameDataParameterTable::Append(0x1000, 0x9000, invalid);
        Check(false, "invalid category should throw");
    } catch (const std::runtime_error&) {
        Check(invalid.destroyed == 1 &&
                  At(invalid, "convert 24580") < At(invalid, "dump row"),
              "append validates category after field conversion");
    }
    auto refresh = Base();
    Check(pal4::GameDataParameterTable::MarkAndRefresh(
              0x1000, refresh) == 0x7777 &&
              refresh.calls == std::vector<std::string>({"mark", "refresh"}),
          "dirty byte precedes refresh and result is forwarded");
}

void CheckRefreshBinding() {
    auto equal = Base();
    equal.words[0x1000 + 128] = 0x5000;
    equal.words[0x1000 + 132] = 0x5008;
    pal4::GameDataParameterTable::RefreshBinding(0x1000, equal);
    Check(std::find(equal.calls.begin(), equal.calls.end(),
                    "assign effects") == equal.calls.end(),
          "equal bound-pointer and field counts skip refresh");

    auto unequal = Base();
    unequal.words[0x1000 + 128] = 0x5000;
    unequal.words[0x1000 + 132] = 0x5004;
    pal4::GameDataParameterTable::RefreshBinding(0x1000, unequal);
    Check(At(unequal, "beginBindField, table:") <
              At(unequal, "assign effects") &&
              At(unequal, "assign effects") <
                  At(unequal, "initialize binding") &&
              At(unequal, "initialize binding") <
                  At(unequal, "bind effects") &&
              At(unequal, "bind effects") <
                  At(unequal, "endBindField, table:"),
          "unequal counts rebind and log begin/end in order");

    auto both_empty = Base();
    both_empty.words[0x1000 + 24] = 0;
    both_empty.words[0x1000 + 128] = 0;
    pal4::GameDataParameterTable::RefreshBinding(0x1000, both_empty);
    Check(std::find(both_empty.calls.begin(), both_empty.calls.end(),
                    "assign effects") == both_empty.calls.end(),
          "both empty field and pointer arrays need no binding");
}

}  // namespace

int main() {
    try {
        CheckReplaceAndInsert();
        CheckErrors();
        CheckAppendAndRefresh();
        CheckRefreshBinding();
        std::cout << "game_data_parameter_row_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
