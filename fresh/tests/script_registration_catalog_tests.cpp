#include "pal4/script_registration_catalog.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptRegistrationCatalogRuntime {
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> pointers;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t next_allocation{0x10000u};

    void RegisterAtExit(std::uint32_t callback) override {
        events.push_back("atexit:" + std::to_string(callback));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        const auto address = next_allocation;
        next_allocation += size + 0x100u;
        events.push_back("alloc:" + std::to_string(size));
        return address;
    }
    std::uint32_t ReadPointer(std::uint32_t address) override {
        return pointers.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WritePointer(std::uint32_t address, std::uint32_t value) override {
        pointers[address] = value;
    }
    void Descriptor(std::uint32_t address, std::uint32_t name_address,
                    const char* name) {
        pointers[address + 8u] = name_address;
        do {
            bytes[name_address++] = static_cast<std::uint8_t>(*name);
        } while (*name++);
    }
};

using Kind = pal4::ScriptRegistrationKind;
using Catalog = pal4::ScriptRegistrationCatalog;

void TestLazyInitialization() {
    Catalog catalog;
    Runtime runtime;
    constexpr std::array kinds{Kind::GlobalFunctions,
                               Kind::GlobalVariables, Kind::ObjectTypes};
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        catalog.SetGuard(kinds[i], 0x80u);
        auto& queue = catalog.GetOrInit(kinds[i],
                                         static_cast<std::uint8_t>(0x40u + i),
                                         runtime);
        Check(queue.seed_byte == 0x40u + i && queue.begin == 0u &&
                  queue.end == 0u && queue.capacity_end == 0u,
              "first getter initializes queue fields");
        Check(catalog.Guard(kinds[i]) == 0x81u,
              "getter preserves unrelated guard bits");
        queue.begin = 0x1000u + static_cast<std::uint32_t>(i);
        queue.end = queue.begin + 4u;
        queue.capacity_end = queue.begin + 16u;
        Check(&catalog.GetOrInit(kinds[i], 0xeeu, runtime) == &queue,
              "repeat getter returns same queue");
        Check(queue.seed_byte == 0x40u + i &&
                  queue.end == queue.begin + 4u,
              "repeat getter retains queue state");
    }
    Check(runtime.events == std::vector<std::string>{
              "atexit:8257824", "atexit:8257936", "atexit:8258048"},
          "cleanup callbacks register once in category order");
}

void TestCleanup() {
    Catalog catalog;
    Runtime runtime;
    auto& functions = catalog.GetOrInit(Kind::GlobalFunctions, 1u, runtime);
    auto& variables = catalog.GetOrInit(Kind::GlobalVariables, 2u, runtime);
    auto& types = catalog.GetOrInit(Kind::ObjectTypes, 3u, runtime);
    functions.begin = 0x1000u;
    functions.end = 0x1008u;
    functions.capacity_end = 0x1020u;
    variables.begin = 0x2000u;
    variables.end = 0x2004u;
    variables.capacity_end = 0x2010u;
    types.begin = 0x3000u;
    types.end = 0x3004u;
    types.capacity_end = 0x3010u;
    catalog.Cleanup(Kind::GlobalFunctions, runtime);
    Check(functions.begin == 0u && functions.end == 0u &&
              functions.capacity_end == 0u && functions.seed_byte == 1u,
          "cleanup clears only vector pointers");
    Check(variables.begin == 0x2000u && variables.end == 0x2004u,
          "cleanup leaves other registries alone");
    Check(catalog.Guard(Kind::GlobalFunctions) == 1u,
          "cleanup leaves initialization guard set");
    catalog.Cleanup(Kind::GlobalFunctions, runtime);
    Check(runtime.events.back() == "free:0", "cleanup calls free for null begin");
    Check(runtime.events[runtime.events.size() - 2] == "free:4096",
          "cleanup frees old begin before clearing it");
    catalog.Cleanup(Kind::GlobalVariables, runtime);
    catalog.Cleanup(Kind::ObjectTypes, runtime);
    Check(runtime.events[runtime.events.size() - 2] == "free:8192" &&
              runtime.events.back() == "free:12288",
          "each cleanup frees its own allocation");
    Check(variables.begin == 0u && variables.end == 0u &&
              variables.capacity_end == 0u && types.begin == 0u &&
              types.end == 0u && types.capacity_end == 0u,
          "all cleanup callbacks clear their own pointers");
    Check(&catalog.GetOrInit(Kind::ObjectTypes, 0xffu, runtime) == &types &&
              types.seed_byte == 3u && runtime.events.back() == "free:12288",
          "getter after cleanup does not reinitialize or register again");
}

void TestClearAll() {
    Catalog catalog;
    Runtime runtime;
    auto& functions = catalog.GetOrInit(Kind::GlobalFunctions, 1u, runtime);
    auto& variables = catalog.GetOrInit(Kind::GlobalVariables, 2u, runtime);
    auto& types = catalog.GetOrInit(Kind::ObjectTypes, 3u, runtime);
    functions.begin = 0x1000u;
    functions.end = 0x1008u;
    functions.capacity_end = 0x1020u;
    variables.begin = 0x2000u;
    variables.end = 0x2004u;
    variables.capacity_end = 0x2010u;
    types.begin = 0x3000u;
    types.end = 0x300cu;
    types.capacity_end = 0x3040u;
    const auto initial_events = runtime.events;
    Check(&catalog.ClearAll({9u, 9u, 9u}, runtime) == &types,
          "clear returns last registry");
    Check(functions.end == functions.begin &&
              variables.end == variables.begin && types.end == types.begin,
          "clear empties all three vectors");
    Check(functions.capacity_end == 0x1020u &&
              variables.capacity_end == 0x2010u &&
              types.capacity_end == 0x3040u,
          "clear retains allocation and capacity");
    Check(runtime.events == initial_events,
          "clear does not register or free initialized queues");

    Catalog fresh;
    Runtime fresh_runtime;
    auto& last = fresh.ClearAll({4u, 5u, 6u}, fresh_runtime);
    Check(last.seed_byte == 6u && last.begin == 0u && last.end == 0u,
          "clear initializes absent queues");
    Check(fresh_runtime.events.size() == 3u,
          "clear registers all three cleanup callbacks on first access");
}

