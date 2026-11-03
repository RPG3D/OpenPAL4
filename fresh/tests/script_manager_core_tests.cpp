#include "pal4/script_manager_core.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IScriptManagerCoreRuntime {
    pal4::ScriptManagerTreeGlobals* globals = nullptr;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::uint32_t> allocations;
    std::size_t allocation_index = 0;
    std::vector<std::string> calls;
    std::uint8_t open_result = 1;
    std::uint8_t close_result = 9;
    std::uint32_t predecessor_result = 0;
    std::uint32_t insert_result = 0x9000u;
    std::uint32_t delete_result = 0x7000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return globals->shared_nil; }
    void FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    void FirstElement(std::uint32_t tree) override {
        calls.emplace_back("first:" + std::to_string(tree));
    }
    std::uint32_t TemporaryCursor() override { return 0x7000u; }
    std::uint32_t ReleaseMemory(std::uint32_t address) override {
        calls.emplace_back("release:" + std::to_string(address));
        return address + 1u;
    }
    void InitializePackage(std::uint32_t package) override {
        calls.emplace_back("init-package:" + std::to_string(package));
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes_requested) override {
        if (bytes_requested != 24u || allocation_index >= allocations.size())
            return 0;
        const auto value = allocations[allocation_index++];
        calls.emplace_back("allocate:" + std::to_string(value));
        return value;
    }
    std::uint32_t ReleasePackageBlocks(std::uint32_t package) override {
        calls.emplace_back("release-package:" + std::to_string(package));
        return 0x55u;
    }
    std::string ReadCString(std::uint32_t address) override {
        return strings[address];
    }
    std::uint8_t OpenPackage(std::uint32_t package,
                             const std::string& path) override {
        calls.emplace_back("open:" + std::to_string(package) + ":" + path);
        return open_result;
    }
    std::uint8_t ClosePackage(std::uint32_t package) override {
        calls.emplace_back("close:" + std::to_string(package));
        return close_result;
    }
    std::uint32_t PredecessorNode(std::uint32_t node) override {
        calls.emplace_back("predecessor:" + std::to_string(node));
        return predecessor_result;
    }
    std::uint32_t InsertNodeAndRebalance(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key,
        std::uint32_t value) override {
        calls.emplace_back("insert:" + std::to_string(tree) + ":" +
                           std::to_string(child) + ":" +
                           std::to_string(parent) + ":" +
                           std::to_string(key) + ":" +
                           std::to_string(value));
        return insert_result;
    }
    std::uint32_t DeleteNode(std::uint32_t tree,
                             std::uint32_t node) override {
        calls.emplace_back("delete:" + std::to_string(tree) + ":" +
                           std::to_string(node));
        return delete_result;
    }
    std::uint8_t CloseScriptHandle(std::uint32_t package,
                                   std::uint32_t handle) override {
        calls.emplace_back("close-handle:" + std::to_string(package) +
                           ":" + std::to_string(handle));
        return 0x23u;
    }
    void ReadPackageData(void* buffer, std::uint32_t count,
                         std::uint32_t handle) override {
        calls.emplace_back("read-data:" + std::to_string(count) + ":" +
                           std::to_string(handle) + ":" +
                           (buffer != nullptr ? "buffer" : "null"));
    }
    std::int32_t SeekPackage(std::uint32_t package,
                             std::uint32_t handle,
                             std::int32_t offset,
                             std::int32_t origin) override {
        calls.emplace_back("seek:" + std::to_string(package) + ":" +
                           std::to_string(handle) + ":" +
                           std::to_string(offset) + ":" +
                           std::to_string(origin));
        return -7;
    }
};

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void TestConstructAndDestruct() {
    pal4::ScriptManagerTreeGlobals globals{};
    Runtime runtime;
    runtime.globals = &globals;
    runtime.allocations = {0x2000u, 0x3000u, 0x4000u, 0x5000u};
    constexpr auto first = 0x10000000u;
    constexpr auto second = 0x20000000u;
    const auto tree1 = first + pal4::ScriptManagerCore::kTreeOffset;
    const auto tree2 = second + pal4::ScriptManagerCore::kTreeOffset;
    runtime.bytes[tree1] = 0xccu;
    runtime.bytes[tree1 + 1u] = 0xddu;

    Check(pal4::ScriptManagerCore::Construct(first, globals, runtime) == first &&
              globals.shared_nil == 0x2000u && globals.instance_count == 1u,
          "first manager creates shared nil and returns object");
    Check(runtime.words[0x2000u] == 0u &&
              runtime.words[0x2004u] == 0u &&
              runtime.words[0x2008u] == 0u &&
              runtime.words[0x2014u] == 1u,
          "shared nil fields match IDA");
    Check(runtime.words[tree1 + 4u] == 0x3000u &&
              runtime.words[tree1 + 12u] == 0u &&
              runtime.words[0x3000u] == 0x3000u &&
              runtime.words[0x3004u] == 0x2000u &&
              runtime.words[0x3008u] == 0x3000u &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[first] == 0u &&
              runtime.bytes[tree1 + 8u] == 0u &&
              runtime.bytes[tree1] == 0xccu &&
              runtime.bytes[tree1 + 1u] == 0xddu,
          "constructor initializes header and preserves undefined flag bytes");
    Check(pal4::ScriptManagerCore::Construct(second, globals, runtime) ==
              second && globals.shared_nil == 0x2000u &&
              globals.instance_count == 2u &&
              runtime.words[tree2 + 4u] == 0x5000u,
          "second manager shares nil with independent header");
    Check(runtime.calls == std::vector<std::string>({
              "init-package:268435460", "allocate:8192", "allocate:12288",
              "init-package:536870916", "allocate:16384", "free:16384",
              "allocate:20480"}),
          "constructor preserves package and allocation order");

    runtime.calls.clear();
    Check(pal4::ScriptManagerCore::Destruct(first, globals, runtime) == 0x55u &&
              globals.instance_count == 1u &&
              globals.shared_nil == 0x2000u &&
              runtime.words[tree1 + 4u] == 0u,
          "first destruction retains shared nil");
    Check(runtime.calls == std::vector<std::string>({
              "release:12288", "release-package:268435460"}),
          "first destruction releases header before package blocks");
    runtime.calls.clear();
    Check(pal4::ScriptManagerCore::Destruct(second, globals, runtime) ==
              0x55u && globals.instance_count == 0u &&
              globals.shared_nil == 0u,
          "last destruction releases shared nil");
    Check(runtime.calls == std::vector<std::string>({
              "release:20480", "release:8192",
              "release-package:536870916"}),
          "last destruction releases header, nil and package in order");
}

