#include "pal4/monster_info_manager.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::NpcInfoRuntimeStub {
    std::uint32_t next = 0x1000;
    std::uint32_t fail_size = 0;
    std::uint32_t stream = 0;
    std::uint32_t stream_count = 0;
    std::uint32_t stream_kind = 0;
    std::uint32_t locale = 14;
    std::uint32_t current_record = 0;
    std::vector<std::uint32_t> item_counts;
    std::vector<std::string> input_strings;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> cleaned_strings;
    std::vector<std::uint32_t> destroyed_xml;
    std::vector<std::uint32_t> xml_kinds;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> checked_xml;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> data_reads;
    std::vector<std::uint32_t> destroyed_facets;
    std::string opened_directory;
    std::string opened_name;
    std::uint32_t closed_stream = 0;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        if (bytes == fail_size) return 0;
        const auto result = next;
        next += bytes + 0x100;
        if (bytes == pal4::MonsterInfoRecord::kBytes)
            current_record = result;
        return result;
    }
    void Free(const std::uint32_t address) override { frees.push_back(address); }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override { words[address] = value; }
    void InitializeString(const std::uint32_t address) override {
        strings[address].clear();
    }
    void AssignString(const std::uint32_t address,
                      const std::string_view value) override {
        strings[address] = value;
    }
    std::string ReadStringValue(const std::uint32_t address) override {
        return strings[address];
    }
    void CleanupString(const std::uint32_t address) override {
        cleaned_strings.push_back(address);
        strings.erase(address);
    }
    void ReadStreamString(std::uint32_t,
                          const std::uint32_t address) override {
        if (input_strings.empty()) return;
        strings[address] = input_strings.front();
        input_strings.erase(input_strings.begin());
    }
    void ReadStreamData(std::uint32_t, const std::uint32_t address,
                        const std::uint32_t bytes) override {
        data_reads.emplace_back(address, bytes);
        if (address == current_record + 64u && !item_counts.empty()) {
            words[address] = item_counts.front();
            item_counts.erase(item_counts.begin());
        }
    }
    std::uint32_t ReadStreamWord(std::uint32_t) override {
        return stream_count;
    }
    std::uint32_t StreamKind(std::uint32_t) override { return stream_kind; }
    std::uint32_t StreamLocale(std::uint32_t) override { return locale; }
    std::uint32_t CreateLocaleFacet(const std::uint32_t source) override {
        return source + 1;
    }
    void DestroyLocaleFacet(const std::uint32_t facet) override {
        destroyed_facets.push_back(facet);
    }
    void CheckXml(const std::uint32_t facet,
                  const std::uint32_t document) override {
        checked_xml.emplace_back(facet, document);
    }
    std::uint32_t InitializeXml(const std::uint32_t storage,
                                const std::uint32_t kind) override {
        xml_kinds.push_back(kind);
        return storage;
    }
    void DestroyXml(const std::uint32_t document) override {
        destroyed_xml.push_back(document);
    }
    std::uint32_t OpenMonsterInfo(const std::string_view directory,
                                  const std::string_view name) override {
        opened_directory = directory;
        opened_name = name;
        return stream;
    }
    void CloseStream(const std::uint32_t value) override {
        closed_stream = value;
    }
};

std::vector<std::uint32_t> visits;
bool Visit(const std::uint32_t record, const std::uint32_t stop) {
    visits.push_back(record);
    return record != stop;
}

