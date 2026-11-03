#include "pal4/animation_string_table_collection.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Call final {
    char kind;
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const Call&) const = default;
};

class Runtime final : public pal4::IAnimationStringTableCollectionRuntime {
public:
    class TableRuntimeImpl final : public pal4::AnimationStringTableRuntimeStub {
    public:
        explicit TableRuntimeImpl(std::vector<Call>& calls) : calls_(calls) {}

        std::map<std::uint32_t, pal4::AnimationStringTableState> tables;
        std::map<std::uint32_t, pal4::AnimationStringFieldState> fields;
        std::map<std::uint32_t, std::uint8_t> references;

        pal4::AnimationStringTableState& TableAt(
            std::uint32_t address) override {
            calls_.push_back({'T', address, 0});
            return tables[address];
        }
        pal4::AnimationStringFieldState& FieldAt(
            std::uint32_t address) override {
            calls_.push_back({'S', address, 0});
            return fields[address];
        }
        std::uint32_t EmptyText() override {
            calls_.push_back({'e', 0, 0});
            return 0x7000u;
        }
        std::int32_t CompareText(std::uint32_t left,
                                  std::uint32_t right) override {
            calls_.push_back({'=', left, right});
            return left == right ? 0 : 1;
        }
        std::uint8_t ReadReference(std::uint32_t address) override {
            calls_.push_back({'r', address, 0});
            return references[address];
        }
        void WriteReference(std::uint32_t address,
                            std::uint8_t value) override {
            calls_.push_back({'w', address, value});
            references[address] = value;
        }
        std::uint32_t Free(std::uint32_t address) override {
            calls_.push_back({'F', address, 0});
            return address + 0x10u;
        }
        void LoadTableText(std::uint32_t table,
                           std::uint32_t file) override {
            calls_.push_back({'L', table, file});
        }
        void ReadRecordCount(std::uint32_t file,
                             std::uint32_t& count) override {
            calls_.push_back({'N', file, 0});
            count = 0;
        }

    private:
        std::vector<Call>& calls_;
    };

    Runtime() : table_runtime(calls) {}
    std::vector<Call> calls;
    std::map<std::uint32_t, std::uint32_t> pointers;
    TableRuntimeImpl table_runtime;
    std::vector<std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::size_t word_index = 0;
    std::size_t allocation_index = 0;
    std::uint32_t opened_file = 0;
    std::uint32_t next_pointer_array = 0x9000u;