void TestLoadAndCleanupPackage() {
    pal4::ScriptManagerTreeGlobals globals{1u, 0x2000u};
    Runtime runtime;
    runtime.globals = &globals;
    constexpr auto object = 0x10000000u;
    const auto tree = object + pal4::ScriptManagerCore::kTreeOffset;
    runtime.words[tree + 4u] = 0x3000u;
    runtime.words[tree + 12u] = 1u;
    runtime.words[0x3000u] = 0x6000u;
    runtime.words[0x3004u] = 0x6000u;
    runtime.words[0x3008u] = 0x6000u;
    runtime.words[0x6000u] = 0x2000u;
    runtime.words[0x6008u] = 0x2000u;
    runtime.bytes[object + pal4::ScriptManagerCore::kPackageOpenOffset] = 1u;
    runtime.strings[object + pal4::ScriptManagerCore::kPackagePathOffset] =
        "first.pak";
    runtime.words[object] = 0x77u;

    Check(pal4::ScriptManagerCore::Load(
              object, "first.pak", 0x88u, runtime) == 1u &&
              runtime.words[object] == 0x77u && runtime.calls.empty(),
          "same open package returns early without changing options");
    Check(pal4::ScriptManagerCore::Load(
              object, "second.pak", 0x88u, runtime) == 1u &&
              runtime.words[object] == 0x88u &&
              runtime.words[tree + 12u] == 0u &&
              runtime.words[0x3000u] == 0x3000u &&
              runtime.words[0x3004u] == 0x2000u &&
              runtime.words[0x3008u] == 0x3000u,
          "different package clears tree before open and replaces options");
    Check(runtime.calls == std::vector<std::string>({
              "free:24576", "close:268435460",
              "open:268435460:second.pak"}),
          "package switch clears nodes before close and open");
    runtime.calls.clear();
    runtime.bytes[object + pal4::ScriptManagerCore::kPackageOpenOffset] = 0u;
    runtime.open_result = 0;
    Check(pal4::ScriptManagerCore::Load(
              object, "third.pak", 0x99u, runtime) == 0u &&
              runtime.words[object] == 0x99u &&
              runtime.calls == std::vector<std::string>({
                  "open:268435460:third.pak"}),
          "open failure still stores requested options");
    runtime.calls.clear();
    Check(pal4::ScriptManagerCore::CleanupPackage(object, runtime) == 9u &&
              runtime.calls == std::vector<std::string>({
                  "close:268435460"}),
          "empty cleanup closes package without node destruction");
    runtime.calls.clear();
    Check(pal4::ScriptManagerCore::Load(
              object, nullptr, 1u, runtime) == 0u &&
              runtime.calls.empty(), "null package path is rejected safely");
}

