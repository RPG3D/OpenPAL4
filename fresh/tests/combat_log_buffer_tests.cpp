#include "pal4/combat_log_buffer.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatLogBufferStorageRuntime,
                       pal4::ICombatLogTextRuntime,
                       pal4::ICombatLogBaseRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_memory = 0x4000u;
    std::uint32_t assigned_result = 0u;
    std::uint32_t assigned_source = 0u;
    std::uint32_t assigned_length = 0u;
    pal4::CombatLogTextSource assigned_kind =
        pal4::CombatLogTextSource::Empty;
    int assign_count = 0;
    int locale_calls = 0;

    Runtime() {
        words[0x1000u + 12u] = 0x1100u;
        words[0x1000u + 16u] = 0x1104u;
        words[0x1000u + 28u] = 0x1108u;
        words[0x1000u + 32u] = 0x110cu;
        words[0x1000u + 44u] = 0x1110u;
        words[0x1000u + 48u] = 0x1114u;
        words[0x1100u] = 0u;
        words[0x1104u] = 0u;
        words[0x1108u] = 0u;
        words[0x110cu] = 0u;
        words[0x1110u] = 0u;
        words[0x1114u] = 0u;
        words[0x1000u + 60u] = 0u;
        words[0x1000u + 64u] = 8u;
        words[0x1000u + 68u] = 0u;
        bytes[0x1000u + 68u] = 0u;
        words[0x8683e0u] = 0u;
        words[0x8683e4u] = 0u;
        words[0x978b34u] = 0xffffffffu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto found = words.find(address);
        if (found == words.end())
            throw std::runtime_error("missing word at " +
                                     std::to_string(address));
        return found->second;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        const auto found = bytes.find(address);
        if (found == bytes.end())
            throw std::runtime_error("missing byte at " +
                                     std::to_string(address));
        return found->second;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        return next_memory;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t size) override {
        for (std::uint32_t i = 0u; i < size; ++i)
            bytes[destination + i] = bytes.at(source + i);
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
    }
    void AssignText(std::uint32_t result, std::uint32_t source,
                    std::uint32_t length,
                    pal4::CombatLogTextSource kind) override {
        assigned_result = result;
        assigned_source = source;
        assigned_length = length;
        assigned_kind = kind;
        ++assign_count;
    }
    std::uint32_t InitializeLocale() override {
        ++locale_calls;
        return 0x6000u;
    }
};

void ExistingCapacityAndEof() {
    Runtime runtime;
    runtime.words[0x110cu] = 0x3000u;
    runtime.words[0x1114u] = 2u;
    Check(pal4::CombatLogBuffer::Add(0x1000u, -1, runtime) == 0 &&
              runtime.allocations.empty() &&
              runtime.words[0x110cu] == 0x3000u,
          "EOF writes nothing");
    Check(pal4::CombatLogBuffer::Add(0x1000u, 'A', runtime) == 'A' &&
              runtime.words[0x110cu] == 0x3001u &&
              runtime.words[0x1114u] == 1u &&
              runtime.bytes[0x3000u] == 'A' &&
              runtime.allocations.empty(),
          "available put area consumes one slot");
    runtime.words[0x1114u] = 0u;
    runtime.words[0x1000u + 68u] = 2u;
    Check(pal4::CombatLogBuffer::Add(0x1000u, 'B', runtime) == -1 &&
              runtime.words[0x110cu] == 0x3001u &&
              runtime.allocations.empty(),
          "fixed buffer refuses growth");
}

void InitialAllocationAndSizeCap() {
    Runtime runtime;
    Check(pal4::CombatLogBuffer::Add(0x1000u, 0x12au,
                                     runtime) == 0x12a &&
              runtime.allocations == std::vector<std::uint32_t>{8u} &&
              runtime.words[0x1100u] == 0x4000u &&
              runtime.words[0x1104u] == 0x4000u &&
              runtime.words[0x1108u] == 0x4000u &&
              runtime.words[0x110cu] == 0x4001u &&
              runtime.words[0x1110u] == 0u &&
              runtime.words[0x1114u] == 7u &&
              runtime.words[0x1000u + 60u] == 0x4000u &&
              runtime.words[0x1000u + 68u] == 1u &&
              runtime.bytes[0x4000u] == 0x2au,
          "fresh buffer establishes all six pointers and counts");

    Runtime large;
    large.words[0x1000u + 64u] = 600u;
    Check(pal4::CombatLogBuffer::Add(0x1000u, 'x', large) == 'x' &&
              large.allocations == std::vector<std::uint32_t>{600u} &&
              large.words[0x1000u + 64u] == 512u &&
              large.words[0x1114u] == 599u,
          "first allocation uses old increment then caps future growth");
}

