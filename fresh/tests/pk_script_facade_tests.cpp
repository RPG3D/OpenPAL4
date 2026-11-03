#include "pal4/pk_script_facade.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

struct CoreRuntime final : pal4::IScriptManagerCoreRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint8_t close_result = 1u;
    std::int32_t seek_result = -7;
    std::uint32_t insert_result = 0x5000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    std::uint32_t SharedNil() override { return 0x2000u; }
    void FreeMemory(std::uint32_t) override {}
    void FirstElement(std::uint32_t) override {}
    std::uint32_t TemporaryCursor() override { return 0; }
    std::uint32_t ReleaseMemory(std::uint32_t) override { return 0; }
    void InitializePackage(std::uint32_t) override {}
    std::uint32_t AllocateMemory(std::uint32_t) override { return 0; }
    std::uint32_t ReleasePackageBlocks(std::uint32_t) override { return 0; }
    std::string ReadCString(std::uint32_t) override { return {}; }
    std::uint8_t OpenPackage(std::uint32_t,
                             const std::string&) override { return 0; }
    std::uint8_t ClosePackage(std::uint32_t) override { return 0; }
    std::uint32_t PredecessorNode(std::uint32_t) override { return 0; }
    std::uint32_t InsertNodeAndRebalance(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t key,
        std::uint32_t value) override {
        calls.emplace_back("insert:" + std::to_string(key) + ":" +
                           std::to_string(value));
        return insert_result;
    }
    std::uint32_t DeleteNode(std::uint32_t,
                             std::uint32_t node) override {
        calls.emplace_back("delete:" + std::to_string(node));
        return 0x7777u;
    }
    std::uint8_t CloseScriptHandle(std::uint32_t,
                                   std::uint32_t handle) override {
        calls.emplace_back("close:" + std::to_string(handle));
        return close_result;
    }
    void ReadPackageData(void*, std::uint32_t bytes,
                         std::uint32_t handle) override {
        calls.emplace_back("read:" + std::to_string(bytes) + ":" +
                           std::to_string(handle));
    }
    std::int32_t SeekPackage(std::uint32_t,
                             std::uint32_t handle,
                             std::int32_t offset,
                             std::int32_t origin) override {
        calls.emplace_back("seek:" + std::to_string(handle) + ":" +
                           std::to_string(offset) + ":" +
                           std::to_string(origin));
        return seek_result;
    }
};

struct ResourceRuntime final : pal4::IPackageResourceManagerRuntime {
    std::vector<std::string> calls;
    std::uint32_t open_result = 0x8000u;
    bool HasPathRewrite() override {
        calls.emplace_back("rewrite?");
        return false;
    }
    void RewritePath(const char*, std::array<char, 256>&) override {}
    std::uint32_t OpenNativeFile(const char* path) override {
        calls.emplace_back("open:" + std::string(path));
        return open_result;
    }
    std::uint8_t NativeFileExists(const char* path) override {
        calls.emplace_back("exists:" + std::string(path));
        return 0x45u;
    }
};

struct Runtime final : pal4::IPkScriptFacadeRuntime {
    CoreRuntime core;
    ResourceRuntime resource;
    std::vector<std::string> calls;
    std::uint32_t scripts = 0x10000000u;

    Runtime() {
        const auto tree = scripts + pal4::ScriptManagerCore::kTreeOffset;
        core.words[tree + 4u] = 0x3000u;
        core.words[0x3000u] = 0x3000u;
        core.words[0x3004u] = 0x2000u;
        core.words[0x3008u] = 0x3000u;
    }
    std::uint32_t InitializeManager() override {
        calls.emplace_back("init");
        return 0xa000u;
    }
    std::uint32_t GetScriptManager(std::uint32_t manager) override {
        calls.emplace_back("get:" + std::to_string(manager));
        return scripts;
    }
    pal4::IScriptManagerCoreRuntime& Scripts() override { return core; }
    pal4::IPackageResourceManagerRuntime& Resources() override {
        return resource;
    }
    void OneScript() {
        core.words[0x3000u] = 0x5000u;
        core.words[0x3004u] = 0x5000u;
        core.words[0x3008u] = 0x5000u;
        core.words[0x5000u] = 0x2000u;
        core.words[0x5008u] = 0x2000u;
        core.words[0x500cu] = 7u;
        core.words[0x5010u] = 0x8000u;
    }
};

