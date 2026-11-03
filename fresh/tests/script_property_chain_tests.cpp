#include "pal4/script_property_chain.h"

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

constexpr std::uint32_t kHead = 0x1000u;
constexpr std::uint32_t kMiddle = 0x2000u;
constexpr std::uint32_t kTail = 0x3000u;
constexpr std::uint32_t kSource = 0x4000u;

struct Runtime final : pal4::IScriptPropertyChainRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t link_result = 0x1234u;
    std::map<std::uint32_t, std::int32_t> values;
    std::uint32_t last_link = 0u;
    std::uint32_t last_offset = 0u;
    std::int32_t last_first = 0;
    std::int32_t last_second = 0;
    std::vector<std::uint32_t> link_targets;
    std::uint32_t throw_link_offset = 0xffffffffu;
    std::uint32_t throws_remaining = 0u;

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
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t CallLink(std::uint32_t link,
                           std::uint32_t offset,
                           std::int32_t first,
                           std::int32_t second) override {
        last_link = link;
        last_offset = offset;
        last_first = first;
        last_second = second;
        link_targets.push_back(link);
        calls.push_back("link:" + std::to_string(offset));
        if (offset == throw_link_offset && throws_remaining != 0u) {
            --throws_remaining;
            throw std::runtime_error("link virtual failure");
        }
        return link_result;
    }
    std::int32_t CallValue(std::uint32_t object,
                           std::uint32_t offset,
                           std::int32_t argument) override {
        Check(argument == 27 || (offset == 16u && argument == 0),
              "value virtual argument");
        calls.push_back("value:" + std::to_string(object) + ":" +
                        std::to_string(offset));
        return values.at(object);
    }
};

void Constants() {
    Check(pal4::ScriptPropertyChain::ReturnZero() == 0u &&
              pal4::ScriptPropertyChain::CountTwo() == 2 &&
              pal4::ScriptPropertyChain::CountEight() == 8,
          "small property constants");
}

void BaseDestructor() {
    Runtime r;
    r.bytes[kHead + 4u] = 0u;
    Check(pal4::ScriptPropertyChain::DestroyBase12(kHead, r) == 0u &&
              r.words.at(kHead) == 0x845fd0u &&
              r.calls == std::vector<std::string>{
                  "write:4096", "read_byte:4100"},
          "inactive destructor writes base vtable and stops");
    r = Runtime{};
    r.bytes[kHead + 4u] = 0x7fu;
    r.words[kHead + 8u] = 0u;
    Check(pal4::ScriptPropertyChain::DestroyBase12(kHead, r) == 0x7fu &&
              r.calls == std::vector<std::string>{
                  "write:4096", "read_byte:4100", "read:4104"},
          "active destructor without link forwards flag byte");
    r = Runtime{};
    r.bytes[kHead + 4u] = 1u;
    r.words[kHead + 8u] = kMiddle;
    Check(pal4::ScriptPropertyChain::DestroyBase12(kHead, r) == 0x34u &&
              r.last_link == kMiddle && r.last_offset == 0u &&
              r.last_first == 1 &&
              r.calls.back() == "link:0",
          "active destructor calls link virtual zero with argument one");
}

void Delegates() {
    using DelegateOne = std::uint8_t (*)(
        std::uint32_t, std::int32_t,
        pal4::IScriptPropertyChainRuntime&);
    using DelegateTwo = std::uint8_t (*)(
        std::uint32_t, std::int32_t, std::int32_t,
        pal4::IScriptPropertyChainRuntime&);
    Runtime r;
    r.words[kHead + 8u] = 0u;
    Check(pal4::ScriptPropertyChain::AllowsAtFour(
              kHead, 27, r) == 1u &&
              pal4::ScriptPropertyChain::AllowsAtEight(
                  kHead, 27, 9, r) == 1u &&
              pal4::ScriptPropertyChain::AllowsAtTwelve(
                  kHead, 27, 9, r) == 1u &&
              r.calls.size() == 3u,
          "absent link permits all three delegates");
    const DelegateOne one = pal4::ScriptPropertyChain::AllowsAtFour;
    const DelegateTwo two[] = {
        pal4::ScriptPropertyChain::AllowsAtEight,
        pal4::ScriptPropertyChain::AllowsAtTwelve};
    r = Runtime{};
    r.words[kHead + 8u] = kMiddle;
    Check(one(kHead, 27, r) == 0x34u &&
              r.last_link == kMiddle && r.last_offset == 4u &&
              r.last_first == 27 && r.last_second == 0,
          "slot four delegates one argument and low byte");
    for (std::uint32_t i = 0u; i < 2u; ++i) {
        Check(two[i](kHead, 27, 9, r) == 0x34u &&
                  r.last_offset == 8u + 4u * i &&
                  r.last_first == 27 && r.last_second == 9,
              "slots eight and twelve delegate both arguments");
    }
    r.link_result = 0x100u;
    Check(two[0](kHead, 27, 9, r) == 0u,
          "delegate return uses low byte only");
}

