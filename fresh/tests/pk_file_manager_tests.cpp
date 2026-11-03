#include "pal4/pk_file_manager.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

struct Memory final : pal4::IScriptManagerCoreRuntime {
    pal4::ScriptManagerTreeGlobals* globals = nullptr;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> packages_initialized;
    std::vector<std::uint32_t> packages_released;
    std::vector<std::uint32_t> memory_released;
    std::vector<std::uint32_t> memory_freed;
    std::vector<std::pair<std::uint32_t, std::string>> opened_packages;
    std::vector<std::uint32_t> closed_packages;
    std::uint8_t open_result = 1u;
    std::uint8_t close_result = 0x27u;
    std::size_t allocation_index = 0;

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
        memory_freed.push_back(address);
    }
    void FirstElement(std::uint32_t) override {}
    std::uint32_t TemporaryCursor() override { return 0x40000000u; }
    std::uint32_t ReleaseMemory(std::uint32_t address) override {
        memory_released.push_back(address);
        return address + 7u;
    }
    void InitializePackage(std::uint32_t address) override {
        packages_initialized.push_back(address);
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        if (allocation_index == allocations.size()) return 0;
        return allocations[allocation_index++];
    }
    std::uint32_t ReleasePackageBlocks(std::uint32_t address) override {
        packages_released.push_back(address);
        return 0;
    }
    std::string ReadCString(std::uint32_t) override { return {}; }
    std::uint8_t OpenPackage(std::uint32_t package,
                             const std::string& path) override {
        opened_packages.emplace_back(package, path);
        return open_result;
    }
    std::uint8_t ClosePackage(std::uint32_t package) override {
        closed_packages.push_back(package);
        return close_result;
    }
    std::uint32_t PredecessorNode(std::uint32_t) override { return 0; }
    std::uint32_t InsertNodeAndRebalance(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t DeleteNode(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    std::uint8_t CloseScriptHandle(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    void ReadPackageData(void*, std::uint32_t, std::uint32_t) override {}
    std::int32_t SeekPackage(
        std::uint32_t, std::uint32_t, std::int32_t,
        std::int32_t) override { return 0; }
};

struct Runtime final : pal4::IPkFileManagerRuntime {
    Memory memory;
    pal4::ScriptManagerTreeGlobals globals;
    std::uint32_t singleton_calls = 0;
    std::uint32_t register_calls = 0;
    std::string name;
    std::uint32_t priority = 0;
    std::function<std::uint32_t()> create;
    std::function<std::uint32_t(std::uint32_t)> destroy;
    std::vector<std::uint32_t> native_positions;
    std::vector<std::string> native_calls;
    std::size_t native_position_index = 0;

    Runtime() { memory.globals = &globals; }

    std::uint32_t GetSingletonManager() override {
        ++singleton_calls;
        return 0x4444u;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t singleton, const char* registered_name,
        std::uint32_t registered_priority,
        std::function<std::uint32_t()> create_callback,
        std::function<std::uint32_t(std::uint32_t)> destroy_callback) override {
        ++register_calls;
        if (singleton != 0x4444u) return 0;
        name = registered_name;
        priority = registered_priority;
        create = std::move(create_callback);
        destroy = std::move(destroy_callback);
        return 0x5555u;
    }
    pal4::IScriptManagerCoreRuntime& Scripts() override { return memory; }
    pal4::ScriptManagerTreeGlobals& TreeGlobals() override { return globals; }
    std::uint32_t NativeFilePosition(std::uint32_t file) override {
        native_calls.emplace_back("tell:" + std::to_string(file));
        return native_position_index < native_positions.size()
                   ? native_positions[native_position_index++] : 0u;
    }
    void NativeFileSeek(std::uint32_t file, std::int32_t offset,
                        std::int32_t origin) override {
        native_calls.emplace_back(
            "seek:" + std::to_string(file) + ":" +
            std::to_string(offset) + ":" + std::to_string(origin));
    }
};

int failures = 0;
void Check(bool okay, const char* message) {
    if (!okay) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void FillAllocations(Memory& memory) {
    memory.allocations.push_back(0x10000000u);
    for (std::uint32_t index = 0; index < 22u; ++index)
        memory.allocations.push_back(0x20000000u + index * 0x100u);
}

void TestLifecycle() {
    Runtime runtime;
    FillAllocations(runtime.memory);
    pal4::PkFileManager manager(runtime);
    Check(manager.Init() == 0x5555u && manager.Init() == 0x5555u,
          "singleton result cached");
    Check(runtime.singleton_calls == 1u && runtime.register_calls == 1u &&
              runtime.name == "pkFileMgr" && runtime.priority == 30u,
          "singleton registered once with callbacks");
    const auto object = runtime.create();
    Check(object == 0x10000000u, "create returns allocated object");
    Check(runtime.memory.packages_initialized.size() == 11u,
          "eleven script managers constructed");
    Check(runtime.globals.instance_count == 11u &&
              runtime.globals.shared_nil == 0x20000000u,
          "all script managers share one nil");
    for (std::uint32_t index = 0; index < 11u; ++index)
        Check(runtime.memory.packages_initialized[index] ==
                  object + pal4::PkFileManager::kFirstScriptOffset +
                      index * pal4::PkFileManager::kScriptStride + 4u,
              "script manager construction order and stride");
    Check(runtime.memory.words[object] == 0u &&
              runtime.memory.words[object + 120u] == 0u &&
              runtime.memory.bytes[object + 28u] == 0u,
          "manager head initialized");
    Check(runtime.destroy(object) == object + 7u,
          "destroy forwards object free result");
    Check(runtime.memory.packages_released.size() == 11u,
          "eleven script managers cleaned");
    Check(runtime.globals.instance_count == 0u &&
              runtime.globals.shared_nil == 0u,
          "final script manager releases shared nil");
    for (std::uint32_t index = 0; index < 11u; ++index)
        Check(runtime.memory.packages_released[index] ==
                  object + pal4::PkFileManager::kFirstScriptOffset +
                      (10u - index) * pal4::PkFileManager::kScriptStride + 4u,
              "script manager reverse cleanup order");
    Check(runtime.memory.memory_released.back() == object,
          "object released after children");
    Check(manager.Destroy(0u) == 0u, "null destroy normalized");
}

void TestAllocationFailures() {
    Runtime no_memory;
    pal4::PkFileManager absent(no_memory);
    Check(absent.Create() == 0u, "object allocation failure");

    Runtime partial;
    partial.memory.allocations = {0x10000000u, 0x20000000u, 0x20000100u,
                                  0x20000200u, 0u};
    pal4::PkFileManager manager(partial);
    Check(manager.Create() == 0u, "child allocation failure");
    Check(partial.memory.packages_initialized.size() == 2u &&
              partial.memory.packages_released.size() == 2u &&
              partial.memory.memory_released.back() == 0x10000000u,
          "partial child construction rolled back and object freed");
}

void TestPagedArrays() {
    Runtime runtime;
    FillAllocations(runtime.memory);
    pal4::PkFileManager manager(runtime);
    const auto object = manager.Create();
    auto& memory = runtime.memory;
    memory.words[object + 120u] = 1u;
    memory.words[object + 88u] = 0x5000u;
    memory.words[object + 84u] = 0x5004u;
    memory.words[object + 92u] = 0x6000u;
    memory.words[0x6000u] = 0x5000u;
    memory.words[object + 112u] = 0x6000u;
    memory.words[object + 72u] = 1u;
    memory.words[object + 40u] = 0x7000u;
    memory.words[object + 36u] = 0x7018u;
    memory.words[object + 44u] = 0x8000u;
    memory.words[0x8000u] = 0x7000u;
    memory.words[object + 64u] = 0x8000u;
    (void)manager.Cleanup(object);
    const auto& released = memory.memory_released;
    Check(released.size() >= 4u &&
              released[released.size() - 4u] == 0x5000u &&
              released[released.size() - 3u] == 0x6000u &&
              released[released.size() - 2u] == 0x7000u &&
              released[released.size() - 1u] == 0x8000u,
          "paged arrays release pages and page lists in reverse field order");
    Check(memory.words[object + 120u] == 0u &&
              memory.words[object + 72u] == 0u,
          "paged array counts cleared");
}

void TestPagedArrayCrossesPage() {
    Runtime runtime;
    FillAllocations(runtime.memory);
    pal4::PkFileManager manager(runtime);
    const auto object = manager.Create();
    auto& memory = runtime.memory;
    memory.words[object + 120u] = 2u;
    memory.words[object + 88u] = 0x9000u;
    memory.words[object + 84u] = 0x9004u;
    memory.words[object + 92u] = 0xa000u;
    memory.words[0xa000u] = 0x9000u;
    memory.words[0xa004u] = 0xb000u;
    memory.words[object + 112u] = 0xa000u;
    (void)manager.Cleanup(object);
    const auto& released = memory.memory_released;
    Check(released.size() >= 3u &&
              released[released.size() - 3u] == 0x9000u &&
              released[released.size() - 2u] == 0xb000u &&
              released[released.size() - 1u] == 0xa000u,
          "cross-page cleanup releases both pages before page list");
    Check(memory.words[object + 120u] == 0u &&
              memory.words[object + 92u] == 0u &&
              memory.words[object + 112u] == 0xa000u,
          "cross-page cursor and count cleared; freed list pointer retained");
}

void TestProcessFile() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    auto& memory = runtime.memory;
    Check(manager.ProcessFile(object, 0u) == 0u &&
              memory.words.find(object) == memory.words.end(),
          "null file leaves manager untouched");
    for (std::uint32_t index = 0; index < 20u; ++index)
        memory.words[file + index * 4u] = 0x4000u + index;
    Check(manager.ProcessFile(object, file) == 1u &&
              memory.words[object] == file &&
              memory.words[object + 4u] == 0x400au &&
              memory.words[object + 8u] == 0x400bu &&
              memory.words[object + 12u] == 0x400cu &&
              memory.words[object + 16u] == 0x4011u &&
              memory.words[object + 20u] == 0x400eu &&
              memory.words[object + 24u] == 0x4013u,
          "file header fields copied in IDA order");
    Check(memory.memory_released.empty(),
          "empty record array does not release pages");

    memory.words[object + 72u] = 1u;
    memory.words[object + 48u] = 0x90000u;
    memory.words[object + 56u] = 0x90018u;
    memory.words[object + 60u] = 0xa004u;
    memory.words[0xa004u] = 0x90000u;
    memory.words[object + 44u] = 0xa000u;
    memory.words[0xa000u] = 0x80000u;
    memory.words[object + 64u] = 0xb000u;
    Check(manager.ProcessFile(object, file) == 1u &&
              memory.memory_released ==
                  std::vector<std::uint32_t>{0x90000u, 0x80000u, 0xb000u},
          "final record releases back page, front page and map");
    Check(memory.words[object + 72u] == 0u &&
              memory.words[object + 48u] == 0u &&
              memory.words[object + 64u] == 0xb000u,
          "record array cursors clear while freed map pointer remains");
}

void TestProcessFileCrossesPage() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    auto& memory = runtime.memory;
    memory.words[object + 72u] = 2u;
    memory.words[object + 48u] = 0x9000u;
    memory.words[object + 56u] = 0x9000u;
    memory.words[object + 60u] = 0xa008u;
    memory.words[0xa008u] = 0x9000u;
    memory.words[0xa004u] = 0x8000u;
    memory.words[object + 44u] = 0xa000u;
    memory.words[0xa000u] = 0x7000u;
    memory.words[object + 64u] = 0xb000u;
    (void)manager.ProcessFile(object, file);
    Check(memory.memory_released ==
              std::vector<std::uint32_t>{0x9000u, 0x8000u,
                                         0x7000u, 0xb000u},
          "cross-page record clear follows IDA back-to-front release order");
}

void TestLoadAllScripts() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    constexpr auto object = 0x10000000u;
    auto& memory = runtime.memory;
    Check(manager.LoadAllScripts(object) == 0u &&
              memory.opened_packages.empty(),
          "unbound manager skips package loading");
    memory.words[object] = 0x20000000u;
    memory.open_result = 7u;
    Check(manager.LoadAllScripts(object) == 7u &&
              memory.opened_packages.size() == 9u,
          "nine packages load and final result is forwarded");
    const std::vector<std::string> paths{
        "gamedata\\PALActor.cpk", "gamedata\\palobject.cpk",
        "gamedata\\Script.cpk", "gamedata\\scenedata.cpk",
        "gamedata\\palweapon.cpk", "gamedata\\ui.cpk",
        "gamedata\\Effect.cpk", "gamedata\\MatFX.cpk",
        "gamedata\\2D.cpk"};
    const std::vector<std::uint32_t> loaders{
        0x670100u, 0x6702e0u, 0x670c80u, 0x6708c0u, 0x671220u,
        0x671400u, 0x6715e0u, 0x6717c0u, 0x6719a0u};
    for (std::uint32_t index = 0; index < 9u; ++index) {
        const auto child = object + pal4::PkFileManager::kFirstScriptOffset +
                           index * pal4::PkFileManager::kScriptStride;
        Check(memory.opened_packages[index] ==
                  std::pair{child + 4u, paths[index]} &&
                  memory.words[child] == loaders[index],
              "package path, child and loader callback preserve IDA order");
    }
}

