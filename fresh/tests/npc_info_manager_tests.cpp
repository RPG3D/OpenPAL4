#include "pal4/npc_info_manager.h"

#include <algorithm>
#include <array>
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
    std::uint32_t count = 0;
    std::uint32_t kind = 0;
    std::uint32_t locale = 17;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> input_names;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> cleaned_strings;
    std::vector<std::uint32_t> destroyed_xml;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> checked_xml;
    std::vector<std::uint32_t> created_facets;
    std::vector<std::uint32_t> destroyed_facets;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> reads;
    std::string opened_directory;
    std::string opened_name;
    std::uint32_t closed_stream = 0;

    std::uint32_t Allocate(const std::uint32_t size) override {
        allocations.push_back(size);
        if (size == fail_size) return 0;
        const auto result = next;
        next += size + 0x100;
        return result;
    }
    void Free(const std::uint32_t address) override { frees.push_back(address); }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
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
        if (!input_names.empty()) {
            strings[address] = input_names.front();
            input_names.erase(input_names.begin());
        }
    }
    void ReadStreamData(std::uint32_t, const std::uint32_t address,
                        const std::uint32_t bytes) override {
        reads.emplace_back(address, bytes);
    }
    std::uint32_t ReadStreamWord(std::uint32_t) override { return count; }
    std::uint32_t StreamKind(std::uint32_t) override { return kind; }
    std::uint32_t StreamLocale(std::uint32_t) override { return locale; }
    std::uint32_t CreateLocaleFacet(const std::uint32_t source) override {
        created_facets.push_back(source);
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
                                std::uint32_t) override { return storage; }
    void DestroyXml(const std::uint32_t document) override {
        destroyed_xml.push_back(document);
    }
    std::uint32_t OpenNpcInfo(const std::string_view directory,
                              const std::string_view name) override {
        opened_directory = directory;
        opened_name = name;
        return stream;
    }
    void CloseStream(const std::uint32_t value) override { closed_stream = value; }
};

std::vector<std::uint32_t> visited;
bool Visit(const std::uint32_t record, const std::uint32_t context) {
    visited.push_back(record);
    return context != record;
}

void RecordLifecycle() {
    Runtime runtime;
    constexpr auto record = 0x7000u;
    pal4::NpcInfoRecord::Initialize(record, runtime);
    Check(runtime.words[record + 76] == 0 && runtime.words[record + 80] == 0 &&
          runtime.strings.contains(record) &&
          runtime.strings.contains(record + 12) &&
          runtime.strings.contains(record + 24), "record constructor layout");
    pal4::NpcInfoRecord::InitializeDocuments(record, runtime);
    Check(runtime.allocations == (std::vector<std::uint32_t>{60, 60}) &&
          runtime.words[record + 76] != 0 &&
          runtime.words[record + 80] != 0, "two XML documents");
    runtime.kind = 2;
    pal4::NpcInfoRecord::Read(record, 0x9000, runtime);
    Check(runtime.reads == (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
              {record + 36, 12}, {record + 48, 12},
              {record + 60, 4}, {record + 64, 4}, {record + 68, 4}}) &&
          runtime.checked_xml.size() == 2 &&
          runtime.checked_xml[0].first == runtime.locale + 1 &&
          runtime.created_facets == (std::vector<std::uint32_t>{17}) &&
          runtime.destroyed_facets == (std::vector<std::uint32_t>{18}),
          "stream fields and XML check");
    const auto first = runtime.words[record + 76];
    const auto second = runtime.words[record + 80];
    pal4::NpcInfoRecord::CleanupDocuments(record, runtime);
    Check(runtime.destroyed_xml == (std::vector<std::uint32_t>{first, second}) &&
          runtime.words[record + 76] == 0 && runtime.words[record + 80] == 0,
          "XML cleanup order and nulling");
    pal4::NpcInfoRecord::CleanupDocuments(record, runtime);
    Check(runtime.destroyed_xml.size() == 2, "repeated XML cleanup");
    pal4::NpcInfoRecord::CleanupStrings(record, runtime);
    Check(runtime.cleaned_strings == (std::vector<std::uint32_t>{
              record + 24, record + 12, record}), "reverse text cleanup");
}

