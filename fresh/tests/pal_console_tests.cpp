#include "pal4/pal_console.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class AccessRuntime final : public pal4::IPalConsoleAccessRuntime {
public:
    std::uint32_t GetSingletonManager() override {
        ++manager_calls;
        return 0x700u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destroy) override {
        ++register_calls;
        Check(manager == 0x700u && name == "palConsole" && id == 1008u &&
                  factory == 0x418f20u && destroy == 0x418f80u,
              "console singleton registration operands match IDA");
        return registered_address;
    }
    std::uint32_t registered_address{0x800u};
    int manager_calls{};
    int register_calls{};
};

struct CopyCall {
    std::uint32_t destination;
    std::uint32_t count;
    std::string source;
};

class ObjectRuntime final : public pal4::IPalConsoleObjectRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x4a84u, "console allocation is 0x4a84 bytes");
        events.push_back("allocate");
        return allocation;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return address + 23u;
    }
    pal4::PalConsoleState& Access(std::uint32_t address) override {
        Check(address == 0x1000u, "console object address is forwarded");
        if (throw_on_access) {
            throw std::runtime_error("access");
        }
        return state;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        writes.push_back({address, value});
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        byte_writes.push_back({address, value});
        bytes[address] = value;
    }
    void CopyString(std::uint32_t destination, const char* source,
                    std::uint32_t count) override {
        copies.push_back({destination, count, source});
    }
    pal4::PalConsoleState state;
    std::uint32_t allocation{0x1000u};
    bool throw_on_access{};
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> byte_writes;
    std::vector<CopyCall> copies;
    std::vector<std::string> events;
};

struct DrawCall {
    std::uint32_t manager;
    std::uint32_t text;
    std::array<std::int32_t, 4> rectangle;
    std::uint32_t alignment;
    std::uint32_t color;
    std::uint32_t flags;
};

class RenderRuntime final : public pal4::IPalConsoleRenderRuntime {
public:
    void DrawFilledRectangle(
        std::uint32_t context,
        const std::array<std::int32_t, 4>& rectangle,
        const std::array<std::uint8_t, 4>& color) override {
        ++fill_calls;
        Check(context == 0x77u && rectangle ==
                  std::array<std::int32_t, 4>{0, 0, 400, 320} &&
                  color == std::array<std::uint8_t, 4>{90, 90, 90, 155},
              "background rectangle has observed geometry and color");
    }
    std::uint32_t GetFontManager() override {
        ++manager_calls;
        return 0x55u;
    }
    void DrawFontRectangle(
        std::uint32_t manager, std::uint32_t text,
        const std::array<std::int32_t, 4>& rectangle,
        std::uint32_t alignment, std::uint32_t color,
        std::uint32_t flags) override {
        draws.push_back({manager, text, rectangle, alignment, color, flags});
    }
    std::uint32_t StringLength(std::uint32_t address) override {
        scanned.push_back(address);
        return lengths[address];
    }
    std::map<std::uint32_t, std::uint32_t> lengths;
    std::vector<std::uint32_t> scanned;
    std::vector<DrawCall> draws;
    int fill_calls{};
    int manager_calls{};
};

class OutputRuntime final : public pal4::IPalConsoleOutputRuntime {
public:
    void FormatToBuffer(char* buffer, std::uint32_t capacity,
                        const char* format, std::va_list args) override {
        events.push_back("format");
        Check(capacity == 256u, "output formatter receives 256-byte buffer");
        std::vsnprintf(buffer, capacity, format, args);
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t bytes) override {
        events.push_back("move");
        Check(destination == 0x1090u && source == 0x10f4u &&
                  bytes == 0x578u,
              "output moves 14 prior 100-byte lines down one slot");
    }
    std::uint32_t CopyString(std::uint32_t destination,
                             const char* source,
                             std::uint32_t count) override {
        events.push_back("copy");
        Check(destination == 0x1608u && count == 99u &&
                  std::string(source) == "HP 42",
              "formatted line is copied into newest 100-byte slot");
        return destination;
    }
    std::vector<std::string> events;
};

