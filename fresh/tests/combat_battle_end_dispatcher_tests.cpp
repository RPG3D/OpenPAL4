#include "pal4/combat_battle_end_dispatcher.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatBattleEndRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t game_entry = 10u;
    bool inactive = false;
    bool fail_copy = false;
    bool fail_rewards = false;
    bool fail_show = false;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
        words[address] = value;
    }
    std::uint32_t CombatSystemInstance() override {
        calls.emplace_back("system");
        return 0x2000u;
    }
    std::uint32_t GameDataInstance() override {
        calls.emplace_back("game");
        return 0x3000u;
    }
    std::uint32_t GameEntry(std::uint32_t game,
                            std::uint32_t index) override {
        Check(game == 0x3000u && index < 2u, "game entry source");
        calls.emplace_back("get-entry:" + std::to_string(index));
        return game_entry;
    }
    void SetGameEntry(std::uint32_t game, std::uint32_t index,
                      std::uint32_t value) override {
        Check(game == 0x3000u && index < 2u,
              "game entry destination");
        calls.emplace_back("set-entry:" + std::to_string(index) + ":" +
                           std::to_string(value));
        game_entry = value;
    }
    std::uint32_t CombatConsole() override {
        calls.emplace_back("console");
        return 0x4000u;
    }
    void UnsubscribeConsole(std::uint32_t console) override {
        Check(console == 0x4000u, "combat console identity");
        calls.emplace_back("unsubscribe");
    }
    bool IsCombatInactive(std::uint32_t system) override {
        Check(system == 0x2000u, "combat system identity");
        calls.emplace_back("inactive");
        return inactive;
    }
    std::uint32_t PrepareReport() override {
        calls.emplace_back("prepare");
        return 0x5000u;
    }
    std::uint32_t CopyReport(std::uint32_t report) override {
        Check(report == 0x5000u, "report copy source");
        calls.emplace_back("copy-report");
        if (fail_copy) throw std::runtime_error("copy report");
        return 0x6000u;
    }
    std::uint32_t ProcessRewards(std::uint32_t report) override {
        Check(report == 0x6000u, "reward report source");
        calls.emplace_back("rewards");
        if (fail_rewards) throw std::runtime_error("rewards");
        return 0x7000u;
    }
    void ShowEnding(std::uint32_t system,
                    std::uint32_t prize) override {
        Check(system == 0x2000u && prize == 0x7000u,
              "ending UI owner and prize");
        calls.emplace_back("show-ending");
        if (fail_show) throw std::runtime_error("show ending");
    }
    void DestroyPrize(std::uint32_t prize) override {
        Check(prize == 0x7000u, "prize cleanup");
        calls.emplace_back("destroy-prize");
    }
    void DestroyCopiedReport(std::uint32_t report) override {
        Check(report == 0x6000u, "copied report cleanup");
        calls.emplace_back("destroy-copy");
    }
    std::uint32_t DestroyReport(std::uint32_t report) override {
        Check(report == 0x5000u, "original report cleanup");
        calls.emplace_back("destroy-report");
        return 0xbeefu;
    }
    void ShowDefeatBanner(std::string_view imageset,
                          std::string_view image, float scale,
                          float width, float height) override {
        Check(imageset == "zi0" && image == "zhangdoushibai" &&
                  scale == 1.0f && width == 400.0f &&
                  height == 280.0f,
              "defeat banner uses original 0x57adfa image scale1, not delay2");
        calls.emplace_back("banner");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 24u, "callback object allocation size");
        calls.emplace_back("allocate");
        return fail_allocation ? 0u : 0x8000u;
    }
    std::uint32_t ScheduleClose(std::uint32_t owner,
                                std::uint32_t key, float delay,
                                std::uint32_t callback) override {
        Check(key == pal4::CombatBattleEndDispatcher::kCloseEventKey &&
                  (callback == 0x8000u || callback == 0u),
              "scheduled close key and callback");
        calls.emplace_back("schedule:" + std::to_string(owner) + ":" +
                           std::to_string(static_cast<int>(delay)) + ":" +
                           std::to_string(callback));
        return 0x91u;
    }
    void EnqueueClose(std::uint32_t queue, std::uint32_t key,
                      std::uint32_t callback) override {
        Check(key == pal4::CombatBattleEndDispatcher::kCloseEventKey &&
                  callback == 0x8000u,
              "queued close key and callback");
        calls.emplace_back("enqueue:" + std::to_string(queue));
    }
    std::uint32_t CleanupQueuedEvent() override {
        calls.emplace_back("cleanup-queue-event");
        return 0x92u;
    }
};

bool Contains(const Runtime& runtime, const std::string& item) {
    for (const auto& call : runtime.calls)
        if (call == item) return true;
    return false;
}