void TestAllocationFailures() {
    pal4::ScriptManagerTreeGlobals globals{};
    Runtime runtime;
    runtime.globals = &globals;
    constexpr auto object = 0x10000000u;
    Check(pal4::ScriptManagerCore::Construct(object, globals, runtime) == 0u &&
              globals.instance_count == 0u && globals.shared_nil == 0u &&
              runtime.calls == std::vector<std::string>({
                  "init-package:268435460", "release-package:268435460"}),
          "nil allocation failure releases initialized package");

    runtime.calls.clear();
    runtime.allocations = {0x2000u};
    runtime.allocation_index = 0;
    Check(pal4::ScriptManagerCore::Construct(object, globals, runtime) == 0u &&
              globals.instance_count == 0u && globals.shared_nil == 0u &&
              runtime.calls == std::vector<std::string>({
                  "init-package:268435460", "allocate:8192",
                  "release:8192", "release-package:268435460"}),
          "header allocation failure rolls back new shared nil");

    runtime.calls.clear();
    runtime.allocations = {0x3000u};
    runtime.allocation_index = 0;
    globals = {2u, 0x2000u};
    Check(pal4::ScriptManagerCore::Construct(object, globals, runtime) == 0u &&
              globals.instance_count == 2u && globals.shared_nil == 0x2000u &&
              runtime.calls == std::vector<std::string>({
                  "init-package:268435460", "allocate:12288",
                  "free:12288", "release-package:268435460"}),
          "header failure retains another manager's shared nil");
}

