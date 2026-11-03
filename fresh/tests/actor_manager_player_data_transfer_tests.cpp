#include "pal4/actor_manager_player_data_transfer.h"

#include <algorithm>
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
constexpr std::uint32_t source = 0x2000u;
constexpr std::uint32_t temporary = 0x7000u;

struct Runtime final : pal4::IActorManagerPlayerDataTransferRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> freed;
    std::uint32_t temp_text = 0x8000u;
    std::uint32_t free_result = 0x654321u;
    std::uint32_t fail_allocation = 0u;
    std::uint32_t state7 = 0u;
    std::uint32_t final_result = 0xfeedu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        trace.emplace_back("read-byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        trace.emplace_back("word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        trace.emplace_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t from,
                   std::uint32_t count) override {
        trace.emplace_back("copy:" + std::to_string(destination) + ":" +
                           std::to_string(from) + ":" +
                           std::to_string(count));
    }
    void CopyProperty2(std::uint32_t destination,
                       std::uint32_t from) override {
        trace.emplace_back("property2:" + std::to_string(destination) +
                           ":" + std::to_string(from));
    }
    void CopyProperty(std::uint32_t destination,
                      std::uint32_t from) override {
        trace.emplace_back("property:" + std::to_string(destination) +
                           ":" + std::to_string(from));
    }
    void CopyArray(std::uint32_t destination,
                   std::uint32_t from) override {
        trace.emplace_back("array:" + std::to_string(destination) + ":" +
                           std::to_string(from));
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t from) override {
        trace.emplace_back("text:" + std::to_string(destination) + ":" +
                           std::to_string(from));
        if (destination == temporary) words[temporary + 4u] = temp_text;
    }
    void EraseText(std::uint32_t record, std::uint32_t start,
                   std::uint32_t count) override {
        trace.emplace_back("erase:" + std::to_string(record) + ":" +
                           std::to_string(start) + ":" +
                           std::to_string(count));
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 1u, "reverse text allocation mode one");
        trace.emplace_back("allocate:" + std::to_string(record) + ":" +
                           std::to_string(length));
        if (record == fail_allocation) return false;
        words[record + 4u] = 0x9000u + record - source;
        return true;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        trace.emplace_back("size:" + std::to_string(record) + ":" +
                           std::to_string(length));
        words[record + 8u] = length;
    }
    void ActorSlot16(std::uint32_t actual_actor,
                     std::uint32_t argument) override {
        Check(actual_actor == actor && argument == 0u,
              "actor virtual slot 16 mode zero");
        trace.emplace_back("slot16");
    }
    std::uint32_t ActorSlot20(std::uint32_t actual_actor) override {
        Check(actual_actor == source + 1620u,
              "save player actor slot 20 before reverse copy");
        trace.emplace_back("slot20");
        return 0x55u;
    }
    std::uint32_t ActorSlot24(std::uint32_t actual_actor,
                              std::uint32_t argument) override {
        Check(actual_actor == source + 1620u && argument == 0x55u,
              "restore saved actor state through slot 24");
        trace.emplace_back("slot24");
        return final_result;
    }
    std::uint32_t ActorSlot52(std::uint32_t actual_actor,
                              std::uint32_t mode) override {
        Check(actual_actor == source + 1620u && mode == 7u,
              "query player state through slot 52 mode seven");
        trace.emplace_back("slot52");
        return state7;
    }
    void ActorSlot68(std::uint32_t actual_actor,
                     std::uint32_t mode,
                     std::uint32_t value) override {
        Check(actual_actor == source + 1620u && mode == 7u &&
                  value == 1u,
              "restore mode seven through slot 68");
        trace.emplace_back("slot68");
    }
    void ActorSlot276(std::uint32_t actual_actor) override {
        Check(actual_actor == source + 1620u,
              "refresh player through slot 276");
        trace.emplace_back("slot276");
    }
    void CleanupObjectLists(std::uint32_t actual_actor) override {
        Check(actual_actor == source + 1620u,
              "cleanup player actor object lists");
        trace.emplace_back("cleanup-lists");
    }
    void ProcessCollections(std::uint32_t player) override {
        Check(player == source, "process player collections");
        trace.emplace_back("process-collections");
    }
    void ResetObjectProperties(std::uint32_t actual_actor) override {
        Check(actual_actor == source + 1620u,
              "reset player actor object properties");
        trace.emplace_back("reset-properties");
    }
    std::uint32_t TemporaryTextRecord() override {
        trace.emplace_back("temporary");
        return temporary;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override {
        Check(record == temporary && value == 0u,
              "temporary text flag reset");
        trace.emplace_back("flag");
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void SetProperty278(std::uint32_t actual_actor,
                        std::uint32_t mode,
                        std::uint32_t record) override {
        Check(actual_actor == actor && mode == 2u &&
                  record == temporary,
              "property 278 receives temporary text in mode two");
        trace.emplace_back("set-property");
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        trace.emplace_back("free");
        return free_result;
    }
};

std::size_t Position(const std::vector<std::string>& trace,
                     const std::string& event) {
    const auto it = std::find(trace.begin(), trace.end(), event);
    Check(it != trace.end(), "expected transfer operation is missing");
    return static_cast<std::size_t>(it - trace.begin());
}

std::string Pair(const char* kind, std::uint32_t destination,
                 std::uint32_t from) {
    return std::string(kind) + ":" + std::to_string(destination) + ":" +
           std::to_string(from);
}

void FullCopyLayoutAndOrder() {
    Runtime runtime;
    for (const auto offset : {708u, 712u})
        runtime.words[source + 1620u + offset] = 0xa000u + offset;
    for (const auto offset : {0u, 4u, 8u, 12u}) {
        runtime.words[source + 2616u + offset] = 0xb000u + offset;
        runtime.words[source + 2632u + offset] = 0xc000u + offset;
    }
    runtime.bytes[source + 2584u] = 0x45u;
    runtime.bytes[runtime.temp_text - 1u] = 2u;
    Check(pal4::ActorManagerPlayerDataTransfer::CopyFromSource(
              actor, source, runtime) == runtime.temp_text - 1u,
          "copy returns retained temporary buffer header");

    const auto& trace = runtime.trace;
    Check(trace[0] == "copy:4100:9816:336" &&
              trace[1] == "copy:4436:10152:336" &&
              trace[2] == Pair("property2", actor + 676u,
                               source + 2296u) &&
              trace[3] == Pair("property", actor + 692u,
                               source + 2312u),
          "two scalar blocks precede two property records");
    Check(runtime.words.at(actor + 708u) == 0xa000u + 708u &&
              runtime.words.at(actor + 712u) == 0xa000u + 712u &&
              runtime.words.at(actor + 1176u) == 0xb000u &&
              runtime.words.at(actor + 1188u) == 0xb00cu &&
              runtime.words.at(actor + 1192u) == 0xc000u &&
              runtime.words.at(actor + 1204u) == 0xc00cu,
          "scalar groups copy their exact 32-bit words");
    Check(Position(trace, "copy:4992:10528:24") <
              Position(trace, Pair("array", actor + 920u,
                                   source + 2360u)),
          "24-byte block precedes embedded array copies");
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        Check(Position(trace, Pair("array", actor + 920u + 16u * i,
                                   source + 2360u + 16u * i)) <
                  Position(trace, Pair("property", actor + 1000u,
                                       source + 2440u)),
              "five initial arrays precede five property copies");
        (void)Position(trace, Pair("property", actor + 1000u + 16u * i,
                                   source + 2440u + 16u * i));
        (void)Position(trace, Pair("text", actor + 1208u + 16u * i,
                                   source + 2648u + 16u * i));
    }
    (void)Position(trace, Pair("array", actor + 1080u, source + 2520u));
    (void)Position(trace, Pair("array", actor + 1096u, source + 2536u));
    for (std::uint32_t i = 0u; i < 4u; ++i)
        (void)Position(trace, Pair("text", actor + 1112u + 16u * i,
                                   source + 2552u + 16u * i));
    Check(Position(trace, Pair("text", actor + 1272u,
                                source + 2712u)) <
              Position(trace, "slot16") &&
              Position(trace, "slot16") <
                  Position(trace, "word:4432") &&
              runtime.words.at(actor + 336u) == 0u,
          "five final texts precede actor update and offset 336 reset");
    Check(runtime.bytes.at(temporary) == 0x45u &&
              runtime.bytes.at(runtime.temp_text - 1u) == 1u &&
              Position(trace, "slot16") <
                  Position(trace, "temporary") &&
              Position(trace, "flag") <
                  Position(trace, Pair("text", temporary,
                                       source + 2584u)) &&
              Position(trace, "set-property") <
                  Position(trace, "read:28676"),
          "temporary text is initialized, assigned, applied and released");
}