void BroadcastAndUpdate() {
    Runtime r;
    r.values[kHead] = 10;
    r.words[kHead + 8u] = 0u;
    Check(pal4::ScriptPropertyChain::BroadcastValue(
              kHead, 27, r) == 10 &&
              r.calls == std::vector<std::string>{
                  "value:4096:344", "read:4104"},
          "empty chain returns head virtual value");
    r = Runtime{};
    r.values[kHead] = 10;
    r.values[kMiddle] = 20;
    r.values[kTail] = 30;
    r.words[kHead + 8u] = kMiddle;
    r.words[kMiddle + 8u] = kTail;
    r.words[kTail + 8u] = 0u;
    Check(pal4::ScriptPropertyChain::BroadcastValue(
              kHead, 27, r) == 30 &&
              r.calls == std::vector<std::string>{
                  "value:4096:344", "read:4104",
                  "value:8192:344", "read:8200",
                  "value:12288:344", "read:12296"},
          "broadcast walks links and returns last value");
    r.calls.clear();
    r.values[kHead] = -7;
    r.values[kMiddle] = -7;
    r.values[kTail] = 30;
    r.bytes[kHead + 4u] = 0u;
    r.bytes[kMiddle + 4u] = 1u;
    pal4::ScriptPropertyChain::UpdateFromCurrent(kHead, 27, r);
    Check(r.words.at(kHead + 8u) == kTail &&
              r.bytes.at(kHead + 4u) == 1u &&
              r.bytes.at(kMiddle + 4u) == 0u &&
              r.calls.front() == "value:4096:16",
          "current virtual value drives matching-link detachment");
}

void AssignLink() {
    Runtime r;
    r.bytes[kHead + 4u] = 1u;
    r.words[kHead + 8u] = kMiddle;
    r.bytes[kSource] = 0u;
    r.words[kSource + 4u] = kTail;
    Check(pal4::ScriptPropertyChain::AssignLinkFromSlot(
              kHead, kSource, r) == 0u &&
              r.words.at(kHead + 8u) == kTail &&
              r.bytes.at(kHead + 4u) == 0u &&
              r.last_link == kMiddle && r.last_offset == 0u &&
              r.calls == std::vector<std::string>{
                  "read:16388", "read:4104", "read_byte:4100",
                  "link:0", "read_byte:16384", "write:4104",
                  "write_byte:4100"},
          "changed link releases owned old link before assignment");
    r = Runtime{};
    r.bytes[kHead + 4u] = 0u;
    r.words[kHead + 8u] = kTail;
    r.bytes[kSource] = 1u;
    r.words[kSource + 4u] = kTail;
    Check(pal4::ScriptPropertyChain::AssignLinkFromSlot(
              kHead, kSource, r) == 0u &&
              r.bytes.at(kHead + 4u) == 1u &&
              r.words.at(kHead + 8u) == kTail &&
              r.calls == std::vector<std::string>{
                  "read:16388", "read:4104", "read_byte:16384",
                  "write_byte:4100", "write:4104"},
          "same pointer promotes ownership without deletion");
    r = Runtime{};
    r.bytes[kHead + 4u] = 1u;
    r.words[kHead + 8u] = kMiddle;
    Check(pal4::ScriptPropertyChain::AssignLinkFromSlot(
              kHead, kHead + 4u, r) == 0x34u &&
              r.last_link == kMiddle && r.last_offset == 0u &&
              r.calls == std::vector<std::string>{
                  "read:4104", "read_byte:4100", "link:0"},
          "alias path returns release callback low byte");
    r = Runtime{};
    r.bytes[kHead + 4u] = 0u;
    r.words[kHead + 8u] = kMiddle;
    Check(pal4::ScriptPropertyChain::AssignLinkFromSlot(
              kHead, kHead + 4u, r) == 0u &&
              r.calls == std::vector<std::string>{
                  "read:4104", "read_byte:4100"},
          "inactive alias does not release link");
}

