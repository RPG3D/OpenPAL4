#include "pal4/game_data_question_registry.h"

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
    std::string action;
    std::string name;
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataFieldRegistryRuntime {
    std::vector<Call> calls;
    std::string fail_name;
    bool fail_sort = false;

    void InitializeCollection(std::uint32_t address) override {
        calls.push_back({"init", {}, address, 0});
    }
    void RegisterCleanup(std::uint32_t callback) override {
        calls.push_back({"atexit", {}, callback, 0});
    }
    void BeginTemporaryText() override {
        calls.push_back({"begin", {}, 0, 0});
    }
    void ReleaseTemporaryText() override {
        calls.push_back({"release", {}, 0, 0});
    }
    void AddProperty(pal4::GameDataFieldKind kind,
                     std::string_view name,
                     std::uint32_t offset) override {
        calls.push_back({"field", std::string(name), offset,
                         kind == pal4::GameDataFieldKind::Integer ? 0u : 1u});
        if (name == fail_name) throw std::runtime_error("field failed");
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
        return 0x67;
    }
};

void CheckRegistrationAndDispatch() {
    Runtime runtime;
    pal4::GameDataQuestionRegistry registry;
    Check(registry.Initialize(runtime) == 0x8ef2dc &&
              runtime.calls == std::vector<Call>{
                  {"init", {}, 0x8ef2dc, 0},
                  {"atexit", {}, 0x4fad80, 0},
                  {"begin", {}, 0, 0},
                  {"field", "ID", 0, 0},
                  {"field", "QUESTION", 4, 1},
                  {"field", "ANSWER_1", 20, 1},
                  {"field", "ANSWER_2", 36, 1},
                  {"field", "ANSWER_3", 52, 1},
                  {"field", "RIGHT_ANSWER", 68, 0},
                  {"field", "WINDOW", 72, 0},
                  {"sort", {}, 0x8ef2dc, 0},
                  {"release", {}, 0, 0}},
          "question schema fields, types and offsets match IDA order");

    runtime.calls.clear();
    Check(registry.ProcessEvent(0x7100, runtime) == 0x67 &&
              runtime.calls == std::vector<Call>{
                  {"begin", {}, 0, 0},
                  {"release", {}, 0, 0},
                  {"event", {}, 0x7100, 0x8ef2dc}},
          "event processing reuses guarded schema and forwards result");
    runtime.calls.clear();
    registry.Cleanup(runtime);
    Check(runtime.calls ==
              std::vector<Call>{{"cleanup", {}, 0x8ef2dc, 0}},
          "cleanup uses the question collection address");
}

void CheckFailures() {
    Runtime runtime;
    pal4::GameDataQuestionRegistry registry;
    runtime.fail_name = "ANSWER_2";
    try {
        static_cast<void>(registry.Initialize(runtime));
        Check(false, "field failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "field failed" &&
                  runtime.calls.size() == 8 &&
                  runtime.calls[6] == Call{"field", "ANSWER_2", 36, 1} &&
                  runtime.calls.back() == Call{"release", {}, 0, 0},
              "field failure stops registration and releases temporary text");
    }
    runtime.calls.clear();
    Check(registry.Initialize(runtime) == 0x8ef2dc &&
              runtime.calls == std::vector<Call>{
                  {"begin", {}, 0, 0}, {"release", {}, 0, 0}},
          "guards stay set after partial registration failure");

    Runtime sort_failure;
    pal4::GameDataQuestionRegistry sort_registry;
    sort_failure.fail_sort = true;
    try {
        static_cast<void>(sort_registry.Initialize(sort_failure));
        Check(false, "sort failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "sort failed" &&
                  sort_failure.calls.back() == Call{"release", {}, 0, 0},
              "sort failure releases temporary text");
    }
}

}  // namespace

int main() {
    try {
        CheckRegistrationAndDispatch();
        CheckFailures();
        std::cout << "game_data_question_registry_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