void TestCleanupAndProcess() {
    Runtime runtime;
    FillAllocations(runtime.memory);
    pal4::PkFileManager manager(runtime);
    const auto object = manager.Create();
    auto& memory = runtime.memory;
    memory.words[object] = 0x30000000u;
    memory.words[object + 72u] = 1u;
    memory.words[object + 48u] = 0x90000u;
    memory.words[object + 56u] = 0x90018u;
    memory.words[object + 60u] = 0xa004u;
    memory.words[0xa004u] = 0x90000u;
    memory.words[object + 44u] = 0xa000u;
    memory.words[0xa000u] = 0x80000u;
    memory.words[object + 64u] = 0xb000u;
    memory.words[object + 120u] = 1u;
    memory.words[object + 96u] = 0xc0000u;
    memory.words[object + 104u] = 0xc0004u;
    memory.words[object + 108u] = 0xf004u;
    memory.words[0xf004u] = 0xc0000u;
    memory.words[object + 92u] = 0xf000u;
    memory.words[0xf000u] = 0xd0000u;
    memory.words[object + 112u] = 0xe000u;
    Check(manager.CleanupAndProcess(object) == 0x27u &&
              memory.words[object] == 0u &&
              memory.words[object + 72u] == 0u &&
              memory.words[object + 120u] == 0u,
          "cleanup unbinds file, clears both arrays and forwards final close");
    Check(memory.memory_released ==
              std::vector<std::uint32_t>{0x90000u, 0x80000u, 0xb000u,
                                         0xc0000u, 0xd0000u, 0xe000u},
          "record array drains before token array");
    const std::vector<std::uint32_t> order{
        0u, 1u, 2u, 3u, 4u, 9u, 6u, 7u, 8u};
    Check(memory.closed_packages.size() == order.size(),
          "nine selected script packages close");
    for (std::size_t index = 0; index < order.size(); ++index)
        Check(memory.closed_packages[index] ==
                  object + pal4::PkFileManager::kFirstScriptOffset +
                      order[index] * pal4::PkFileManager::kScriptStride + 4u,
              "script cleanup preserves the unusual IDA package order");
}