void TemporaryReleaseBranches() {
    for (const auto references : {0u, 0xffu}) {
        Runtime runtime;
        runtime.bytes[runtime.temp_text - 1u] =
            static_cast<std::uint8_t>(references);
        Check(pal4::ActorManagerPlayerDataTransfer::CopyFromSource(
                  actor, source, runtime) == runtime.free_result &&
                  runtime.trace.back() == "free" &&
                  runtime.freed == std::vector<std::uint32_t>{
                      runtime.temp_text - 1u},
              "zero and 255 reference headers free the temporary text");
    }
    Runtime single_reference;
    single_reference.bytes[single_reference.temp_text - 1u] = 1u;
    Check(pal4::ActorManagerPlayerDataTransfer::CopyFromSource(
              actor, source, single_reference) ==
              single_reference.temp_text - 1u &&
              single_reference.bytes.at(single_reference.temp_text - 1u) == 0u &&
              std::find(single_reference.trace.begin(),
                        single_reference.trace.end(), "free") ==
                  single_reference.trace.end(),
          "one reference decrements without freeing");
    Runtime empty;
    empty.temp_text = 0u;
    Check(pal4::ActorManagerPlayerDataTransfer::CopyFromSource(
              actor, source, empty) == 0u &&
              empty.trace.back() == "read:28676",
          "null temporary pointer returns zero without header access");
}

