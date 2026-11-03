#include "pal4/story_data_table.h"

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

struct Runtime final : pal4::IStoryDataIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool fail_copy = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    std::uint32_t CopyUnique(std::uint32_t begin, std::uint32_t end,
                             std::uint32_t destination) override {
        calls.push_back("unique:" + std::to_string(begin) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(destination));
        if (fail_copy) throw std::runtime_error("copy failed");
        if (begin == end) return destination;
        auto output = destination;
        auto previous = begin;
        words[output] = words[begin];
        output += 36;
        for (auto current = begin + 36; current != end; current += 36) {
            if (words[previous] != words[current]) {
                words[output] = words[current];
                previous = current;
                output += 36;
            }
        }
        return output;
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        calls.push_back("destroy:" + std::to_string(record));
        return record + 1;
    }
    void InsertKey(std::uint32_t array, std::uint32_t end,
                   std::uint32_t key, std::uint32_t index) override {
        calls.push_back("insert:" + std::to_string(array) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(index));
        words[end] = key;
        words[end + 4] = index;
        words[array + 8] = end + 8;
    }
};

void Seed(Runtime& runtime, const std::vector<std::int32_t>& keys) {
    constexpr std::uint32_t table = 0x7000;
    runtime.words[table + 96] = keys.empty() ? 0 : 0x9000;
    runtime.words[table + 100] = keys.empty()
        ? 0 : 0x9000 + 36 * static_cast<std::uint32_t>(keys.size());
    runtime.words[table + 8] = 0xa000;
    runtime.words[table + 12] = 0xa000;
    for (std::size_t index = 0; index < keys.size(); ++index)
        runtime.words[0x9000 + 36 * static_cast<std::uint32_t>(index)] =
            static_cast<std::uint32_t>(keys[index]);
}

void CheckCompaction() {
    Runtime runtime;
    Seed(runtime, {3, 3, 1, 3});
    pal4::StoryDataTable::RebuildIndex(0x7000, runtime);
    Check(runtime.calls[0] == "unique:36864:37008:36864" &&
              runtime.calls[1] == "destroy:36972" &&
              runtime.words[0x7064] == 36972 &&
              runtime.words[0x9000] == 3 &&
              runtime.words[0x9024] == 1 &&
              runtime.words[0x9048] == 3 &&
              runtime.words[0x700c] == 0xa018,
          "first adjacent duplicate triggers compaction and tail destruction");
    Check(runtime.words[0xa000] == 1 &&
              runtime.words[0xa004] == 1 &&
              runtime.words[0xa008] == 3 &&
              runtime.words[0xa010] == 3,
          "rebuilt index sorts signed keys and retains physical row positions");
}

void CheckNoDuplicatesAndSignedSort() {
    Runtime runtime;
    Seed(runtime, {-2, 3, 1});
    pal4::StoryDataTable::RebuildIndex(0x7000, runtime);
    Check(runtime.calls[0] == "unique:36972:36972:36972" &&
              runtime.words[0x7064] == 36972 &&
              runtime.words[0xa000] == static_cast<std::uint32_t>(-2) &&
              runtime.words[0xa004] == 0 &&
              runtime.words[0xa008] == 1 &&
              runtime.words[0xa00c] == 2 &&
              runtime.words[0xa010] == 3 &&
              runtime.words[0xa014] == 1,
          "no duplicate calls empty copy range and sorts negative keys first");
    for (const auto& call : runtime.calls)
        Check(call.rfind("destroy:", 0) != 0,
              "no duplicate leaves record destruction empty");
}

void CheckEmptyAndFailure() {
    Runtime empty;
    Seed(empty, {});
    pal4::StoryDataTable::RebuildIndex(0x7000, empty);
    Check(empty.calls[0] == "unique:0:0:0" &&
              empty.words[0x700c] == 0xa000,
          "empty rows still reset the key index");

    Runtime failed;
    Seed(failed, {4, 4});
    failed.fail_copy = true;
    try {
        pal4::StoryDataTable::RebuildIndex(0x7000, failed);
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  failed.calls.size() == 1 &&
                  failed.words[0x7064] == 0x9048 &&
                  failed.words[0x700c] == 0xa000,
              "failed compaction stops before tail destruction and index reset");
    }

    Runtime range;
    Check(pal4::StoryDataTable::DestroyRange(
              0x9000, 0x9048, range) == 0x9025 &&
              range.calls == std::vector<std::string>{
                  "destroy:36864", "destroy:36900"} &&
              pal4::StoryDataTable::DestroyRange(
                  0x9000, 0x9000, range) == 0,
          "range destructor uses 36-byte stride and returns last result");
}

}  // namespace

int main() {
    try {
        CheckCompaction();
        CheckNoDuplicatesAndSignedSort();
        CheckEmptyAndFailure();
        std::cout << "story_data_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