void TestInstallScriptCallbacks() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    Check(manager.InstallScriptCallbacks(object) == 0u &&
              memory.allocation_sizes.empty(),
          "unbound file skips callback installation");
    memory.words[object] = file;
    const std::vector<std::uint32_t> fields{40u, 44u, 48u, 68u, 56u, 76u};
    const std::vector<std::uint32_t> wrappers{
        0x66e9a0u, 0x66e9e0u, 0x66ea10u,
        0x66ea40u, 0x66ea60u, 0x66ead0u};
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        memory.words[file + fields[index]] = 0x7000u + index;
    memory.allocations = {0x50000u, 0x60000u};
    Check(manager.InstallScriptCallbacks(object) == file &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0xff0u, 8u} &&
              memory.words[object + 72u] == 1u,
          "first record allocates page and two-slot map");
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        Check(memory.words[0x507f8u + index * 4u] == 0x7000u + index &&
                  memory.words[file + fields[index]] == wrappers[index],
              "record saves original callbacks and installs script wrappers");
    Check(memory.words[object + 32u] == 0x50000u &&
              memory.words[object + 40u] == 0x507f8u &&
              memory.words[object + 56u] == 0x50810u &&
              memory.words[object + 60u] == 0x60004u &&
              memory.words[object + 68u] == 2u,
          "first record iterator layout");

    const auto allocation_count = memory.allocation_sizes.size();
    Check(manager.InstallScriptCallbacks(object) == file &&
              memory.words[object + 72u] == 2u &&
              memory.allocation_sizes.size() == allocation_count,
          "second record reuses current page");
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        Check(memory.words[0x50810u + index * 4u] == wrappers[index],
              "second record saves installed callbacks");
}

void TestAllocateCallbackPage() {
    Runtime runtime;
    auto& memory = runtime.memory;
    constexpr auto array = 0x1000001cu;
    memory.allocations = {0x50000u, 0x60000u, 0x70000u, 0x80000u};

    Check(pal4::PkFileManager::AllocateCallbackPage(array, memory) ==
              0x60004u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0xff0u, 8u} &&
              memory.words[0x60004u] == 0x50000u &&
              memory.words[array + 4u] == 0x50000u &&
              memory.words[array + 12u] == 0x507f8u &&
              memory.words[array + 28u] == 0x507f8u &&
              memory.words[array + 40u] == 2u,
          "first callback page returns centered two-slot map cursor");

    memory.words[array + 44u] = 1u;
    Check(pal4::PkFileManager::AllocateCallbackPage(array, memory) ==
              0x80004u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0xff0u, 8u, 0xff0u, 8u} &&
              memory.memory_released ==
                  std::vector<std::uint32_t>{0x60000u} &&
              memory.words[array + 4u] == 0x50000u &&
              memory.words[array + 12u] == 0x507f8u &&
              memory.words[array + 16u] == 0x80000u &&
              memory.words[array + 20u] == 0x70000u &&
              memory.words[array + 28u] == 0x70000u &&
              memory.words[array + 32u] == 0x80004u &&
              memory.words[0x80000u] == 0x50000u &&
              memory.words[0x80004u] == 0x70000u,
          "callback map growth retains front cursor and starts new page");
}

