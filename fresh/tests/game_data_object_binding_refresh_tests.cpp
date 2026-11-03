#include "pal4/game_data_object_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataObjectBindingRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::string fail_on;
    std::uint32_t release_result = 0x12345678;

    void Record(std::string event) {
        events.push_back(event);
        if (event == fail_on) throw std::runtime_error("binding failed");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        Check(name == 0x9000 + 52, "binding log uses table name slot");
        Record("log:" + std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x9000 + 108 &&
                  source == 0x9000 + 20,
              "Effect assignment uses binding and raw field arrays");
        Record("assign");
    }
    void RegisterField(std::uint32_t binding, std::string_view name,
                       std::uint32_t offset,
                       pal4::GameDataFieldKind kind) override {
        Check(binding == 0x9000 + 108, "schema uses object binding");
        Record("field:" + std::string(name) + ":" +
               std::to_string(offset) + ":" +
               std::to_string(static_cast<unsigned>(kind)));
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x9000 + 108, "bind uses object binding");
        Record("bind");
    }
    std::uint32_t ReleaseLogBuffer() override {
        Record("release");
        return release_result;
    }
};

Runtime Mismatched() {
    Runtime runtime;
    runtime.words[0x9000 + 128] = 0x6000;
    runtime.words[0x9000 + 132] = 0x6000 + 2 * 4;
    runtime.words[0x9000 + 24] = 0x5000;
    runtime.words[0x9000 + 28] = 0x5000 + 3 * 60;
    return runtime;
}

void CheckRebind() {
    Runtime runtime = Mismatched();
    Check(pal4::GameDataObjectTable::RefreshBindings(
              0x9000, runtime) == runtime.release_result &&
              runtime.events == std::vector<std::string>{
                  "log:beginBindField, table:", "assign",
                  "field:ID:4:2", "field:NAME:20:2",
                  "field:POSITION:36:2", "field:TYPE:52:1",
                  "bind", "log:endBindField, table:", "release"},
          "refresh binds four Object fields between ordered logs");
}

void CheckMatchingCounts() {
    Runtime runtime = Mismatched();
    runtime.words[0x9000 + 132] = 0x6000 + 3 * 4;
    Check(pal4::GameDataObjectTable::RefreshBindings(
              0x9000, runtime) == 0 && runtime.events.empty(),
          "matching populated counts skip binding and return EAX zero");

    Runtime empty;
    Check(pal4::GameDataObjectTable::RefreshBindings(
              0x9000, empty) == 0x9000 + 20 &&
              empty.events.empty(),
          "matching null source retains the native table-plus-20 result");

    Runtime negative;
    negative.words[0x9000 + 128] = 0x6000;
    negative.words[0x9000 + 132] = 0x6000 - 4;
    negative.words[0x9000 + 24] = 0x5000;
    negative.words[0x9000 + 28] = 0x5000 - 60;
    Check(pal4::GameDataObjectTable::RefreshBindings(
              0x9000, negative) == 1 && negative.events.empty(),
          "negative equal counts preserve signed-division EAX residue");
}

void CheckFailures() {
    for (const auto& stage : std::vector<std::string>{
             "log:beginBindField, table:", "assign",
             "field:ID:4:2", "field:NAME:20:2",
             "field:POSITION:36:2", "field:TYPE:52:1",
             "bind", "log:endBindField, table:"}) {
        Runtime runtime = Mismatched();
        runtime.fail_on = stage;
        try {
            static_cast<void>(pal4::GameDataObjectTable::RefreshBindings(
                0x9000, runtime));
            Check(false, "binding stage failure must propagate");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == "binding failed" &&
                      runtime.events.size() >= 2 &&
                      runtime.events[runtime.events.size() - 2] == stage &&
                      runtime.events.back() == "release",
                  "every binding failure releases temporary log buffer");
        }
    }
}

}  // namespace

int main() {
    try {
        CheckRebind();
        CheckMatchingCounts();
        CheckFailures();
        std::cout << "game_data_object_binding_refresh_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
