#include "pal4/skill_property_registry.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string kind;
    std::string name;
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::ISkillPropertyRegistryRuntime {
    std::vector<Call> calls;
    std::string fail_name;
    std::uint32_t fail_parameter = 99;
    bool fail_sort = false;
    std::uint8_t event_result = 0x91;

    void InitializeCollection(std::uint32_t address) override {
        calls.push_back({"initialize_collection", {}, address, 0});
    }
    int RegisterCleanup(std::uint32_t address) override {
        calls.push_back({"register_cleanup", {}, address, 0x4f37e0});
        return -7;
    }
    void BeginTemporaryText() override {
        calls.push_back({"begin_text", {}, 0, 0});
    }
    void ReleaseTemporaryText() override {
        calls.push_back({"release_text", {}, 0, 0});
    }
    void AddNamedProperty(pal4::SkillPropertyKind kind,
                          std::string_view name,
                          std::uint32_t offset) override {
        calls.push_back({"named", std::string(name), offset,
                         kind == pal4::SkillPropertyKind::Basic ? 0u : 1u});
        if (name == fail_name) throw std::runtime_error("named failed");
    }
    void AddParameterProperty(std::uint32_t address,
                              std::uint32_t offset) override {
        calls.push_back({"parameter", {}, address, offset});
        if ((offset - 20) / 4 == fail_parameter)
            throw std::runtime_error("parameter failed");
    }
    void Sort(std::uint32_t collection) override {
        calls.push_back({"sort", {}, collection, 0});
        if (fail_sort) throw std::runtime_error("sort failed");
    }
    void CleanupCollection(std::uint32_t collection) override {
        calls.push_back({"cleanup", {}, collection, 0});
    }
    std::uint8_t ProcessEvent(std::uint32_t input,
                              std::uint32_t collection) override {
        calls.push_back({"event", {}, input, collection});
        return event_result;
    }
};

void CheckInitialSchema() {
    Runtime runtime;
    pal4::SkillPropertyRegistry registry;
    Check(registry.Initialize(runtime) == 0x8ef210 &&
              runtime.calls.size() == 26 &&
              runtime.calls[0] ==
                  Call{"initialize_collection", {}, 0x8ef210, 0} &&
              runtime.calls[1] ==
                  Call{"register_cleanup", {}, 0x8ef210, 0x4f37e0} &&
              runtime.calls[2] == Call{"begin_text", {}, 0, 0},
          "collection and cleanup are initialized before temporary text");
    const std::vector<Call> named = {
        {"named", "ID", 0, 0},
        {"named", "TYPE", 4, 0},
        {"named", "TRIGER", 8, 0},
        {"named", "EXIST_TIME", 12, 0},
        {"named", "FUNCTION", 16, 0},
        {"named", "ATTACHED_SKILL", 56, 0},
        {"named", "MUTEX_ID", 60, 0},
        {"named", "STATUS_ID", 64, 0},
        {"named", "EF", 68, 1},
        {"named", "BIND", 84, 1},
        {"named", "EF1", 100, 1},
        {"named", "BIND1", 116, 1},
    };
    for (std::size_t index = 0; index < named.size(); ++index)
        Check(runtime.calls[3 + index] == named[index],
              "named property name, type, offset and order match IDA");
    for (std::uint32_t index = 0; index < 9; ++index)
        Check(runtime.calls[15 + index] ==
                  Call{"parameter", {}, 0x8ef248u + 16 * index,
                       20 + 4 * index},
              "parameter property uses matching static text slot and offset");
    Check(runtime.calls[24] == Call{"sort", {}, 0x8ef210, 0} &&
              runtime.calls[25] == Call{"release_text", {}, 0, 0},
          "collection sort precedes temporary text release");

    runtime.calls.clear();
    Check(registry.Initialize(runtime) == 0x8ef210 &&
              runtime.calls ==
                  std::vector<Call>{{"begin_text", {}, 0, 0},
                                    {"release_text", {}, 0, 0}},
          "later calls keep native guards and still create temporary text");
}

void CheckEventAndCleanup() {
    Runtime runtime;
    pal4::SkillPropertyRegistry registry;
    Check(registry.ProcessEvent(0x7000, runtime) == 0x91 &&
              runtime.calls.back() ==
                  Call{"event", {}, 0x7000, 0x8ef210},
          "event dispatch follows registry initialization");
    runtime.calls.clear();
    registry.Cleanup(runtime);
    Check(runtime.calls ==
              std::vector<Call>{{"cleanup", {}, 0x8ef210, 0}},
          "cleanup passes the static collection address");
}

void CheckFailures() {
    Runtime runtime;
    pal4::SkillPropertyRegistry registry;
    runtime.fail_name = "TRIGER";
    try {
        static_cast<void>(registry.Initialize(runtime));
        Check(false, "named property failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "named failed" &&
                  runtime.calls.size() == 7 &&
                  runtime.calls[5] == Call{"named", "TRIGER", 8, 0} &&
                  runtime.calls.back() ==
                      Call{"release_text", {}, 0, 0},
              "failure releases temporary text before propagating");
    }
    runtime.calls.clear();
    Check(registry.Initialize(runtime) == 0x8ef210 &&
              runtime.calls.size() == 2,
          "native property guard stays set after partial failure");

    Runtime param_failure;
    pal4::SkillPropertyRegistry param_registry;
    param_failure.fail_parameter = 4;
    try {
        static_cast<void>(param_registry.Initialize(param_failure));
        Check(false, "parameter property failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "parameter failed" &&
                  param_failure.calls.size() == 21 &&
                  param_failure.calls.back() ==
                      Call{"release_text", {}, 0, 0},
              "parameter failure stops before later fields and sort");
    }
}

}  // namespace

int main() {
    try {
        CheckInitialSchema();
        CheckEventAndCleanup();
        CheckFailures();
        std::cout << "skill_property_registry_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