void TestGlobalFunctionRegistration() {
    Catalog catalog;
    Runtime runtime;
    runtime.Descriptor(0x2000u, 0x3000u, "giFoo");
    runtime.Descriptor(0x2100u, 0x3100u, "giFoo");
    runtime.Descriptor(0x2200u, 0x3200u, "giBar");
    runtime.Descriptor(0x2300u, 0x3300u, "GIFoo");
    runtime.Descriptor(0x2400u, 0x3400u, "giBaz");
    auto& queue = catalog.GetOrInit(Kind::GlobalFunctions, 7u, runtime);
    catalog.RegisterGlobalFunction(0x2000u, 8u, runtime);
    Check(queue.end - queue.begin == 4u &&
              queue.capacity_end - queue.begin == 4u &&
              runtime.ReadPointer(queue.begin) == 0x2000u,
          "first registration allocates one pointer slot");
    const auto first_allocation = queue.begin;
    const auto event_count = runtime.events.size();
    catalog.RegisterGlobalFunction(0x2100u, 9u, runtime);
    Check(queue.begin == first_allocation && runtime.events.size() == event_count &&
              queue.seed_byte == 7u,
          "same name does not append, allocate or reinitialize");
    catalog.RegisterGlobalFunction(0x2200u, 9u, runtime);
    Check(queue.end - queue.begin == 8u &&
              queue.capacity_end - queue.begin == 8u &&
              runtime.ReadPointer(queue.begin) == 0x2000u &&
              runtime.ReadPointer(queue.begin + 4u) == 0x2200u,
          "second distinct name reallocates and preserves insertion order");
    catalog.RegisterGlobalFunction(0x2300u, 9u, runtime);
    Check(queue.end - queue.begin == 12u &&
              queue.capacity_end - queue.begin == 16u,
          "growth doubles capacity once count exceeds one");
    const auto third_allocation = queue.begin;
    catalog.RegisterGlobalFunction(0x2400u, 9u, runtime);
    Check(queue.begin == third_allocation && queue.end - queue.begin == 16u &&
              runtime.ReadPointer(queue.begin + 8u) == 0x2300u &&
              runtime.ReadPointer(queue.begin + 12u) == 0x2400u,
          "case-sensitive name and spare-capacity append");
    Check(runtime.events == std::vector<std::string>{
              "atexit:8257824", "alloc:4", "free:0", "alloc:8",
              "free:65536", "alloc:16", "free:65796"},
          "allocation and release follow original growth path");
    catalog.ClearAll({0u, 0u, 0u}, runtime);
    catalog.RegisterGlobalFunction(0x2000u, 9u, runtime);
    Check(queue.begin == third_allocation && queue.end == queue.begin + 4u &&
              runtime.ReadPointer(queue.begin) == 0x2000u,
          "registration reuses retained capacity after clear");
}

void TestGlobalVariableRegistration() {
    Catalog catalog;
    Runtime runtime;
    runtime.Descriptor(0x4000u, 0x5000u, "MazeVar");
    runtime.Descriptor(0x4100u, 0x5100u, "MazeVar");
    runtime.Descriptor(0x4200u, 0x5200u, "mazevar");
    runtime.Descriptor(0x4300u, 0x5300u, "ShopVar");
    runtime.Descriptor(0x4400u, 0x5400u, "QuestVar");
    catalog.RegisterGlobalVariable(0x4000u, 3u, runtime);
    auto& variables = catalog.GetOrInit(Kind::GlobalVariables, 8u, runtime);
    auto& functions = catalog.GetOrInit(Kind::GlobalFunctions, 9u, runtime);
    Check(variables.seed_byte == 3u && functions.begin == 0u &&
              runtime.ReadPointer(variables.begin) == 0x4000u,
          "variable registration initializes only its own descriptor vector");
    const auto event_count = runtime.events.size();
    catalog.RegisterGlobalVariable(0x4100u, 8u, runtime);
    Check(runtime.events.size() == event_count &&
              variables.end == variables.begin + 4u,
          "duplicate variable name is ignored");
    catalog.RegisterGlobalVariable(0x4200u, 8u, runtime);
    catalog.RegisterGlobalVariable(0x4300u, 8u, runtime);
    const auto expanded_begin = variables.begin;
    Check(variables.end - variables.begin == 12u &&
              variables.capacity_end - variables.begin == 16u &&
              runtime.ReadPointer(variables.begin + 4u) == 0x4200u,
          "case-sensitive variable names grow the vector");
    catalog.RegisterGlobalVariable(0x4400u, 8u, runtime);
    Check(variables.begin == expanded_begin &&
              runtime.ReadPointer(variables.begin + 12u) == 0x4400u &&
              functions.end == functions.begin,
          "variable spare-capacity append does not affect function registry");
}

}  // namespace

int main() {
    TestLazyInitialization();
    TestCleanup();
    TestClearAll();
    TestGlobalFunctionRegistration();
    TestGlobalVariableRegistration();
}