void PrepareGrowth(Runtime& runtime, std::uint32_t flags) {
    runtime.words[0x1100u] = 0x3000u;
    runtime.words[0x1104u] = 0x3000u;
    runtime.words[0x1108u] = 0x3001u;
    runtime.words[0x110cu] = 0x3004u;
    runtime.words[0x1110u] = 3u;
    runtime.words[0x1114u] = 0u;
    runtime.words[0x1000u + 60u] = 0x3000u;
    runtime.words[0x1000u + 68u] = flags;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.bytes[0x3000u + i] = static_cast<std::uint8_t>('a' + i);
}

void GrowAndRelocate() {
    for (const auto flags : {1u, 5u}) {
        Runtime runtime;
        PrepareGrowth(runtime, flags);
        Check(pal4::CombatLogBuffer::Add(0x1000u, 'X', runtime) == 'X' &&
                  runtime.allocations == std::vector<std::uint32_t>{12u} &&
                  runtime.frees == std::vector<std::uint32_t>{0x3000u} &&
                  runtime.words[0x1100u] == 0x4000u &&
                  runtime.words[0x1104u] == 0x4000u &&
                  runtime.words[0x110cu] == 0x4005u &&
                  runtime.words[0x1114u] == 7u &&
                  runtime.words[0x1000u + 60u] == 0x4000u &&
                  runtime.bytes[0x4000u] == 'a' &&
                  runtime.bytes[0x4003u] == 'd' &&
                  runtime.bytes[0x4004u] == 'X',
              "growth copies bytes and relocates output pointers");
        Check(runtime.words[0x1108u] == ((flags & 4u) ? 0x4000u :
                                               0x4001u) &&
                  runtime.words[0x1110u] == ((flags & 4u) ? 0u : 4u),
              "input pointer follows append or rewind flag");
    }
}

void RemoveBranches() {
    Runtime runtime;
    runtime.words[0x1100u] = 0x3000u;
    runtime.words[0x1108u] = 0x3002u;
    runtime.words[0x1110u] = 1u;
    runtime.bytes[0x3001u] = 'a';
    runtime.words[0x1000u + 68u] = 2u;
    Check(pal4::CombatLogBuffer::Remove(0x1000u, 'b', runtime) == -1 &&
              runtime.words[0x1108u] == 0x3002u,
          "fixed input rejects mismatched character");
    Check(pal4::CombatLogBuffer::Remove(0x1000u, 'a', runtime) == 'a' &&
              runtime.words[0x1108u] == 0x3001u &&
              runtime.words[0x1110u] == 2u,
          "matching byte is put back");
    Check(pal4::CombatLogBuffer::Remove(0x1000u, -1, runtime) == 0 &&
              runtime.words[0x1108u] == 0x3000u &&
              runtime.words[0x1110u] == 3u,
          "EOF rewinds without writing");
    Check(pal4::CombatLogBuffer::Remove(0x1000u, 'a', runtime) == -1,
          "cannot rewind before base");
    runtime.words[0x1108u] = 0u;
    Check(pal4::CombatLogBuffer::Remove(0x1000u, 'a', runtime) == -1,
          "null input pointer fails");

    runtime.words[0x1108u] = 0x3001u;
    runtime.words[0x1000u + 68u] = 0u;
    runtime.bytes[0x3000u] = 'a';
    Check(pal4::CombatLogBuffer::Remove(0x1000u, 'b', runtime) == 'b' &&
              runtime.bytes[0x3000u] == 'b',
          "writable input permits replacement byte");
}

