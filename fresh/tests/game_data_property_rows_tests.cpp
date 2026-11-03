#include "pal4/game_data_property_table.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataPropertyRowRuntime,
                       pal4::IGameDataPropertyAppendRuntime,
                       pal4::IGameDataPropertyRefreshRuntime,
                       pal4::IGameDataPropertyBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::GameDataPropertyRecord parsed{};
    std::uint32_t count = 3;
    std::uint32_t row_size = 3;
    std::uint32_t position = 0x3000;
    bool parse_ok = true;
    bool copied = false;
    bool inserted = false;
    bool appended = false;
    std::uint32_t destination = 0;
    std::uint32_t inserted_index = 0;
    std::uint32_t refresh_result = 0xabcdef;

    Runtime() {
        words[0x1018] = 0x2000;
        words[0x101c] = 0x20b4;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3010;
        words[0x1060] = 0x4000;
        words[0x1064] = 0x4150;
        words[0x1080] = 0x6000;
        words[0x1084] = 0x600c;
        words[0x3000] = 12;
        words[0x3004] = 1;
        words[0x6000] = 0x7000;
        words[0x6004] = 0x7010;
        words[0x6008] = 0x7020;
        parsed[0] = 12;
        parsed[1] = 4;
        parsed[2] = 6;
        parsed[3] = 5;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x107c, "bound pointer collection offset");
        return count;
    }
    void LogBind(std::string_view prefix, std::uint32_t name) override {
        Check(name == 0x1034, "binding uses table name");
        calls.emplace_back(prefix);
    }
    void AssignEffects(std::uint32_t dst, std::uint32_t src) override {
        Check(dst == 0x106c && src == 0x1014, "binding addresses");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t binding) override {
        Check(binding == 0x106c, "structure binding address");
        calls.emplace_back("init structures");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x106c, "effect binding address");
        calls.emplace_back("bind effects");
    }
    void InitializeRecord(pal4::GameDataPropertyRecord& record) override {
        record = parsed;
        calls.emplace_back("init record");
    }
    void DestroyTemporaryRecordTexts(pal4::GameDataPropertyRecord&) override {
        calls.emplace_back("destroy seven texts");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::GameDataPropertyRecord& record) override {
        Check(binding == 0x106c && row == 0x9000, "parse arguments");
        record = parsed;
        calls.emplace_back("parse");
        return parse_ok;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == 0x3000 && end == 0x3010 && key == parsed[0],
              "signed key search arguments");
        calls.emplace_back("find");
        return position;
    }
    void CopyRecord(std::uint32_t dst,
                    const pal4::GameDataPropertyRecord& record) override {
        Check(record == parsed, "copy full Property record");
        destination = dst;
        copied = true;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataPropertyRecord& record) override {
        Check(array == 0x105c && end == words[0x1064] && record == parsed,
              "insert full record at array end");
        inserted = true;
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && at == position && key == parsed[0],
              "insert eight-byte key/index entry");
        inserted_index = index;
        calls.emplace_back("insert key");
    }
    std::string DumpRow(std::uint32_t table, std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x9000, "error row identity");
        return " row=9000";
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x9000, "append row identity");
        return row_size;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x9000, "property row identity");
        return 0x8000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataPropertyRecord& record,
                      std::uint32_t property) override {
        Check(field == 0x7000 + (property - 0x8000) * 16,
              "field pointer order");
        record[0] = 12;
        calls.push_back("convert " + std::to_string(property - 0x8000));
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataPropertyRecord& record) override {
        Check(array == 0x105c && end == 0x4150 && record == parsed,
              "append at current record end");
        appended = true;
        calls.emplace_back("append");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1058 && value == 1, "dirty byte");
        calls.emplace_back("dirty");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "refresh table");
        calls.emplace_back("refresh");
        return refresh_result;
    }
};

void CheckRowPaths() {
    Runtime r;
    Check(pal4::GameDataPropertyTable::AddOrReplace(0x1000, 0x9000, r) == 1,
          "existing row succeeds");
    Check(r.copied && r.destination == 0x4000 + 168 && !r.inserted,
          "existing key replaces indexed record");
    Check(r.calls.front() == "read 4120" &&
              r.calls[r.calls.size() - 1] == "destroy seven texts",
          "normal path releases temporary texts");

    Runtime rebinding;
    rebinding.count = 2;
    rebinding.position = 0x3010;
    rebinding.words[0x1060] = 0;
    Check(pal4::GameDataPropertyTable::AddOrReplace(
              0x1000, 0x9000, rebinding) == 1 &&
              rebinding.inserted && rebinding.inserted_index == 0,
          "unbound fields rebind and empty array inserts index zero");
    const auto bind = std::find(rebinding.calls.begin(), rebinding.calls.end(),
                                "beginBindField, table:");
    Check(bind != rebinding.calls.end() &&
              *(bind + 1) == "assign" && *(bind + 2) == "init structures" &&
              *(bind + 3) == "bind effects" &&
              *(bind + 4) == "endBindField, table:",
          "rebind call order");

    Runtime missing;
    missing.words[0x3000] = 13;
    Check(pal4::GameDataPropertyTable::AddOrReplace(0x1000, 0x9000, missing) == 1 &&
              missing.inserted && missing.inserted_index == 2,
          "missing key appends after two 168-byte records");
}

void CheckFailures() {
    for (int mode = 0; mode < 4; ++mode) {
        Runtime r;
        if (mode == 0) r.parse_ok = false;
        if (mode == 1) r.parsed[1] = 5;
        if (mode == 2) r.parsed[2] = 7;
        if (mode == 3) r.parsed[3] = 6;
        try {
            (void)pal4::GameDataPropertyTable::AddOrReplace(0x1000, 0x9000, r);
            Check(false, "invalid row must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow > row=9000" &&
                      r.calls.back() == "destroy seven texts" &&
                      !r.copied && !r.inserted,
                  "error includes dump and releases temporary texts");
        }
    }
}