void SingletonRegistrationIsGuarded() {
    pal4::PalConsoleAccessState state{0x80u, 0u};
    AccessRuntime runtime;
    Check(pal4::PalConsoleAccess::Get(state, runtime) == 0x800u &&
              state.guard == 0x81u && state.instance == 0x800u,
          "first console access registers and caches");
    Check(pal4::PalConsoleAccess::Get(state, runtime) == 0x800u &&
              runtime.manager_calls == 1 && runtime.register_calls == 1,
          "guarded access does not register again");
    pal4::PalConsoleAccessState failed{};
    runtime.registered_address = 0u;
    Check(pal4::PalConsoleAccess::Get(failed, runtime) == 0u &&
              pal4::PalConsoleAccess::Get(failed, runtime) == 0u &&
              runtime.register_calls == 2,
          "failed registration still sets guard bit");
}

void ConstructorWritesObservedFieldsOnly() {
    ObjectRuntime runtime;
    runtime.state.Bytes().fill(0xa5u);
    Check(pal4::PalConsole::Create(0x7cu, runtime) == 0x1000u,
          "factory returns constructed object");
    const auto& state = runtime.state;
    Check(state.ReadDword(0u) == 0u && state.ReadDword(4u) == 0u &&
              state.ReadDword(8u) == 300u &&
              state.ReadDword(0x0cu) == 400u &&
              state.ReadDword(0x10u) == 320u &&
              state.ReadDword(0x1cu) == 400u &&
              state.ReadDword(0x20u) == 300u,
          "console dimensions and state fields match IDA");
    Check(state.ReadDword(0x24u) == 0xffffff00u &&
              state.ReadDword(0x28u) == 0xffffffffu,
          "console text colors match IDA");
    Check(state.Bytes().at(0x2cu) == 0x5fu &&
              state.Bytes().at(0x2du) == 0u &&
              state.Bytes().at(0x8fu) == 0u &&
              state.Bytes().at(0x90u) == 0u &&
              state.Bytes().at(0x66bu) == 0u &&
              state.Bytes().at(0x66cu) == 0u &&
              state.Bytes().at(0x4a6bu) == 0u,
          "command and output buffers are zeroed with underscore prompt");
    Check(state.ReadDword(pal4::PalConsoleState::kCommandCount) == 0u &&
              state.Bytes().at(0x4a70u) == 0x7cu &&
              state.Bytes().at(0x4a71u) == 0xa5u &&
              state.ReadDword(pal4::PalConsoleState::kHistoryBegin) == 0u &&
              state.ReadDword(0x4a80u) == 0u,
          "tail fields preserve unspecified bytes");
    Check(runtime.events == std::vector<std::string>{"allocate"},
          "constructor does not call external helpers for empty history");
}

void FactoryFailureAndDestroy() {
    ObjectRuntime runtime;
    runtime.allocation = 0u;
    Check(pal4::PalConsole::Create(0xaau, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "failed allocation skips constructor");
    runtime = ObjectRuntime{};
    runtime.throw_on_access = true;
    try {
        static_cast<void>(pal4::PalConsole::Create(0u, runtime));
        Check(false, "construction must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>({
                  "allocate", "free:4096"}),
              "failed construction releases allocation");
    }
    runtime = ObjectRuntime{};
    pal4::PalConsole::Destroy(0u, runtime);
    Check(runtime.events.empty(), "null destroy does not call runtime");
    pal4::PalConsole::Destroy(0x1000u, runtime);
    Check(runtime.events == std::vector<std::string>({
              "free:0", "free:4096"}),
          "destroy cleans history before freeing object");
}