void TestRemoveCallbackPage() {
    constexpr auto array = 0x1000001cu;
    Runtime remaining;
    auto& live = remaining.memory;
    live.words[array + 32u] = 0x60008u;
    live.words[array + 44u] = 1u;
    live.words[0x60008u] = 0x70000u;
    live.words[0x60004u] = 0x50000u;
    Check(pal4::PkFileManager::RemoveCallbackPage(array, live) ==
              0x60004u &&
              live.memory_released ==
                  std::vector<std::uint32_t>{0x70000u} &&
              live.words[array + 20u] == 0x50000u &&
              live.words[array + 24u] == 0x50ff0u &&
              live.words[array + 28u] == 0x50ff0u &&
              live.words[array + 32u] == 0x60004u,
          "callback page removal restores previous back page");

    Runtime last;
    auto& empty = last.memory;
    empty.words[array + 16u] = 0x60004u;
    empty.words[array + 32u] = 0x60008u;
    empty.words[array + 36u] = 0x60000u;
    empty.words[array + 44u] = 0u;
    empty.words[0x60008u] = 0x70000u;
    empty.words[0x60004u] = 0x50000u;
    Check(pal4::PkFileManager::RemoveCallbackPage(array, empty) ==
              0x60007u &&
              empty.memory_released ==
                  std::vector<std::uint32_t>{
                      0x70000u, 0x50000u, 0x60000u},
          "last callback page releases both pages and map");
    for (std::uint32_t offset = 4u; offset <= 32u; offset += 4u)
        Check(empty.words[array + offset] == 0u,
              "last callback page clears both iterator blocks");
}

void TestAllocateScriptDataPage() {
    Runtime runtime;
    auto& memory = runtime.memory;
    constexpr auto array = 0x1000004cu;
    memory.allocations = {0x50000u, 0x60000u, 0x70000u, 0x80000u};
    Check(pal4::PkFileManager::AllocateScriptDataPage(array, memory) ==
              0x60004u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{4096u, 8u} &&
              memory.words[array + 8u] == 0x51000u &&
              memory.words[array + 12u] == 0x50800u &&
              memory.words[array + 28u] == 0x50800u &&
              memory.words[0x60004u] == 0x50000u,
          "first script-token page centers 4-byte records in 4096 bytes");

    memory.words[array + 44u] = 1u;
    Check(pal4::PkFileManager::AllocateScriptDataPage(array, memory) ==
              0x80004u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{4096u, 8u, 4096u, 8u} &&
              memory.memory_released ==
                  std::vector<std::uint32_t>{0x60000u} &&
              memory.words[array + 12u] == 0x50800u &&
              memory.words[array + 16u] == 0x80000u &&
              memory.words[array + 20u] == 0x70000u &&
              memory.words[array + 24u] == 0x71000u &&
              memory.words[array + 28u] == 0x70000u &&
              memory.words[0x80000u] == 0x50000u &&
              memory.words[0x80004u] == 0x70000u,
          "script-token page growth preserves front and replaces map");
}

void TestGrowPageMap() {
    Runtime runtime;
    auto& memory = runtime.memory;
    constexpr auto array = 0x1000004cu;
    memory.words[array + 16u] = 0x60004u;
    memory.words[array + 32u] = 0x60008u;
    memory.words[array + 36u] = 0x60000u;
    memory.words[array + 40u] = 4u;
    memory.words[0x60004u] = 0x50000u;
    memory.words[0x60008u] = 0x70000u;
    memory.allocations = {0x80000u};
    Check(pal4::PkFileManager::GrowPageMap(array, 6u, memory) ==
              0x80004u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{24u} &&
              memory.words[0x80004u] == 0x50000u &&
              memory.words[0x80008u] == 0x70000u &&
              memory.words[array + 36u] == 0x80000u &&
              memory.words[array + 40u] == 6u &&
              memory.memory_released ==
                  std::vector<std::uint32_t>{0x60000u},
          "page-map growth copies live slots into centered new map");

    Check(pal4::PkFileManager::GrowPageMap(array, 8u, memory) == 0u &&
              memory.words[array + 36u] == 0x80000u &&
              memory.words[array + 40u] == 6u &&
              memory.memory_released.size() == 1u,
          "page-map allocation failure preserves existing map");
}

void TestResetPageIterator() {
    Runtime runtime;
    auto& memory = runtime.memory;
    constexpr auto iterator = 0x40000u;
    for (std::uint32_t offset = 0u; offset < 20u; offset += 4u)
        memory.words[iterator + offset] = 0xdeadbeefu;
    Check(pal4::PkFileManager::ResetPageIterator(iterator, memory) ==
              iterator &&
              memory.words[iterator + 16u] == 0xdeadbeefu,
          "page iterator reset returns storage and preserves adjacent word");
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        Check(memory.words[iterator + offset] == 0u,
              "page iterator reset clears its four words");
}