    std::uint32_t ReadPointer(std::uint32_t slot) override {
        calls.push_back({'P', slot, 0});
        return pointers[slot];
    }
    void WritePointer(std::uint32_t slot,
                      std::uint32_t value) override {
        calls.push_back({'W', slot, value});
        pointers[slot] = value;
    }
    std::uint32_t AllocatePointerArray(std::uint32_t bytes) override {
        calls.push_back({'M', bytes, next_pointer_array});
        return next_pointer_array;
    }
    pal4::IAnimationStringTableRuntime& TableRuntime() override {
        return table_runtime;
    }
    std::uint32_t OpenFile(std::uint32_t path) override {
        calls.push_back({'O', path, 0});
        return opened_file;
    }
    void ReadWord(std::uint32_t file, std::uint32_t& value) override {
        calls.push_back({'R', file, 0});
        if (word_index < words.size()) value = words[word_index++];
    }
    void CloseFile(std::uint32_t file) override {
        calls.push_back({'C', file, 0});
    }
    std::uint32_t AllocateTable() override {
        const auto table = allocation_index < allocations.size()
                               ? allocations[allocation_index++] : 0u;
        calls.push_back({'A', table, 32});
        return table;
    }
    std::uint32_t PrepareEmptyTableTemplate(std::uint32_t path) override {
        calls.push_back({'E', path, 0x5000u});
        table_runtime.tables[0x5000u].header = path & 0xffu;
        return 0x5000u;
    }
    void ReleaseEmptyTableTemplate(std::uint32_t source) override {
        calls.push_back({'e', source, 0});
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::AnimationStringTableCollectionState state{
        0xabcdef01u, 0x1000u, 0x100cu, 0x1010u};
    runtime.pointers = {{0x1000u, 0x2000u},
                        {0x1004u, 0u},
                        {0x1008u, 0x3000u}};
    runtime.table_runtime.fields[0x2000u].text = 0x6000u;
    runtime.table_runtime.fields[0x3000u].text = 0u;
    Check(pal4::AnimationStringTableCollection::Find(
              state, 0x6000u, runtime) == 0x2000u &&
              runtime.calls == std::vector<Call>{
                  {'P', 0x1000u, 0}, {'S', 0x2000u, 0},
                  {'=', 0x6000u, 0x6000u}},
          "lookup returns the first table with a matching name");
    runtime.calls.clear();
    Check(pal4::AnimationStringTableCollection::Find(
              state, 0x7000u, runtime) == 0x3000u &&
              runtime.calls == std::vector<Call>{
                  {'P', 0x1000u, 0}, {'S', 0x2000u, 0},
                  {'=', 0x6000u, 0x7000u}, {'P', 0x1004u, 0},
                  {'P', 0x1008u, 0}, {'S', 0x3000u, 0},
                  {'e', 0, 0}, {'=', 0x7000u, 0x7000u}},
          "lookup skips null slots and substitutes empty text");
    runtime.calls.clear();
    Check(pal4::AnimationStringTableCollection::Find(
              state, 0x8000u, runtime) == 0u &&
              runtime.calls.size() == 8u,
          "lookup returns null after all tables differ");

    runtime.calls.clear();
    runtime.table_runtime.tables[0x2000u] = {
        0, 0x4000u, 3u, 4u, 0, 0, 0, 0};
    runtime.table_runtime.tables[0x3000u] = {
        0, 0x5000u, 5u, 6u, 0, 0, 0, 0};
    runtime.table_runtime.references[0x3fffu] = 2u;
    runtime.table_runtime.references[0x4fffu] = 0u;
    Check(pal4::AnimationStringTableCollection::Clear(
              state, runtime) == 0x100cu &&
              state.begin == 0x1000u && state.end == 0x1000u &&
              state.limit == 0x1010u && state.reserved == 0xabcdef01u &&
              runtime.table_runtime.references[0x3fffu] == 1u &&
              runtime.calls == std::vector<Call>{
                  {'P', 0x1000u, 0}, {'T', 0x2000u, 0},
                  {'F', 0u, 0}, {'r', 0x3fffu, 0},
                  {'w', 0x3fffu, 1u}, {'F', 0x2000u, 0},
                  {'P', 0x1004u, 0},
                  {'P', 0x1008u, 0}, {'T', 0x3000u, 0},
                  {'F', 0u, 0}, {'r', 0x4fffu, 0},
                  {'F', 0x4fffu, 0}, {'F', 0x3000u, 0}},
          "clear destroys each nonnull table and retains pointer allocation");

    runtime.calls.clear();
    Check(pal4::AnimationStringTableCollection::Release(
              state, runtime) == 0x1010u &&
              state.begin == 0u && state.end == 0u &&
              state.limit == 0u && state.reserved == 0xabcdef01u &&
              runtime.calls == std::vector<Call>{{'F', 0x1000u, 0}},
          "release frees retained pointer array and clears its triple");
    runtime.calls.clear();
    Check(pal4::AnimationStringTableCollection::Release(
              state, runtime) == 0x10u &&
              runtime.calls == std::vector<Call>{{'F', 0u, 0}},
          "repeat release forwards a null free");

    runtime.calls.clear();
    state = {0, 0x1000u, 0x1005u, 0x1008u};
    Check(pal4::AnimationStringTableCollection::Find(
              state, 0x6000u, runtime) == 0u &&
              runtime.calls.empty(),
          "malformed pointer range is rejected");

    Runtime loader;
    pal4::AnimationStringTableCollectionState loaded{
        0xabu, 0x1000u, 0x1000u, 0x1008u};
    Check(!pal4::AnimationStringTableCollection::LoadFromFile(
              loaded, 0x6012u, loader) &&
              loader.calls == std::vector<Call>{{'O', 0x6012u, 0}},
          "failed open follows clear and returns false");

    loader.calls.clear();
    loader.opened_file = 0x3000u;
    loader.words = {0u};
    Check(!pal4::AnimationStringTableCollection::LoadFromFile(
              loaded, 0x6012u, loader) &&
              loader.calls == std::vector<Call>{
                  {'O', 0x6012u, 0}, {'R', 0x3000u, 0}},
          "wrong magic returns false without closing the native file");

    loader.calls.clear();
    loader.words = {20070323u, 0u};
    loader.word_index = 0;
    Check(pal4::AnimationStringTableCollection::LoadFromFile(
              loaded, 0x6012u, loader) &&
              loaded.begin == 0x1000u && loaded.end == 0x1000u &&
              loaded.limit == 0x1008u && loaded.reserved == 0xabu &&
              loader.calls == std::vector<Call>{
                  {'O', 0x6012u, 0}, {'R', 0x3000u, 0},
                  {'R', 0x3000u, 0}, {'C', 0x3000u, 0}},
          "empty file closes and retains pointer-array capacity");

    loader.calls.clear();
    loader.words = {20070323u, 2u};
    loader.word_index = 0;
    loader.allocations = {0x2000u, 0u};
    Check(pal4::AnimationStringTableCollection::LoadFromFile(
              loaded, 0x6012u, loader) &&
              loaded.end == 0x1008u &&
              loader.pointers[0x1000u] == 0x2000u &&
              loader.pointers[0x1004u] == 0u &&
              loader.table_runtime.tables[0x2000u].header == 0x12u &&
              loader.table_runtime.tables[0x2000u].tag == 0u &&
              loader.calls.front() == Call{'O', 0x6012u, 0} &&
              loader.calls.back() == Call{'C', 0x3000u, 0},
          "loader constructs and loads each table, inserting null on allocation failure");
    Check(std::count(loader.calls.begin(), loader.calls.end(),
                     Call{'L', 0x2000u, 0x3000u}) == 1 &&
              std::count(loader.calls.begin(), loader.calls.end(),
                         Call{'W', 0x1000u, 0x2000u}) == 1 &&
              std::count(loader.calls.begin(), loader.calls.end(),
                         Call{'W', 0x1004u, 0u}) == 1 &&
              std::count(loader.calls.begin(), loader.calls.end(),
                         Call{'e', 0x5000u, 0}) == 1,
          "loader reuses rebuilt table constructor and table loader");

    Runtime insert_runtime;
    pal4::AnimationStringTableCollectionState inserted{
        0x44u, 0x1000u, 0x100cu, 0x1018u};
    insert_runtime.pointers = {{0x1000u, 1u}, {0x1004u, 2u},
                               {0x1008u, 3u}};
    Check(pal4::AnimationStringTableCollection::Insert(
              inserted, 0x1004u, 2u, 9u, insert_runtime) == 0x1014u &&
              inserted.begin == 0x1000u && inserted.limit == 0x1018u &&
              insert_runtime.pointers[0x1000u] == 1u &&
              insert_runtime.pointers[0x1004u] == 9u &&
              insert_runtime.pointers[0x1008u] == 9u &&
              insert_runtime.pointers[0x100cu] == 2u &&
              insert_runtime.pointers[0x1010u] == 3u,
          "in-place insertion shifts a long tail and preserves order");

    insert_runtime.calls.clear();
    inserted.end = 0x100cu;
    Check(pal4::AnimationStringTableCollection::Insert(
              inserted, 0x1008u, 3u, 7u, insert_runtime) == 0x1018u &&
              insert_runtime.pointers[0x1000u] == 1u &&
              insert_runtime.pointers[0x1004u] == 9u &&
              insert_runtime.pointers[0x1008u] == 7u &&
              insert_runtime.pointers[0x100cu] == 7u &&
              insert_runtime.pointers[0x1010u] == 7u &&
              insert_runtime.pointers[0x1014u] == 9u,
          "in-place insertion also handles a short tail");

    insert_runtime.calls.clear();
    inserted.end = 0x100cu;
    inserted.limit = 0x100cu;
    insert_runtime.pointers[0x1000u] = 1u;
    insert_runtime.pointers[0x1004u] = 2u;
    insert_runtime.pointers[0x1008u] = 3u;
    Check(pal4::AnimationStringTableCollection::Insert(
              inserted, 0x1004u, 1u, 8u, insert_runtime) == 0x9010u &&
              inserted.begin == 0x9000u && inserted.end == 0x9010u &&
              inserted.limit == 0x9018u && inserted.reserved == 0x44u &&
              insert_runtime.pointers[0x9000u] == 1u &&
              insert_runtime.pointers[0x9004u] == 8u &&
              insert_runtime.pointers[0x9008u] == 2u &&
              insert_runtime.pointers[0x900cu] == 3u &&
              std::count(insert_runtime.calls.begin(), insert_runtime.calls.end(),
                         Call{'F', 0x1000u, 0}) == 1,
          "growth doubles capacity, copies prefix and tail, and frees old array");

    insert_runtime.calls.clear();
    const auto unchanged = inserted;
    Check(pal4::AnimationStringTableCollection::Insert(
              inserted, inserted.begin + 2u, 1u, 4u,
              insert_runtime) == unchanged.end &&
              inserted.end == unchanged.end && insert_runtime.calls.empty(),
          "unaligned insertion position is rejected");
    Check(pal4::AnimationStringTableCollection::Insert(
              inserted, inserted.end, 0u, 4u,
              insert_runtime) == unchanged.end &&
              insert_runtime.calls.empty(),
          "zero-count insertion is inert");
    insert_runtime.next_pointer_array = 0;
    Check(pal4::AnimationStringTableCollection::Insert(
              inserted, inserted.end, 3u, 4u,
              insert_runtime) == unchanged.end &&
              inserted.begin == unchanged.begin &&
              inserted.end == unchanged.end,
          "failed growth retains the old pointer array");

    Runtime empty_runtime;
    pal4::AnimationStringTableCollectionState empty{0x55u, 0u, 0u, 0u};
    Check(pal4::AnimationStringTableCollection::Insert(
              empty, 0u, 2u, 0x77u, empty_runtime) == 0x9008u &&
              empty.begin == 0x9000u && empty.end == 0x9008u &&
              empty.limit == 0x9008u && empty.reserved == 0x55u &&
              empty_runtime.pointers[0x9000u] == 0x77u &&
              empty_runtime.pointers[0x9004u] == 0x77u,
          "empty array grows to the requested number of pointers");
    empty_runtime.calls.clear();
    Check(pal4::AnimationStringTableCollection::Insert(
              empty, empty.end, 0xffffffffu, 1u,
              empty_runtime) == empty.end &&
              empty_runtime.calls.empty(),
          "pointer-byte overflow is rejected without allocation");
    empty.end = empty.begin + 2u;
    Check(pal4::AnimationStringTableCollection::Insert(
              empty, empty.begin, 1u, 1u,
              empty_runtime) == empty.end &&
              empty_runtime.calls.empty(),
          "malformed pointer range is rejected without writes");
    empty = {0u, 4u, 0x80000004u, 0x80000004u};
    Check(pal4::AnimationStringTableCollection::Insert(
              empty, empty.end, 1u, 1u,
              empty_runtime) == empty.end &&
              empty_runtime.calls.empty(),
          "doubling capacity overflow is rejected before allocation");

    std::cout << "animation_string_table_collection_tests passed\n";
}
