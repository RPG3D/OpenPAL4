#include "pal4/monster_manager_tree_walker.h"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct CallbackCall {
    std::uint32_t callback{};
    std::uint32_t object{};
    std::uint32_t context{};
    bool operator==(const CallbackCall&) const = default;
};

struct StopCall {
    std::uint32_t object{};
    float value{};
    bool operator==(const StopCall&) const = default;
};

struct Runtime final : pal4::MonsterManagerTreeWalkRuntimeStub {
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t InvokeCallback(
        std::uint32_t callback, std::uint32_t object,
        std::uint32_t context) override {
        callbacks.push_back({callback, object, context});
        return callback_results.at(callbacks.size() - 1u);
    }
    void StopPursuit(std::uint32_t object, float value) override {
        stops.push_back({object, value});
    }
    void ClearStopPursuit(std::uint32_t object) override {
        cleared.push_back(object);
    }
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> callback_results;
    std::vector<CallbackCall> callbacks;
    std::vector<StopCall> stops;
    std::vector<std::uint32_t> cleared;
};

constexpr std::uint32_t kManager = 0x5000u;
constexpr std::uint32_t kHeader = 0x6000u;
constexpr std::uint32_t kNil = 0x9000u;

void TestEmptyTree() {
    Runtime runtime;
    runtime.words = {{0x502cu, kHeader}, {kHeader, kHeader}};
    const pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerTreeWalker::ForEach(
              kManager, 0x1234u, 0x5678u, globals, runtime) == kHeader &&
              runtime.callbacks.empty(),
          "empty callback walk returns header without invoking callback");
    Check(pal4::MonsterManagerTreeWalker::StopPursuitAll(
              kManager, 0.5f, globals, runtime) == kHeader &&
              runtime.stops.empty(),
          "empty pursuit walk does not call objects");
    Check(pal4::MonsterManagerTreeWalker::ClearStopPursuitAll(
              kManager, globals, runtime) == kHeader &&
              runtime.cleared.empty(),
          "empty clear walk does not call objects");
}

void TestCallbackStopAndSuccess() {
    Runtime runtime;
    constexpr std::uint32_t node = 0x7100u;
    runtime.words = {
        {0x502cu, kHeader}, {kHeader, node},
        {kHeader + 4u, node}, {kHeader + 8u, node},
        {node + 4u, kHeader}, {node + 8u, kNil},
        {node + 0x1cu, 0xa000u}};
    const pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    runtime.callback_results = {0u};
    Check(pal4::MonsterManagerTreeWalker::ForEach(
              kManager, 0x1234u, 0x5678u, globals, runtime) == 0u &&
              runtime.callbacks == std::vector<CallbackCall>{
                  {0x1234u, 0xa000u, 0x5678u}},
          "zero callback result stops traversal immediately");
    runtime.callbacks.clear();
    runtime.callback_results = {0x8000u};
    Check(pal4::MonsterManagerTreeWalker::ForEach(
              kManager, 0x1234u, 0x5678u, globals, runtime) == node &&
              runtime.callbacks.size() == 1u,
          "nonzero callback result advances and returns traversal register");
}

void TestThreeNodeOrderAndPursuitArgument() {
    Runtime runtime;
    constexpr std::uint32_t root = 0x7100u;
    constexpr std::uint32_t left = 0x7200u;
    constexpr std::uint32_t right = 0x7300u;
    runtime.words = {
        {0x502cu, kHeader}, {kHeader, left},
        {kHeader + 4u, root}, {kHeader + 8u, right},
        {root, left}, {root + 4u, kHeader},
        {root + 8u, right}, {root + 0x1cu, 0xa100u},
        {left + 4u, root}, {left + 8u, kNil},
        {left + 0x1cu, 0xa200u},
        {right, kNil}, {right + 4u, root},
        {right + 8u, kNil}, {right + 0x1cu, 0xa300u}};
    const pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    runtime.callback_results = {1u, 1u, 1u};
    Check(pal4::MonsterManagerTreeWalker::ForEach(
              kManager, 0x1111u, 0x2222u, globals, runtime) == kHeader &&
              runtime.callbacks == std::vector<CallbackCall>{
                  {0x1111u, 0xa200u, 0x2222u},
                  {0x1111u, 0xa100u, 0x2222u},
                  {0x1111u, 0xa300u, 0x2222u}},
          "callback walks objects in tree order with unchanged context");
    Check(pal4::MonsterManagerTreeWalker::StopPursuitAll(
              kManager, 0.75f, globals, runtime) == kHeader &&
              runtime.stops == std::vector<StopCall>{
                  {0xa200u, 0.75f}, {0xa100u, 0.75f},
                  {0xa300u, 0.75f}},
          "pursuit stop walks the same tree and forwards x87 value");
    Check(pal4::MonsterManagerTreeWalker::ClearStopPursuitAll(
              kManager, globals, runtime) == kHeader &&
              runtime.cleared == std::vector<std::uint32_t>{
                  0xa200u, 0xa100u, 0xa300u},
          "clear flag visits every object in tree order");
}

}  // namespace

int main() {
    TestEmptyTree();
    TestCallbackStopAndSuccess();
    TestThreeNodeOrderAndPursuitArgument();
}