void TestSetPageIterators() {
    Runtime runtime;
    auto& memory = runtime.memory;
    memory.words[0x60004u] = 0x50000u;
    Check(pal4::PkFileManager::SetCallbackPageIterator(
              0x40000u, 0x50300u, 0x60004u, memory) == 0x40000u &&
              memory.words[0x40000u] == 0x50000u &&
              memory.words[0x40004u] == 0x50ff0u &&
              memory.words[0x40008u] == 0x50300u &&
              memory.words[0x4000cu] == 0x60004u,
          "callback iterator loads page slot and 4080-byte end");
    Check(pal4::PkFileManager::SetScriptDataPageIterator(
              0x40020u, 0x50800u, 0x60004u, memory) == 0x40020u &&
              memory.words[0x40020u] == 0x50000u &&
              memory.words[0x40024u] == 0x51000u &&
              memory.words[0x40028u] == 0x50800u &&
              memory.words[0x4002cu] == 0x60004u,
          "script-token iterator loads page slot and 4096-byte end");
}

void TestInstallScriptCallbacksPageGrowth() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    memory.words[object] = file;
    memory.words[object + 72u] = 1u;
    memory.words[object + 32u] = 0x50000u;
    memory.words[object + 36u] = 0x50ff0u;
    memory.words[object + 40u] = 0x507f8u;
    memory.words[object + 44u] = 0x60004u;
    memory.words[object + 48u] = 0x50000u;
    memory.words[object + 52u] = 0x50ff0u;
    memory.words[object + 56u] = 0x50fd8u;
    memory.words[object + 60u] = 0x60004u;
    memory.words[object + 64u] = 0x60000u;
    memory.words[object + 68u] = 2u;
    memory.words[0x60004u] = 0x50000u;
    memory.allocations = {0x70000u, 0x80000u};
    Check(manager.InstallScriptCallbacks(object) == file &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0xff0u, 8u} &&
              memory.memory_released == std::vector<std::uint32_t>{0x60000u},
          "page-end insertion grows and releases old map");
    Check(memory.words[object + 44u] == 0x80000u &&
              memory.words[object + 60u] == 0x80004u &&
              memory.words[0x80000u] == 0x50000u &&
              memory.words[0x80004u] == 0x70000u &&
              memory.words[object + 56u] == 0x70000u &&
              memory.words[object + 72u] == 2u,
          "page growth recenters map and starts next back page");
}

void TestInstallScriptCallbacksAtPageBoundary() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    memory.words[object] = file;
    memory.words[object + 72u] = 1u;
    memory.words[object + 48u] = 0x50000u;
    memory.words[object + 52u] = 0x50ff0u;
    memory.words[object + 56u] = 0x50ff0u;
    memory.words[object + 60u] = 0x60004u;
    memory.words[object + 64u] = 0x60000u;
    memory.words[object + 68u] = 4u;
    memory.words[0x60004u] = 0x50000u;
    memory.allocations = {0x70000u};
    Check(manager.InstallScriptCallbacks(object) == file &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0xff0u} &&
              memory.words[0x60008u] == 0x70000u &&
              memory.words[object + 56u] == 0x70018u &&
              memory.words[object + 72u] == 2u,
          "full back page uses available map slot");
}

void TestInstallScriptCallbacksAllocationFailure() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    memory.words[object] = file;
    Check(manager.InstallScriptCallbacks(object) == 0u &&
              memory.words[object + 72u] == 0u,
          "first page allocation failure keeps count zero");
    memory.allocation_sizes.clear();
    memory.allocations = {0x50000u};
    Check(manager.InstallScriptCallbacks(object) == 0u &&
              memory.memory_released == std::vector<std::uint32_t>{0x50000u} &&
              memory.words[object + 72u] == 0u,
          "map allocation failure releases first page");

    Runtime end_runtime;
    pal4::PkFileManager end_manager(end_runtime);
    auto& end = end_runtime.memory;
    end.words[object] = file;
    end.words[object + 72u] = 1u;
    end.words[object + 52u] = 0x50ff0u;
    end.words[object + 56u] = 0x50fd8u;
    Check(end_manager.InstallScriptCallbacks(object) == 0u &&
              end.words[object + 56u] == 0x50fd8u &&
              end.words[object + 72u] == 1u,
          "next-page allocation failure restores back cursor");
}

void TestRestoreFileCallbacks() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    Check(manager.RestoreFileCallbacks(object) == 0u &&
              memory.allocation_sizes.empty(),
          "unbound file skips callback restore");
    memory.words[object] = file;
    const std::vector<std::uint32_t> fields{40u, 44u, 48u, 68u, 56u, 76u};
    for (std::uint32_t index = 0; index < fields.size(); ++index) {
        memory.words[object + 4u + index * 4u] = 0x7000u + index;
        memory.words[file + fields[index]] = 0x8000u + index;
    }
    memory.allocations = {0x50000u, 0x60000u};
    Check(manager.RestoreFileCallbacks(object) == 0x7005u &&
              memory.words[object + 72u] == 1u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0xff0u, 8u},
          "restore pushes a record and returns saved sixth callback");
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        Check(memory.words[0x507f8u + index * 4u] == 0x8000u + index &&
                  memory.words[file + fields[index]] == 0x7000u + index,
              "restore saves current callbacks and writes manager header");
}

void TestPopFileCallbacks() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    Check(manager.PopFileCallbacks(object) == 0u,
          "unbound file has no callback record");
    memory.words[object] = file;
    Check(manager.PopFileCallbacks(object) == 0u,
          "bound file with empty record array returns zero");
    const std::vector<std::uint32_t> fields{40u, 44u, 48u, 68u, 56u, 76u};
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        memory.words[file + fields[index]] = 0x7000u + index;
    memory.allocations = {0x50000u, 0x60000u};
    (void)manager.InstallScriptCallbacks(object);
    (void)manager.InstallScriptCallbacks(object);
    Check(manager.PopFileCallbacks(object) == 1u &&
              memory.words[object + 72u] == 1u &&
              memory.memory_released.empty(),
          "first pop retains shared record page");
    const std::vector<std::uint32_t> wrappers{
        0x66e9a0u, 0x66e9e0u, 0x66ea10u,
        0x66ea40u, 0x66ea60u, 0x66ead0u};
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        Check(memory.words[file + fields[index]] == wrappers[index],
              "first pop restores callbacks saved by second push");
    Check(manager.PopFileCallbacks(object) == 0x60007u &&
              memory.words[object + 72u] == 0u &&
              memory.memory_released ==
                  std::vector<std::uint32_t>{0x50000u, 0x60000u},
          "final pop releases page and map and forwards free result");
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        Check(memory.words[file + fields[index]] == 0x7000u + index,
              "final pop restores original callbacks");
}