void CleanupReleasesHistoryReferences() {
    ObjectRuntime runtime;
    auto& state = runtime.state;
    state.WriteDword(pal4::PalConsoleState::kHistoryBegin, 0x9000u);
    state.WriteDword(pal4::PalConsoleState::kHistoryEnd, 0x9030u);
    state.WriteDword(pal4::PalConsoleState::kHistoryCapacity, 0x9040u);
    runtime.words[0x9004u] = 0xa001u;
    runtime.words[0x9014u] = 0xa101u;
    runtime.words[0x9024u] = 0xa201u;
    runtime.bytes[0xa000u] = 0u;
    runtime.bytes[0xa100u] = 2u;
    runtime.bytes[0xa200u] = 0xffu;
    Check(pal4::PalConsole::Cleanup(0x1000u, runtime) == 0x9017u,
          "cleanup returns history storage free result");
    Check(runtime.events == std::vector<std::string>({
              "free:40960", "free:41472", "free:36864"}),
          "zero and sentinel reference counts free text before vector");
    Check(runtime.byte_writes ==
              std::vector<std::pair<std::uint32_t, std::uint8_t>>{{0xa100u, 1u}},
          "positive reference count is decremented");
    for (std::uint32_t record = 0x9000u; record < 0x9030u;
         record += 0x10u) {
        Check(runtime.words[record + 4u] == 0u &&
                  runtime.words[record + 8u] == 0u &&
                  runtime.words[record + 12u] == 0u,
              "each history record's three payload words are cleared");
    }
    Check(state.ReadDword(pal4::PalConsoleState::kHistoryBegin) == 0u &&
              state.ReadDword(pal4::PalConsoleState::kHistoryEnd) == 0u &&
              state.ReadDword(pal4::PalConsoleState::kHistoryCapacity) == 0u,
          "history range fields are reset");
}

void RenderVisitsFifteenLinesAndSkipsEmptyOnes() {
    ObjectRuntime objects;
    static_cast<void>(pal4::PalConsole::Construct(0x1000u, 0u, objects));
    RenderRuntime runtime;
    pal4::PalConsole::Render(0x1000u, 0x77u, objects.state, runtime);
    Check(runtime.fill_calls == 0 && runtime.manager_calls == 0 &&
              runtime.scanned.empty(), "hidden console renders nothing");
    objects.state.WriteDword(0u, 1u);
    runtime.lengths[0x1608u] = 4u;
    runtime.lengths[0x1090u] = 2u;
    pal4::PalConsole::Render(0x1000u, 0x77u, objects.state, runtime);
    Check(runtime.fill_calls == 1 && runtime.scanned.size() == 15u &&
              runtime.scanned.front() == 0x1608u &&
              runtime.scanned.back() == 0x1090u,
          "visible console scans 15 lines from newest to oldest");
    Check(runtime.manager_calls == 3 && runtime.draws.size() == 3u,
          "prompt and two nonempty lines draw through font manager");
    Check(runtime.draws.at(0).text == 0x102cu &&
              runtime.draws.at(0).rectangle ==
                  std::array<std::int32_t, 4>{0, 300, 400, 320} &&
              runtime.draws.at(0).color == 0xffffff00u &&
              runtime.draws.at(0).flags == 0u,
          "prompt draw uses observed rect and color");
    Check(runtime.draws.at(1).text == 0x1608u &&
              runtime.draws.at(1).rectangle ==
                  std::array<std::int32_t, 4>{0, 282, 400, 300} &&
              runtime.draws.at(2).text == 0x1090u &&
              runtime.draws.at(2).rectangle ==
                  std::array<std::int32_t, 4>{0, 30, 400, 48} &&
              runtime.draws.at(2).flags == 8u,
          "line text uses 18-pixel spacing and 100-byte records");
}

