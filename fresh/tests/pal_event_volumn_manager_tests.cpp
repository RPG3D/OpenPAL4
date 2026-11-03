#include "pal4/pal_event_volumn_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalEventVolumnRegistryRuntime {
    std::vector<std::uint32_t> calls;
    std::uint32_t result = 0x700u;
    std::uint32_t SingletonManager() override {
        calls.push_back(1u);
        return 0x500u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destructor) override {
        Check(manager == 0x500u && name == "palEventVolumnMgr" &&
                  id == 1012u && factory == 0x64ea50u &&
                  destructor == 0x64eab0u,
              "IDA singleton registration identity and callbacks");
        calls.push_back(2u);
        return result;
    }
};

struct Memory final : pal4::IPalEventVolumnMemory {
    std::uint32_t next_address = 0x1000u;
    bool fail_object = false;
    std::vector<std::uint32_t> sizes;
    std::vector<std::uint32_t> freed;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::PalEventVolumnState> objects;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        sizes.push_back(bytes);
        if (bytes == 52u && fail_object) return 0u;
        const auto address = next_address;
        next_address += 0x1000u;
        if (bytes == 52u) objects[address] = {};
        return address;
    }
    void Free(std::uint32_t address) override { freed.push_back(address); }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        words[address + offset] = value;
    }
    pal4::PalEventVolumnState& ObjectAt(std::uint32_t address) override {
        return objects.at(address);
    }
};

struct CleanupRuntime final : pal4::IPalEventVolumnCleanupRuntime {
    std::map<std::uint32_t, pal4::PalEventVolumnState> objects;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::PalEventVolumnState& ObjectAt(std::uint32_t address) override {
        calls.push_back("object:" + std::to_string(address));
        return objects.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void DestroyRecord(std::uint32_t address) override {
        calls.push_back("record:" + std::to_string(address));
    }
    void ClearTree(pal4::PalEventVolumnState&,
                   pal4::PalEventVolumnSharedTree&) override {
        calls.push_back("tree");
    }
    int Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return -7;
    }
};

}  // namespace

int main() {
    pal4::PalEventVolumnRegistrationState state;
    state.guard = 0x80u;
    Runtime runtime;
    Check(pal4::PalEventVolumnManager::RegisterSingleton(state, runtime) ==
              0x700u && state.guard == 0x81u && state.instance == 0x700u &&
              runtime.calls == std::vector<std::uint32_t>{1u, 2u},
          "first registration preserves guard bits and stores instance");
    runtime.calls.clear();
    Check(pal4::PalEventVolumnManager::RegisterSingleton(state, runtime) ==
              0x700u && runtime.calls.empty(),
          "second registration returns cached instance");

    pal4::PalEventVolumnRegistrationState failed;
    runtime.result = 0u;
    Check(pal4::PalEventVolumnManager::RegisterSingleton(failed, runtime) ==
              0u && failed.guard == 1u && failed.instance == 0u,
          "failed registration still sets native guard");
    runtime.calls.clear();
    Check(pal4::PalEventVolumnManager::RegisterSingleton(failed, runtime) ==
              0u && runtime.calls.empty(),
          "failed registration is not retried after guard is set");

    Memory memory;
    pal4::PalEventVolumnSharedTree shared;
    const auto first = pal4::PalEventVolumnManager::Create(shared, memory);
    Check(first == 0x1000u &&
              memory.sizes == std::vector<std::uint32_t>{52u, 24u, 24u} &&
              shared.nil == 0x2000u && shared.references == 1u &&
              memory.words[0x2000u] == 0u &&
              memory.words[0x2004u] == 0u &&
              memory.words[0x2008u] == 0u &&
              memory.words[0x2014u] == 1u &&
              memory.words[0x3000u] == 0x3000u &&
              memory.words[0x3004u] == 0x2000u &&
              memory.words[0x3008u] == 0x3000u &&
              memory.words[0x3014u] == 0u &&
              memory.objects[first].words[8u] == 0x84b490u &&
              memory.objects[first].words[10u] == 0x3000u &&
              memory.objects[first].words[6u] == 1u &&
              memory.objects[first].words[7u] == 1u &&
              memory.objects[first].words[12u] == 0u,
          "first construction establishes shared nil and instance head");

    const auto second = pal4::PalEventVolumnManager::Create(shared, memory);
    Check(second == 0x4000u && shared.nil == 0x2000u &&
              shared.references == 2u &&
              memory.freed == std::vector<std::uint32_t>{0x5000u} &&
              memory.objects[second].words[10u] == 0x6000u &&
              memory.words[0x6004u] == 0x2000u,
          "second construction reuses shared nil and frees candidate");

    Memory failing_memory;
    failing_memory.fail_object = true;
    pal4::PalEventVolumnSharedTree unused_shared;
    Check(pal4::PalEventVolumnManager::Create(
              unused_shared, failing_memory) == 0u &&
              failing_memory.sizes == std::vector<std::uint32_t>{52u} &&
              unused_shared.nil == 0u && unused_shared.references == 0u,
          "factory allocation failure skips construction");

    CleanupRuntime cleanup;
    auto& object = cleanup.objects[0x1000u];
    object.words[1u] = 0x4000u;
    object.words[2u] = 0x4008u;
    object.words[3u] = 0x4010u;
    object.words[4u] = 0x555u;
    object.words[5u] = 1u;
    object.words[10u] = 0x3000u;
    object.words[12u] = 2u;
    cleanup.words[0x4000u] = 0x5000u;
    cleanup.words[0x4004u] = 0u;
    pal4::PalEventVolumnSharedTree shared_cleanup{0x2000u, 1u};
    Check(pal4::PalEventVolumnManager::Destroy(
              0x1000u, shared_cleanup, cleanup) == -7 &&
              cleanup.calls == std::vector<std::string>{
                  "object:4096", "record:20480", "tree", "free:12288",
                  "free:8192", "free:16384", "free:4096"} &&
              shared_cleanup.nil == 0u &&
              shared_cleanup.references == 0u &&
              object.words[1u] == 0u && object.words[2u] == 0u &&
              object.words[3u] == 0u && object.words[4u] == 0u &&
              object.words[5u] == 0u && object.words[10u] == 0u &&
              object.words[12u] == 0u,
          "destroy clears records, tree, shared nil, buffer and object");

    pal4::PalEventVolumnState sibling;
    sibling.words[10u] = 0x6000u;
    pal4::PalEventVolumnSharedTree retained{0x2000u, 2u};
    CleanupRuntime sibling_runtime;
    Check(pal4::PalEventVolumnManager::Cleanup(
              sibling, retained, sibling_runtime) == -7 &&
              retained.nil == 0x2000u && retained.references == 1u &&
              sibling_runtime.calls == std::vector<std::string>{
                  "tree", "free:24576", "free:0"},
          "last shared nil remains while another instance exists");
    sibling_runtime.calls.clear();
    Check(pal4::PalEventVolumnManager::Destroy(
              0u, retained, sibling_runtime) == 0 &&
              sibling_runtime.calls.empty(),
          "null deleting wrapper performs no cleanup");
}