void TestPopFileCallbacksCrossPage() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    constexpr auto file = 0x30000000u;
    memory.words[object] = file;
    memory.words[object + 72u] = 2u;
    memory.words[object + 48u] = 0x70000u;
    memory.words[object + 56u] = 0x70000u;
    memory.words[object + 60u] = 0x60008u;
    memory.words[object + 44u] = 0x60004u;
    memory.words[object + 64u] = 0x60000u;
    memory.words[0x60004u] = 0x50000u;
    memory.words[0x60008u] = 0x70000u;
    const std::vector<std::uint32_t> fields{40u, 44u, 48u, 68u, 56u, 76u};
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        memory.words[0x50fd8u + index * 4u] = 0x9000u + index;
    Check(manager.PopFileCallbacks(object) == 1u &&
              memory.memory_released == std::vector<std::uint32_t>{0x70000u} &&
              memory.words[object + 56u] == 0x50fd8u &&
              memory.words[object + 60u] == 0x60004u,
          "pop crosses empty back page to previous record page");
    for (std::uint32_t index = 0; index < fields.size(); ++index)
        Check(memory.words[file + fields[index]] == 0x9000u + index,
              "cross-page pop reads last record from prior page");
}

void TestGetLastScriptData() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    Check(manager.GetLastScriptData(object) == 0u,
          "unbound script data returns zero");
    memory.words[object] = 0x30000000u;
    Check(manager.GetLastScriptData(object) == 0u,
          "empty script data returns zero");
    memory.words[object + 120u] = 2u;
    memory.words[object + 96u] = 0x50000u;
    memory.words[object + 104u] = 0x50008u;
    memory.words[0x50004u] = 0x1234u;
    Check(manager.GetLastScriptData(object) == 0x1234u,
          "last word reads current page");
    memory.words[object + 104u] = 0x60000u;
    memory.words[object + 96u] = 0x60000u;
    memory.words[object + 108u] = 0x70008u;
    memory.words[0x70004u] = 0x50000u;
    memory.words[0x50ffcu] = 0x5678u;
    Check(manager.GetLastScriptData(object) == 0x5678u,
          "last word crosses to previous page at boundary");
}

void TestAddScriptData() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    Check(manager.AddScriptData(object, 0x1234u) == 0u &&
              memory.allocation_sizes.empty(),
          "unbound file rejects token");
    memory.words[object] = 0x30000000u;
    Check(manager.AddScriptData(object, 0u) == 0x30000000u &&
              memory.allocation_sizes.empty(),
          "zero token returns bound file without allocating");
    memory.allocations = {0x50000u, 0x60000u};
    Check(manager.AddScriptData(object, 0x1234u) == 1u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0x1000u, 8u} &&
              memory.words[0x50800u] == 0x1234u &&
              memory.words[object + 88u] == 0x50800u &&
              memory.words[object + 104u] == 0x50804u &&
              memory.words[object + 120u] == 1u,
          "first token allocates page/map and writes at midpoint");
    Check(manager.AddScriptData(object, 0x5678u) == 2u &&
              memory.allocation_sizes.size() == 2u &&
              memory.words[0x50804u] == 0x5678u &&
              manager.GetLastScriptData(object) == 0x5678u,
          "next token reuses page and getter observes final value");
}

void TestAddScriptDataPageGrowth() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    memory.words[object] = 0x30000000u;
    memory.words[object + 120u] = 1u;
    memory.words[object + 80u] = 0x50000u;
    memory.words[object + 88u] = 0x50800u;
    memory.words[object + 92u] = 0x60004u;
    memory.words[object + 96u] = 0x50000u;
    memory.words[object + 100u] = 0x51000u;
    memory.words[object + 104u] = 0x50ffcu;
    memory.words[object + 108u] = 0x60004u;
    memory.words[object + 112u] = 0x60000u;
    memory.words[object + 116u] = 2u;
    memory.words[0x60004u] = 0x50000u;
    memory.allocations = {0x70000u, 0x80000u};
    Check(manager.AddScriptData(object, 0x9999u) == 2u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0x1000u, 8u} &&
              memory.memory_released == std::vector<std::uint32_t>{0x60000u},
          "token at page end grows page map");
    Check(memory.words[0x50ffcu] == 0x9999u &&
              memory.words[0x80000u] == 0x50000u &&
              memory.words[0x80004u] == 0x70000u &&
              memory.words[object + 104u] == 0x70000u &&
              manager.GetLastScriptData(object) == 0x9999u,
          "new empty back page leaves last token on prior page");
}

void TestAddScriptDataAllocationFailure() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    memory.words[object] = 0x30000000u;
    Check(manager.AddScriptData(object, 1u) == 0u &&
              memory.words[object + 120u] == 0u,
          "first token page allocation failure");
    memory.allocations = {0x50000u};
    Check(manager.AddScriptData(object, 1u) == 0u &&
              memory.memory_released == std::vector<std::uint32_t>{0x50000u} &&
              memory.words[object + 120u] == 0u,
          "token map allocation failure frees page");
}

void TestRemoveScriptData() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    Check(manager.RemoveScriptData(object) == 0u,
          "empty token array removal returns zero");
    memory.words[object] = 0x30000000u;
    memory.allocations = {0x50000u, 0x60000u};
    (void)manager.AddScriptData(object, 0x1111u);
    (void)manager.AddScriptData(object, 0x2222u);
    memory.words[object] = 0u;
    Check(manager.RemoveScriptData(object) == 1u &&
              memory.words[object + 104u] == 0x50804u &&
              memory.memory_released.empty(),
          "token pop decrements count and back pointer without file gate");
    Check(manager.RemoveScriptData(object) == 0x60007u &&
              memory.words[object + 120u] == 0u &&
              memory.memory_released ==
                  std::vector<std::uint32_t>{0x50000u, 0x60000u},
          "last token releases page/map and returns map free result");
}

