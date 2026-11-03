#include "pal4/actor_property_hooks.h"

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

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kActor = 0x2000u;
constexpr std::uint32_t kOwner = 0x3000u;
constexpr std::uint32_t kPending = 0x4000u;
constexpr std::uint32_t kRecord = 0x5000u;

struct Runtime final : pal4::IActorPropertyHookRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::int32_t> actor_values;
    std::size_t actor_cursor = 0u;
    std::int32_t forwarded_value = 0;
    std::int32_t adjusted_delta = 0;
    std::uint32_t adjusted_pending = 0u;
    std::int32_t dispatch_result = 17;
    std::uint32_t dispatch_key = 0u;
    std::int32_t owner_set_result = 29;
    std::int32_t finish_result = 31;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void ResetTemporaryHandle(std::uint32_t&,
                              std::uint32_t&) override {
        throw std::runtime_error("unexpected handle reset");
    }
    std::int32_t ActorIntVirtual(std::uint32_t actor,
                                 std::uint32_t offset,
                                 std::uint32_t index) override {
        Check(actor == kActor && offset == 52u && index == 7u,
              "actor integer getter arguments");
        calls.emplace_back("actor_get");
        Check(actor_cursor < actor_values.size(), "actor getter count");
        return actor_values[actor_cursor++];
    }
    void PendingAdjustVirtual(std::uint32_t pending,
                              std::uint32_t offset,
                              std::uint32_t index,
                              std::int32_t delta) override {
        Check(offset == 84u && index == 7u,
              "pending adjustment arguments");
        adjusted_pending = pending;
        adjusted_delta = delta;
        calls.emplace_back("pending_adjust");
    }
    std::int32_t OwnerSetVirtual(std::uint32_t owner,
                                 std::uint32_t offset,
                                 std::uint32_t actor,
                                 std::uint32_t index,
                                 std::int32_t value) override {
        Check(owner == kOwner && offset == 168u && actor == kActor,
              "owner setter arguments");
        forwarded_value = value;
        calls.push_back("owner_set:" + std::to_string(index));
        return owner_set_result;
    }
    std::int32_t OwnerFinishVirtual(std::uint32_t owner,
                                    std::uint32_t offset,
                                    std::uint32_t actor,
                                    std::uint32_t value) override {
        Check(owner == kOwner && offset == 240u && actor == kActor &&
                  value == 0u,
              "owner finish arguments");
        calls.emplace_back("owner_finish");
        return finish_result;
    }
    std::uint32_t TemporaryRecord() override {
        calls.emplace_back("temporary_record");
        return kRecord;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x6000u;
    }
    std::int32_t DispatchEvent(std::uint32_t router,
                               std::uint32_t key,
                               std::uint32_t record,
                               std::uint32_t mode) override {
        Check(router == 0x6000u && record == kRecord && mode == 1u &&
                  words.at(kRecord) == kActor + 716u,
              "facet event arguments and record");
        dispatch_key = key;
        calls.emplace_back("dispatch");
        return dispatch_result;
    }
};

void ConstructorsAndFlag() {
    Runtime r;
    Check(pal4::ActorPropertyHooks::SetPendingPointer(
              kObject, kPending, r) == kPending &&
              r.words.at(kObject + 12u) == kPending,
          "pending pointer assignment");
    r = Runtime{};
    Check(pal4::ActorPropertyHooks::ConstructPendingPointer(
              kObject, r) == kObject &&
              r.bytes.at(kObject + 4u) == 0u &&
              r.words.at(kObject + 8u) == 0u &&
              r.words.at(kObject + 12u) == 0u &&
              r.words.at(kObject) == 0x8463f0u &&
              r.calls == std::vector<std::string>{
                  "byte:4100", "write:4104", "write:4096",
                  "write:4108"},
          "pending pointer constructor order");
    r = Runtime{};
    Check(pal4::ActorPropertyHooks::ConstructPendingGate(
              kObject, r) == kObject &&
              r.words.at(kObject) == 0x846550u &&
              r.calls == std::vector<std::string>{
                  "byte:4100", "write:4104", "write:4096"},
          "pending gate constructor order");
    r = Runtime{};
    Check(pal4::ActorPropertyHooks::ConstructReactiveGate(
              kObject, r) == kObject &&
              r.words.at(kObject) == 0x8466b0u &&
              r.bytes.at(kObject + 12u) == 0u &&
              r.calls == std::vector<std::string>{
                  "byte:4100", "write:4104", "byte:4108",
                  "write:4096"},
          "reactive gate constructor order");
    r.calls.clear();
    pal4::ActorPropertyHooks::NotifyReactiveGate(kObject, 9115u, r);
    Check(r.calls.empty(), "other event leaves reactive flag alone");
    pal4::ActorPropertyHooks::NotifyReactiveGate(kObject, 9116u, r);
    Check(r.bytes.at(kObject + 12u) == 1u &&
              r.calls == std::vector<std::string>{"byte:4108"},
          "event 9116 sets byte flag");
}