void RecordReadAndCleanup() {
    Runtime runtime;
    constexpr auto record = 0x8000u;
    runtime.current_record = record;
    pal4::MonsterInfoRecord::Initialize(record, runtime);
    Check(runtime.strings.contains(record) &&
          runtime.strings.contains(record + 12u) &&
          runtime.words[record + 52u] == 0 &&
          runtime.words[record + 76u] == 0,
          "80-byte record initialization");
    pal4::MonsterInfoRecord::InitializeDocuments(record, runtime);
    Check(runtime.allocation_sizes == (std::vector<std::uint32_t>{60, 60}) &&
          runtime.xml_kinds == (std::vector<std::uint32_t>{2, 3}),
          "monster XML document types");
    runtime.item_counts = {3, 0};
    runtime.stream_kind = 2;
    pal4::MonsterInfoRecord::Read(record, 0x9000, runtime);
    const auto array = runtime.words[record + 68u];
    Check(array != 0 && runtime.allocation_sizes.back() == 12 &&
          runtime.data_reads.size() == 10 &&
          runtime.data_reads[0] == std::pair{record + 24u, 12u} &&
          runtime.data_reads[6] == std::pair{record + 64u, 4u} &&
          runtime.data_reads[7] == std::pair{array, 4u} &&
          runtime.checked_xml.size() == 2 &&
          runtime.checked_xml[0].first == 15u &&
          runtime.destroyed_facets == (std::vector<std::uint32_t>{15u}),
          "stream field order, item array and facet checks");
    runtime.stream_kind = 0;
    runtime.checked_xml.clear();
    pal4::MonsterInfoRecord::Read(record, 0x9000, runtime);
    Check(runtime.words[record + 68u] == array &&
          runtime.checked_xml.empty() &&
          runtime.allocation_sizes.size() == 3,
          "zero count retains prior array; non XML stream skips facet");
    const auto xml_a = runtime.words[record + 72u];
    const auto xml_b = runtime.words[record + 76u];
    pal4::MonsterInfoRecord::CleanupData(record, runtime);
    Check(runtime.destroyed_xml == (std::vector<std::uint32_t>{xml_a, xml_b}) &&
          runtime.frees == (std::vector<std::uint32_t>{xml_a, xml_b, array}) &&
          runtime.words[record + 68u] == 0 &&
          runtime.words[record + 72u] == 0 &&
          runtime.words[record + 76u] == 0,
          "XML then integer-array cleanup");
    pal4::MonsterInfoRecord::CleanupData(record, runtime);
    Check(runtime.frees.size() == 3, "repeated record cleanup");
    pal4::MonsterInfoRecord::CleanupStrings(record, runtime);
    Check(runtime.cleaned_strings ==
          (std::vector<std::uint32_t>{record + 12u, record}),
          "reverse two-string cleanup");
}

void ManagerLoadAndTraversal() {
    Runtime runtime;
    pal4::MonsterInfoSharedTree shared;
    pal4::MonsterInfoManager first(0x8000u, shared, runtime);
    pal4::MonsterInfoManager second(0x8100u, shared, runtime);
    Check(first.Initialize() && second.Initialize() && shared.instances == 2 &&
          runtime.allocation_sizes ==
              (std::vector<std::uint32_t>{36, 36, 36, 36}) &&
          runtime.words[0x8000] == 0x844904u &&
          runtime.frees.size() == 1,
          "shared nil and separate heads");
    Check(!first.Load("arena/", "mst") &&
          runtime.opened_directory == "arena/" &&
          runtime.opened_name == "mst" &&
          runtime.words[0x8000 + 32u] == 0,
          "missing file keeps empty tree");
    runtime.stream = 0x9000;
    runtime.stream_count = 2;
    runtime.input_strings = {"beta", "actor-b", "alpha", "actor-a"};
    runtime.item_counts = {1, 0};
    Check(first.Load("arena/", "mst") && runtime.closed_stream == 0x9000 &&
          runtime.words[0x8000 + 32u] == 2 &&
          first.Find("alpha") && first.Find("beta") &&
          !first.Find("missing"), "two loaded monster records");
    const auto alpha = first.Find("alpha");
    const auto beta = first.Find("beta");
    visits.clear();
    Check(first.ForEach(Visit, 0) && visits ==
              (std::vector<std::uint32_t>{alpha, beta}),
          "name ordered traversal");
    visits.clear();
    Check(!first.ForEach(Visit, alpha) && visits.size() == 1,
          "visitor early stop");
    runtime.stream_count = 0;
    Check(first.Load("arena/", "empty") && !first.Find("alpha") &&
          runtime.words[0x8000 + 32u] == 0,
          "empty reload releases old records");
    first.Destroy();
    Check(shared.instances == 1 && shared.sentinel != 0,
          "first destructor retains nil");
    second.Destroy();
    Check(shared.instances == 0 && shared.sentinel == 0 &&
          runtime.cleaned_strings.back() == 0x8104u,
          "last destructor frees shared nil");
}

void AllocationFailures() {
    Runtime runtime;
    pal4::MonsterInfoSharedTree shared;
    pal4::MonsterInfoManager manager(0x8000u, shared, runtime);
    runtime.fail_size = 36;
    Check(!manager.Initialize() && shared.instances == 0,
          "nil allocation failure");
    runtime.fail_size = 0;
    Check(manager.Initialize(), "initialization retry");
    runtime.fail_size = 80;
    runtime.stream = 0x9000;
    runtime.stream_count = 1;
    Check(!manager.Load("arena/", "mst") &&
          runtime.closed_stream == runtime.stream &&
          runtime.words[0x8000 + 32u] == 0,
          "record allocation failure closes stream");
    manager.Destroy();
}

}  // namespace

int main() {
    try {
        RecordReadAndCleanup();
        ManagerLoadAndTraversal();
        AllocationFailures();
        std::cout << "monster_info_manager_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