void TestRemoveScriptDataCrossPage() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    constexpr auto object = 0x10000000u;
    memory.words[object + 120u] = 2u;
    memory.words[object + 96u] = 0x70000u;
    memory.words[object + 104u] = 0x70000u;
    memory.words[object + 108u] = 0x60008u;
    memory.words[object + 92u] = 0x60004u;
    memory.words[0x60004u] = 0x50000u;
    memory.words[0x60008u] = 0x70000u;
    Check(manager.RemoveScriptData(object) == 1u &&
              memory.memory_released == std::vector<std::uint32_t>{0x70000u} &&
              memory.words[object + 104u] == 0x50ffcu &&
              memory.words[object + 108u] == 0x60004u,
          "token pop frees empty next page and steps to prior page");
}

void TestLoadStandaloneScript() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    auto& memory = runtime.memory;
    Check(manager.LoadScript("gamedata\\test.cpk", 0x1234u) == 0u &&
              memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0x1001d0u},
          "standalone script outer allocation failure");

    Runtime success;
    success.memory.allocations = {0x10000000u, 0x20000000u, 0x20000100u};
    pal4::PkFileManager success_manager(success);
    Check(success_manager.LoadScript("gamedata\\test.cpk", 0x1234u) ==
              0x10000000u &&
              success.memory.allocation_sizes ==
                  std::vector<std::uint32_t>{0x1001d0u, 24u, 24u} &&
              success.memory.opened_packages ==
                  std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x10000004u, "gamedata\\test.cpk"}} &&
              success.memory.words[0x10000000u] == 0x1234u &&
              success.globals.instance_count == 1u &&
              success.memory.memory_released.empty(),
          "standalone script construct and load retain object");

    Runtime failed_open;
    failed_open.memory.allocations = {0x10000000u, 0x20000000u,
                                      0x20000100u};
    failed_open.memory.open_result = 0u;
    pal4::PkFileManager failed_manager(failed_open);
    Check(failed_manager.LoadScript("gamedata\\missing.cpk", 0x5678u) == 0u &&
              failed_open.globals.instance_count == 0u &&
              failed_open.globals.shared_nil == 0u &&
              failed_open.memory.memory_released.back() == 0x10000000u &&
              failed_open.memory.packages_released.size() == 1u,
          "failed package load tears down tree, package and outer object");

    Runtime failed_construct;
    failed_construct.memory.allocations = {0x10000000u, 0u};
    pal4::PkFileManager construct_manager(failed_construct);
    Check(construct_manager.LoadScript("gamedata\\test.cpk", 1u) == 0u &&
              failed_construct.memory.opened_packages.empty() &&
              failed_construct.memory.memory_released.back() == 0x10000000u,
          "failed inner constructor frees outer object without loading");
}

void TestDestroyStandaloneScript() {
    Runtime runtime;
    runtime.memory.allocations = {0x10000000u, 0x20000000u, 0x20000100u};
    pal4::PkFileManager manager(runtime);
    Check(manager.DestroyScript(0u) == 0u,
          "null standalone script destroy returns zero");
    const auto script = manager.LoadScript("gamedata\\test.cpk", 0x1234u);
    Check(manager.DestroyScript(script) == script + 7u &&
              runtime.memory.closed_packages ==
                  std::vector<std::uint32_t>{script + 4u} &&
              runtime.memory.packages_released ==
                  std::vector<std::uint32_t>{script + 4u} &&
              runtime.memory.memory_released.back() == script &&
              runtime.globals.instance_count == 0u &&
              runtime.globals.shared_nil == 0u,
          "standalone destroy closes package before tree and outer release");
}

void TestAddAndProcessScript() {
    Runtime runtime;
    runtime.memory.allocations = {0x50000u, 0x60000u,
                                  0x70000u, 0x80000u};
    pal4::PkFileManager manager(runtime);
    constexpr auto file = 0x30000000u;
    runtime.memory.words[0x5555u] = file;
    runtime.memory.words[file + 40u] = 0x1111u;
    Check(manager.AddAndProcessScript(0x1234u) == file &&
              runtime.singleton_calls == 1u &&
              runtime.register_calls == 1u &&
              runtime.memory.words[0x5555u + 120u] == 1u &&
              runtime.memory.words[0x5555u + 72u] == 1u &&
              runtime.memory.words[0x50800u] == 0x1234u &&
              runtime.memory.words[0x707f8u] == 0x1111u &&
              runtime.memory.words[file + 40u] == 0x66e9a0u,
          "global wrapper adds token then installs callbacks via cached manager");
}

void TestAddPrimaryScriptAndProcess() {
    Runtime runtime;
    runtime.memory.allocations = {0x50000u, 0x60000u,
                                  0x70000u, 0x80000u};
    pal4::PkFileManager manager(runtime);
    constexpr auto file = 0x30000000u;
    runtime.memory.words[0x5555u] = file;
    Check(manager.AddPrimaryScriptAndProcess() == file &&
              runtime.memory.words[0x50800u] ==
                  0x5555u + pal4::PkFileManager::kFirstScriptOffset &&
              runtime.memory.words[0x5555u + 120u] == 1u &&
              runtime.memory.words[0x5555u + 72u] == 1u &&
              runtime.register_calls == 1u,
          "primary wrapper pushes first embedded script address");
}

void TestAddObjectScriptAndProcess() {
    Runtime runtime;
    runtime.memory.allocations = {0x50000u, 0x60000u,
                                  0x70000u, 0x80000u};
    pal4::PkFileManager manager(runtime);
    constexpr auto file = 0x30000000u;
    runtime.memory.words[0x5555u] = file;
    Check(manager.AddObjectScriptAndProcess() == file &&
              runtime.memory.words[0x50800u] ==
                  0x5555u + pal4::PkFileManager::kFirstScriptOffset +
                      pal4::PkFileManager::kScriptStride &&
              runtime.memory.words[0x5555u + 120u] == 1u &&
              runtime.memory.words[0x5555u + 72u] == 1u,
          "object wrapper pushes second embedded script address");
}