void PeekBranches() {
    Runtime runtime;
    Check(pal4::CombatLogBuffer::Peek(0x1000u, runtime) == -1,
          "null input pointer has no byte");
    runtime.words[0x1108u] = 0x3000u;
    runtime.words[0x1110u] = 2u;
    runtime.bytes[0x3000u] = 'q';
    Check(pal4::CombatLogBuffer::Peek(0x1000u, runtime) == 'q' &&
              runtime.words[0x1110u] == 2u,
          "available input byte is returned without consuming it");

    runtime.words[0x1110u] = 0u;
    runtime.bytes[0x1000u + 68u] = 4u;
    Check(pal4::CombatLogBuffer::Peek(0x1000u, runtime) == -1,
          "fixed input refuses extension");
    runtime.bytes[0x1000u + 68u] = 0u;
    Check(pal4::CombatLogBuffer::Peek(0x1000u, runtime) == -1,
          "missing output pointer has no extension");

    runtime.words[0x110cu] = 0x3000u;
    runtime.words[0x1000u + 60u] = 0x3000u;
    Check(pal4::CombatLogBuffer::Peek(0x1000u, runtime) == -1,
          "reader at both output boundaries has no byte");
    runtime.words[0x110cu] = 0x3003u;
    Check(pal4::CombatLogBuffer::Peek(0x1000u, runtime) == 'q' &&
              runtime.words[0x1000u + 60u] == 0x3003u &&
              runtime.words[0x1110u] == 3u &&
              runtime.words[0x1108u] == 0x3000u,
          "writer expands readable span without moving reader");

    runtime.words[0x1110u] = 0u;
    runtime.words[0x110cu] = 0x3000u;
    runtime.words[0x1000u + 60u] = 0x3002u;
    Check(pal4::CombatLogBuffer::Peek(0x1000u, runtime) == 'q' &&
              runtime.words[0x1110u] == 2u,
          "existing read limit can extend past writer");
}

void PrepareSeek(Runtime& runtime) {
    runtime.words[0x1100u] = 0x3000u;
    runtime.words[0x1108u] = 0x3002u;
    runtime.words[0x110cu] = 0x3004u;
    runtime.words[0x1110u] = 4u;
    runtime.words[0x1114u] = 6u;
    runtime.words[0x1000u + 60u] = 0x3008u;
    runtime.words[0x2004u] = 0xabcdef01u;
}

void CheckSeekResult(const Runtime& runtime, std::uint32_t value) {
    Check(runtime.words.at(0x2000u) == value &&
              runtime.words.at(0x2004u) == 0xabcdef01u &&
              runtime.words.at(0x2008u) == 0u &&
              runtime.words.at(0x200cu) == 0u &&
              runtime.words.at(0x2010u) == 0xffffffffu,
          "20-byte position result layout and untouched second word");
}

void RelativeSeekBranches() {
    for (const auto origin : {0u, 1u, 2u}) {
        Runtime runtime;
        PrepareSeek(runtime);
        const auto offset = origin == 0u ? 3 :
                            origin == 1u ? -1 : -2;
        const auto expected = origin == 0u ? 3u :
                              origin == 1u ? 1u : 6u;
        Check(pal4::CombatLogBuffer::SeekRelative(
                  0x1000u, 0x2000u, offset, origin, 1u,
                  runtime) == 0x2000u,
              "relative seek returns result address");
        CheckSeekResult(runtime, expected);
        Check(runtime.words[0x1108u] == 0x3000u + expected &&
                  runtime.words[0x1110u] == 6u - expected &&
                  runtime.words[0x110cu] == 0x3004u,
              "input-only seek moves reader and remaining count");
    }

    Runtime both;
    PrepareSeek(both);
    static_cast<void>(pal4::CombatLogBuffer::SeekRelative(
        0x1000u, 0x2000u, -2, 2u, 3u, both));
    CheckSeekResult(both, 6u);
    Check(both.words[0x1108u] == 0x3006u &&
              both.words[0x1110u] == 0u &&
              both.words[0x110cu] == 0x3006u &&
              both.words[0x1114u] == 4u,
          "combined seek makes output follow input");

    Runtime output;
    PrepareSeek(output);
    static_cast<void>(pal4::CombatLogBuffer::SeekRelative(
        0x1000u, 0x2000u, 2, 1u, 2u, output));
    CheckSeekResult(output, 6u);
    Check(output.words[0x110cu] == 0x3006u &&
              output.words[0x1114u] == 4u &&
              output.words[0x1108u] == 0x3002u,
          "output-only current-origin seek leaves reader");

    for (const auto [offset, origin, mode] : {
             std::tuple{-1, 0u, 1u}, std::tuple{9, 0u, 1u},
             std::tuple{0, 1u, 3u}, std::tuple{0, 3u, 2u},
             std::tuple{0, 0u, 0u}}) {
        Runtime invalid;
        PrepareSeek(invalid);
        static_cast<void>(pal4::CombatLogBuffer::SeekRelative(
            0x1000u, 0x2000u, offset, origin,
            static_cast<std::uint8_t>(mode), invalid));
        CheckSeekResult(invalid, 0xffffffffu);
        Check(invalid.words[0x1108u] == 0x3002u &&
                  invalid.words[0x110cu] == 0x3004u,
              "invalid relative seek leaves both cursors");
    }

    Runtime refresh;
    PrepareSeek(refresh);
    refresh.words[0x110cu] = 0x300au;
    static_cast<void>(pal4::CombatLogBuffer::SeekRelative(
        0x1000u, 0x2000u, -1, 2u, 2u, refresh));
    CheckSeekResult(refresh, 9u);
    Check(refresh.words[0x1000u + 60u] == 0x300au &&
              refresh.words[0x110cu] == 0x3009u,
          "writer refreshes end before end-origin seek");
}

