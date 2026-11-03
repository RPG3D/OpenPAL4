#include "pal4/effect_system_factory_registry.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Call {
    std::string action;
    std::string name;
    std::uint32_t address{};
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IEffectSystemFactoryRuntime {
    std::vector<Call> calls;
    std::unordered_set<std::string> existing;
    std::uint32_t count = 0u;
    std::uint8_t release_result = 0x5au;

    std::uint32_t RegisterEffectSystem() override {
        calls.push_back({"register", "", 0x1000u});
        return 0x1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1024u, "registration count offset");
        calls.push_back({"count", "", address});
        return count;
    }
    void ClearRegistry(std::uint32_t tree) override {
        Check(tree == 0x1018u, "registry tree offset");
        calls.push_back({"clear", "", tree});
    }
    bool HasFactory(std::uint32_t tree,
                    std::string_view name) override {
        Check(tree == 0x1018u, "lookup tree offset");
        calls.push_back({"find", std::string(name), 0u});
        return existing.contains(std::string(name));
    }
    void InsertFactory(std::uint32_t tree,
                       std::string_view name,
                       std::uint32_t creator) override {
        Check(tree == 0x1018u, "insert tree offset");
        calls.push_back({"insert", std::string(name), creator});
    }
    std::uint8_t ReleaseName(std::string_view name) override {
        calls.push_back({"release", std::string(name), 0u});
        return release_result;
    }
};

void TestAllFactories() {
    Runtime runtime;
    runtime.count = 3u;
    Check(pal4::EffectSystemFactoryRegistry::InitializeEvents(runtime) ==
              runtime.release_result &&
              runtime.calls[0] == Call{"register", "", 0x1000u} &&
              runtime.calls[1] == Call{"count", "", 0x1024u} &&
              runtime.calls[2] == Call{"clear", "", 0x1018u},
          "existing registrations clear before factory loop");
    std::size_t index = 3u;
    for (const auto& factory :
         pal4::EffectSystemFactoryRegistry::kFactories) {
        Check(runtime.calls[index++] ==
                  Call{"find", std::string(factory.name), 0u},
              "each registration path probes its exact factory name");
        Check(runtime.calls[index++] ==
                  Call{"insert", std::string(factory.name),
                       factory.creator},
              "factory dispatch keeps IDA name, address and order");
        Check(runtime.calls[index++] ==
                  Call{"release", std::string(factory.name), 0u},
              "temporary name is released after each registration");
    }
    Check(index == runtime.calls.size(), "no extra factory operations");
}

void TestDuplicates() {
    Runtime runtime;
    runtime.existing = {"ClumpObject", "Sound3DObject", "EffectObject"};
    (void)pal4::EffectSystemFactoryRegistry::InitializeEvents(runtime);
    Check(runtime.calls[0].action == "register" &&
              runtime.calls[1].action == "count",
          "empty registry avoids clear");
    std::size_t inserts = 0u;
    std::size_t finds = 0u;
    std::size_t releases = 0u;
    for (const auto& call : runtime.calls) {
        if (call.action == "insert") {
            ++inserts;
            Check(!runtime.existing.contains(call.name),
                  "existing manual factory is not inserted twice");
        }
        if (call.action == "find") ++finds;
        if (call.action == "release") ++releases;
        Check(call.action != "clear", "zero count skips prior-tree clear");
    }
    Check(inserts == 8u && finds == 11u && releases == 11u,
          "both paths skip duplicates and release each name");
}

void TestProcessEvent() {
    Runtime runtime;
    Check(pal4::EffectSystemFactoryRegistry::ProcessEvent(
              0x1018u, "ThunderObject", 0x63e5b0u, runtime) == 1u &&
              runtime.calls ==
                  std::vector<Call>{{"find", "ThunderObject", 0u},
                                    {"insert", "ThunderObject",
                                     0x63e5b0u}},
          "absent factory is copied and inserted");
    runtime.calls.clear();
    runtime.existing.insert("ThunderObject");
    Check(pal4::EffectSystemFactoryRegistry::ProcessEvent(
              0x1018u, "ThunderObject", 0x63e5b0u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<Call>{{"find", "ThunderObject", 0u}},
          "existing factory is not copied or inserted");
}

}  // namespace

int main() {
    TestAllFactories();
    TestDuplicates();
    TestProcessEvent();
}