void CommandCapacityAndAddressing() {
    ObjectRuntime runtime;
    Check(pal4::PalConsole::AddCommand(
              0x1000u, 0x1234u, "help", "description", runtime),
          "first command is accepted");
    Check(runtime.writes.at(0) ==
              std::pair<std::uint32_t, std::uint32_t>{0x166cu, 0x1234u} &&
              runtime.copies.at(0).destination == 0x1670u &&
              runtime.copies.at(0).count == 32u &&
              runtime.copies.at(1).destination == 0x1690u &&
              runtime.copies.at(1).count == 100u,
          "first command uses 136-byte record and strncpy bounds");
    runtime.state.WriteDword(pal4::PalConsoleState::kCommandCount, 63u);
    Check(pal4::PalConsole::AddCommand(
              0x1000u, 0x5678u, "last", "last description", runtime) &&
              runtime.state.ReadDword(pal4::PalConsoleState::kCommandCount) == 64u &&
              runtime.writes.at(1).first == 0x166cu + 63u * 136u,
          "slot 63 is the last accepted command");
    const auto writes_before = runtime.writes.size();
    Check(!pal4::PalConsole::AddCommand(
              0x1000u, 0u, "blocked", "blocked", runtime) &&
              runtime.writes.size() == writes_before,
          "count 64 rejects without writing");
    runtime.state.WriteDword(pal4::PalConsoleState::kCommandCount,
                             0xffffffffu);
    Check(pal4::PalConsole::AddCommand(
              0x1000u, 0x9u, "negative", "negative", runtime) &&
              runtime.writes.back().first == 0x166cu - 136u &&
              runtime.state.ReadDword(pal4::PalConsoleState::kCommandCount) == 0u,
          "signed negative count retains original out-of-range address math");
}

void OutputLineAndRectangles() {
    OutputRuntime output;
    Check(pal4::PalConsole::AddOutputLine(
              0x1000u, output, "HP %d", 42) == 0x1608u &&
              output.events == std::vector<std::string>({
                  "format", "move", "copy"}),
          "output formatting, line shift and final return are ordered");

    pal4::PalConsoleState state;
    state.Bytes().fill(0xa5u);
    Check(pal4::PalConsole::CopyPromptRectangle(
              state, {1u, 2u, 3u, 4u}) == 3u &&
              state.ReadDword(0x04u) == 1u &&
              state.ReadDword(0x08u) == 2u &&
              state.ReadDword(0x0cu) == 3u &&
              state.ReadDword(0x10u) == 4u,
          "first rectangle copier writes exactly four prompt fields");
    Check(pal4::PalConsole::CopyBackgroundRectangle(
              state, {5u, 6u, 7u, 8u}) == 7u &&
              state.ReadDword(0x14u) == 5u &&
              state.ReadDword(0x18u) == 6u &&
              state.ReadDword(0x1cu) == 7u &&
              state.ReadDword(0x20u) == 8u &&
              state.ReadDword(0u) == 0xa5a5a5a5u,
          "second rectangle copier leaves unrelated state intact");
    Check(pal4::PalConsole::InitializeRectangles(
              state, 1024, 769) == 1024u &&
              state.ReadDword(0x04u) == 0u &&
              state.ReadDword(0x08u) == 386u &&
              state.ReadDword(0x0cu) == 1024u &&
              state.ReadDword(0x10u) == 404u &&
              state.ReadDword(0x14u) == 0u &&
              state.ReadDword(0x18u) == 0u &&
              state.ReadDword(0x1cu) == 1024u &&
              state.ReadDword(0x20u) == 384u &&
              state.ReadDword(0x24u) == 0xff20ee20u &&
              state.ReadDword(0x28u) == 0xffeeeeeeu,
          "rect initializer uses width and half-height plus fixed offsets");
    static_cast<void>(pal4::PalConsole::InitializeRectangles(state, 640, -3));
    Check(state.ReadDword(0x08u) == 1u &&
              state.ReadDword(0x10u) == 19u &&
              state.ReadDword(0x20u) == 0xffffffffu,
          "negative odd height is halved toward zero");
}

void EmbeddedHistoryCleanupEntry() {
    ObjectRuntime runtime;
    Check(pal4::PalConsoleHistory::Cleanup(
              runtime.state, runtime) == 23u &&
              runtime.events == std::vector<std::string>{"free:0"},
          "embedded history cleanup accepts an empty range");
}

}  // namespace

int main() {
    SingletonRegistrationIsGuarded();
    ConstructorWritesObservedFieldsOnly();
    FactoryFailureAndDestroy();
    CleanupReleasesHistoryReferences();
    RenderVisitsFifteenLinesAndSkipsEmptyOnes();
    CommandCapacityAndAddressing();
    OutputLineAndRectangles();
    EmbeddedHistoryCleanupEntry();
    std::cout << "8 pal console test cases passed\n";
}
