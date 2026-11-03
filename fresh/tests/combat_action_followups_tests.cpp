#include "pal4/combat_action_followups.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatActionFollowupsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::int32_t converted = 0;
    bool fixed_conversion = false;
    bool throw_on_reset = false;
    std::int32_t free_result = 43;
    std::uint32_t notify_result = 0x1234u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return fixed_conversion ? converted : static_cast<std::int32_t>(value);
    }
    std::uint32_t EnumerateTargets(std::uint32_t actor,
                                   std::uint8_t selector) override {
        calls.push_back("enumerate " + std::to_string(actor) + " " +
                        std::to_string(selector));
        return 5000u;
    }
    std::uint32_t VectorBegin(std::uint32_t vector) override {
        calls.push_back("begin " + std::to_string(vector));
        return 6000u;
    }
    std::uint32_t VectorEnd(std::uint32_t vector) override {
        calls.push_back("end " + std::to_string(vector));
        return words[5996u];
    }
    void ResetSlot(std::uint32_t actor, std::uint32_t slot,
                   std::int32_t value) override {
        calls.push_back("reset " + std::to_string(actor) + " " +
                        std::to_string(slot) + " " + std::to_string(value));
        if (throw_on_reset) throw std::runtime_error("reset failed");
    }
    void RefreshActor(std::uint32_t actor) override {
        calls.push_back("refresh " + std::to_string(actor));
    }
    std::int32_t FreeVector(std::uint32_t vector) override {
        calls.push_back("free " + std::to_string(vector));
        return free_result;
    }
    std::int32_t TransferValue(std::uint32_t actor,
                               std::int32_t value) override {
        calls.push_back("transfer " + std::to_string(actor) + " " +
                        std::to_string(value));
        return actor == 20u ? 91 : 72;
    }
    void CleanupCondition(std::uint32_t actor, std::int32_t id,
                          std::uint32_t mode) override {
        calls.push_back("cleanup " + std::to_string(actor) + " " +
                        std::to_string(id) + " " + std::to_string(mode));
    }
    std::uint32_t NotifyCleanup(std::uint32_t actor) override {
        calls.push_back("notify " + std::to_string(actor));
        return notify_result;
    }
};

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[1000u + offset] = std::bit_cast<std::uint32_t>(value);
}

void ResetOtherTargets() {
    Runtime empty;
    empty.words[5996u] = 6000u;
    Check(pal4::CombatActionFollowups::ResetOtherTargets(0x1234u, empty) ==
              43 &&
              empty.calls == std::vector<std::string>{
                  "enumerate 4660 52", "begin 5000", "end 5000", "free 5000"},
          "empty target collection returns vector release result");

    Runtime mixed;
    mixed.words[5996u] = 6016u;
    mixed.words[6000u] = 0x1234u;
    mixed.words[6004u] = 0u;
    mixed.words[6008u] = 77u;
    mixed.words[6012u] = 0x1234u;
    Check(pal4::CombatActionFollowups::ResetOtherTargets(0x1234u, mixed) ==
              43 &&
              mixed.calls == std::vector<std::string>{
                  "enumerate 4660 52", "begin 5000", "end 5000",
                  "read 6000", "read 6004", "reset 0 7 0", "refresh 0",
                  "read 6008", "reset 77 7 0", "refresh 77", "read 6012",
                  "free 5000"},
          "only pointer-identical entries are skipped, in native order");

    Runtime throws;
    throws.words[5996u] = 6004u;
    throws.words[6000u] = 77u;
    throws.throw_on_reset = true;
    bool caught = false;
    try {
        static_cast<void>(
            pal4::CombatActionFollowups::ResetOtherTargets(0x1234u, throws));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls.back() == "free 5000",
          "temporary vector is released when reset throws");
}

void Transfer() {
    for (const auto value : {7, -3, 0, std::numeric_limits<std::int32_t>::min()}) {
        Runtime runtime;
        runtime.fixed_conversion = true;
        runtime.converted = value;
        SetFloat(runtime, 20u, 1.0f);
        const auto negative = std::bit_cast<std::int32_t>(
            0u - std::bit_cast<std::uint32_t>(value));
        Check(pal4::CombatActionFollowups::Transfer(1000u, 20u, 30u,
                                                    runtime) == 91 &&
                  runtime.calls == std::vector<std::string>{
                      "read 1020", "convert 1.000000",
                      "transfer 30 " + std::to_string(negative),
                      "transfer 20 " + std::to_string(value)},
              "transfer negates source delta and forwards target result");
    }
}

void CleanupConditions() {
    Runtime zero;
    for (std::uint32_t i = 0; i < 9u; ++i) SetFloat(zero, 20u + i * 4u, 0.0f);
    Check(pal4::CombatActionFollowups::CleanupConditions(1000u, 99u,
                                                         zero) == 0u &&
              zero.calls.size() == 18u &&
              zero.calls.front() == "read 1020" &&
              zero.calls.back() == "convert 0.000000",
          "all nine zero IDs are read without cleanup notification");

    Runtime mixed;
    const float values[9]{0.0f, 3.9f, 0.0f, -2.8f, 0.0f,
                          0.0f, 7.0f, 0.0f, 5.0f};
    for (std::uint32_t i = 0; i < 9u; ++i)
        SetFloat(mixed, 20u + i * 4u, values[i]);
    Check(pal4::CombatActionFollowups::CleanupConditions(1000u, 99u,
                                                         mixed) == 0x34u &&
              mixed.calls.size() == 23u &&
              mixed.calls[4] == "cleanup 99 3 0" &&
              mixed.calls[9] == "cleanup 99 -2 0" &&
              mixed.calls[16] == "cleanup 99 7 0" &&
              mixed.calls[21] == "cleanup 99 5 0" &&
              mixed.calls.back() == "notify 99",
          "nonzero IDs are cleaned in order and notification returns low byte");
}

}  // namespace

int main() {
    try {
        ResetOtherTargets();
        Transfer();
        CleanupConditions();
        std::cout << "combat action followups tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