void ReverseLayoutAndSharedText() {
    Runtime runtime;
    for (const auto offset : {708u, 712u})
        runtime.words[actor + offset] = 0xa000u + offset;
    for (const auto offset : {0u, 4u, 8u, 12u}) {
        runtime.words[actor + 1176u + offset] = 0xb000u + offset;
        runtime.words[actor + 1192u + offset] = 0xc000u + offset;
    }
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        const auto from = actor + 1208u + 16u * i;
        const auto to = source + 2648u + 16u * i;
        const auto pointer = 0x8000u + 0x100u * i;
        runtime.words[from + 4u] = pointer;
        runtime.words[from + 8u] = 3u + i;
        runtime.words[from + 12u] = 12u + i;
        runtime.bytes[pointer - 1u] = 1u;
        runtime.words[to + 4u] = 0xa000u + 0x100u * i;
        runtime.bytes[0xa000u + 0x100u * i - 1u] = 2u;
    }
    Check(pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
              actor, source, runtime) == runtime.final_result,
          "reverse transfer forwards final actor return");
    const auto& trace = runtime.trace;
    Check(trace[0] == "slot20" &&
              trace[1] == "copy:9816:4100:336" &&
              trace[2] == "copy:10152:4436:336" &&
              trace[3] == Pair("property2", source + 2296u,
                               actor + 676u) &&
              trace[4] == Pair("property", source + 2312u,
                               actor + 692u),
          "reverse transfer saves state before mirrored block copies");
    Check(runtime.words.at(source + 2328u) == 0xa000u + 708u &&
              runtime.words.at(source + 2332u) == 0xa000u + 712u &&
              runtime.words.at(source + 2616u) == 0xb000u &&
              runtime.words.at(source + 2628u) == 0xb00cu &&
              runtime.words.at(source + 2632u) == 0xc000u &&
              runtime.words.at(source + 2644u) == 0xc00cu,
          "reverse scalar groups retain their offsets");
    (void)Position(trace, "copy:10528:4992:24");
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        (void)Position(trace, Pair("array", source + 2360u + 16u * i,
                                   actor + 920u + 16u * i));
        (void)Position(trace, Pair("property", source + 2440u + 16u * i,
                                   actor + 1000u + 16u * i));
        const auto to = source + 2648u + 16u * i;
        const auto pointer = 0x8000u + 0x100u * i;
        Check(runtime.words.at(to + 4u) == pointer &&
                  runtime.words.at(to + 8u) == 3u + i &&
                  runtime.words.at(to + 12u) == 12u + i &&
                  runtime.bytes.at(pointer - 1u) == 2u &&
                  runtime.bytes.at(0xa000u + 0x100u * i - 1u) == 1u,
              "final text shares source and updates both reference counts");
    }
    (void)Position(trace, Pair("array", source + 2520u, actor + 1080u));
    (void)Position(trace, Pair("array", source + 2536u, actor + 1096u));
    for (std::uint32_t i = 0u; i < 4u; ++i)
        (void)Position(trace, Pair("text", source + 2552u + 16u * i,
                                   actor + 1112u + 16u * i));
    Check(Position(trace, "cleanup-lists") <
              Position(trace, "process-collections") &&
              Position(trace, "process-collections") <
                  Position(trace, "reset-properties") &&
              Position(trace, "reset-properties") <
                  Position(trace, "slot276") &&
              Position(trace, "slot276") <
                  Position(trace, "slot52") &&
              Position(trace, "slot52") <
                  Position(trace, "slot68") &&
              trace.back() == "slot24",
          "reverse cleanup and actor state restoration order");

    Runtime already_active;
    already_active.state7 = 1u;
    (void)pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
        actor, source, already_active);
    Check(std::find(already_active.trace.begin(),
                    already_active.trace.end(), "slot68") ==
              already_active.trace.end(),
          "active mode seven skips slot 68");
}