void AbsoluteSeekBranches() {
    Runtime both;
    PrepareSeek(both);
    Check(pal4::CombatLogBuffer::SeekPosition(
              0x1000u, 0x2000u, 2, 4, 3u, both) == 0x2000u,
          "absolute seek returns result address");
    CheckSeekResult(both, 6u);
    Check(both.words[0x1108u] == 0x3006u &&
              both.words[0x110cu] == 0x3006u &&
              both.words[0x1110u] == 0u &&
              both.words[0x1114u] == 4u,
          "absolute seek updates both cursor families");

    Runtime output;
    PrepareSeek(output);
    output.words[0x1108u] = 0u;
    static_cast<void>(pal4::CombatLogBuffer::SeekPosition(
        0x1000u, 0x2000u, 1, 2, 2u, output));
    CheckSeekResult(output, 3u);
    Check(output.words[0x110cu] == 0x3003u &&
              output.words[0x1114u] == 7u,
          "absolute output-only seek adjusts capacity");

    for (const auto [first, second, mode] : {
             std::tuple{-1, 0, 3u}, std::tuple{4, 5, 1u},
             std::tuple{2, 0, 0u}}) {
        Runtime invalid;
        PrepareSeek(invalid);
        static_cast<void>(pal4::CombatLogBuffer::SeekPosition(
            0x1000u, 0x2000u, first, second,
            static_cast<std::uint8_t>(mode), invalid));
        CheckSeekResult(invalid, 0xffffffffu);
        Check(invalid.words[0x1108u] == 0x3002u &&
                  invalid.words[0x110cu] == 0x3004u,
              "invalid absolute seek leaves cursors");
    }
}

void StringExtractionBranches() {
    for (const auto [flags, writer, reader, expected_source,
                     expected_length, expected_kind] : {
             std::tuple{0u, 0x3003u, 0x3002u, 0x3000u, 5u,
                        pal4::CombatLogTextSource::Output},
             std::tuple{0u, 0u, 0x3102u, 0x3100u, 5u,
                        pal4::CombatLogTextSource::Input},
             std::tuple{2u, 0x3003u, 0x3102u, 0x3100u, 5u,
                        pal4::CombatLogTextSource::Input},
             std::tuple{4u, 0u, 0x3002u, 0u, 0u,
                        pal4::CombatLogTextSource::Empty},
             std::tuple{6u, 0x3003u, 0x3002u, 0u, 0u,
                        pal4::CombatLogTextSource::Empty}}) {
        Runtime runtime;
        runtime.words[0x1000u + 68u] = flags;
        runtime.words[0x110cu] = writer;
        runtime.words[0x1108u] = reader;
        runtime.words[0x1100u] = 0x3100u;
        runtime.words[0x1104u] = 0x3000u;
        runtime.words[0x1110u] = 3u;
        runtime.words[0x1000u + 60u] = 0x3005u;
        Check(pal4::CombatLogBuffer::GetString(0x1000u, 0x2000u,
                                               runtime) == 0x2000u &&
                  runtime.assign_count == 1 &&
                  runtime.assigned_result == 0x2000u &&
                  runtime.assigned_source == expected_source &&
                  runtime.assigned_length == expected_length &&
                  runtime.assigned_kind == expected_kind,
              "buffer extraction source and length by mode");
    }
}