void EarlyReturn() {
    Runtime runtime;
    runtime.words[0x2000u + 508u] = 6u;
    Check(pal4::CombatBattleEndDispatcher::Handle(
              0x1000u, runtime) == 6u &&
              runtime.calls == std::vector<std::string>{
                  "system", "read:8700"},
          "state six returns before event and console work");
}

void RewardPaths() {
    for (const bool inactive : {false, true}) {
        Runtime runtime;
        runtime.inactive = inactive;
        runtime.words[0x1000u] = 1u;
        Check(pal4::CombatBattleEndDispatcher::Handle(
                  0x1000u, runtime) == 0xbeefu &&
                  runtime.words[0x2000u + 508u] == 6u &&
                  runtime.game_entry == 11u &&
                  runtime.words[0x7000u + 16u] == 1u,
              "both victory modes update state, metric and prize result");
        Check(Contains(runtime, "set-entry:0:11") &&
                  Contains(runtime, "unsubscribe") &&
                  Contains(runtime, "prepare") &&
                  Contains(runtime, "copy-report") &&
                  Contains(runtime, "rewards") &&
                  Contains(runtime, "show-ending") &&
                  runtime.calls[runtime.calls.size() - 3] ==
                      "destroy-prize" &&
                  runtime.calls[runtime.calls.size() - 2] ==
                      "destroy-copy" &&
                  runtime.calls.back() == "destroy-report",
              "victory composes report and cleans temporaries in reverse");
    }
}

void DefeatAndQueuedEvents() {
    Runtime defeat;
    defeat.words[0x1000u] = 0u;
    Check(pal4::CombatBattleEndDispatcher::Handle(
              0x1000u, defeat) == 0x91u &&
              Contains(defeat, "banner") &&
              Contains(defeat, "schedule:8192:4:32768") &&
              defeat.words[0x8000u] ==
                  pal4::CombatBattleEndDispatcher::kCallbackVtable &&
              defeat.words[0x8004u] == 0x2000u &&
              defeat.words[0x8008u] ==
                  pal4::CombatBattleEndDispatcher::kEndCallback &&
              defeat.words[0x800cu] == 0u &&
              defeat.words[0x8010u] == 0u,
          "active defeat shows banner and schedules four-second close");

    Runtime active_other;
    active_other.words[0x1000u] = 2u;
    active_other.words[0x2000u + 412u] = 0x9000u;
    Check(pal4::CombatBattleEndDispatcher::Handle(
              0x1000u, active_other) == 0x92u &&
              Contains(active_other, "set-entry:1:11") &&
              Contains(active_other, "enqueue:36876") &&
              active_other.words[0x8010u] == 2u &&
              active_other.calls.back() == "cleanup-queue-event",
          "active non-victory event enters the queue immediately");

    Runtime inactive_other;
    inactive_other.inactive = true;
    inactive_other.words[0x1000u] = 3u;
    inactive_other.words[0x2000u + 412u] = 0x9000u;
    Check(pal4::CombatBattleEndDispatcher::Handle(
              0x1000u, inactive_other) == 0x91u &&
              Contains(inactive_other, "schedule:36864:0:32768") &&
              inactive_other.words[0x8010u] == 3u &&
              !Contains(inactive_other, "banner"),
          "inactive non-victory event schedules immediate close");

    Runtime failed_allocation;
    failed_allocation.fail_allocation = true;
    failed_allocation.words[0x1000u] = 0u;
    Check(pal4::CombatBattleEndDispatcher::Handle(
              0x1000u, failed_allocation) == 0x91u &&
              Contains(failed_allocation, "schedule:8192:4:0"),
          "native null callback allocation is forwarded to scheduler");
}

void RewardFailures() {
    for (int failure = 0; failure != 3; ++failure) {
        Runtime runtime;
        runtime.words[0x1000u] = 1u;
        runtime.fail_copy = failure == 0;
        runtime.fail_rewards = failure == 1;
        runtime.fail_show = failure == 2;
        bool raised = false;
        try {
            static_cast<void>(pal4::CombatBattleEndDispatcher::Handle(
                0x1000u, runtime));
        } catch (const std::runtime_error&) {
            raised = true;
        }
        Check(raised && runtime.calls.back() == "destroy-report",
              "failed reward pipeline always destroys source report");
        Check(Contains(runtime, "destroy-copy") == (failure != 0) &&
                  Contains(runtime, "destroy-prize") == (failure == 2),
              "only constructed reward temporaries are destroyed");
    }
}

}  // namespace

int main() {
    try {
        EarlyReturn();
        RewardPaths();
        DefeatAndQueuedEvents();
        RewardFailures();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