void TestRemainingEmbeddedScriptWrappers() {
    using Method = std::uint32_t (pal4::PkFileManager::*)();
    const std::array<Method, 7> wrappers{
        &pal4::PkFileManager::AddGlobalScriptAndProcess,
        &pal4::PkFileManager::AddSceneScriptAndProcess,
        &pal4::PkFileManager::AddWeaponScriptAndProcess,
        &pal4::PkFileManager::AddUiScriptAndProcess,
        &pal4::PkFileManager::AddEffectScriptAndProcess,
        &pal4::PkFileManager::AddMatFxScriptAndProcess,
        &pal4::PkFileManager::Add2dScriptAndProcess};
    for (std::uint32_t index = 0; index < wrappers.size(); ++index) {
        Runtime runtime;
        runtime.memory.allocations = {0x50000u, 0x60000u,
                                      0x70000u, 0x80000u};
        pal4::PkFileManager manager(runtime);
        constexpr auto file = 0x30000000u;
        runtime.memory.words[0x5555u] = file;
        Check((manager.*wrappers[index])() == file &&
                  runtime.memory.words[0x50800u] ==
                      0x5555u + pal4::PkFileManager::kFirstScriptOffset +
                          (index + 2u) * pal4::PkFileManager::kScriptStride &&
                  runtime.memory.words[0x5555u + 120u] == 1u &&
                  runtime.memory.words[0x5555u + 72u] == 1u,
              "each embedded wrapper pushes its indexed child address");
    }
}

void TestCleanupAllScripts() {
    Runtime runtime;
    runtime.memory.allocations = {0x50000u, 0x60000u,
                                  0x70000u, 0x80000u};
    pal4::PkFileManager manager(runtime);
    constexpr auto file = 0x30000000u;
    runtime.memory.words[0x5555u] = file;
    runtime.memory.words[file + 40u] = 0x1234u;
    (void)manager.AddPrimaryScriptAndProcess();
    Check(manager.CleanupAllScripts() == 0x80007u &&
              runtime.memory.words[0x5555u + 120u] == 0u &&
              runtime.memory.words[0x5555u + 72u] == 0u &&
              runtime.memory.words[file + 40u] == 0x1234u &&
              runtime.memory.memory_released ==
                  std::vector<std::uint32_t>{0x50000u, 0x60000u,
                                             0x70000u, 0x80000u},
          "cleanup wrapper removes token then restores callbacks");
}

void TestGetScriptSize() {
    Runtime runtime;
    pal4::PkFileManager manager(runtime);
    constexpr auto file = 0x1234u;
    runtime.native_positions = {5u, 100u};
    Check(manager.GetScriptSize(file) == 100u &&
              runtime.native_calls ==
                  std::vector<std::string>{"tell:4660", "seek:4660:0:2",
                                           "tell:4660", "seek:4660:5:0"},
          "native file size seeks to end and restores original position");

    Runtime script_runtime;
    pal4::PkFileManager script_manager(script_runtime);
    constexpr auto script = 0x20000000u;
    constexpr auto tree = script + pal4::ScriptManagerCore::kTreeOffset;
    constexpr auto header = 0x30000000u;
    constexpr auto node = 0x40000000u;
    constexpr auto handle = 0x50000000u;
    script_runtime.globals.shared_nil = 0x90000000u;
    script_runtime.memory.words[0x5555u] = 0x60000000u;
    script_runtime.memory.words[0x5555u + 120u] = 1u;
    script_runtime.memory.words[0x5555u + 96u] = 0x70000000u;
    script_runtime.memory.words[0x5555u + 104u] = 0x70000004u;
    script_runtime.memory.words[0x70000000u] = script;
    script_runtime.memory.words[tree + 4u] = header;
    script_runtime.memory.words[header + 4u] = node;
    script_runtime.memory.words[node + 12u] = file;
    script_runtime.memory.words[node] = script_runtime.globals.shared_nil;
    script_runtime.memory.words[node + 8u] =
        script_runtime.globals.shared_nil;
    script_runtime.memory.words[node + 16u] = handle;
    script_runtime.memory.words[handle + 36u] = 0x4321u;
    Check(script_manager.GetScriptSize(file) == 0x4321u &&
              script_runtime.native_calls.empty(),
          "script size reads keyed package handle length without native seek");
    Check(script_manager.GetScriptSize(file + 1u) == 0u &&
              script_runtime.native_calls.empty(),
          "missing keyed script returns zero without native fallback");
}

}  // namespace

int main() {
    TestLifecycle();
    TestAllocationFailures();
    TestPagedArrays();
    TestPagedArrayCrossesPage();
    TestProcessFile();
    TestProcessFileCrossesPage();
    TestLoadAllScripts();
    TestCleanupAndProcess();
    TestAllocateCallbackPage();
    TestRemoveCallbackPage();
    TestAllocateScriptDataPage();
    TestGrowPageMap();
    TestResetPageIterator();
    TestSetPageIterators();
    TestInstallScriptCallbacks();
    TestInstallScriptCallbacksPageGrowth();
    TestInstallScriptCallbacksAtPageBoundary();
    TestInstallScriptCallbacksAllocationFailure();
    TestRestoreFileCallbacks();
    TestPopFileCallbacks();
    TestPopFileCallbacksCrossPage();
    TestGetLastScriptData();
    TestAddScriptData();
    TestAddScriptDataPageGrowth();
    TestAddScriptDataAllocationFailure();
    TestRemoveScriptData();
    TestRemoveScriptDataCrossPage();
    TestLoadStandaloneScript();
    TestDestroyStandaloneScript();
    TestAddAndProcessScript();
    TestAddPrimaryScriptAndProcess();
    TestAddObjectScriptAndProcess();
    TestRemainingEmbeddedScriptWrappers();
    TestCleanupAllScripts();
    TestGetScriptSize();
    return failures == 0 ? 0 : 1;
}
