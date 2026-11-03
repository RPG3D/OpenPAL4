#include "pal4/cast_magic_sequence_task.h"
#include "pal4/start_attack_task.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kMagic = 0x4000u;
constexpr std::uint32_t kRun = 0x5000u;
constexpr std::uint32_t kChild = 0x6000u;
constexpr std::uint32_t kHeader = 0x7000u;
constexpr std::uint32_t kText = 0x8088u;

struct Runtime final : pal4::ICastMagicSequenceRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kMagic}, {kHeader + 4u, kText},
        {kHeader + 8u, 5u}, {kHeader + 12u, 8u},
        {0x8f9784u, 0xa000u}, {0x8f970cu, 0xb000u},
        {kChild + 68u, 0x9000u}, {kChild + 72u, 0u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{kText - 1u, 1u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> factories;
    std::vector<std::uint32_t> added_texts;
    std::array<float, 3> run_position{};
    std::uint32_t run_source = 0u;
    std::uint32_t mode = 1u;
    std::uint32_t cast_text = kHeader;
    bool host_text = false;
    bool TryAssignHostText(std::uint32_t destination, std::uint32_t source) override {
        if (!host_text) return false;
        Check(destination == kChild + 64u && source == kHeader, "host text boundary");
        calls.push_back("host_text");
        return true;
    }
    bool allocate = true;
    std::uint8_t erased = 0x5au;
    std::uint32_t next_child = 0u;
    std::uint32_t fail_factory = 0u;
    std::uint32_t alias_first_length = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == kChild + 72u && cast_text == kChild + 64u &&
            alias_first_length != 0u) {
            return std::exchange(alias_first_length, 0u);
        }
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte");
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::array<float, 3> ActorChildPosition(std::uint32_t actor) override {
        Check(actor == kTarget || actor == kSource, "child position actor");
        calls.push_back("child_position");
        return {1.0f, 2.0f, 3.0f};
    }
    std::array<float, 3> ActorTargetPosition(std::uint32_t actor) override {
        Check(actor == kTarget, "target position actor");
        calls.push_back("target_position");
        return {4.0f, 5.0f, 6.0f};
    }
    void NotifySource(std::uint32_t actor) override {
        Check(actor == kSource, "notify source actor");
        calls.push_back("notify");
    }
    std::uint32_t MagicMode(std::uint32_t magic) override {
        Check(magic == kMagic, "magic mode input");
        calls.push_back("magic_mode");
        return mode;
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        factories.push_back(factory);
        calls.push_back("create");
        if (factory == fail_factory) return 0u;
        if (next_child != 0u) return std::exchange(next_child, 0u);
        return factories.size() == 1u && words[kTask + 44u] != words[kTask + 48u]
                   ? kRun : kChild;
    }
    void AddTaskAndInitialize(std::uint32_t container, std::uint32_t child,
                              std::uint32_t text,
                              const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && (child == kRun || child == kChild) &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "GOM task arguments");
        calls.push_back("add");
        added_texts.push_back(text);
    }
    void SetApproachTaskTarget(std::uint32_t child, std::uint32_t source,
                          const std::array<float, 3>& position) override {
        Check(child == kRun, "run task identity");
        calls.push_back("set_target");
        run_source = source;
        run_position = position;
    }
    std::uint32_t ActorCastText(std::uint32_t source,
                                std::uint32_t magic) override {
        Check(source == kSource && magic == kMagic, "cast text arguments");
        calls.push_back("cast_text");
        return cast_text;
    }
    void SetTextErrorFlag(std::uint32_t header, std::uint8_t flag) override {
        Check(header == kChild + 64u && flag == 1u, "share error flag");
        calls.push_back("error_flag");
    }
    bool AllocateText(std::uint32_t header, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(header == kChild + 64u && flag <= 1u, "text allocation args");
        calls.push_back("allocate");
        if (allocate) words[header + 4u] = 0xa100u;
        (void)length;
        return allocate;
    }
    void CopyText(std::uint32_t header, std::uint32_t source,
                  std::uint32_t length) override {
        Check(header == kChild + 64u && source == kText,
              "text copy addresses");
        calls.push_back("copy:" + std::to_string(length));
    }
    void DetachText(std::uint32_t header) override {
        Check(header == kChild + 64u, "detach address");
        calls.push_back("detach");
    }
    void MoveTextBytes(std::uint32_t, std::uint32_t,
                       std::uint32_t length) override {
        Check(length == 0u, "alias move size");
        calls.push_back("move");
    }
    void SetTextSize(std::uint32_t header, std::uint32_t length) override {
        Check(header == kChild + 64u && length == 5u, "alias size");
        calls.push_back("set_size");
    }
    std::uint8_t EraseText(std::uint32_t header, std::uint32_t start,
                           std::uint32_t count) override {
        Check(header == kChild + 64u && start == 0u && count == 0u,
              "alias erase arguments");
        calls.push_back("erase");
        return erased;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void SharedTextAndMovement() {
    for (std::uint32_t mode : {1u, 3u, 4u}) {
        Runtime r;
        r.mode = mode;
        Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 0x88u,
              "shared text returns data pointer low byte");
        Check(r.factories == std::vector<std::uint32_t>{
                  pal4::StartAttackApproachFactory::kPrimaryVtable,
                  pal4::CastMagicChildFactory::kVtable} &&
                  r.added_texts == std::vector<std::uint32_t>{0xa000u, 0xb000u},
              "factory and text order");
        Check(r.run_source == kSource &&
                  r.run_position == (mode == 1u
                      ? std::array<float, 3>{1, 2, 3}
                      : std::array<float, 3>{4, 5, 6}),
              "mode selects target coordinates");
        Check(r.words[kChild + 320u] == kTarget &&
                  r.words[kChild + 316u] == kSource &&
                  r.words[kChild + 324u] == kMagic &&
                  r.words[kChild + 60u] == kSource &&
                  r.words[kChild + 68u] == kText &&
                  r.words[kChild + 72u] == 5u &&
                  r.words[kChild + 76u] == 8u &&
                  r.bytes[kText - 1u] == 2u &&
                  r.bytes[kChild + 332u] == 1u,
              "cast child fields and shared reference");
        Check(Index(r, "child_position") < Index(r, "notify") &&
                  Index(r, "notify") < Index(r, "magic_mode") &&
                  Index(r, "set_target") < Index(r, "cast_text") &&
                  Index(r, "cast_text") < Index(r, "write:24896"),
              "IDA operation order");
    }
}

