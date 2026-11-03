#include "pal4/script_registration_dispatch.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Event {
    std::string kind;
    std::vector<std::uint32_t> args;
};

struct Runtime final : pal4::IScriptRegistrationDispatchRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Event> events;
    std::uint32_t next_allocation{0x10000u};
    pal4::ScriptRegistrationCatalog* clear_functions_on_engine_call{};
    pal4::ScriptRegistrationCatalog* clear_types_on_engine_call{};

    void RegisterAtExit(std::uint32_t callback) override {
        events.push_back({"atexit", {callback}});
    }
    void Free(std::uint32_t address) override {
        events.push_back({"free", {address}});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        const auto result = next_allocation;
        next_allocation += bytes + 0x100u;
        events.push_back({"allocate", {bytes, result}});
        return result;
    }
    std::uint32_t ReadPointer(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WritePointer(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void RegisterFunction(
        std::uint32_t name, const pal4::ScriptFunctionPointerWords& callback,
        std::uint32_t calling_convention) override {
        std::vector<std::uint32_t> args{name, calling_convention};
        args.insert(args.end(), callback.begin(), callback.end());
        events.push_back({"function", args});
        if (clear_functions_on_engine_call) {
            auto& queue = clear_functions_on_engine_call->GetOrInit(
                pal4::ScriptRegistrationKind::GlobalFunctions, 0u, *this);
            queue.end = queue.begin;
        }
    }
    void RegisterObjectType(std::uint32_t name, std::uint32_t size,
                            std::uint32_t flags) override {
        events.push_back({"type", {name, size, flags}});
        if (clear_types_on_engine_call) {
            auto& queue = clear_types_on_engine_call->GetOrInit(
                pal4::ScriptRegistrationKind::ObjectTypes, 0u, *this);
            queue.end = queue.begin;
        }
    }
    void RegisterObjectBehavior(
        std::uint32_t type_name, std::uint32_t behavior,
        const char* declaration,
        const pal4::ScriptFunctionPointerWords& callback,
        std::uint32_t calling_convention) override {
        Check(std::string(declaration) == "void f()", "behavior declaration");
        std::vector<std::uint32_t> args{type_name, behavior,
                                        calling_convention};
        args.insert(args.end(), callback.begin(), callback.end());
        events.push_back({"behavior", args});
    }
    void RegisterObjectMethod(
        std::uint32_t type_name, std::uint32_t method_name,
        const pal4::ScriptFunctionPointerWords& callback,
        std::uint32_t calling_convention) override {
        std::vector<std::uint32_t> args{type_name, method_name,
                                        calling_convention};
        args.insert(args.end(), callback.begin(), callback.end());
        events.push_back({"method", args});
    }
    void RegisterObjectProperty(std::uint32_t type_name,
                                std::uint32_t property_name,
                                std::uint32_t offset) override {
        events.push_back({"object-property", {type_name, property_name, offset}});
    }
    void RegisterGlobalProperty(std::uint32_t name,
                                std::uint32_t address) override {
        events.push_back({"global-property", {name, address}});
    }
    void RecordRegistration(std::uint32_t name,
                            std::uint32_t description) override {
        events.push_back({"record", {name, description}});
    }
    void Callback(std::uint32_t address, std::uint32_t first_word) {
        for (std::uint32_t i = 0; i < 8u; ++i) {
            words[address + i * 4u] = first_word + i;
        }
    }
};

using Catalog = pal4::ScriptRegistrationCatalog;
using Dispatcher = pal4::ScriptRegistrationDispatcher;
using Kind = pal4::ScriptRegistrationKind;

void Add(Catalog& catalog, Runtime& runtime, Kind kind,
         std::uint32_t slot, std::uint32_t descriptor) {
    auto& queue = catalog.GetOrInit(kind, 1u, runtime);
    if (queue.begin == 0u) {
        queue.begin = slot;
        queue.end = slot;
        queue.capacity_end = slot + 16u;
    }
    runtime.WritePointer(queue.end, descriptor);
    queue.end += 4u;
}

void TestFullRegistrationOrderAndParameters() {
    Catalog catalog;
    Runtime runtime;
    Add(catalog, runtime, Kind::GlobalFunctions, 0x7000u, 0x2000u);
    Add(catalog, runtime, Kind::ObjectTypes, 0x7100u, 0x2100u);
    Add(catalog, runtime, Kind::GlobalVariables, 0x7200u, 0x2400u);

    runtime.words[0x2000u] = 0x4000u;
    runtime.words[0x2008u] = 0x3000u;
    runtime.Callback(0x2010u, 10u);
    runtime.words[0x2100u] = 0x4100u;
    runtime.words[0x2108u] = 0x3100u;
    runtime.words[0x2110u] = 24u;
    runtime.Callback(0x2118u, 20u);
    runtime.Callback(0x2138u, 30u);
    runtime.words[0x215cu] = 0x6000u;
    runtime.words[0x2160u] = 0x6004u;
    runtime.words[0x216cu] = 0x6100u;
    runtime.words[0x2170u] = 0x6104u;
    runtime.words[0x6000u] = 0x2200u;
    runtime.words[0x2200u] = 0x4200u;
    runtime.words[0x2208u] = 0x3200u;
    runtime.Callback(0x2210u, 40u);
    runtime.words[0x6100u] = 0x2300u;
    runtime.words[0x2300u] = 0x4300u;
    runtime.words[0x2308u] = 0x3300u;
    runtime.words[0x2334u] = 12u;
    runtime.words[0x2400u] = 0x4400u;
    runtime.words[0x2408u] = 0x3400u;
    runtime.words[0x2430u] = 0x9000u;
    runtime.events.clear();

    Dispatcher dispatcher(catalog);
    Check(dispatcher.RegisterAll({1u, 1u, 1u}, runtime) == 1,
          "dispatcher returns one after registration");
    const std::vector<std::string> expected{
        "function", "record", "type", "behavior", "behavior", "record",
        "method", "record", "object-property", "record",
        "global-property", "record"};
    Check(runtime.events.size() == expected.size(), "registration call count");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        Check(runtime.events[i].kind == expected[i], "registration order");
    }
    Check(runtime.events[0].args == std::vector<std::uint32_t>{
              0x3000u, 0u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u},
          "global callback is passed as eight raw words with convention zero");
    Check(runtime.events[2].args ==
              std::vector<std::uint32_t>{0x3100u, 24u, 1u},
          "object type registration arguments");
    Check(runtime.events[3].args == std::vector<std::uint32_t>{
              0x3100u, 0u, 3u, 20u, 21u, 22u, 23u, 24u, 25u, 26u, 27u} &&
              runtime.events[4].args == std::vector<std::uint32_t>{
              0x3100u, 1u, 3u, 30u, 31u, 32u, 33u, 34u, 35u, 36u, 37u},
          "object construction and destruction callback words");
    Check(runtime.events[6].args == std::vector<std::uint32_t>{
              0x3100u, 0x3200u, 2u, 40u, 41u, 42u, 43u, 44u, 45u, 46u, 47u},
          "object method callback words and convention");
    Check(runtime.events[8].args ==
              std::vector<std::uint32_t>{0x3100u, 0x3300u, 12u} &&
              runtime.events[10].args ==
              std::vector<std::uint32_t>{0x3400u, 0x9000u},
          "object offset and global address");
    Check(runtime.events[1].args ==
              std::vector<std::uint32_t>{0x3000u, 0x4000u} &&
              runtime.events[5].args ==
              std::vector<std::uint32_t>{0x3100u, 0x4100u} &&
              runtime.events[11].args ==
              std::vector<std::uint32_t>{0x3400u, 0x4400u},
          "descriptor registration records use name and first dword");
    for (const auto kind : {Kind::GlobalFunctions, Kind::ObjectTypes,
                            Kind::GlobalVariables}) {
        const auto& queue = catalog.GetOrInit(kind, 9u, runtime);
        Check(queue.end == queue.begin && queue.capacity_end > queue.begin,
              "registration clears queues without freeing capacity");
    }
    runtime.events.clear();
    Check(dispatcher.RegisterAll({9u, 9u, 9u}, runtime) == 1 &&
              runtime.events.empty(),
          "second dispatch of cleared queues makes no engine calls");
}