void TreeLifecycleAndLookup() {
    Runtime runtime;
    pal4::NpcInfoSharedTree shared;
    pal4::NpcInfoManager first(0x8000, shared, runtime);
    pal4::NpcInfoManager second(0x8100, shared, runtime);
    Check(first.Initialize() && second.Initialize() && shared.instances == 2 &&
          runtime.allocations == (std::vector<std::uint32_t>{36, 36, 36, 36}) &&
          runtime.frees.size() == 1 &&
          runtime.words[0x8000] == 0x844900, "shared sentinel and tree heads");
    const auto record = first.CreateOrGet("npc_a", "OM05",
                                          {1, 2, 3}, {4, 5, 6});
    Check(record && runtime.allocations[4] == 84 &&
          runtime.allocations[5] == 60 && runtime.allocations[6] == 60 &&
          runtime.allocations[7] == 36 &&
          runtime.strings[record] == "npc_a" &&
          runtime.strings[record + 12] == "OM05" &&
          runtime.words[record + 36] == 1 &&
          runtime.words[record + 56] == 6 &&
          runtime.words[record + 60] == 1 &&
          runtime.words[record + 64] == 1 &&
          runtime.words[0x8000 + 32] == 1, "new NPC fields and tree node");
    const auto prior_allocations = runtime.allocations.size();
    Check(first.CreateOrGet("npc_a", "ignored", {0, 0, 0}, {0, 0, 0}) == record &&
          runtime.allocations.size() == prior_allocations,
          "duplicate name returns existing record");
    Check(first.Find("npc_a") == record && first.Find("missing") == 0,
          "lookup hit and miss");
    visited.clear();
    Check(first.ForEach(Visit, 0) && visited ==
          (std::vector<std::uint32_t>{record}), "tree traversal");
    visited.clear();
    Check(!first.ForEach(Visit, record) && visited.size() == 1,
          "visitor early stop");
    Check(!first.Remove("missing") && runtime.words[0x8000 + 16] == 0 &&
          first.Remove("npc_a") && runtime.words[0x8000 + 32] == 0 &&
          !first.Remove("npc_a"), "remove hit, miss and repeat");
    first.Destroy();
    Check(shared.instances == 1 && shared.sentinel != 0,
          "first destructor retains shared sentinel");
    second.Destroy();
    Check(shared.instances == 0 && shared.sentinel == 0 &&
          runtime.cleaned_strings.back() == 0x8104,
          "final destructor releases sentinel and manager text");
}

void StreamLoadAndFailures() {
    Runtime runtime;
    pal4::NpcInfoSharedTree shared;
    pal4::NpcInfoManager manager(0x8000, shared, runtime);
    Check(manager.Initialize(), "manager initialization");
    Check(!manager.Load("scene/", "npc") &&
          runtime.opened_directory == "scene/" &&
          runtime.opened_name == "npc", "missing file keeps tree");
    runtime.stream = 0x9000;
    runtime.count = 2;
    runtime.kind = 1;
    runtime.input_names = {"beta", "res-b", "unused", "alpha", "res-a", "unused"};
    Check(manager.Load("scene/", "npc") && runtime.closed_stream == 0x9000 &&
          manager.Find("alpha") && manager.Find("beta") &&
          runtime.checked_xml.size() == 4 &&
          runtime.words[0x8000 + 32] == 2, "two loaded NPC records");
    const auto alpha = manager.Find("alpha");
    const auto beta = manager.Find("beta");
    visited.clear();
    Check(manager.ForEach(Visit, 0) && visited.size() == 2 &&
          visited[0] == manager.Find("alpha"), "name ordered traversal");
    runtime.count = 0;
    Check(manager.Load("scene/", "empty") && !manager.Find("alpha") &&
          runtime.words[0x8000 + 32] == 0,
          "successful empty load clears prior records");
    const auto alpha_free = std::find(runtime.frees.begin(), runtime.frees.end(), alpha);
    const auto beta_free = std::find(runtime.frees.begin(), runtime.frees.end(), beta);
    Check(alpha_free != runtime.frees.end() && beta_free != runtime.frees.end() &&
          std::distance(runtime.frees.begin(), beta_free) ==
              std::distance(runtime.frees.begin(), alpha_free) + 3,
          "all XML and record storage released before tree nodes");
    manager.Destroy();

    Runtime failure;
    pal4::NpcInfoSharedTree failed_shared;
    failure.fail_size = 36;
    pal4::NpcInfoManager failed(0x8100, failed_shared, failure);
    Check(!failed.Initialize() && failed_shared.instances == 0,
          "sentinel allocation failure");
    failure.fail_size = 0;
    Check(failed.Initialize(), "retry initialization");
    failure.fail_size = 84;
    Check(!failed.CreateOrGet("x", "y", {0, 0, 0}, {0, 0, 0}) &&
          failure.words[0x8100 + 32] == 0,
          "record allocation failure");
    failed.Destroy();
}

}  // namespace

int main() {
    try {
        RecordLifecycle();
        TreeLifecycleAndLookup();
        StreamLoadAndFailures();
        std::cout << "npc_info_manager_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
