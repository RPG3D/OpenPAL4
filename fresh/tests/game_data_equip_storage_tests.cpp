#include "pal4/game_data_equip_storage.h"

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

struct Runtime final : pal4::IGameDataEquipStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1060, 0x4000}, {0x1064, 0x47d0}, {0x1068, 0x4960},
        {0x1008, 0x5000}, {0x100c, 0x5028}, {0x1010, 0x5030}};
    std::map<std::uint32_t, pal4::GameDataEquipRecord> records;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x6000, 0x7000};

    Runtime() {
        const std::uint32_t ids[5] = {9, 9, 0xffffffff, 3, 9};
        for (std::uint32_t i = 0; i < 5; ++i) {
            pal4::GameDataEquipRecord record{};
            record[0] = 100 + i;
            record[36] = ids[i];
            record[99] = 200 + i;
            records[0x4000 + i * 400] = record;
            keys[0x5000 + i * 8] = {ids[i], i};
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    pal4::GameDataEquipRecord ReadRecord(
        std::uint32_t address) override {
        return records.at(address);
    }
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override {
        records[destination] = records.at(source);
        calls.push_back("assign " + std::to_string(destination) + " " +
                        std::to_string(source));
    }
    void CopyConstructRecord(std::uint32_t destination,
                             std::uint32_t source) override {
        records[destination] = records.at(source);
        calls.push_back("construct " + std::to_string(destination) + " " +
                        std::to_string(source));
    }
    void DestroyTextRecord(std::uint32_t text) override {
        calls.push_back("text " + std::to_string(text));
    }
    void AppendKey(std::uint32_t array,
                   const pal4::GameDataAiKeyEntry& entry) override {
        Check(array == 0x1004, "key array owner");
        const auto end = words[0x100c];
        keys[end] = entry;
        words[0x100c] = end + 8;
        calls.emplace_back("append key");
    }
    void WriteKey(std::uint32_t address,
                  const pal4::GameDataAiKeyEntry& entry) override {
        keys[address] = entry;
        calls.push_back("key " + std::to_string(address));
    }
    pal4::GameDataAiKeyEntry ReadKey(std::uint32_t address) override {
        return keys.at(address);
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        Check(!allocations.empty(), "unexpected allocation");
        const auto result = allocations.front();
        allocations.erase(allocations.begin());
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

std::string Text(std::uint32_t record, std::uint32_t index) {
    return "text " + std::to_string(record + 152 + index * 16);
}

void CheckLifecycle() {
    Runtime runtime;
    pal4::GameDataEquipRecordLifecycle::Destroy(0x4000, runtime);
    Check(runtime.calls.size() == 13 &&
              runtime.calls.front() == Text(0x4000, 12) &&
              runtime.calls.back() == Text(0x4000, 0),
          "record destructor visits all 13 text fields in reverse order");

    runtime.calls.clear();
    pal4::GameDataEquipRecordLifecycle::DestroyRange(
        0x4000, 0x4000 + 800, runtime);
    Check(runtime.calls.size() == 26 &&
              runtime.calls.front() == Text(0x4000, 12) &&
              runtime.calls[12] == Text(0x4000, 0) &&
              runtime.calls[13] == Text(0x4000 + 400, 12) &&
              runtime.calls.back() == Text(0x4000 + 400, 0),
          "range destructor advances 400 bytes after each complete record");
}

void CheckSort() {
    Runtime runtime;
    pal4::GameDataEquipStorage::Sort(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x4640 &&
              runtime.records[0x4000][0] == 100 &&
              runtime.records[0x4190][0] == 102 &&
              runtime.records[0x4320][0] == 103 &&
              runtime.records[0x44b0][0] == 104 &&
              runtime.records[0x44b0][99] == 204,
          "adjacent duplicate is removed with complete record assignment");
    Check(At(runtime, "assign 16784 17184") <
              At(runtime, Text(0x4000 + 1600, 12)) &&
              At(runtime, Text(0x4000 + 1600, 0)) <
                  At(runtime, "append key"),
          "trailing record text is destroyed after compaction and before keys");
    Check(runtime.words[0x100c] == 0x5020 &&
              runtime.keys[0x5000].key == 0xffffffff &&
              runtime.keys[0x5000].record_index == 1 &&
              runtime.keys[0x5008].key == 3 &&
              runtime.keys[0x5008].record_index == 2 &&
              runtime.keys[0x5010].key == 9 &&
              runtime.keys[0x5010].record_index == 0 &&
              runtime.keys[0x5018].key == 9 &&
              runtime.keys[0x5018].record_index == 3,
          "signed key sort keeps indices of physical compacted records");

    Runtime empty;
    empty.words[0x1064] = empty.words[0x1060];
    pal4::GameDataEquipStorage::Sort(0x1000, empty);
    Check(empty.words[0x1064] == 0x4000 &&
              empty.words[0x100c] == 0x5000 &&
              std::find(empty.calls.begin(), empty.calls.end(),
                        "append key") == empty.calls.end(),
          "empty record array resets the key end without appending");
}

void CheckReserve() {
    Runtime both;
    Check(pal4::GameDataEquipStorage::Reserve(0x1000, 8, both) ==
              0x7028 &&
              both.words[0x1060] == 0x6000 &&
              both.words[0x1064] == 0x67d0 &&
              both.words[0x1068] == 0x6c80 &&
              both.words[0x1008] == 0x7000 &&
              both.words[0x100c] == 0x7028 &&
              both.words[0x1010] == 0x7040 &&
              both.records[0x6640][0] == 104 &&
              both.keys[0x7020].record_index == 4,
          "record and key arrays grow with used elements intact");
    Check(both.calls.front() == "allocate 3200" &&
              At(both, "construct 24576 16384") <
                  At(both, Text(0x4000, 12)) &&
              At(both, Text(0x4000 + 1600, 0)) <
                  At(both, "free 16384") &&
              At(both, "free 16384") < At(both, "allocate 64"),
          "all new text is constructed before old text and buffer release");

    Runtime records_only;
    records_only.words[0x1010] = 0x5080;
    Check(pal4::GameDataEquipStorage::Reserve(
              0x1000, 8, records_only) == 16 &&
              records_only.words[0x1060] == 0x6000 &&
              records_only.words[0x1008] == 0x5000,
          "record-only growth returns prior key capacity");

    Runtime keys_only;
    keys_only.words[0x1068] = 0x4c80;
    keys_only.allocations = {0x7000};
    Check(pal4::GameDataEquipStorage::Reserve(
              0x1000, 8, keys_only) == 0x7028 &&
              keys_only.words[0x1060] == 0x4000 &&
              keys_only.calls.front() == "allocate 64",
          "key-only growth leaves record text untouched");

    Runtime enough;
    enough.words[0x1068] = 0x4c80;
    enough.words[0x1010] = 0x5080;
    Check(pal4::GameDataEquipStorage::Reserve(
              0x1000, 8, enough) == 16 && enough.calls.empty(),
          "sufficient capacities perform no writes");

    Runtime empty;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    empty.words[0x1008] = empty.words[0x100c] = empty.words[0x1010] = 0;
    Check(pal4::GameDataEquipStorage::Reserve(
              0x1000, 3, empty) == 0x7000 &&
              empty.words[0x1060] == empty.words[0x1064] &&
              empty.words[0x1068] == 0x6000 + 1200 &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1010] == 0x7018,
          "null arrays allocate requested capacity with zero used elements");

    Runtime failed;
    failed.allocations = {0, 0};
    Check(pal4::GameDataEquipStorage::Reserve(
              0x1000, 8, failed) == 40 &&
              failed.words[0x1060] == 0 &&
              failed.words[0x1064] == 2000 &&
              failed.words[0x1008] == 0 &&
              failed.words[0x100c] == 40,
          "native null allocation still updates array triplets");
}

void CheckInsert() {
    Runtime spare;
    pal4::GameDataEquipRecord inserted{};
    inserted[0] = 777;
    inserted[36] = 42;
    inserted[99] = 888;
    spare.records[0x9000] = inserted;
    Check(pal4::GameDataEquipStorage::Insert(
              0x105c, 0x4320, 0x9000, spare) == 0x4320 &&
              spare.words[0x1060] == 0x4000 &&
              spare.words[0x1064] == 0x4960 &&
              spare.records[0x4320][0] == 777 &&
              spare.records[0x44b0][0] == 102 &&
              spare.records[0x4640][0] == 103 &&
              spare.records[0x47d0][0] == 104 &&
              spare.calls.front() == "construct 18384 17984" &&
              At(spare, "construct 18384 17984") <
                  At(spare, "assign 17184 36864"),
          "spare Equip capacity constructs tail then assigns moved text");

    Runtime full;
    full.records[0x9000] = inserted;
    full.words[0x1068] = full.words[0x1064];
    Check(pal4::GameDataEquipStorage::Insert(
              0x105c, 0x4190, 0x9000, full) == 0x6190 &&
              full.words[0x1060] == 0x6000 &&
              full.words[0x1064] == 0x6960 &&
              full.words[0x1068] == 0x6fa0 &&
              full.records[0x6000][0] == 100 &&
              full.records[0x6190][0] == 777 &&
              full.records[0x6320][0] == 101 &&
              full.records[0x67d0][0] == 104 &&
              full.calls.front() == "allocate 4000" &&
              At(full, "construct 24976 36864") <
                  At(full, Text(0x4000, 12)) &&
              At(full, Text(0x4000 + 1600, 0)) <
                  At(full, "free 16384"),
          "full Equip array doubles and releases old text after new copies");

    Runtime empty;
    empty.records[0x9000] = inserted;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    Check(pal4::GameDataEquipStorage::Insert(
              0x105c, 0, 0x9000, empty) == 0x6000 &&
              empty.words[0x1060] == 0x6000 &&
              empty.words[0x1064] == 0x6190 &&
              empty.words[0x1068] == 0x6190 &&
              empty.records[0x6000][0] == 777 &&
              empty.calls.front() == "allocate 400",
          "empty Equip array grows to one string-owning record");
}

pal4::GameDataEquipRecord SourceRecord() {
    pal4::GameDataEquipRecord record{};
    record[0] = 777;
    record[36] = 42;
    record[99] = 888;
    return record;
}

void CheckCopyHelpers() {
    Runtime size;
    Check(pal4::GameDataEquipStorage::Size(0x105c, size) == 5,
          "Equip size divides live byte range by 400");
    size.words[0x1060] = 0;
    Check(pal4::GameDataEquipStorage::Size(0x105c, size) == 0,
          "null Equip array has zero size");

    Runtime range;
    Check(pal4::GameDataEquipStorage::CopyConstructRange(
              0x4000, 0x4320, 0x6000, range) == 0x6320 &&
              range.records[0x6000][0] == 100 &&
              range.records[0x6190][0] == 101 &&
              range.calls == std::vector<std::string>{
                                 "construct 24576 16384",
                                 "construct 24976 16784"},
          "half-open range constructs two complete records in order");
    range.calls.clear();
    Check(pal4::GameDataEquipStorage::CopyConstructRange(
              0x4000, 0x4000, 0x7000, range) == 0x7000 &&
              range.calls.empty(),
          "empty copy range returns original destination");

    Runtime repeated;
    repeated.records[0x9000] = SourceRecord();
    Check(pal4::GameDataEquipStorage::CopyRepeated(
              0x6000, 3, 0x9000, repeated) == 0x9000 &&
              repeated.records[0x6000][0] == 777 &&
              repeated.records[0x6190][36] == 42 &&
              repeated.records[0x6320][99] == 888 &&
              repeated.calls.size() == 3,
          "repeated helper constructs source at 400-byte intervals");

    Runtime assigned;
    assigned.records[0x9000] = SourceRecord();
    Check(pal4::GameDataEquipStorage::AssignRepeated(
              0x4000, 0x44b0, 0x9000, assigned) == 0x44b0 &&
              assigned.records[0x4000][0] == 777 &&
              assigned.records[0x4190][36] == 42 &&
              assigned.records[0x4320][99] == 888 &&
              assigned.calls == std::vector<std::string>{
                                    "assign 16384 36864",
                                    "assign 16784 36864",
                                    "assign 17184 36864"},
          "repeated assignment covers every existing 400-byte record");
    assigned.calls.clear();
    Check(pal4::GameDataEquipStorage::AssignRepeated(
              0x4000, 0x4000, 0x9000, assigned) == 0x4000 &&
              assigned.calls.empty(),
          "empty assignment range returns its end without writes");

    Runtime reversed;
    Check(pal4::GameDataEquipStorage::CopyConstructReverse(
              0x4000, 0x44b0, 0x74b0, reversed) == 0x7000 &&
              reversed.records[0x7000][0] == 100 &&
              reversed.records[0x7190][0] == 101 &&
              reversed.records[0x7320][0] == 102 &&
              reversed.calls == std::vector<std::string>{
                                    "construct 29472 17184",
                                    "construct 29072 16784",
                                    "construct 28672 16384"},
          "reverse construction preserves final order while visiting backward");
    reversed.calls.clear();
    Check(pal4::GameDataEquipStorage::CopyConstructReverse(
              0x4000, 0x4000, 0x8000, reversed) == 0x8000 &&
              reversed.calls.empty(),
          "empty reverse range returns original destination end");
}

void CheckInsertRepeated() {
    Runtime growth;
    growth.records[0x9000] = SourceRecord();
    Check(pal4::GameDataEquipStorage::InsertRepeated(
              0x105c, 0x4320, 2, 0x9000, growth) == 35 &&
              growth.words[0x1060] == 0x6000 &&
              growth.words[0x1064] == 0x6af0 &&
              growth.words[0x1068] == 0x6fa0 &&
              growth.records[0x6000][0] == 100 &&
              growth.records[0x6190][0] == 101 &&
              growth.records[0x6320][0] == 777 &&
              growth.records[0x64b0][0] == 777 &&
              growth.records[0x6640][0] == 102 &&
              growth.records[0x6960][0] == 104 &&
              growth.calls.front() == "allocate 4000" &&
              At(growth, "construct 25776 36864") <
                  At(growth, Text(0x4000, 12)) &&
              At(growth, Text(0x4000 + 1600, 0)) <
                  At(growth, "free 16384"),
          "growth copies prefix, two values and suffix before old cleanup");

    Runtime suffix_short;
    suffix_short.records[0x9000] = SourceRecord();
    suffix_short.words[0x1068] = 0x4c80;
    Check(pal4::GameDataEquipStorage::InsertRepeated(
              0x105c, 0x4640, 2, 0x9000, suffix_short) == 0x4af0 &&
              suffix_short.words[0x1060] == 0x4000 &&
              suffix_short.words[0x1064] == 0x4af0 &&
              suffix_short.records[0x4640][0] == 777 &&
              suffix_short.records[0x47d0][0] == 777 &&
              suffix_short.records[0x4960][0] == 104 &&
              suffix_short.calls.front() ==
                  "construct 18784 17984",
          "short suffix moves one old record after two inserted values");

    Runtime suffix_long;
    suffix_long.records[0x9000] = SourceRecord();
    suffix_long.words[0x1068] = 0x4c80;
    Check(pal4::GameDataEquipStorage::InsertRepeated(
              0x105c, 0x4190, 2, 0x9000, suffix_long) == 800 &&
              suffix_long.words[0x1064] == 0x4af0 &&
              suffix_long.records[0x4000][0] == 100 &&
              suffix_long.records[0x4190][0] == 777 &&
              suffix_long.records[0x4320][0] == 777 &&
              suffix_long.records[0x44b0][0] == 101 &&
              suffix_long.records[0x4640][0] == 102 &&
              suffix_long.records[0x47d0][0] == 103 &&
              suffix_long.records[0x4960][0] == 104 &&
              suffix_long.calls.front() ==
                  "construct 18384 17584",
          "long suffix constructs tail and shifts middle before assignment");

    Runtime zero;
    zero.records[0x9000] = SourceRecord();
    Check(pal4::GameDataEquipStorage::InsertRepeated(
              0x105c, 0x4190, 0, 0x9000, zero) == 0 &&
              zero.words[0x1064] == 0x47d0 && zero.calls.empty(),
          "zero-count insertion leaves storage untouched");
}

bool SameEquipId(const pal4::GameDataEquipRecord& left,
                 const pal4::GameDataEquipRecord& right) {
    return left[36] == right[36];
}

bool SameEquipPayload(const pal4::GameDataEquipRecord& left,
                      const pal4::GameDataEquipRecord& right) {
    return left[0] == right[0];
}

void CheckCompactAdjacent() {
    Runtime runtime;
    Check(pal4::GameDataEquipStorage::CompactAdjacent(
              0x4000, 0x47d0, 0x4000, SameEquipId, runtime) ==
              0x4640 &&
              runtime.records[0x4000][0] == 100 &&
              runtime.records[0x4190][0] == 102 &&
              runtime.records[0x4320][0] == 103 &&
              runtime.records[0x44b0][0] == 104 &&
              runtime.calls.front() == "assign 16384 16384" &&
              At(runtime, "assign 16784 17184") <
                  At(runtime, "assign 17584 17984"),
          "Equip compactor string-assigns retained 400-byte records in order");

    Runtime callback;
    Check(pal4::GameDataEquipStorage::CompactAdjacent(
              0x4000, 0x47d0, 0x4000, SameEquipPayload, callback) ==
              0x47d0 && callback.records[0x4640][0] == 104,
          "Equip comparator parameter controls duplicate elimination");
    callback.calls.clear();
    Check(pal4::GameDataEquipStorage::CompactAdjacent(
              0x4000, 0x4000, 0x6000, SameEquipId, callback) ==
              0x6000 && callback.calls.empty(),
          "empty Equip range does not assign text");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckSort();
        CheckReserve();
        CheckInsert();
        CheckCopyHelpers();
        CheckInsertRepeated();
        CheckCompactAdjacent();
        std::cout << "game_data_equip_storage_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