void TestEmptyCatalogInitialization() {
    Catalog catalog;
    Runtime runtime;
    Dispatcher dispatcher(catalog);
    Check(dispatcher.RegisterAll({4u, 5u, 6u}, runtime) == 1,
          "empty registration still succeeds");
    Check(runtime.events.size() == 3u &&
              runtime.events[0].args[0] == 0x7e0120u &&
              runtime.events[1].args[0] == 0x7e0200u &&
              runtime.events[2].args[0] == 0x7e0190u,
          "empty dispatch lazily initializes function, type, variable queues");
}

void TestVectorBoundaryAfterEngineCall() {
    Catalog catalog;
    Runtime runtime;
    Add(catalog, runtime, Kind::GlobalFunctions, 0x7000u, 0x2000u);
    runtime.words[0x2000u] = 0x4000u;
    runtime.words[0x2008u] = 0x3000u;
    runtime.Callback(0x2010u, 10u);
    runtime.events.clear();
    runtime.clear_functions_on_engine_call = &catalog;
    Dispatcher dispatcher(catalog);
    bool threw = false;
    try {
        (void)dispatcher.RegisterAll({1u, 1u, 1u}, runtime);
    } catch (const std::out_of_range& error) {
        threw = std::string(error.what()) == "invalid vector<T> subscript";
    }
    Check(threw && runtime.events.size() == 1u &&
              runtime.events[0].kind == "function",
          "post-call vector recheck uses host out_of_range boundary");
}

void TestObjectVectorBoundaryAfterEngineCall() {
    Catalog catalog;
    Runtime runtime;
    (void)catalog.GetOrInit(Kind::GlobalFunctions, 1u, runtime);
    Add(catalog, runtime, Kind::ObjectTypes, 0x7100u, 0x2100u);
    runtime.words[0x2100u] = 0x4100u;
    runtime.words[0x2108u] = 0x3100u;
    runtime.words[0x2110u] = 24u;
    runtime.Callback(0x2118u, 20u);
    runtime.Callback(0x2138u, 30u);
    runtime.events.clear();
    runtime.clear_types_on_engine_call = &catalog;
    Dispatcher dispatcher(catalog);
    bool threw = false;
    try {
        (void)dispatcher.RegisterAll({1u, 1u, 1u}, runtime);
    } catch (const std::out_of_range& error) {
        threw = std::string(error.what()) == "invalid vector<T> subscript";
    }
    Check(threw && runtime.events.size() == 3u &&
              runtime.events[0].kind == "type" &&
              runtime.events[1].kind == "behavior" &&
              runtime.events[2].kind == "behavior",
          "object-vector recheck follows behavior registration");
}

}  // namespace

int main() {
    TestFullRegistrationOrderAndParameters();
    TestEmptyCatalogInitialization();
    TestVectorBoundaryAfterEngineCall();
    TestObjectVectorBoundaryAfterEngineCall();
}
