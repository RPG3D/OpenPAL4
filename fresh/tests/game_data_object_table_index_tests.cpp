#include "pal4/game_data_object_table_index.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataObjectTableIndexRuntime {
    std::vector<std::uint32_t> assigned;
    std::vector<std::uint32_t> destroyed;

    void AssignRecord(pal4::GameDataObjectRecord& destination,
                      const pal4::GameDataObjectRecord& source) override {
        assigned.push_back(source[1]);
        destination = source;
    }
    void DestroyRecord(pal4::GameDataObjectRecord& record) override {
        destroyed.push_back(record[1]);
    }
};

pal4::GameDataObjectRecord Record(std::uint32_t key,
                                 std::uint32_t marker) {
    pal4::GameDataObjectRecord record{};
    record[0] = key;
    record[1] = marker;
    record[13] = marker + 100;
    return record;
}

void CheckCompaction() {
    std::vector<pal4::GameDataObjectRecord> records{
        Record(2, 10), Record(2, 11), Record(1, 12),
        Record(1, 13), Record(2, 14), Record(3, 15)};
    std::vector<pal4::GameDataObjectIndexEntry> index{{99, 99}};
    Runtime runtime;
    pal4::GameDataObjectTableIndex::Rebuild(records, index, runtime);
    Check(records.size() == 4 &&
              records[0][1] == 10 && records[1][1] == 12 &&
              records[2][1] == 14 && records[3][1] == 15 &&
              records[1][13] == 112 && records[3][13] == 115,
          "adjacent duplicate runs compact while full records survive");
    Check(runtime.assigned == std::vector<std::uint32_t>{12, 14, 15} &&
              runtime.destroyed.size() == 2,
          "compaction uses text-aware assignment and tail destruction");
    Check(index.size() == 4 &&
              index[0].key == 1 && index[0].record_index == 1 &&
              index[1].key == 2 && index[1].record_index == 0 &&
              index[2].key == 2 && index[2].record_index == 2 &&
              index[3].key == 3 && index[3].record_index == 3,
          "signed index sort retains source positions for repeated keys");
    const auto count = runtime.assigned.size();
    pal4::GameDataObjectTableIndex::Rebuild(records, index, runtime);
    Check(runtime.assigned.size() == count && records.size() == 4,
          "second rebuild retains later nonadjacent recurrence");
}

void CheckEdges() {
    Runtime runtime;
    std::vector<pal4::GameDataObjectRecord> records;
    std::vector<pal4::GameDataObjectIndexEntry> index{{1, 2}};
    pal4::GameDataObjectTableIndex::Rebuild(records, index, runtime);
    Check(index.empty() && runtime.assigned.empty() &&
              runtime.destroyed.empty(),
          "empty source clears stale index");
    records = {Record(0xffffffffu, 1), Record(1, 2), Record(0, 3)};
    pal4::GameDataObjectTableIndex::Rebuild(records, index, runtime);
    Check(index.size() == 3 && index[0].key == -1 &&
              index[0].record_index == 0 &&
              index[1].key == 0 && index[1].record_index == 2 &&
              index[2].key == 1 && index[2].record_index == 1,
          "high-bit hash sorts as a signed key");
    Check(runtime.assigned.empty() && runtime.destroyed.empty(),
          "unique records do not need deep assignment or destruction");
}

}  // namespace

int main() {
    try {
        CheckCompaction();
        CheckEdges();
        std::cout << "game_data_object_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