void TestScriptIndex() {
    pal4::ScriptManagerTreeGlobals globals{1u, 0x2000u};
    Runtime runtime;
    runtime.globals = &globals;
    constexpr auto object = 0x10000000u;
    const auto tree = object + pal4::ScriptManagerCore::kTreeOffset;
    runtime.words[tree + 4u] = 0x3000u;
    runtime.words[0x3000u] = 0x4000u;
    runtime.words[0x3004u] = 0x5000u;
    runtime.words[0x3008u] = 0x5000u;
    runtime.words[0x5000u] = 0x4000u;
    runtime.words[0x5008u] = 0x2000u;
    runtime.words[0x500cu] = 0x80000000u;
    runtime.words[0x5010u] = 0x99u;
    runtime.words[0x4000u] = 0x2000u;
    runtime.words[0x4008u] = 0x2000u;
    runtime.words[0x400cu] = 1u;
    runtime.words[0x4010u] = 0x42u;

    Check(pal4::ScriptManagerCore::GetScript(
              object, 0x80000000u, runtime) == 0x99u &&
              pal4::ScriptManagerCore::GetScript(object, 1u, runtime) ==
                  0x42u,
          "script lookup uses unsigned keys and returns node payload");
    Check(pal4::ScriptManagerCore::GetScript(
              object, 0x7fffffffu, runtime) == 0u,
          "missing key below high-bit node is absent");
    Check(pal4::ScriptManagerCore::InsertScript(
              object, 1u, 0x55u, runtime) == 0x4000u &&
              runtime.calls.empty(),
          "existing key returns node without reinsertion");

    Check(pal4::ScriptManagerCore::InsertScript(
              object, 0x7fffffffu, 0x55u, runtime) == 0x9000u &&
              runtime.calls == std::vector<std::string>({
                  "insert:" + std::to_string(tree) +
                  ":8192:16384:2147483647:85"}),
          "new key inserts after unsigned predecessor");
    runtime.calls.clear();
    Check(pal4::ScriptManagerCore::InsertScript(
              object, 0u, 0x55u, runtime) == 0x9000u &&
              runtime.calls == std::vector<std::string>({
                  "insert:" + std::to_string(tree) +
                  ":8192:16384:0:85"}),
          "new minimum uses header-left fast path");
    runtime.calls.clear();
    runtime.bytes[tree + 8u] = 1u;
    Check(pal4::ScriptManagerCore::InsertScript(
              object, 0x90000000u, 0x66u, runtime) == 0x9000u &&
              runtime.calls == std::vector<std::string>({
                  "insert:" + std::to_string(tree) +
                  ":8192:20480:2415919104:102"}),
          "multi-insert flag forwards unmatched high-bit key");
    runtime.calls.clear();
    Check(pal4::ScriptManagerCore::RemoveScript(
              object, 1u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::string>({
                  "delete:" + std::to_string(tree) + ":16384"}),
          "remove delegates exact node deletion");
    runtime.calls.clear();
    Check(pal4::ScriptManagerCore::RemoveScript(
              object, 2u, runtime) == 0x3000u &&
              runtime.calls.empty(),
          "remove returns header for absent key");

    runtime.calls.clear();
    runtime.bytes[tree + 8u] = 0u;
    runtime.words[0x5008u] = 0x6000u;
    runtime.words[0x6000u] = 0x2000u;
    runtime.words[0x6008u] = 0x2000u;
    runtime.words[0x600cu] = 0xf0000000u;
    runtime.predecessor_result = 0x5000u;
    Check(pal4::ScriptManagerCore::InsertScript(
              object, 0xa0000000u, 0x77u, runtime) == 0x9000u &&
              runtime.calls == std::vector<std::string>({
                  "predecessor:24576",
                  "insert:" + std::to_string(tree) +
                  ":8192:24576:2684354560:119"}),
          "left insertion consults predecessor before rebalancing");
}

void TestScriptHandleOperations() {
    pal4::ScriptManagerTreeGlobals globals{1u, 0x2000u};
    Runtime runtime;
    runtime.globals = &globals;
    constexpr auto object = 0x10000000u;
    const auto tree = object + pal4::ScriptManagerCore::kTreeOffset;
    runtime.words[tree + 4u] = 0x3000u;
    runtime.words[0x3004u] = 0x5000u;
    runtime.words[0x5000u] = 0x2000u;
    runtime.words[0x5008u] = 0x2000u;
    runtime.words[0x500cu] = 7u;
    runtime.words[0x5010u] = 0x8000u;

    Check(pal4::ScriptManagerCore::CloseScript(
              object, 8u, runtime) == 0u && runtime.calls.empty(),
          "missing script close does not touch package");
    Check(pal4::ScriptManagerCore::CloseScript(
              object, 7u, runtime) == 0x23u &&
              runtime.calls == std::vector<std::string>({
                  "close-handle:268435460:32768"}),
          "script close forwards package and handle");
    runtime.calls.clear();
    char buffer[4]{};
    Check(pal4::ScriptManagerCore::ReadScript(
              object, 8u, buffer, 4u, runtime) == 0u &&
              runtime.calls.empty(),
          "missing script read returns zero");
    Check(pal4::ScriptManagerCore::ReadScript(
              object, 7u, buffer, 4u, runtime) == 4u &&
              runtime.calls == std::vector<std::string>({
                  "read-data:4:32768:buffer"}),
          "script read returns requested bytes after native call");
    runtime.calls.clear();
    runtime.words[0x8000u + 36u] = 10u;
    runtime.words[0x8000u + 40u] = 8u;
    Check(!pal4::ScriptManagerCore::IsScriptLoaded(
              object, 8u, runtime) &&
              !pal4::ScriptManagerCore::IsScriptLoaded(
                  object, 7u, runtime),
          "missing or incomplete script reports false");
    runtime.words[0x8000u + 40u] = 9u;
    Check(pal4::ScriptManagerCore::IsScriptLoaded(
              object, 7u, runtime),
          "position at size minus one reports loaded");
    runtime.words[0x8000u + 36u] = 0u;
    runtime.words[0x8000u + 40u] = 0u;
    Check(!pal4::ScriptManagerCore::IsScriptLoaded(
              object, 7u, runtime),
          "zero length retains unsigned underflow behavior");
    Check(pal4::ScriptManagerCore::SeekScript(
              object, 8u, -2, 1, runtime) == 0 && runtime.calls.empty(),
          "missing script seek is gated");
    Check(pal4::ScriptManagerCore::SeekScript(
              object, 7u, -2, 1, runtime) == -7 &&
              runtime.calls == std::vector<std::string>({
                  "seek:268435460:32768:-2:1"}),
          "script seek forwards all native arguments and result");
}

}  // namespace

int main() {
    TestConstructAndDestruct();
    TestLoadAndCleanupPackage();
    TestAllocationFailures();
    TestScriptIndex();
    TestScriptHandleOperations();
    return failures == 0 ? 0 : 1;
}
