#include "pal4/game_data_shop_goods_table.h"

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

struct Runtime final : pal4::IGameDataShopGoodsRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::string> fields;
    std::uint32_t collection_size = 0;
    std::uint32_t position = 0;
    std::uint32_t key = 0x1234;
    std::uint32_t state = 2;
    std::uint32_t inserted_index = 0xffffffffu;
    std::uint32_t assigned_destination = 0;
    std::uint32_t inserted_array = 0;
    std::uint32_t inserted_end = 0;
    std::uint32_t inserted_key_array = 0;
    std::uint32_t found_begin = 0;
    std::uint32_t found_end = 0;
    bool parse_result = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        events.push_back("size:" + std::to_string(address));
        return collection_size;
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        events.push_back("log:" + std::string(prefix) +
                         std::to_string(name));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        events.push_back("assign-effects:" +
                         std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void RegisterField(std::uint32_t binding, std::string_view name,
                       std::uint32_t offset,
                       pal4::GameDataFieldKind kind) override {
        Check(binding == 0x9000 + 108, "schema uses Good binding");
        fields.push_back(std::string(name) + ":" +
                         std::to_string(offset) + ":" +
                         std::to_string(static_cast<unsigned>(kind)));
        events.push_back("field:" + std::string(name));
    }
    void BindEffects(std::uint32_t binding) override {
        events.push_back("bind:" + std::to_string(binding));
    }
    void InitializeRecord(pal4::GameDataShopGoodsRecord&) override {
        events.push_back("initialize");
    }
    void DestroyTemporaryRecord(
        pal4::GameDataShopGoodsRecord&) override {
        events.push_back("destroy-temporary");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::GameDataShopGoodsRecord& record) override {
        events.push_back("parse:" + std::to_string(binding) + ":" +
                         std::to_string(row));
        record[0] = key;
        record[1] = state;
        record[8] = 0xaabbccdd;
        return parse_result;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t requested) override {
        events.push_back("find");
        found_begin = begin;
        found_end = end;
        Check(requested == key, "lookup receives parsed key");
        return position;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataShopGoodsRecord& record) override {
        events.push_back("insert-record");
        inserted_array = array;
        inserted_end = end;
        Check(record[0] == key && record[8] == 0xaabbccdd,
              "insert receives full Good record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t requested,
                   std::uint32_t index) override {
        events.push_back("insert-key");
        inserted_key_array = array;
        inserted_index = index;
        Check(at == position && requested == key,
              "key insert receives lower-bound position");
    }
    void AssignRecord(std::uint32_t destination,
                      const pal4::GameDataShopGoodsRecord& record) override {
        events.push_back("assign-record");
        assigned_destination = destination;
        Check(record[0] == key && record[8] == 0xaabbccdd,
              "replacement receives full Good record");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        events.push_back("dump");
        Check(table == 0x9000 && row == 0x44,
              "failure dumps the original row");
        return "row detail";
    }
};

Runtime MakeRuntime() {
    Runtime runtime;
    constexpr std::uint32_t table = 0x9000;
    runtime.words[table + 24] = 0x5000;
    runtime.words[table + 28] = 0x5000 + 2 * 60;
    runtime.words[table + 8] = 0x6000;
    runtime.words[table + 12] = 0x6010;
    runtime.words[table + 96] = 0x7000;
    runtime.words[table + 100] = 0x7000 + 3 * 36;
    runtime.position = 0x6010;
    return runtime;
}

void CheckInsertAndBind() {
    Runtime runtime = MakeRuntime();
    runtime.collection_size = 1;
    Check(pal4::GameDataShopGoodsTable::AddOrReplace(
              0x9000, 0x44, runtime) == 1 &&
              runtime.fields == std::vector<std::string>{
                  "ID:0:1", "SHOP_ID:20:2", "GOODS_ID:8:1",
                  "GOODS_TYPE:4:1", "OPEN_CONDITION:12:1",
                  "OPEN_NUM:16:1"} &&
              runtime.inserted_array == 0x9000 + 92 &&
              runtime.inserted_end == 0x7000 + 3 * 36 &&
              runtime.inserted_key_array == 0x9000 + 4 &&
              runtime.inserted_index == 3 &&
              runtime.found_begin == 0x6000 &&
              runtime.found_end == 0x6010,
          "Good insert binds six fields and uses 36-byte record index");
    Check(runtime.events == std::vector<std::string>{
              "size:36988", "log:beginBindField, table:36916",
              "assign-effects:36972:36884",
              "field:ID", "field:SHOP_ID", "field:GOODS_ID",
              "field:GOODS_TYPE", "field:OPEN_CONDITION",
              "field:OPEN_NUM", "bind:36972",
              "log:endBindField, table:36916", "initialize",
              "parse:36972:68", "find", "insert-record",
              "insert-key", "destroy-temporary"},
          "Good bind, parse, insertion and temporary cleanup order");
}

void CheckReplaceAndSignedInsert() {
    Runtime runtime = MakeRuntime();
    runtime.collection_size = 2;
    runtime.position = 0x6000;
    runtime.words[0x6000] = runtime.key;
    runtime.words[0x6004] = 2;
    Check(pal4::GameDataShopGoodsTable::AddOrReplace(
              0x9000, 0x44, runtime) == 1 &&
              runtime.fields.empty() &&
              runtime.assigned_destination == 0x7000 + 2 * 36 &&
              runtime.inserted_index == 0xffffffffu &&
              runtime.events.back() == "destroy-temporary",
          "matching key replaces indexed record without rebinding");

    runtime = MakeRuntime();
    runtime.collection_size = 2;
    runtime.key = 0xffffffffu;
    runtime.position = 0x6000;
    runtime.words[0x6000] = 1;
    Check(pal4::GameDataShopGoodsTable::AddOrReplace(
              0x9000, 0x44, runtime) == 1 &&
              runtime.inserted_index == 3 &&
              runtime.assigned_destination == 0,
          "signed negative key inserts before positive lower-bound key");

    runtime = MakeRuntime();
    runtime.collection_size = 2;
    runtime.words[0x9000 + 96] = 0;
    runtime.words[0x9000 + 100] = 0;
    Check(pal4::GameDataShopGoodsTable::AddOrReplace(
              0x9000, 0x44, runtime) == 1 &&
              runtime.inserted_index == 0,
          "empty record array inserts at index zero");
}

void CheckFailures() {
    for (const auto fail_parse : {true, false}) {
        Runtime runtime = MakeRuntime();
        runtime.collection_size = 2;
        runtime.parse_result = !fail_parse;
        runtime.state = fail_parse ? 2 : 3;
        try {
            static_cast<void>(pal4::GameDataShopGoodsTable::AddOrReplace(
                0x9000, 0x44, runtime));
            Check(false, "invalid Good row must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow >row detail" &&
                      runtime.events.back() == "destroy-temporary" &&
                      runtime.events[runtime.events.size() - 2] == "dump",
                  "parse and state failures dump row and clean temporary");
        }
    }
}

}  // namespace

int main() {
    try {
        CheckInsertAndBind();
        CheckReplaceAndSignedInsert();
        CheckFailures();
        std::cout << "game_data_shop_goods_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
