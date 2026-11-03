#include "pal4/game_data_shop_goods_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataShopGoodsAppendRuntime,
                       pal4::IGameDataShopGoodsRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t count = 2;
    std::uint32_t row_size = 2;
    std::uint32_t cleanup_result = 0x9876;
    std::uint32_t appended_array = 0;
    std::uint32_t appended_end = 0;
    std::uint32_t refreshed_table = 0;
    std::uint32_t flag_address = 0;
    std::uint8_t flag_value = 0;
    bool fail_conversion = false;
    bool invalid_state = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        events.push_back("size:" + std::to_string(address));
        return count;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        events.push_back("row-size:" + std::to_string(row));
        return row_size;
    }
    void InitializeRecord(pal4::GameDataShopGoodsRecord&) override {
        events.push_back("initialize");
    }
    std::uint32_t CleanupTemporaryRecord(
        pal4::GameDataShopGoodsRecord&) override {
        events.push_back("cleanup");
        return cleanup_result;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x44, "conversion uses input row");
        events.push_back("property:" + std::to_string(index));
        return 0x8000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataShopGoodsRecord& record,
                      std::uint32_t property) override {
        events.push_back("convert:" + std::to_string(field));
        if (fail_conversion) throw std::runtime_error("conversion failed");
        Check(property == 0x8000 + (field - 0xa000) / 4,
              "field and row property indices match");
        record[(field - 0xa000) / 4] = property;
        record[1] = invalid_state ? 3u : 2u;
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataShopGoodsRecord& record) override {
        events.push_back("append");
        appended_array = array;
        appended_end = end;
        Check(count == 0 ? (record[0] == 0 && record[1] == 0)
                         : (record[0] == 0x8000 && record[1] == 2),
              "append receives converted Good record");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x9000 && row == 0x44,
              "error dump receives source table and row");
        events.push_back("dump");
        return "row detail";
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.push_back("mark");
        flag_address = address;
        flag_value = value;
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        events.push_back("refresh");
        refreshed_table = table;
        return 0x12345678;
    }
};

void CheckAppend() {
    Runtime runtime;
    runtime.words[0x9000 + 128] = 0xa100;
    runtime.words[0xa100] = 0xa000;
    runtime.words[0xa104] = 0xa004;
    runtime.words[0x9000 + 100] = 0x7000;
    Check(pal4::GameDataShopGoodsTable::Append(
              0x9000, 0x44, runtime) == 0x9876 &&
              runtime.appended_array == 0x9000 + 92 &&
              runtime.appended_end == 0x7000 &&
              runtime.events == std::vector<std::string>{
                  "initialize", "size:36988", "row-size:68",
                  "size:36988", "read:36992", "read:41216",
                  "property:0", "convert:40960", "size:36988",
                  "read:36992", "read:41220", "property:1",
                  "convert:40964", "size:36988", "read:36964",
                  "append", "cleanup"},
          "Good append converts fields in index order and forwards cleanup result");
}

void CheckFailuresAndEmpty() {
    Runtime mismatch;
    mismatch.row_size = 1;
    try {
        static_cast<void>(pal4::GameDataShopGoodsTable::Append(
            0x9000, 0x44, mismatch));
        Check(false, "row size mismatch must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow >row detail" &&
                  mismatch.events.back() == "cleanup",
              "size mismatch dumps and cleans temporary");
    }

    Runtime empty;
    empty.count = 0;
    empty.row_size = 0;
    Check(pal4::GameDataShopGoodsTable::Append(
              0x9000, 0x44, empty) == empty.cleanup_result &&
              empty.events[empty.events.size() - 2] == "append",
          "zero-field Good row can append with initial state zero");

    Runtime invalid;
    invalid.count = 1;
    invalid.row_size = 1;
    invalid.words[0x9000 + 128] = 0xa100;
    invalid.words[0xa100] = 0xa000;
    invalid.invalid_state = true;
    try {
        static_cast<void>(pal4::GameDataShopGoodsTable::Append(
            0x9000, 0x44, invalid));
        Check(false, "invalid Good state must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow >row detail" &&
                  invalid.events.back() == "cleanup",
              "invalid state dumps and cleans temporary");
    }

    Runtime interrupted;
    interrupted.words[0x9000 + 128] = 0xa100;
    interrupted.words[0xa100] = 0xa000;
    interrupted.fail_conversion = true;
    try {
        static_cast<void>(pal4::GameDataShopGoodsTable::Append(
            0x9000, 0x44, interrupted));
        Check(false, "conversion failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "conversion failed" &&
                  interrupted.events.back() == "cleanup",
              "conversion failure cleans temporary record");
    }
}

void CheckRefresh() {
    Runtime runtime;
    Check(pal4::GameDataShopGoodsTable::MarkAndRefresh(
              0x9000, runtime) == 0x12345678 &&
              runtime.events == std::vector<std::string>{
                  "mark", "refresh"} &&
              runtime.flag_address == 0x9000 + 88 &&
              runtime.flag_value == 1 &&
              runtime.refreshed_table == 0x9000,
          "Good refresh sets dirty byte before forwarding result");
}

}  // namespace

int main() {
    try {
        CheckAppend();
        CheckFailuresAndEmpty();
        CheckRefresh();
        std::cout << "game_data_shop_goods_table_append_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
