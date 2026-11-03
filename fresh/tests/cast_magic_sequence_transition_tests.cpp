#include "pal4/cast_magic_sequence_task.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kOwner = 0x4000u;
constexpr std::uint32_t kMagic = 0x5000u;

struct Runtime final : pal4::ICastMagicTransitionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kOwner}, {kTask + 48u, kTarget},
        {kTask + 52u, kMagic}, {kSource + 0x324u, 1u},
        {kTarget + 0x324u, 2u}};
    std::map<std::uint32_t, bool> ready{{kSource, true}, {kTarget, true}};
    std::vector<std::uint32_t> primary;
    std::vector<std::uint32_t> secondary;
    std::map<std::uint32_t, std::size_t> cursor;
    std::vector<std::string> calls;
    bool can_cast = false;
    bool property = false;
    bool throw_on_current = false;
    std::uint32_t opened = 0u;
    std::uint32_t action_source = 0u;
    std::uint32_t action_target = 0u;
    std::uint32_t action_magic = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.push_back("ready:" + std::to_string(actor));
        return ready[actor];
    }
    std::uint32_t OpenIterator(std::uint32_t actor,
                               std::uint32_t offset) override {
        Check((offset == 464u && actor == kTarget) ||
                  (offset == 472u && actor == kOwner),
              "iterator owner and virtual slot");
        calls.push_back("open:" + std::to_string(offset));
        ++opened;
        return offset;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        calls.push_back("end");
        const auto& values = iterator == 464u ? primary : secondary;
        return cursor[iterator] >= values.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        calls.push_back("current");
        if (throw_on_current) throw std::runtime_error("iterator current");
        const auto& values = iterator == 464u ? primary : secondary;
        return values.at(cursor[iterator]);
    }
    void IteratorNext(std::uint32_t iterator) override {
        calls.push_back("next");
        ++cursor[iterator];
    }
    void DestroyIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check((iterator == 464u || iterator == 472u) && flags == 1u,
              "deleting iterator destructor flags");
        calls.push_back("destroy:" + std::to_string(iterator));
    }
    bool CanCastMagic(std::uint32_t actor,
                      std::uint32_t magic) override {
        Check(actor == kSource && magic == kMagic, "cast gate arguments");
        calls.push_back("can_cast");
        return can_cast;
    }
    bool ActorPropertySlot8(std::uint32_t actor,
                            std::uint32_t index) override {
        Check(actor == kSource && index == 3u, "property slot arguments");
        calls.push_back("property");
        return property;
    }
    std::uint32_t MagicAction(std::uint32_t magic,
                              std::uint32_t source,
                              std::uint32_t target) override {
        calls.push_back("magic_action");
        action_magic = magic;
        action_source = source;
        action_target = target;
        return 0xaaaau;
    }
    std::uint32_t NormalAction(std::uint32_t source,
                               std::uint32_t target,
                               std::uint32_t mode) override {
        Check(mode == 0u, "normal action mode");
        calls.push_back("normal_action");
        action_source = source;
        action_target = target;
        return 0xbbbbu;
    }
};

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void InitialGatesAndDirectTarget() {
    Runtime r;
    r.ready[kSource] = false;
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0u &&
              r.calls == std::vector<std::string>{"ready:8192"},
          "unready source returns before task reads");

    r = Runtime{};
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0xbbbbu &&
              r.action_source == kSource && r.action_target == kTarget &&
              r.opened == 0u && !Called(r, "property"),
          "ready distinct-group target uses normal action and short circuit");
}

void MagicActionAndPropertyGate() {
    Runtime r;
    r.can_cast = true;
    r.property = true;
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0xaaaau &&
              r.action_magic == kMagic && r.action_source == kSource &&
              r.action_target == kTarget && Called(r, "property"),
          "both gates choose magic action");

    r = Runtime{};
    r.can_cast = true;
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0xbbbbu &&
              Called(r, "property"),
          "false property falls through to normal action");
}

void FirstAndSecondCandidateScans() {
    Runtime r;
    r.ready[kTarget] = false;
    r.primary = {kSource, 0x6000u, 0x7000u};
    r.ready[0x7000u] = true;
    r.words[0x7000u + 0x324u] = 2u;
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0xbbbbu &&
              r.action_target == 0x7000u && Called(r, "destroy:464") &&
              r.cursor[464u] == 2u,
          "first iterator skips source and unready candidates");

    r = Runtime{};
    r.ready[kTarget] = false;
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0u &&
              Called(r, "destroy:464") && !Called(r, "can_cast"),
          "empty first iterator returns without action");

    r = Runtime{};
    r.words[kTarget + 0x324u] = 1u;
    r.secondary = {kSource, 0x8000u, 0x9000u};
    r.ready[0x9000u] = true;
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0xbbbbu &&
              r.action_target == 0x9000u && Called(r, "destroy:472") &&
              r.cursor[472u] == 2u,
          "same group scans owner candidates");

    r = Runtime{};
    r.words[kTask + 48u] = kSource;
    Check(pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r) == 0u &&
              Called(r, "destroy:472"),
          "self target also scans and returns zero when empty");
}

void ExceptionReleasesIterator() {
    Runtime r;
    r.ready[kTarget] = false;
    r.primary = {kSource};
    r.throw_on_current = true;
    bool caught = false;
    try { (void)pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Called(r, "destroy:464"),
          "first iterator releases on exception");

    r = Runtime{};
    r.words[kTarget + 0x324u] = 1u;
    r.secondary = {kSource};
    r.throw_on_current = true;
    caught = false;
    try { (void)pal4::CastMagicSequenceTask::CreateNext(kTask, kSource, r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Called(r, "destroy:472"),
          "second iterator releases on exception");
}

}  // namespace

int main() {
    InitialGatesAndDirectTarget();
    MagicActionAndPropertyGate();
    FirstAndSecondCandidateScans();
    ExceptionReleasesIterator();
    std::cout << "CastMagic transition tests passed\n";
}