void InitializeFromTextBranches() {
    Runtime owned;
    owned.words[0x1000u + 68u] = 1u;
    owned.bytes[0x1000u + 68u] = 1u;
    owned.words[0x1100u] = 0x3000u;
    owned.words[0x2000u + 4u] = 0x5000u;
    owned.words[0x2000u + 8u] = 3u;
    owned.bytes[0x5000u] = 'a';
    owned.bytes[0x5001u] = 'b';
    owned.bytes[0x5002u] = 'c';
    Check(pal4::CombatLogBuffer::InitializeFromText(
              0x1000u, 0x2000u, owned) == 1u &&
              owned.frees == std::vector<std::uint32_t>{0x3000u} &&
              owned.allocations == std::vector<std::uint32_t>{3u} &&
              owned.words[0x1100u] == 0x4000u &&
              owned.words[0x1108u] == 0x4000u &&
              owned.words[0x1110u] == 3u &&
              owned.words[0x1104u] == 0x4000u &&
              owned.words[0x110cu] == 0x4000u &&
              owned.words[0x1114u] == 3u &&
              owned.words[0x1000u + 60u] == 0x4003u &&
              owned.words[0x1000u + 64u] == 32u &&
              owned.words[0x1000u + 68u] == 1u &&
              owned.bytes[0x4000u] == 'a' &&
              owned.bytes[0x4002u] == 'c',
          "owned buffer replaced with copied text and both cursor areas");

    for (const auto flags : {2u, 4u, 6u}) {
        Runtime runtime;
        runtime.words[0x1000u + 68u] = flags;
        runtime.bytes[0x1000u + 68u] =
            static_cast<std::uint8_t>(flags);
        runtime.words[0x2000u + 4u] = 0x5000u;
        runtime.words[0x2000u + 8u] = 3u;
        for (std::uint32_t i = 0u; i < 3u; ++i)
            runtime.bytes[0x5000u + i] = 'x';
        const auto result = pal4::CombatLogBuffer::InitializeFromText(
            0x1000u, 0x2000u, runtime);
        if (flags == 6u) {
            Check(result == 0x1114u && runtime.allocations.empty() &&
                      runtime.words[0x1100u] == 0u &&
                      runtime.words[0x1108u] == 0u &&
                      runtime.words[0x110cu] == 0u,
                  "simultaneous fixed input/output clears all pointers");
        } else if (flags == 2u) {
            Check(result == 3u && runtime.words[0x1108u] == 0x4000u &&
                      runtime.words[0x1110u] == 3u &&
                      runtime.words[0x110cu] == 0u,
                  "input-only initialization leaves output pointer empty");
        } else {
            Check(result == 5u && runtime.words[0x1108u] == 0x4000u &&
                      runtime.words[0x1110u] == 0u &&
                      runtime.words[0x110cu] == 0x4000u &&
                      runtime.words[0x1114u] == 3u,
                  "output-only initialization seeds empty input cursor");
        }
    }

    Runtime empty;
    empty.words[0x2000u + 4u] = 0u;
    empty.words[0x2000u + 8u] = 0u;
    Check(pal4::CombatLogBuffer::InitializeFromText(
              0x1000u, 0x2000u, empty) == 0x1114u &&
              empty.allocations.empty() &&
              empty.words[0x1114u] == 0u,
          "empty text does not allocate");
}

void CleanupStorageBranches() {
    for (const auto flags : {0x100u, 0x105u}) {
        Runtime runtime;
        runtime.words[0x1000u + 68u] = flags;
        runtime.bytes[0x1000u + 68u] =
            static_cast<std::uint8_t>(flags);
        runtime.words[0x1000u + 60u] = 0x5000u;
        runtime.words[0x1100u] = 0x3000u;
        Check(pal4::CombatLogBuffer::CleanupStorage(
                  0x1000u, runtime) == (flags & ~1u) &&
                  runtime.words[0x1000u + 68u] == (flags & ~1u) &&
                  runtime.words[0x1000u + 60u] == 0u &&
                  runtime.words[0x1100u] == 0x3000u &&
                  runtime.frees.size() == ((flags & 1u) ? 1u : 0u),
              "storage cleanup clears ownership without resetting base");
    }
}