void CheckAppendAndRefresh() {
    Runtime r;
    pal4::GameDataPropertyTable::Append(0x1000, 0x9000, r);
    Check(r.appended && r.calls.back() == "destroy seven texts",
          "append converts and releases temporary record");
    const auto first = std::find(r.calls.begin(), r.calls.end(), "convert 0");
    const auto second = std::find(r.calls.begin(), r.calls.end(), "convert 1");
    const auto third = std::find(r.calls.begin(), r.calls.end(), "convert 2");
    const auto appended = std::find(r.calls.begin(), r.calls.end(), "append");
    Check(first < second && second < third && third < appended &&
              appended < r.calls.end(),
          "append converts bound fields in order");
    r.calls.clear();
    Check(pal4::GameDataPropertyTable::MarkAndRefresh(0x1000, r) ==
              0xabcdef && r.calls == std::vector<std::string>{"dirty", "refresh"},
          "mark precedes refresh and returns its result");

    for (int mode = 0; mode < 2; ++mode) {
        Runtime bad;
        if (mode == 0) bad.row_size = 2;
        else bad.parsed[3] = 6;
        try {
            pal4::GameDataPropertyTable::Append(0x1000, 0x9000, bad);
            Check(false, "invalid append must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()).find("row=9000") != std::string::npos &&
                      bad.calls.back() == "destroy seven texts" && !bad.appended,
                  "append failure dumps row and cleans seven texts");
        }
    }
}

pal4::GameDataPropertyRecord Record(std::int32_t key, std::uint32_t marker) {
    pal4::GameDataPropertyRecord record{};
    record[0] = static_cast<std::uint32_t>(key);
    record[41] = marker;
    return record;
}

struct IndexRuntime final : pal4::IGameDataPropertyIndexRuntime {
    std::vector<std::uint32_t> assigned;
    std::vector<std::uint32_t> destroyed;
    void AssignRecord(pal4::GameDataPropertyRecord& destination,
                      const pal4::GameDataPropertyRecord& source) override {
        assigned.push_back(source[41]);
        destination = source;
    }
    void DestroyRecord(pal4::GameDataPropertyRecord& record) override {
        destroyed.push_back(record[41]);
    }
};

void CheckIndex() {
    std::vector<pal4::GameDataPropertyRecord> records{
        Record(4, 1), Record(4, 2), Record(-2, 3),
        Record(4, 4), Record(7, 5), Record(7, 6)};
    std::vector<pal4::GameDataPropertyKeyIndex> index{{99, 99}};
    IndexRuntime runtime;
    pal4::GameDataPropertyIndex::Rebuild(records, index, runtime);
    Check(records.size() == 4 && records[0][41] == 1 &&
              records[1][41] == 3 && records[2][41] == 4 &&
              records[3][41] == 5,
          "only adjacent duplicate runs compact, preserving first record");
    Check(index == std::vector<pal4::GameDataPropertyKeyIndex>{
                       {-2, 1}, {4, 0}, {4, 2}, {7, 3}},
          "index maps compacted records and sorts signed keys");
    Check(runtime.assigned == std::vector<std::uint32_t>{3, 4, 5} &&
              runtime.destroyed == std::vector<std::uint32_t>{5, 6},
          "text-aware record assignment precedes tail destruction");
    records.clear();
    pal4::GameDataPropertyIndex::Rebuild(records, index, runtime);
    Check(index.empty(), "empty records clear stale index");
}

void CheckBindingRefresh() {
    Runtime equal;
    pal4::GameDataPropertyTable::RefreshBinding(0x1000, equal);
    Check(std::none_of(equal.calls.begin(), equal.calls.end(),
                       [](const std::string& call) {
                           return call == "assign";
                       }),
          "matching three bound pointers and fields skip rebinding");

    Runtime mismatch;
    mismatch.words[0x1084] = 0x6008;
    pal4::GameDataPropertyTable::RefreshBinding(0x1000, mismatch);
    const auto start = std::find(mismatch.calls.begin(), mismatch.calls.end(),
                                 "beginBindField, table:");
    Check(start != mismatch.calls.end() &&
              *(start + 1) == "assign" &&
              *(start + 2) == "init structures" &&
              *(start + 3) == "bind effects" &&
              *(start + 4) == "endBindField, table:",
          "mismatched bound pointers trigger ordered rebind");

    Runtime no_pointers;
    no_pointers.words[0x1080] = 0;
    pal4::GameDataPropertyTable::RefreshBinding(0x1000, no_pointers);
    Check(std::find(no_pointers.calls.begin(), no_pointers.calls.end(),
                    "read 4228") == no_pointers.calls.end() &&
              std::find(no_pointers.calls.begin(), no_pointers.calls.end(),
                        "assign") != no_pointers.calls.end(),
          "null pointer collection counts as zero without reading its end");

    Runtime empty;
    empty.words[0x1080] = 0;
    empty.words[0x1018] = 0;
    pal4::GameDataPropertyTable::RefreshBinding(0x1000, empty);
    Check(std::find(empty.calls.begin(), empty.calls.end(),
                    "assign") == empty.calls.end(),
          "two empty collections do not rebind");
}

}  // namespace

int main() {
    try {
        CheckRowPaths();
        CheckFailures();
        CheckAppendAndRefresh();
        CheckIndex();
        CheckBindingRefresh();
        std::cout << "game_data_property_rows_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