void DetachAndPropagate() {
    Runtime r;
    r.bytes[kHead + 4u] = 1u;
    r.words[kHead + 8u] = kMiddle;
    r.bytes[kMiddle + 4u] = 1u;
    r.words[kMiddle + 8u] = kTail;
    Check(pal4::ScriptPropertyChain::DetachMatchingNext(
              kHead, 27, r) == 0x1234u &&
              r.words.at(kHead + 8u) == kTail &&
              r.bytes.at(kHead + 4u) == 1u &&
              r.bytes.at(kMiddle + 4u) == 0u &&
              r.calls[r.calls.size() - 2u] == "link:24" &&
              r.calls.back() == "link:0",
          "detach transfers successor and calls notify before release");
    r = Runtime{};
    r.bytes[kHead + 4u] = 0u;
    r.words[kHead + 8u] = kMiddle;
    r.bytes[kMiddle + 4u] = 0u;
    r.words[kMiddle + 8u] = kTail;
    r.words[kTail + 8u] = 0u;
    r.values[kMiddle] = 5;
    r.values[kTail] = 9;
    pal4::ScriptPropertyChain::PropagateByValue(
        kHead, 27, 5, r);
    Check(r.words.at(kHead + 8u) == kTail &&
              r.calls.front() == "read:4104" &&
              r.calls[1] == "value:8192:16" &&
              r.calls.back() == "read:12296",
          "propagation detaches matching node then advances past others");
    r = Runtime{};
    r.bytes[kHead + 4u] = 0u;
    r.words[kHead + 8u] = kMiddle;
    r.bytes[kMiddle + 4u] = 1u;
    r.words[kMiddle + 8u] = kMiddle;
    (void)pal4::ScriptPropertyChain::DetachMatchingNext(
        kHead, 27, r);
    Check(r.bytes.at(kHead + 4u) == 1u &&
              r.bytes.at(kMiddle + 4u) == 0u &&
              r.words.at(kHead + 8u) == kMiddle,
          "self-linked successor promotes ownership flag");
}

void ExceptionCleanup() {
    Runtime r;
    r.bytes[kHead + 4u] = 1u;
    r.words[kHead + 8u] = kMiddle;
    r.bytes[kSource] = 1u;
    r.words[kSource + 4u] = kTail;
    r.throw_link_offset = 0u;
    r.throws_remaining = 1u;
    bool threw = false;
    try {
        (void)pal4::ScriptPropertyChain::AssignLinkFromSlot(
            kHead, kSource, r);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && r.link_targets ==
                       std::vector<std::uint32_t>{kMiddle, kTail} &&
              r.words.at(kHead + 8u) == kMiddle &&
              r.bytes.at(kHead + 4u) == 1u,
          "failed old-link release cleans source temporary");

    r = Runtime{};
    r.bytes[kHead + 4u] = 1u;
    r.words[kHead + 8u] = kMiddle;
    r.bytes[kMiddle + 4u] = 1u;
    r.words[kMiddle + 8u] = kTail;
    r.throw_link_offset = 24u;
    r.throws_remaining = 1u;
    threw = false;
    try {
        (void)pal4::ScriptPropertyChain::DetachMatchingNext(
            kHead, 27, r);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && r.calls[r.calls.size() - 2u] == "link:24" &&
              r.calls.back() == "link:0" &&
              r.link_targets ==
                  std::vector<std::uint32_t>{kMiddle, kMiddle},
          "failed node notification releases owned temporary");

    r = Runtime{};
    r.bytes[kHead + 4u] = 0u;
    r.words[kHead + 8u] = kMiddle;
    r.bytes[kMiddle + 4u] = 0u;
    r.words[kMiddle + 8u] = kTail;
    r.throw_link_offset = 24u;
    r.throws_remaining = 1u;
    threw = false;
    try {
        (void)pal4::ScriptPropertyChain::DetachMatchingNext(
            kHead, 27, r);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && r.calls.back() == "link:24" &&
              r.link_targets ==
                  std::vector<std::uint32_t>{kMiddle},
          "unowned temporary is not released during unwind");
}

}  // namespace

int main() {
    try {
        Constants();
        BaseDestructor();
        Delegates();
        BroadcastAndUpdate();
        AssignLink();
        DetachAndPropagate();
        ExceptionCleanup();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
