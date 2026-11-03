#include "pal4/game_data_three_field_registry.h"

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
        return 0xa5;
    }
};

void CheckSchema(pal4::GameDataThreeFieldSchema schema,
                 std::uint32_t collection, std::uint32_t cleanup,
                 std::string_view second_name) {
    pal4::GameDataThreeFieldRegistry registry(schema);
    Runtime runtime;
    Check(registry.Initialize(runtime) == collection &&
              runtime.calls == std::vector<Call>{
                  {"init", {}, collection, 0},
                  {"atexit", {}, cleanup, 0},
                  {"begin", {}, 0, 0},
                  {"field", "ID", 0, 0},
                  {"field", std::string(second_name), 4, 1},
                  {"field", "MEMO", 20, 1},
                  {"sort", {}, collection, 0},
                  {"release", {}, 0, 0}},
          "initial schema, offsets and call order match IDA");

    runtime.calls.clear();
    Check(registry.ProcessEvent(0x7000, runtime) == 0xa5 &&
              runtime.calls == std::vector<Call>{
                  {"begin", {}, 0, 0},
                  {"release", {}, 0, 0},
                  {"event", {}, 0x7000, collection}},
          "repeated initialization skips guarded setup but still creates text");
    runtime.calls.clear();
    registry.Cleanup(runtime);
    Check(runtime.calls == std::vector<Call>{{"cleanup", {}, collection, 0}},
          "cleanup uses the schema-specific collection");

    pal4::GameDataThreeFieldRegistry failed_registry(schema);
    Runtime failed;
    failed.fail_name = std::string(second_name);
    try {
        static_cast<void>(failed_registry.Initialize(failed));
        Check(false, "field failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "field failed" &&
                  failed.calls.size() == 6 &&
                  failed.calls[4] ==
                      Call{"field", std::string(second_name), 4, 1} &&
                  failed.calls.back() == Call{"release", {}, 0, 0},
              "failure releases temporary text and stops registration");
    }
    failed.calls.clear();
    Check(failed_registry.Initialize(failed) == collection &&
              failed.calls == std::vector<Call>{
                  {"begin", {}, 0, 0}, {"release", {}, 0, 0}},
          "native guards remain set after partial registration failure");

    pal4::GameDataThreeFieldRegistry sort_registry(schema);
    Runtime sort_failed;
    sort_failed.fail_sort = true;
    try {
        static_cast<void>(sort_registry.Initialize(sort_failed));
        Check(false, "sort failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "sort failed" &&
                  sort_failed.calls.back() == Call{"release", {}, 0, 0},
              "sort failure releases temporary text");
    }
}

}  // namespace

int main() {
    try {
        CheckSchema(pal4::GameDataThreeFieldSchema::NameMemo,
                    0x8ef344, 0x4fa430, "NAME");
        CheckSchema(pal4::GameDataThreeFieldSchema::ChapterMemo,
                    0x8ef310, 0x4fa760, "CHAPTER");
        std::cout << "game_data_three_field_registry_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
