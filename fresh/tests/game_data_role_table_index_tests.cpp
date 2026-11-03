#include "pal4/game_data_role_table_index.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataRoleTableIndexRuntime {
    std::vector<std::uint32_t> assigned;
    std::vector<std::uint32_t> destroyed;

    void AssignRecord(pal4::GameDataRoleTableRecord& destination,
                      const pal4::GameDataRoleTableRecord& source) override {
        assigned.push_back(source[3]);
        destination = source;
    }
    void DestroyRecord(pal4::GameDataRoleTableRecord& record) override {
        destroyed.push_back(record[3]);
    }
};

pal4::GameDataRoleTableRecord Record(std::uint32_t key,
                                    std::uint32_t marker) {
    pal4::GameDataRoleTableRecord record{};
    record[57] = key;
    record[3] = marker;
    return record;
}

void CheckCompaction() {
    std::vector<pal4::GameDataRoleTableRecord> records{
        Record(1002, 10), Record(1002, 11), Record(1001, 12),
        Record(1001, 13), Record(1002, 14), Record(1003, 15)};
    std::vector<pal4::GameDataRoleIndexEntry> index{{9999, 99}};
    Runtime runtime;
    pal4::GameDataRoleTableIndex::Rebuild(records, index, runtime);
    Check(records.size() == 4 &&
              records[0][3] == 10 && records[1][3] == 12 &&
              records[2][3] == 14 && records[3][3] == 15,
          "adjacent duplicate runs compact while later key recurrence remains");
    Check(runtime.assigned == std::vector<std::uint32_t>{12, 14, 15} &&
              runtime.destroyed.size() == 2,
          "text-aware record assignment and tail destruction");
    Check(index.size() == 4 &&
              index[0].key == 1001 && index[0].record_index == 1 &&
              index[1].key == 1002 && index[1].record_index == 0 &&
              index[2].key == 1002 && index[2].record_index == 2 &&
              index[3].key == 1003 && index[3].record_index == 3,
          "signed index sort preserves record positions and equal-key order");
    const auto assignments = runtime.assigned.size();
    pal4::GameDataRoleTableIndex::Rebuild(records, index, runtime);
    Check(records.size() == 4 && runtime.assigned.size() == assignments &&
              index.size() == 4,
          "second rebuild retains nonadjacent repeated key");
}

void CheckEdges() {
    Check(pal4::GameDataRoleTableIndex::EqualKey(
              Record(7, 1), Record(7, 2)) &&
              !pal4::GameDataRoleTableIndex::EqualKey(
                  Record(7, 1), Record(8, 1)),
          "Role comparator reads only the key at +228");
    Runtime runtime;
    std::vector<pal4::GameDataRoleTableRecord> records;
    std::vector<pal4::GameDataRoleIndexEntry> index{{1, 2}};
    pal4::GameDataRoleTableIndex::Rebuild(records, index, runtime);
    Check(index.empty() && runtime.assigned.empty() &&
              runtime.destroyed.empty(), "empty records clear stale index");
    records = {Record(0xffffffffu, 1), Record(1, 2), Record(0, 3)};
    pal4::GameDataRoleTableIndex::Rebuild(records, index, runtime);
    Check(index.size() == 3 && index[0].key == -1 &&
              index[1].key == 0 && index[2].key == 1 &&
              index[0].record_index == 0 &&
              index[1].record_index == 2 &&
              index[2].record_index == 1,
          "key ordering uses signed comparison");
    Check(runtime.assigned.empty() && runtime.destroyed.empty(),
          "unique records require no text-aware copy or destruction");
}

}  // namespace

int main() {
    try {
        CheckCompaction();
        CheckEdges();
        std::cout << "game_data_role_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
