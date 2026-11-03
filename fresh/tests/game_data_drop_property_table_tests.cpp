#include "pal4/game_data_drop_property_table.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataDropPropertyTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t collection_size = 2;
    std::uint32_t row_size = 2;
    std::uint32_t found = 0x5000;
    std::uint32_t append_result = 0x7777;

    Runtime() {
        words[0x1018] = 0x2000;
        words[0x101c] = 0x2078;
        words[0x1080] = 0x3000;
        words[0x3000] = 0x4000;
        words[0x3004] = 0x4100;
        words[0x1008] = 0x5000;
        words[0x100c] = 0x5010;
        words[0x5000] = 0xfffffffe;
        words[0x5004] = 1;
        words[0x1060] = 0x6000;
        words[0x1064] = 0x6008;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x107c, "bound collection address");
        return collection_size;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x9000, "row address");
        calls.emplace_back("row size");
        return row_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t name) override {
        Check(name == 0x1034, "table name address");
        calls.emplace_back(prefix);
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "effect assignment addresses");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t object) override {
        Check(object == 0x106c, "effect initializer address");
        calls.emplace_back("initialize");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "effect binding address");
        calls.emplace_back("bind");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x9000 && index < 2, "row property index");
        calls.push_back("property " + std::to_string(index));
        return 0xa000 + index;
    }
    void ConvertField(std::uint32_t field, std::uint32_t& value,
                      std::uint32_t property) override {
        Check((field == 0x4000 && property == 0xa000) ||
                  (field == 0x4100 && property == 0xa001),
              "bound field/property pair");
        calls.push_back("convert " + std::to_string(field));
        value = field == 0x4000 ? 17 : 0xfffffffe;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == 0x5000 && end == 0x5010 && key == 0xfffffffe,
              "signed scalar key and search span");
        calls.emplace_back("find");
        return found;
    }
    void InsertValue(std::uint32_t array, std::uint32_t position,
                     std::uint32_t value) override {
        Check(array == 0x105c && position == 0x6008 &&
                  value == 0xfffffffe,
              "scalar value insertion arguments");
        calls.emplace_back("insert value");
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && position == found &&
                  key == 0xfffffffe && index == 2,
              "key insertion uses previous scalar count");
        calls.emplace_back("insert key");
    }
    std::uint32_t AppendValue(std::uint32_t array, std::uint32_t end,
                              std::uint32_t value) override {
        Check(array == 0x105c && end == 0x6008 && value == 0xfffffffe,
              "scalar append arguments");
        calls.emplace_back("append");
        return append_result;
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x9000, "error row context");
        calls.emplace_back("dump");
        return ": row 17";
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1058 && value == 1, "dirty flag");
        calls.emplace_back("flag");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "refresh table");
        calls.emplace_back("refresh");
        return 0x1234;
    }
};

std::size_t At(const Runtime& runtime, const std::string& value) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 value);
    Check(found != runtime.calls.end(), "expected runtime call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckAddOrReplace() {
    Runtime replace;
    Check(pal4::GameDataDropPropertyTable::AddOrReplace(
              0x1000, 0x9000, replace) == 1 &&
              replace.words[0x6004] == 0xfffffffe &&
              At(replace, "convert 16384") < At(replace, "convert 16640") &&
              At(replace, "convert 16640") < At(replace, "find") &&
              replace.calls.back() == "write 24580",
          "matching key overwrites indexed four-byte value");

    Runtime insert;
    insert.found = 0x5010;
    Check(pal4::GameDataDropPropertyTable::AddOrReplace(
              0x1000, 0x9000, insert) == 1 &&
              At(insert, "insert value") < At(insert, "insert key"),
          "missing key appends scalar before sorted map entry");

    Runtime rebind;
    rebind.words[0x101c] = 0x203c;
    Check(pal4::GameDataDropPropertyTable::AddOrReplace(
              0x1000, 0x9000, rebind) == 1 &&
              rebind.calls[0] == "beginBindField, table:" &&
              rebind.calls[1] == "assign" &&
              rebind.calls[2] == "initialize" &&
              rebind.calls[3] == "bind" &&
              rebind.calls[4] == "endBindField, table:" &&
              rebind.calls[5] == "row size",
          "changed 60-byte field count triggers ordered rebind");
}

void CheckAppendAndMismatch() {
    Runtime append;
    Check(pal4::GameDataDropPropertyTable::Append(
              0x1000, 0x9000, append) == 0x7777 &&
              append.calls.back() == "append",
          "append forwards scalar insertion result");

    for (bool replace : {false, true}) {
        Runtime mismatch;
        mismatch.row_size = 1;
        try {
            if (replace)
                (void)pal4::GameDataDropPropertyTable::AddOrReplace(
                    0x1000, 0x9000, mismatch);
            else
                (void)pal4::GameDataDropPropertyTable::Append(
                    0x1000, 0x9000, mismatch);
            Check(false, "row mismatch must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow >: row 17" &&
                      mismatch.calls.back() == "dump" &&
                      std::find(mismatch.calls.begin(), mismatch.calls.end(),
                                "convert 16384") == mismatch.calls.end(),
                  "row mismatch dumps before conversion");
        }
    }
}

void CheckRefresh() {
    Runtime runtime;
    Check(pal4::GameDataDropPropertyTable::MarkAndRefresh(
              0x1000, runtime) == 0x1234 &&
              runtime.calls == std::vector<std::string>{"flag", "refresh"},
          "dirty flag precedes refresh and return is forwarded");
}

}  // namespace

int main() {
    try {
        CheckAddOrReplace();
        CheckAppendAndMismatch();
        CheckRefresh();
        std::cout << "game_data_drop_property_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