void SameActorAndFallbackText() {
    Runtime r;
    r.words[kTask + 48u] = kSource;
    r.words[0x8f970cu] = 0u;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 0x88u &&
              r.factories == std::vector<std::uint32_t>{
                  pal4::CastMagicChildFactory::kVtable} &&
              r.added_texts == std::vector<std::uint32_t>{0x841964u} &&
              r.words[kChild + 320u] == kSource,
          "same actor skips movement and uses empty text");

    r = Runtime{};
    r.words[0x8f9784u] = 0u;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 0x88u &&
              r.added_texts.front() == 0x841964u,
          "run task text falls back to empty global");
}

void TextCopyAndFailure() {
    Runtime r;
    r.bytes[kText - 1u] = 0xfeu;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 5u &&
              Index(r, "allocate") < Index(r, "copy:5") &&
              r.words[kChild + 72u] == 5u &&
              r.bytes[0xa105u] == 0u &&
              r.bytes[kChild + 332u] == 1u,
          "saturated source is copied and terminated");

    r = Runtime{};
    r.words[kHeader + 8u] = 0u;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 0u &&
              Index(r, "allocate") < Index(r, "copy:0"),
          "zero-length source takes allocation path");

    r = Runtime{};
    r.bytes[kText - 1u] = 0xfeu;
    r.allocate = false;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 0u &&
              r.bytes[kChild + 332u] == 1u &&
              std::find(r.calls.begin(), r.calls.end(), "copy:5") == r.calls.end(),
          "allocation failure still sets completion byte");
}

void AliasAndNullChildren() {
    Runtime r;
    r.cast_text = kChild + 64u;
    r.words[kChild + 72u] = 7u;
    r.words[kChild + 76u] = 8u;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == r.erased &&
              Index(r, "detach") < Index(r, "erase") &&
              std::find(r.calls.begin(), r.calls.end(), "move") == r.calls.end() &&
              r.bytes[kChild + 332u] == 1u,
          "alias text follows detach and erase path");

    r = Runtime{};
    r.cast_text = kChild + 64u;
    r.words[kChild + 72u] = 4u;
    r.alias_first_length = 5u;
    bool threw = false;
    try { (void)pal4::CastMagicSequenceTask::Start(kTask, r); }
    catch (const std::out_of_range&) { threw = true; }
    Check(threw, "alias length overflow throws");

    r = Runtime{};
    r.fail_factory = pal4::StartAttackApproachFactory::kPrimaryVtable;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 0u &&
              r.factories.size() == 1u && r.added_texts.empty(),
          "null RunTask stops safely");

    r = Runtime{};
    r.fail_factory = pal4::CastMagicChildFactory::kVtable;
    Check(pal4::CastMagicSequenceTask::Start(kTask, r) == 0u &&
              r.factories.size() == 2u && r.added_texts.size() == 1u,
          "null CastMagic child stops safely after RunTask");
}

void FactorySlot() {
    pal4::CastMagicChildFactorySlot slot{};
    Check(pal4::CastMagicChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845a5cu,
          "factory writes IDA vtable and returns original slot");
}

}  // namespace

int main() {
    {
        Runtime host; host.host_text = true;
        host.words.erase(kHeader + 8u);
        Check(pal4::CastMagicSequenceTask::Start(kTask, host) == 1u &&
              host.bytes[kChild + 332u] == 1u &&
              std::count(host.calls.begin(), host.calls.end(), "host_text") == 1,
              "host assignment completes without reading legacy COW metadata");
    }
    FactorySlot();
    SharedTextAndMovement();
    SameActorAndFallbackText();
    TextCopyAndFailure();
    AliasAndNullChildren();
    std::cout << "CastMagic sequence task tests passed\n";
}
