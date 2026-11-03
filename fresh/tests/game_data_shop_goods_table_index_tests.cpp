#include "pal4/game_data_shop_goods_table_index.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataShopGoodsTableIndexRuntime {
    std::vector<std::uint32_t> assigned;
    std::vector<std::uint32_t> destroyed;
    bool fail_assignment = false;

    void AssignRecord(pal4::GameDataShopGoodsRecord& destination,
                      const pal4::GameDataShopGoodsRecord& source) override {
        assigned.push_back(source[2]);
        if (fail_assignment) throw std::runtime_error("assign failed");
        destination = source;
    }
    void DestroyRecord(pal4::GameDataShopGoodsRecord& record) override {
        destroyed.push_back(record[2]);
    }
};

pal4::GameDataShopGoodsRecord Record(std::uint32_t key,
                                    std::uint32_t marker) {
    pal4::GameDataShopGoodsRecord record{};
    record[0] = key;
    record[1] = 2;
    record[2] = marker;
    record[8] = marker + 100;
    return record;
}

void CheckCompaction() {
    std::vector<pal4::GameDataShopGoodsRecord> records{
        Record(2, 10), Record(2, 11), Record(1, 12),
        Record(1, 13), Record(2, 14), Record(3, 15)};
    std::vector<pal4::GameDataShopGoodsIndexEntry> index{{99, 99}};
    Runtime runtime;
    pal4::GameDataShopGoodsTableIndex::Rebuild(records, index, runtime);
    Check(records.size() == 4 &&
              records[0][2] == 10 && records[1][2] == 12 &&
              records[2][2] == 14 && records[3][2] == 15 &&
              records[1][8] == 112 && records[3][8] == 115,
          "Good compaction keeps first item from each adjacent key run");
    Check(runtime.assigned == std::vector<std::uint32_t>{12, 14, 15} &&
              runtime.destroyed.size() == 2,
          "Good compaction assigns full records and destroys discarded tail");
    Check(index.size() == 4 &&
              index[0].key == 1 && index[0].record_index == 1 &&
              index[1].key == 2 && index[1].record_index == 0 &&
              index[2].key == 2 && index[2].record_index == 2 &&
              index[3].key == 3 && index[3].record_index == 3,
          "signed index retains physical positions for duplicate keys");
    const auto prior_assignments = runtime.assigned.size();
    pal4::GameDataShopGoodsTableIndex::Rebuild(records, index, runtime);
    Check(runtime.assigned.size() == prior_assignments &&
              records.size() == 4,
          "later nonadjacent recurrence survives repeated rebuild");
}

void CheckEdges() {
    Runtime runtime;
    std::vector<pal4::GameDataShopGoodsRecord> records;
    std::vector<pal4::GameDataShopGoodsIndexEntry> index{{1, 2}};
    pal4::GameDataShopGoodsTableIndex::Rebuild(records, index, runtime);
    Check(index.empty() && runtime.assigned.empty() &&
              runtime.destroyed.empty(),
          "empty Good records clear stale index");
    records = {Record(0xffffffffu, 1), Record(1, 2), Record(0, 3)};
    pal4::GameDataShopGoodsTableIndex::Rebuild(records, index, runtime);
    Check(index.size() == 3 && index[0].key == -1 &&
              index[0].record_index == 0 &&
              index[1].key == 0 && index[1].record_index == 2 &&
              index[2].key == 1 && index[2].record_index == 1 &&
              runtime.assigned.empty() && runtime.destroyed.empty(),
          "Good keys sort signed without changing unique records");

    Runtime failed;
    failed.fail_assignment = true;
    records = {Record(2, 10), Record(2, 11), Record(3, 12)};
    try {
        pal4::GameDataShopGoodsTableIndex::Rebuild(records, index, failed);
        Check(false, "record assignment failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "assign failed" &&
                  failed.assigned == std::vector<std::uint32_t>{12} &&
                  failed.destroyed.empty() && records.size() == 3,
              "assignment failure stops before tail cleanup and index reset");
    }
}

void CheckKeyComparison() {
    const auto first = Record(0xffffffffu, 1);
    const auto same_key = Record(0xffffffffu, 2);
    const auto other = Record(1, 1);
    Check(pal4::GameDataShopGoodsTableIndex::KeysEqual(first, same_key) &&
              !pal4::GameDataShopGoodsTableIndex::KeysEqual(first, other) &&
              pal4::GameDataShopGoodsTableIndex::KeysEqual(first, first),
          "key comparator reads only the first DWORD, including high bit");
}

}  // namespace

int main() {
    try {
        CheckCompaction();
        CheckEdges();
        CheckKeyComparison();
        std::cout << "game_data_shop_goods_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