void ReverseTextBranches() {
    const auto from = actor + 1208u;
    const auto to = source + 2648u;
    Runtime deep_copy;
    deep_copy.words[from + 4u] = 0x8000u;
    deep_copy.words[from + 8u] = 4u;
    deep_copy.bytes[0x7fffu] = 0xfeu;
    (void)pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
        actor, source, deep_copy);
    const auto buffer = deep_copy.words.at(to + 4u);
    Check(Position(deep_copy.trace, "allocate:10840:4") <
              Position(deep_copy.trace,
                       "copy:" + std::to_string(buffer) + ":32768:4") &&
              Position(deep_copy.trace, "size:10840:4") >
                  Position(deep_copy.trace,
                           "copy:" + std::to_string(buffer) + ":32768:4"),
          "nonshareable text allocates, copies and sets size");

    Runtime allocation_failure;
    allocation_failure.words[from + 8u] = 0u;
    allocation_failure.fail_allocation = to;
    (void)pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
        actor, source, allocation_failure);
    Check(Position(allocation_failure.trace, "allocate:10840:0") <
              Position(allocation_failure.trace, "cleanup-lists") &&
              std::find(allocation_failure.trace.begin(),
                        allocation_failure.trace.end(),
                        "read:5308") == allocation_failure.trace.end(),
          "zero-length allocation failure skips source pointer read");

    Runtime old_owned;
    old_owned.words[from + 4u] = 0x8000u;
    old_owned.words[from + 8u] = 2u;
    old_owned.bytes[0x7fffu] = 1u;
    old_owned.words[to + 4u] = 0xa000u;
    old_owned.bytes[0x9fffu] = 0xffu;
    (void)pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
        actor, source, old_owned);
    Check(Position(old_owned.trace, "free") <
              Position(old_owned.trace, "word:10844") &&
              old_owned.freed == std::vector<std::uint32_t>{0x9fffu},
          "owned destination text is freed before shared pointer write");

    Runtime old_zero;
    old_zero.words[from + 4u] = 0x8000u;
    old_zero.words[from + 8u] = 2u;
    old_zero.bytes[0x7fffu] = 1u;
    old_zero.words[to + 4u] = 0xa000u;
    old_zero.bytes[0x9fffu] = 0u;
    (void)pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
        actor, source, old_zero);
    Check(old_zero.freed == std::vector<std::uint32_t>{0x9fffu},
          "zero-reference destination text is freed before sharing");

    Runtime empty_source;
    empty_source.words[from + 8u] = 1u;
    empty_source.words[from + 12u] = 4u;
    empty_source.bytes[0x841963u] = 1u;
    (void)pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
        actor, source, empty_source);
    Check(empty_source.words.at(to + 4u) == 0x841964u &&
              empty_source.bytes.at(0x841963u) == 2u,
          "null source pointer shares the native empty-text fallback");

    Runtime alias;
    constexpr auto alias_actor = source + 1440u;
    (void)pal4::ActorManagerPlayerDataTransfer::CopyToPlayer(
        alias_actor, source, alias);
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        const auto record = source + 2648u + 16u * i;
        (void)Position(alias.trace,
                       "erase:" + std::to_string(record) +
                           ":0:4294967295");
        (void)Position(alias.trace,
                       "erase:" + std::to_string(record) + ":0:0");
    }
    Check(std::find(alias.trace.begin(), alias.trace.end(),
                    "allocate:10840:0") == alias.trace.end(),
          "self-assignment erases instead of allocating final texts");
}

}  // namespace

int main() {
    try {
        FullCopyLayoutAndOrder();
        TemporaryReleaseBranches();
        ReverseLayoutAndSharedText();
        ReverseTextBranches();
        std::cout << "actor_manager_player_data_transfer_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