void StreamBufferConstruction() {
    for (const auto references : {2u, 0xffffffffu}) {
        Runtime runtime;
        runtime.words[0x950a08u] = 0x6000u;
        runtime.words[0x6000u + 4u] = references;
        Check(pal4::CombatLogBuffer::ConstructBase(
                  0x1000u, runtime) == 0x1000u &&
                  runtime.locale_calls == 1 &&
                  runtime.words[0x1000u + 52u] == 0x6000u &&
                  runtime.words[0x6000u + 4u] ==
                      (references == 0xffffffffu ? references : 3u) &&
                  runtime.words[0x1000u] == 0x868310u &&
                  runtime.words[0x1000u + 12u] == 0x1004u &&
                  runtime.words[0x1000u + 16u] == 0x1008u &&
                  runtime.words[0x1000u + 28u] == 0x1014u &&
                  runtime.words[0x1000u + 32u] == 0x1018u &&
                  runtime.words[0x1000u + 44u] == 0x1024u &&
                  runtime.words[0x1000u + 48u] == 0x1028u &&
                  runtime.words[0x1000u + 4u] == 0u &&
                  runtime.words[0x1000u + 8u] == 0u &&
                  runtime.words[0x1000u + 20u] == 0u &&
                  runtime.words[0x1000u + 24u] == 0u &&
                  runtime.words[0x1000u + 36u] == 0u &&
                  runtime.words[0x1000u + 40u] == 0u,
              "streambuf constructor initializes embedded pointers and locale");
    }
}

void ExtendedStorageInitialization() {
    for (const auto flags : {0u, 2u, 4u, 6u}) {
        Runtime runtime;
        runtime.bytes[0x5000u] = 'a';
        runtime.bytes[0x5001u] = 'b';
        runtime.bytes[0x5002u] = 'c';
        const auto result = pal4::CombatLogBuffer::InitializeStorage(
            0x1000u, 0x5000u, 3, flags, runtime);
        if (flags == 6u) {
            Check(result == 0x14u && runtime.allocations.empty() &&
                      runtime.words[0x1108u] == 0u &&
                      runtime.words[0x110cu] == 0u &&
                      runtime.words[0x1000u + 68u] == 6u,
                  "fixed input/output returns low byte of output-count ref");
            continue;
        }
        Check(result == static_cast<std::uint8_t>(flags | 1u) &&
                  runtime.allocations ==
                      std::vector<std::uint32_t>{3u} &&
                  runtime.bytes[0x4000u] == 'a' &&
                  runtime.bytes[0x4002u] == 'c' &&
                  runtime.words[0x1000u + 60u] == 0x4003u &&
                  runtime.words[0x1000u + 68u] == (flags | 1u),
              "extended initializer copies storage and sets ownership");
        Check(runtime.words[0x1108u] == 0x4000u &&
                  runtime.words[0x1110u] == ((flags & 4u) ? 0u : 3u) &&
                  runtime.words[0x110cu] == ((flags & 2u) ? 0u : 0x4000u),
              "extended initializer follows input/output flags");
    }
    Runtime empty;
    Check(pal4::CombatLogBuffer::InitializeStorage(
              0x1000u, 0u, 0, 0u, empty) == 0x14u &&
              empty.allocations.empty(),
          "zero length skips allocation");
}

void FlagConversion() {
    constexpr std::uint8_t kExpected[]{6u, 2u, 4u, 0u,
                                        6u, 2u, 4u, 0u};
    for (std::uint8_t flags = 0u; flags < 8u; ++flags)
        Check(pal4::CombatLogBuffer::ConvertFlags(flags) == kExpected[flags],
              "flag conversion uses only input and output bits");
}

}  // namespace

int main() {
    try {
        ExistingCapacityAndEof();
        InitialAllocationAndSizeCap();
        GrowAndRelocate();
        RemoveBranches();
        PeekBranches();
        RelativeSeekBranches();
        AbsoluteSeekBranches();
        StringExtractionBranches();
        InitializeFromTextBranches();
        CleanupStorageBranches();
        StreamBufferConstruction();
        ExtendedStorageInitialization();
        FlagConversion();
        std::cout << "combat log buffer tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