int failures = 0;
void Check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void TestLoadAndUnload() {
    Runtime runtime;
    Check(pal4::PkScriptFacade::LoadByName(
              99u, 7u, "scene.csb", runtime) == 1u &&
              runtime.calls == std::vector<std::string>({
                  "init", "get:40960"}) &&
              runtime.resource.calls == std::vector<std::string>({
                  "rewrite?", "open:scene.csb"}) &&
              runtime.core.calls == std::vector<std::string>({
                  "insert:7:32768"}),
          "load obtains manager, opens resource and inserts ID/handle");
    runtime.calls.clear();
    runtime.resource.calls.clear();
    runtime.core.calls.clear();
    runtime.resource.open_result = 0;
    Check(pal4::PkScriptFacade::LoadByName(
              0u, 7u, "missing.csb", runtime) == 3u &&
              runtime.core.calls.empty(),
          "missing resource returns code three without insertion");
    runtime.calls.clear();
    runtime.resource.calls.clear();
    runtime.scripts = 0;
    Check(pal4::PkScriptFacade::LoadByName(
              0u, 7u, "x.csb", runtime) == 3u &&
              runtime.resource.calls.empty() &&
              runtime.calls == std::vector<std::string>({
                  "init", "get:40960"}),
          "missing script manager gates resource open");

    Runtime loaded;
    loaded.OneScript();
    Check(pal4::PkScriptFacade::Unload(7u, loaded) == 0x7777u &&
              loaded.core.calls == std::vector<std::string>({
                  "close:32768", "delete:20480"}),
          "unload closes handle before removing tree node");
    loaded.core.calls.clear();
    loaded.core.close_result = 0;
    Check(pal4::PkScriptFacade::Unload(7u, loaded) == 0u &&
              loaded.core.calls == std::vector<std::string>({
                  "close:32768"}),
          "failed handle close retains node");
    loaded.core.calls.clear();
    Check(pal4::PkScriptFacade::Unload(8u, loaded) == 0u &&
              loaded.core.calls.empty(),
          "missing script unload does nothing");
}

void TestReadStatusSeekAndExists() {
    Runtime runtime;
    runtime.scripts = 0;
    char buffer[4]{};
    Check(pal4::PkScriptFacade::Read(
              7u, buffer, 4u, runtime) == 0u &&
              !pal4::PkScriptFacade::IsLoaded(7u, runtime) &&
              pal4::PkScriptFacade::Seek(7u, -2, 1u, runtime) == 0u &&
              pal4::PkScriptFacade::FileExists(
                  9u, "x.csb", runtime) == 0u &&
              runtime.resource.calls.empty(),
          "missing global script manager gates all four operations");

    runtime.scripts = 0x10000000u;
    runtime.OneScript();
    runtime.calls.clear();
    Check(pal4::PkScriptFacade::Read(
              7u, buffer, 4u, runtime) == 4u &&
              runtime.core.calls == std::vector<std::string>({
                  "read:4:32768"}),
          "global read forwards to rebuilt core");
    runtime.core.calls.clear();
    runtime.core.words[0x8000u + 36u] = 10u;
    runtime.core.words[0x8000u + 40u] = 9u;
    Check(pal4::PkScriptFacade::IsLoaded(7u, runtime),
          "global loaded-state uses handle position");
    Check(pal4::PkScriptFacade::Seek(7u, -2, 1u, runtime) ==
              0xfffffff9ull &&
              pal4::PkScriptFacade::Seek(7u, -2, 2u, runtime) ==
                  0xfffffff9ull &&
              pal4::PkScriptFacade::Seek(7u, -2, 3u, runtime) ==
                  0xfffffff9ull &&
              runtime.core.calls == std::vector<std::string>({
                  "seek:32768:-2:0", "seek:32768:-2:1",
                  "seek:32768:-2:2"}),
          "seek modes map to origins zero through two with zero high word");
    runtime.core.calls.clear();
    Check(pal4::PkScriptFacade::Seek(7u, 1, 0u, runtime) == 0u &&
              pal4::PkScriptFacade::Seek(7u, 1, 4u, runtime) == 0u &&
              runtime.core.calls.empty(),
          "unsupported seek modes leave result zero");
    Check(pal4::PkScriptFacade::FileExists(
              9u, "x.csb", runtime) == 0x45u &&
              runtime.resource.calls == std::vector<std::string>({
                  "rewrite?", "exists:x.csb"}),
          "existence wrapper gates on manager and delegates path query");
}

}  // namespace

int main() {
    TestLoadAndUnload();
    TestReadStatusSeekAndExists();
    return failures == 0 ? 0 : 1;
}