void PendingAdjustment() {
    Runtime r;
    r.words[kObject + 8u] = kOwner;
    Check(pal4::ActorPropertyHooks::ForwardWithPendingAdjustment(
              kObject, kActor, 6u, 12, r) == 29 &&
              r.forwarded_value == 12 &&
              r.calls == std::vector<std::string>{
                  "read:4104", "owner_set:6"},
          "other slot forwards without touching pending pointer");
    r = Runtime{};
    r.words[kObject + 8u] = kOwner;
    r.words[kObject + 12u] = kPending;
    r.actor_values = {10, 11};
    Check(pal4::ActorPropertyHooks::ForwardWithPendingAdjustment(
              kObject, kActor, 7u, 5, r) == 29 &&
              r.adjusted_pending == kPending &&
              r.adjusted_delta == -5 &&
              r.forwarded_value == 11 &&
              r.words.at(kObject + 12u) == 0u &&
              r.actor_cursor == 2u &&
              r.calls == std::vector<std::string>{
                  "read:4108", "actor_get", "actor_get",
                  "read:4108", "pending_adjust", "write:4108",
                  "read:4104", "owner_set:7"},
          "decreasing slot seven adjusts pending then forwards reread");
    r = Runtime{};
    r.words[kObject + 8u] = kOwner;
    r.words[kObject + 12u] = kPending;
    r.actor_values = {10};
    Check(pal4::ActorPropertyHooks::ForwardWithPendingAdjustment(
              kObject, kActor, 7u, 12, r) == 29 &&
              r.forwarded_value == 12 && r.actor_cursor == 1u &&
              r.words.at(kObject + 12u) == 0u,
          "nondecreasing slot seven only clears pending pointer");
}

void EventAndGate() {
    Runtime r;
    r.words[0x8b59c0u] = 0x7000u;
    r.words[kObject + 12u] = 1u;
    Check(pal4::ActorPropertyHooks::DispatchThenSetPending(
              kObject, kActor, kPending, r) == 17 &&
              r.dispatch_key == 0x7000u &&
              r.words.at(kObject + 12u) == kPending &&
              r.calls[r.calls.size() - 2u] == "dispatch" &&
              r.calls.back() == "write:4108",
          "event completes before pending pointer assignment");
    r = Runtime{};
    r.words[kObject + 12u] = kPending;
    Check(pal4::ActorPropertyHooks::ForwardUnlessPending(
              kObject, kActor, 7u, 3, r) == 0 &&
              r.words.at(kObject + 12u) == 0u &&
              r.calls == std::vector<std::string>{
                  "read:4108", "write:4108"},
          "pending slot seven is consumed without owner call");
    r = Runtime{};
    r.words[kObject + 8u] = kOwner;
    Check(pal4::ActorPropertyHooks::ForwardUnlessPending(
              kObject, kActor, 6u, 3, r) == 29 &&
              r.forwarded_value == 3 &&
              r.calls == std::vector<std::string>{
                  "read:4104", "owner_set:6"},
          "other slot forwards directly");
    r = Runtime{};
    r.words[0x8b59c0u] = 0x7000u;
    r.words[kObject + 8u] = kOwner;
    Check(pal4::ActorPropertyHooks::DispatchThenFinish(
              kObject, kActor, r) == 31 &&
              r.dispatch_key == 0x7000u &&
              r.calls[r.calls.size() - 3u] == "dispatch" &&
              r.calls[r.calls.size() - 2u] == "read:4104" &&
              r.calls.back() == "owner_finish",
          "finish reads owner only after event dispatch");
}

void ReactiveAdjustment() {
    Runtime r;
    r.words[kObject + 8u] = kOwner;
    r.bytes[kObject + 12u] = 1u;
    r.actor_values = {10, 11};
    Check(pal4::ActorPropertyHooks::ForwardWithReactiveAdjustment(
              kObject, kActor, 7u, 5, r) == 29 &&
              r.forwarded_value == 16 && r.actor_cursor == 2u &&
              r.bytes.at(kObject + 12u) == 1u &&
              r.calls == std::vector<std::string>{
                  "read_byte:4108", "actor_get", "actor_get",
                  "read:4104", "owner_set:7"},
          "reactive slot seven rereads and adjusts without clearing flag");
    r = Runtime{};
    r.words[kObject + 8u] = kOwner;
    r.bytes[kObject + 12u] = 1u;
    r.actor_values = {10};
    Check(pal4::ActorPropertyHooks::ForwardWithReactiveAdjustment(
              kObject, kActor, 7u, 12, r) == 29 &&
              r.forwarded_value == 12 && r.actor_cursor == 1u,
          "nondecreasing reactive value forwards unchanged");
    r = Runtime{};
    r.words[kObject + 8u] = kOwner;
    Check(pal4::ActorPropertyHooks::ForwardWithReactiveAdjustment(
              kObject, kActor, 6u, 12, r) == 29 &&
              r.calls == std::vector<std::string>{
                  "read:4104", "owner_set:6"},
          "other slot skips reactive flag and actor getter");
}

}  // namespace

int main() {
    try {
        ConstructorsAndFlag();
        PendingAdjustment();
        EventAndGate();
        ReactiveAdjustment();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
