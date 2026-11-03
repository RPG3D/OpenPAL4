#include "pal4/ui_text_buffer_record.h"
#include "pal4/dialog_text_record.h"
#include "pal4/effect_object_tree.h"
#include "pal4/effect_name_count_tree.h"
#include "pal4/effect_system_factory_text_tree.h"
#include "pal4/effect_system_task_factory_tree.h"

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

struct Runtime final : pal4::IUiTextBufferRecordRuntime {
    static constexpr std::uint32_t destination = 0x1000u;
    static constexpr std::uint32_t source = 0x2000u;
    static constexpr std::uint32_t empty = 0x9001u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool allocation_succeeds = true;
    bool throw_on_flag = false;

    Runtime() {
        bytes[source] = 7u;
        words[source + 4u] = 0x8001u;
        words[source + 8u] = 3u;
        words[source + 12u] = 8u;
        words[source + 16u] = 0x1234u;
        bytes[0x8000u] = 1u;
        bytes[0x8001u] = 'a';
        bytes[0x8002u] = 'b';
        bytes[0x8003u] = 'c';
        bytes[empty - 1u] = 0xffu;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EmptyText() override { return empty; }
    void ThrowRange() override { throw std::out_of_range("text range"); }
    void DetachAndReset(std::uint32_t record) override {
        calls.push_back("reset");
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    bool AllocateText(std::uint32_t record,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        calls.push_back("allocate " + std::to_string(length) +
                        " " + std::to_string(mode));
        if (!allocation_succeeds) return false;
        words[record + 4u] = 0xa001u;
        words[record + 12u] = length + 8u;
        return true;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        calls.push_back("size " + std::to_string(length));
        words[record + 8u] = length;
    }
    void SetErrorFlag(std::uint32_t,
                      std::uint8_t flag) override {
        calls.push_back("flag " + std::to_string(flag));
        if (throw_on_flag) throw std::runtime_error("flag");
    }
    void CopyBytes(std::uint32_t target, std::uint32_t source_address,
                   std::uint32_t length) override {
        calls.push_back("copy " + std::to_string(length));
        for (std::uint32_t i = 0; i < length; ++i)
            bytes[target + i] = bytes.at(source_address + i);
    }
};

void CheckNullAndShared() {
    Runtime ignored;
    pal4::UiTextBufferRecord::InitializeAndCopy(
        0u, ignored.source, ignored);
    Check(ignored.calls.empty() &&
              ignored.words.find(ignored.destination + 4u) ==
                  ignored.words.end(),
          "null destination has no effects");

    Runtime shared;
    pal4::UiTextBufferRecord::InitializeAndCopy(
        shared.destination, shared.source, shared);
    Check(shared.bytes.at(shared.destination) == 7u &&
              shared.words.at(shared.destination + 4u) == 0x8001u &&
              shared.words.at(shared.destination + 8u) == 3u &&
              shared.words.at(shared.destination + 12u) == 8u &&
              shared.words.at(shared.destination + 16u) == 0x1234u &&
              shared.bytes.at(0x8000u) == 2u &&
              shared.calls == std::vector<std::string>{"flag 1"},
          "shareable text copies metadata and increments reference");
}

void CheckAllocatedCopy() {
    Runtime unique;
    unique.bytes[0x8000u] = 0xfeu;
    pal4::UiTextBufferRecord::InitializeAndCopy(
        unique.destination, unique.source, unique);
    Check(unique.calls == (std::vector<std::string>{
              "allocate 3 1", "copy 3"}) &&
              unique.words.at(unique.destination + 4u) == 0xa001u &&
              unique.words.at(unique.destination + 8u) == 3u &&
              unique.words.at(unique.destination + 16u) == 0x1234u &&
              unique.bytes.at(0xa001u) == 'a' &&
              unique.bytes.at(0xa002u) == 'b' &&
              unique.bytes.at(0xa003u) == 'c' &&
              unique.bytes.at(0xa004u) == 0u &&
              unique.bytes.at(0x8000u) == 0xfeu,
          "saturated reference forces owned byte copy");

    Runtime empty;
    empty.words[empty.source + 4u] = 0u;
    empty.words[empty.source + 8u] = 0u;
    pal4::UiTextBufferRecord::InitializeAndCopy(
        empty.destination, empty.source, empty);
    Check(empty.calls == (std::vector<std::string>{
              "allocate 0 1", "copy 0"}) &&
              empty.bytes.at(0xa001u) == 0u &&
              empty.words.at(empty.destination + 8u) == 0u,
          "empty text still requests an allocation");

    Runtime failed;
    failed.allocation_succeeds = false;
    failed.bytes[0x8000u] = 0xffu;
    pal4::UiTextBufferRecord::InitializeAndCopy(
        failed.destination, failed.source, failed);
    Check(failed.calls == std::vector<std::string>{"allocate 3 1"} &&
              failed.words.at(failed.destination + 4u) == 0u &&
              failed.words.at(failed.destination + 8u) == 0u &&
              failed.words.at(failed.destination + 16u) == 0x1234u,
          "allocation failure keeps initialized text fields");
}

void CheckSelfAndFailureOrder() {
    Runtime self;
    self.bytes[self.destination] = 9u;
    self.words[self.destination + 4u] = 0x8001u;
    self.words[self.destination + 8u] = 3u;
    self.words[self.destination + 12u] = 8u;
    self.words[self.destination + 16u] = 0xbeefu;
    pal4::UiTextBufferRecord::InitializeAndCopy(
        self.destination, self.destination, self);
    Check(self.calls == (std::vector<std::string>{"reset", "reset"}) &&
              self.bytes.at(self.destination) == 9u &&
              self.words.at(self.destination + 4u) == 0u &&
              self.words.at(self.destination + 8u) == 0u &&
              self.words.at(self.destination + 16u) == 0xbeefu,
          "self-copy resets twice after initial zeroing");

    Runtime failing;
    failing.throw_on_flag = true;
    bool threw = false;
    try {
        pal4::UiTextBufferRecord::InitializeAndCopy(
            failing.destination, failing.source, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.words.at(failing.destination + 4u) == 0u &&
              failing.words.at(failing.destination + 8u) == 0u &&
              failing.words.find(failing.destination + 16u) ==
                  failing.words.end() &&
              failing.bytes.at(0x8000u) == 1u,
          "sharing failure occurs before ownership and tail field");
}

void CheckDialogTextRecord() {
    constexpr std::uint32_t fields = 0x3000u;
    Runtime shared;
    shared.words[fields] = 11u;
    shared.words[fields + 4u] = 22u;
    shared.words[fields + 8u] = 33u;
    Check(pal4::DialogTextRecord::InitializeAndCopy(
              shared.destination, shared.source, fields, shared) ==
              shared.destination &&
          shared.words.at(shared.destination + 4u) == 0x8001u &&
          shared.bytes.at(0x8000u) == 2u &&
          shared.words.at(shared.destination + 16u) == 11u &&
          shared.words.at(shared.destination + 20u) == 22u &&
          shared.words.at(shared.destination + 24u) == 33u,
          "Dialog record shares text and copies all three tail words");

    Runtime owned;
    owned.words[fields] = 1u;
    owned.words[fields + 4u] = 2u;
    owned.words[fields + 8u] = 3u;
    owned.bytes[0x8000u] = 0xfeu;
    (void)pal4::DialogTextRecord::InitializeAndCopy(
        owned.destination, owned.source, fields, owned);
    Check(owned.calls == (std::vector<std::string>{
              "allocate 3 1", "copy 3"}) &&
          owned.bytes.at(0xa001u) == 'a' &&
          owned.bytes.at(0xa004u) == 0u &&
          owned.words.at(owned.destination + 24u) == 3u,
          "Dialog record copies saturated text to owned storage");

    Runtime self;
    self.bytes[self.destination] = 9u;
    self.words[self.destination + 4u] = 0x8001u;
    self.words[self.destination + 8u] = 3u;
    self.words[self.destination + 12u] = 8u;
    self.words[fields] = 4u;
    self.words[fields + 4u] = 5u;
    self.words[fields + 8u] = 6u;
    (void)pal4::DialogTextRecord::InitializeAndCopy(
        self.destination, self.destination, fields, self);
    Check(self.calls == (std::vector<std::string>{"reset", "reset"}) &&
          self.words.at(self.destination + 8u) == 0u &&
          self.words.at(self.destination + 24u) == 6u,
          "Dialog self-copy preserves native reset path before tail copy");

    Runtime failing;
    failing.words[fields] = 1u;
    failing.words[fields + 4u] = 2u;
    failing.words[fields + 8u] = 3u;
    failing.throw_on_flag = true;
    bool threw = false;
    try {
        (void)pal4::DialogTextRecord::InitializeAndCopy(
            failing.destination, failing.source, fields, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw &&
          failing.words.find(failing.destination + 16u) ==
              failing.words.end(),
          "Dialog tail is not copied when text initialization throws");
}

void CheckDialogSingleSourceCopies() {
    Runtime ignored;
    pal4::DialogTextRecord::CopyTextAndValue(0u, ignored.source, ignored);
    pal4::DialogTextRecord::CopyWholeRecord(0u, ignored.source, ignored);
    Check(ignored.calls.empty() &&
          ignored.words.find(ignored.destination + 4u) == ignored.words.end(),
          "nullable Dialog copy entries skip null destinations");

    Runtime short_record;
    pal4::DialogTextRecord::CopyTextAndValue(
        short_record.destination, short_record.source, short_record);
    Check(short_record.words.at(short_record.destination + 16u) == 0x1234u &&
          short_record.words.at(short_record.destination + 4u) == 0x8001u &&
          short_record.bytes.at(0x8000u) == 2u,
          "20-byte Dialog copy preserves text sharing and value");

    Runtime whole;
    whole.words[whole.source + 20u] = 0x5678u;
    whole.words[whole.source + 24u] = 0x9abcu;
    whole.bytes[0x8000u] = 0xffu;
    pal4::DialogTextRecord::CopyWholeRecord(
        whole.destination, whole.source, whole);
    Check(whole.calls == (std::vector<std::string>{
              "allocate 3 1", "copy 3"}) &&
          whole.words.at(whole.destination + 16u) == 0x1234u &&
          whole.words.at(whole.destination + 20u) == 0x5678u &&
          whole.words.at(whole.destination + 24u) == 0x9abcu,
          "28-byte Dialog copy preserves all values after owned text copy");

    Runtime alternate;
    pal4::DialogTextRecord::CopyTextAndValueAlternate(
        alternate.destination, alternate.source, alternate);
    Check(alternate.words.at(alternate.destination + 16u) == 0x1234u &&
          alternate.words.at(alternate.destination + 4u) == 0x8001u &&
          alternate.bytes.at(0x8000u) == 2u,
          "alternate 20-byte Dialog entry copies text and value");
    Runtime alternate_null;
    pal4::DialogTextRecord::CopyTextAndValueAlternate(
        0u, alternate_null.source, alternate_null);
    Check(alternate_null.calls.empty(),
          "alternate 20-byte Dialog entry skips null destination");

    Runtime effect;
    pal4::EffectObjectRecord::CopyTextAndValue(
        effect.destination, effect.source, effect);
    Check(effect.words.at(effect.destination + 16u) == 0x1234u &&
          effect.words.at(effect.destination + 4u) == 0x8001u &&
          effect.bytes.at(0x8000u) == 2u,
          "effect-object record shares text and preserves its value");
    Runtime effect_null;
    pal4::EffectObjectRecord::CopyTextAndValue(
        0u, effect_null.source, effect_null);
    Check(effect_null.calls.empty(),
          "effect-object record skips null destination");

    Runtime count;
    pal4::EffectNameCountRecord::CopyTextAndCount(
        count.destination, count.source, count);
    Check(count.words.at(count.destination + 16u) == 0x1234u &&
          count.words.at(count.destination + 4u) == 0x8001u &&
          count.bytes.at(0x8000u) == 2u,
          "name-count record shares text and preserves its count");
    Runtime count_owned;
    count_owned.bytes[0x8000u] = 0xffu;
    pal4::EffectNameCountRecord::CopyTextAndCount(
        count_owned.destination, count_owned.source, count_owned);
    Check(count_owned.calls == (std::vector<std::string>{
              "allocate 3 1", "copy 3"}) &&
          count_owned.words.at(count_owned.destination + 16u) == 0x1234u,
          "saturated name-count text is copied into owned storage");
    Runtime count_null;
    pal4::EffectNameCountRecord::CopyTextAndCount(
        0u, count_null.source, count_null);
    Check(count_null.calls.empty(),
          "name-count record skips null destination");
}

void CheckEffectFactoryTextCopy() {
    Runtime shared;
    pal4::EffectSystemFactoryTextTree::CopyTextBuffer(
        shared.destination, shared.source, shared);
    Check(shared.words.at(shared.destination + 16u) == 0x1234u &&
              shared.words.at(shared.destination + 4u) == 0x8001u &&
              shared.bytes.at(0x8000u) == 2u,
          "factory record shares name and copies creator callback");

    Runtime saturated;
    saturated.bytes[0x8000u] = 0xfeu;
    pal4::EffectSystemFactoryTextTree::CopyTextBuffer(
        saturated.destination, saturated.source, saturated);
    Check(saturated.calls == (std::vector<std::string>{
              "allocate 3 1", "copy 3"}) &&
              saturated.words.at(saturated.destination + 4u) == 0xa001u &&
              saturated.words.at(saturated.destination + 16u) == 0x1234u,
          "saturated factory name gets owned storage");

    Runtime null_destination;
    pal4::EffectSystemFactoryTextTree::CopyTextBuffer(
        0u, null_destination.source, null_destination);
    Check(null_destination.calls.empty(),
          "null factory record is ignored");
}

void CheckEffectTaskFactoryTextCopy() {
    Runtime shared;
    Check(pal4::EffectSystemTaskFactoryTree::CopyRecord(
              shared.destination, shared.source, shared) ==
              shared.destination &&
              shared.words.at(shared.destination + 16u) == 0x1234u &&
              shared.words.at(shared.destination + 4u) == 0x8001u &&
              shared.bytes.at(0x8000u) == 2u,
          "task factory record shares name and copies creator callback");

    Runtime saturated;
    saturated.bytes[0x8000u] = 0xfeu;
    pal4::EffectSystemTaskFactoryTree::CopyRecordIfPresent(
        saturated.destination, saturated.source, saturated);
    Check(saturated.calls == (std::vector<std::string>{
              "allocate 3 1", "copy 3"}) &&
              saturated.words.at(saturated.destination + 4u) == 0xa001u &&
              saturated.words.at(saturated.destination + 16u) == 0x1234u,
          "saturated task factory name gets owned storage");

    Runtime null_destination;
    pal4::EffectSystemTaskFactoryTree::CopyRecordIfPresent(
        0u, null_destination.source, null_destination);
    Check(null_destination.calls.empty() &&
              null_destination.words.find(null_destination.destination + 4u) ==
                  null_destination.words.end(),
          "null task factory record skips text copy");
}

}  // namespace

int main() {
    try {
        CheckNullAndShared();
        CheckAllocatedCopy();
        CheckSelfAndFailureOrder();
        CheckDialogTextRecord();
        CheckDialogSingleSourceCopies();
        CheckEffectFactoryTextCopy();
        CheckEffectTaskFactoryTextCopy();
        std::cout << "ui_text_buffer_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
