#include "pal4/game_data_equip_relation_table.h"

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

struct Runtime final : pal4::IGameDataEquipRelationStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1060, 0x4000}, {0x1064, 0x403c}, {0x1068, 0x4048},
        {0x1008, 0x5000}, {0x100c, 0x5028}, {0x1010, 0x5030}};
    std::map<std::uint32_t, pal4::GameDataEquipRelationRecord> records;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x6000, 0x7000};

    Runtime() {
        const std::uint32_t ids[5] = {9, 9, 0xffffffff, 3, 9};
        for (std::uint32_t i = 0; i < 5; ++i) {
            records[0x4000 + i * 12] = {ids[i], 100 + i, 200 + i};
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
    pal4::GameDataEquipRelationRecord ReadRecord(
        std::uint32_t address) override {
        return records.at(address);
    }
    void WriteRecord(
        std::uint32_t address,
        const pal4::GameDataEquipRelationRecord& record) override {
        records[address] = record;
        calls.push_back("record " + std::to_string(address));
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

void CheckSort() {
    Runtime runtime;
    pal4::GameDataEquipRelationStorage::Sort(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x4030 &&
              runtime.records[0x4000] ==
                  pal4::GameDataEquipRelationRecord{9, 100, 200} &&
              runtime.records[0x400c] ==
                  pal4::GameDataEquipRelationRecord{0xffffffff, 102, 202} &&
              runtime.records[0x4018] ==
                  pal4::GameDataEquipRelationRecord{3, 103, 203} &&
              runtime.records[0x4024] ==
                  pal4::GameDataEquipRelationRecord{9, 104, 204},
          "only adjacent equal IDs collapse; full records retain order");
    Check(runtime.words[0x100c] == 0x5020 &&
              runtime.keys[0x5000].key == 0xffffffff &&
              runtime.keys[0x5000].record_index == 1 &&
              runtime.keys[0x5008].key == 3 &&
              runtime.keys[0x5008].record_index == 2 &&
              runtime.keys[0x5010].key == 9 &&
              runtime.keys[0x5010].record_index == 0 &&
              runtime.keys[0x5018].key == 9 &&
              runtime.keys[0x5018].record_index == 3,
          "rebuilt key map sorts signed IDs and preserves physical indices");

    Runtime empty;
    empty.words[0x1064] = empty.words[0x1060];
    pal4::GameDataEquipRelationStorage::Sort(0x1000, empty);
    Check(empty.words[0x1064] == 0x4000 &&
              empty.words[0x100c] == 0x5000,
          "empty value range clears key end");
}

void CheckReserve() {
    Runtime both;
    Check(pal4::GameDataEquipRelationStorage::Reserve(
              0x1000, 8, both) == 0x7028 &&
              both.words[0x1060] == 0x6000 &&
              both.words[0x1064] == 0x603c &&
              both.words[0x1068] == 0x6060 &&
              both.words[0x1008] == 0x7000 &&
              both.words[0x100c] == 0x7028 &&
              both.words[0x1010] == 0x7040 &&
              both.records[0x6030][1] == 104 &&
              both.keys[0x7020].record_index == 4 &&
              both.calls.front() == "allocate 96",
          "both arrays grow while copying live records and key pairs");

    Runtime records_only;
    records_only.words[0x1010] = 0x5080;
    Check(pal4::GameDataEquipRelationStorage::Reserve(
              0x1000, 8, records_only) == 16 &&
              records_only.words[0x1060] == 0x6000 &&
              records_only.words[0x1008] == 0x5000,
          "record-only growth returns existing key capacity");

    Runtime keys_only;
    keys_only.words[0x1068] = 0x40c0;
    keys_only.allocations = {0x7000};
    Check(pal4::GameDataEquipRelationStorage::Reserve(
              0x1000, 8, keys_only) == 0x7028 &&
              keys_only.words[0x1060] == 0x4000 &&
              keys_only.calls.front() == "allocate 64",
          "key-only growth preserves record storage");

    Runtime enough;
    enough.words[0x1068] = 0x40c0;
    enough.words[0x1010] = 0x5080;
    Check(pal4::GameDataEquipRelationStorage::Reserve(
              0x1000, 8, enough) == 16 && enough.calls.empty(),
          "sufficient capacity performs no allocation or copy");

    Runtime empty;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    empty.words[0x1008] = empty.words[0x100c] = empty.words[0x1010] = 0;
    Check(pal4::GameDataEquipRelationStorage::Reserve(
              0x1000, 3, empty) == 0x7000 &&
              empty.words[0x1060] == empty.words[0x1064] &&
              empty.words[0x1068] == 0x6024 &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1010] == 0x7018,
          "null arrays allocate requested capacities with zero used count");

    Runtime failed;
    failed.allocations = {0, 0};
    Check(pal4::GameDataEquipRelationStorage::Reserve(
              0x1000, 8, failed) == 40 &&
              failed.words[0x1060] == 0 &&
              failed.words[0x1064] == 60 &&
              failed.words[0x1008] == 0 &&
              failed.words[0x100c] == 40,
          "native null allocation still updates array triplets");
}

void CheckInsert() {
    const pal4::GameDataEquipRelationRecord inserted{42, 777, 888};
    Runtime spare;
    Check(pal4::GameDataEquipRelationStorage::Insert(
              0x105c, 0x4018, inserted, spare) == 0x4018 &&
              spare.words[0x1060] == 0x4000 &&
              spare.words[0x1064] == 0x4048 &&
              spare.records[0x4018] == inserted &&
              spare.records[0x4024][0] == 0xffffffff &&
              spare.records[0x4030][0] == 3 &&
              spare.records[0x403c][0] == 9 &&
              spare.calls.front() != "allocate 120",
          "spare capacity shifts complete 12-byte records in place");

    Runtime full;
    full.words[0x1068] = full.words[0x1064];
    Check(pal4::GameDataEquipRelationStorage::Insert(
              0x105c, 0x400c, inserted, full) == 0x600c &&
              full.words[0x1060] == 0x6000 &&
              full.words[0x1064] == 0x6048 &&
              full.words[0x1068] == 0x6078 &&
              full.records[0x6000][0] == 9 &&
              full.records[0x600c] == inserted &&
              full.records[0x6018][1] == 101 &&
              full.records[0x603c][1] == 104 &&
              full.calls.front() == "allocate 120" &&
              full.calls.back() == "word 4196",
          "full array doubles capacity and inserts at original index");

    Runtime empty;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    Check(pal4::GameDataEquipRelationStorage::Insert(
              0x105c, 0, inserted, empty) == 0x6000 &&
              empty.words[0x1060] == 0x6000 &&
              empty.words[0x1064] == 0x600c &&
              empty.words[0x1068] == 0x600c &&
              empty.records[0x6000] == inserted &&
              empty.calls.front() == "allocate 12",
          "empty array grows to one relation record");
}

void CheckCopyIfAllocated() {
    Runtime runtime;
    Check(pal4::GameDataEquipRelationStorage::CopyIfAllocated(
              0, 0x4000, runtime) == 0 && runtime.calls.empty(),
          "null destination returns without source access");
    Check(pal4::GameDataEquipRelationStorage::CopyIfAllocated(
              0x7000, 0x400c, runtime) == 0x7000 &&
              runtime.records[0x7000] == runtime.records[0x400c] &&
              runtime.calls ==
                  std::vector<std::string>{"record 28672"},
          "non-null destination copies all three DWORDs and returns itself");
}

bool SameRelationId(const pal4::GameDataEquipRelationRecord& left,
                    const pal4::GameDataEquipRelationRecord& right) {
    return left[0] == right[0];
}

bool SameRelationValue(const pal4::GameDataEquipRelationRecord& left,
                       const pal4::GameDataEquipRelationRecord& right) {
    return left[1] == right[1];
}

void CheckCompactAdjacent() {
    Runtime runtime;
    Check(pal4::GameDataEquipRelationStorage::CompactAdjacent(
              0x4000, 0x403c, 0x4000, SameRelationId, runtime) ==
              0x4030 &&
              runtime.records[0x4000][1] == 100 &&
              runtime.records[0x400c][1] == 102 &&
              runtime.records[0x4018][1] == 103 &&
              runtime.records[0x4024][1] == 104,
          "relation compactor keeps first of adjacent equal-ID run");
    Check(runtime.calls.front() == "record 16384" &&
              runtime.calls.back() == "record 16420",
          "relation compactor also copies the first record to itself");

    Runtime callback;
    Check(pal4::GameDataEquipRelationStorage::CompactAdjacent(
              0x4000, 0x403c, 0x6000, SameRelationValue, callback) ==
              0x603c && callback.records[0x6030][1] == 104,
          "caller-supplied comparator controls what counts as a duplicate");
    callback.calls.clear();
    Check(pal4::GameDataEquipRelationStorage::CompactAdjacent(
              0x4000, 0x4000, 0x7000, SameRelationId, callback) ==
              0x7000 && callback.calls.empty(),
          "empty relation range leaves destination unchanged");
}

}  // namespace

int main() {
    try {
        CheckSort();
        CheckReserve();
        CheckInsert();
        CheckCopyIfAllocated();
        CheckCompactAdjacent();
        std::cout << "game_data_equip_relation_storage_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
