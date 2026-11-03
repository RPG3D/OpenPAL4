#include "pal4/actor_manager_player_capture_chance.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t target = 0x2000u;

struct Runtime final : pal4::IActorManagerPlayerCaptureRuntime {
    std::map<std::uint32_t, std::int32_t> words;
    std::vector<std::string> trace;
    std::int32_t state = 0;
    std::int32_t kind = 1;
    std::uint32_t random = 249u;
    float divisor = 100.0f;
    float weight = 0.2f;
    float base = 0.1f;

    Runtime() {
        words[target + 804u] = 1;
        words[target + 1192u] = 2;
        words[target + 1184u] = 50;
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::int32_t ActorSlot144(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "capture actor slot 144");
        trace.emplace_back("state");
        return state;
    }
    std::int32_t TargetSlot168(std::uint32_t actual_target) override {
        Check(actual_target == target, "capture target slot 168");
        trace.emplace_back("kind");
        return kind;
    }
    std::uint32_t Database() override {
        trace.emplace_back("database");
        return 0x3000u;
    }
    float FloatElement(std::uint32_t values,
                       std::uint32_t index) override {
        Check(values == 0x3000u + 184u &&
                  index >= 12u && index <= 14u,
              "capture parameter table offset and index");
        trace.emplace_back("float:" + std::to_string(index));
        if (index == 12u) return divisor;
        if (index == 13u) return weight;
        return base;
    }
    std::uint32_t RandomNumber() override {
        trace.emplace_back("random");
        return random;
    }
};

void EarlyGates() {
    using Chance = pal4::ActorManagerPlayerCaptureChance;
    Runtime runtime;
    runtime.words[target + 804u] = 0;
    Check(!Chance::Check(actor, target, runtime) &&
              runtime.trace == std::vector<std::string>{"read:8996"},
          "target status must equal one before actor virtual call");
    for (const auto state : {1, 4, 5, 6}) {
        Runtime blocked;
        blocked.state = state;
        Check(!Chance::Check(actor, target, blocked) &&
                  blocked.trace == std::vector<std::string>{
                      "read:8996", "state"},
              "actor states one and four through six are blocked");
    }
    for (const auto state : {-1, 0, 2, 3, 7}) {
        Runtime allowed;
        allowed.state = state;
        Check(Chance::Check(actor, target, allowed),
              "other signed actor states reach the probability decision");
    }
    Runtime same;
    same.words[actor + 804u] = 1;
    Check(!Chance::Check(actor, actor, same) &&
              same.trace == std::vector<std::string>{
                  "read:4900", "state"},
          "same actor and target are rejected before target kind");
    for (const auto kind : {0, 3, -1}) {
        Runtime invalid;
        invalid.kind = kind;
        Check(!Chance::Check(actor, target, invalid) &&
                  invalid.trace.back() == "kind",
              "only target kinds one and two are eligible");
    }
}

void ThresholdAndIndex() {
    using Chance = pal4::ActorManagerPlayerCaptureChance;
    Runtime first;
    Check(Chance::Check(actor, target, first) &&
              first.trace == std::vector<std::string>{
                  "read:8996", "state", "kind", "read:9384",
                  "read:9376", "database", "float:12", "database",
                  "float:13", "database", "float:14", "random"},
          "index two loads its level and three DB factors in order");
    first.random = 251u;
    Check(!Chance::Check(actor, target, first),
          "kind one probability threshold rejects larger random sample");

    Runtime second;
    second.kind = 2;
    second.random = 299u;
    Check(Chance::Check(actor, target, second),
          "kind two receives 0.1 instead of 0.05 bonus");
    second.random = 301u;
    Check(!Chance::Check(actor, target, second),
          "kind two threshold still rejects larger sample");

    Runtime high_index;
    high_index.words[target + 1192u] = 4;
    high_index.random = 0u;
    Check(Chance::Check(actor, target, high_index) &&
              high_index.trace[3] == "read:9384" &&
              high_index.trace[4] == "database",
          "index four is converted directly without a table read");
    Runtime negative;
    negative.words[target + 1192u] = -5;
    negative.random = 0u;
    negative.base = -1.0f;
    Check(!Chance::Check(actor, target, negative) &&
              negative.trace[4] == "database",
          "negative index remains signed and bypasses the table");

    Runtime modulo;
    modulo.random = 1249u;
    Check(Chance::Check(actor, target, modulo),
          "random number is reduced modulo one thousand");
    Runtime zero_threshold;
    zero_threshold.base = -0.05f;
    zero_threshold.weight = 0.0f;
    zero_threshold.random = 0u;
    Check(!Chance::Check(actor, target, zero_threshold),
          "comparison is strict at a zero threshold");
}

}  // namespace

int main() {
    try {
        EarlyGates();
        ThresholdAndIndex();
        std::cout << "actor_manager_player_capture_chance_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
