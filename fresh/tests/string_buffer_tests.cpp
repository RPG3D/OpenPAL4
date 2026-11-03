#include "pal4/string_buffer.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::IStringBufferRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t size) override {
        events.push_back("alloc:" + std::to_string(size));
        const auto address = next_allocation;
        next_allocation += size + 0x10U;
        return address;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return free_result;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        return static_cast<std::uint32_t>(memory[address]) |
               (static_cast<std::uint32_t>(memory[address + 1]) << 8U) |
               (static_cast<std::uint32_t>(memory[address + 2]) << 16U) |
               (static_cast<std::uint32_t>(memory[address + 3]) << 24U);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        for (std::uint32_t i = 0; i < 4; ++i) {
            memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8U));
        }
    }
    void Copy(const std::uint32_t destination, const std::uint32_t source,
              const std::uint32_t size) override {
        if (on_copy) on_copy();
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::to_string(source) + ":" + std::to_string(size));
        std::vector<std::uint8_t> temporary;
        for (std::uint32_t i = 0; i < size; ++i) temporary.push_back(memory[source + i]);
        for (std::uint32_t i = 0; i < size; ++i) memory[destination + i] = temporary[i];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
        memory[address] = value;
    }
    std::uint32_t FindSubstring(const std::uint32_t string,
                                const std::uint32_t substring) override {
        events.push_back("strstr:" + std::to_string(string) + ":" +
                         std::to_string(substring));
        const auto found = ReadCString(string).find(ReadCString(substring));
        return found == std::string::npos ? 0 : string + static_cast<std::uint32_t>(found);
    }
    std::uint32_t FindCharacter(const std::uint32_t string,
                                const std::int32_t character) override {
        events.push_back("strchr:" + std::to_string(string) + ":" +
                         std::to_string(character));
        const auto text = ReadCString(string);
        if (character == 0) return string + static_cast<std::uint32_t>(text.size());
        const auto found = text.find(static_cast<char>(character));
        return found == std::string::npos ? 0 : string + static_cast<std::uint32_t>(found);
    }
    std::uint32_t FindLastCharacter(const std::uint32_t string,
                                    const std::int32_t character) override {
        events.push_back("strrchr:" + std::to_string(string) + ":" +
                         std::to_string(character));
        const auto text = ReadCString(string);
        if (character == 0) return string + static_cast<std::uint32_t>(text.size());
        const auto found = text.rfind(static_cast<char>(character));
        return found == std::string::npos ? 0 : string + static_cast<std::uint32_t>(found);
    }
    std::int32_t CompareCaseInsensitive(const std::uint32_t left,
                                        const std::uint32_t right) override {
        events.push_back("strcmpi:" + std::to_string(left) + ":" +
                         std::to_string(right));
        auto a = ReadCString(left);
        auto b = ReadCString(right);
        std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return a.compare(b);
    }
    void ConvertUppercase(const std::uint32_t string) override { Transform(string, true); }
    void ConvertLowercase(const std::uint32_t string) override { Transform(string, false); }

    void Put(const std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0; i < text.size(); ++i) memory[address + static_cast<std::uint32_t>(i)] = static_cast<std::uint8_t>(text[i]);
        memory[address + static_cast<std::uint32_t>(text.size())] = 0;
    }
    void PutDword(const std::uint32_t address, const std::uint32_t value) {
        for (std::uint32_t i = 0; i < 4; ++i) memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8U));
    }
    [[nodiscard]] std::string ReadCString(const std::uint32_t address) const {
        std::string result;
        for (std::uint32_t i = 0; i < 4096; ++i) {
            const auto it = memory.find(address + i);
            const auto value = it == memory.end() ? 0 : it->second;
            if (value == 0) break;
            result.push_back(static_cast<char>(value));
        }
        return result;
    }

    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> events;
    std::function<void()> on_copy;
    std::uint32_t next_allocation = 0x5000;
    std::uint32_t free_result = 0xdeadbeef;
private:
    void Transform(const std::uint32_t string, const bool upper) {
        events.push_back(std::string(upper ? "strupr:" : "strlwr:") + std::to_string(string));
        for (std::uint32_t i = 0; i < 4096; ++i) {
            auto& value = memory[string + i];
            if (value == 0) break;
            value = static_cast<std::uint8_t>(upper ? std::toupper(value) : std::tolower(value));
        }
    }
};

void CreationAndConcatenation() {
    FakeRuntime r;
    r.Put(0x1000, "abc"); r.Put(0x1100, "xy");
    auto a = pal4::StringBuffer::Create(0x1000, 3, r);
    Check(a.State().length == 3 && a.State().capacity == 11 && r.ReadCString(a.State().data) == "abc", "create copies, terminates, and stores metadata");
    Check(r.events[0] == "alloc:11" && r.events[1] == "copy:20480:4096:3", "create allocation/copy order");
    auto b = pal4::StringBuffer::Concatenate(0x1000, 3, 0x1100, 2, r);
    Check(b.State().length == 5 && b.State().capacity == 13 && r.ReadCString(b.State().data) == "abcxy", "concatenate appends both spans and terminates");
    pal4::StringBuffer destination;
    const pal4::StringBufferState left{0x1000u, 3u, 100u};
    const pal4::StringBufferState right{0x1100u, 2u, 200u};
    r.events.clear();
    Check(&destination.ConcatenateRecords(left, right, r) == &destination &&
              destination.State().length == 5u &&
              destination.State().capacity == 13u &&
              r.ReadCString(destination.State().data) == "abcxy",
          "record wrapper forwards source fields and returns destination");
    Check(r.events == std::vector<std::string>{
                          "alloc:13", "copy:20536:4096:3",
                          "copy:20539:4352:2", "write:20541:0"},
          "record wrapper preserves concatenation operation order");
    r.Put(0x1200u, "z");
    r.events.clear();
    Check(&destination.ConcatenateRecordAndCString(
              left, 0x1200u, r) == &destination &&
              destination.State().length == 4u &&
              destination.State().capacity == 12u &&
              r.ReadCString(destination.State().data) == "abcz",
          "C-string wrapper scans length and returns the destination");
    Check(r.events == std::vector<std::string>{
              "read:4608", "read:4609", "alloc:12",
              "copy:" + std::to_string(destination.State().data) +
                  ":4096:3",
              "copy:" + std::to_string(destination.State().data + 3u) +
                  ":4608:1",
              "write:" + std::to_string(destination.State().data + 4u) +
                  ":0"},
          "C-string wrapper measures before allocation and forwards spans");
}

void CopyConstructPreservesLayout() {
    FakeRuntime r;
    r.Put(0x1000u, "abc");
    auto source = pal4::StringBuffer::Create(0x1000u, 3u, r);
    pal4::StringBuffer copy;
    r.events.clear();
    Check(&copy.CopyConstruct(source, r) == &copy &&
              copy.State().data != source.State().data &&
              copy.State().length == 3u && copy.State().capacity == 11u &&
              r.ReadCString(copy.State().data) == "abc",
          "copy constructor duplicates source content and metadata");
    Check(r.events == std::vector<std::string>{
              "alloc:11",
              "copy:" + std::to_string(copy.State().data) + ":" +
                  std::to_string(source.State().data) + ":3",
              "write:" + std::to_string(copy.State().data + 3u) + ":0"},
          "copy constructor preserves allocation, copy, terminator order");

    pal4::StringBuffer empty_source({0u, 0u, 99u});
    pal4::StringBuffer empty_copy;
    r.events.clear();
    Check(&empty_copy.CopyConstruct(empty_source, r) == &empty_copy &&
              empty_copy.State().length == 0u &&
              empty_copy.State().capacity == 8u &&
              r.ReadCString(empty_copy.State().data).empty(),
          "empty source still allocates eight bytes and writes NUL");
}

void EmbeddedInitializersPreserveAllocationQuirks() {
    FakeRuntime r;
    pal4::StringBuffer empty;
    Check(&empty.InitializeEmpty(r) == &empty && empty.State().data == 0x5000 &&
              empty.State().length == 0 && empty.State().capacity == 0x20 &&
              r.memory[0x5000] == 0,
          "default initializer allocates 32 bytes and writes an empty string");
    r.events.clear();
    pal4::StringBuffer null_source;
    Check(&null_source.InitializeFromCString(0, r) == &null_source &&
              null_source.State().data == 0 && null_source.State().length == 0 &&
              null_source.State().capacity == 8 && r.events.empty(),
          "null source initializer uses the observed inline empty state");
    r.Put(0x1000, "abc");
    pal4::StringBuffer source;
    Check(&source.InitializeFromCString(0x1000, r) == &source &&
              source.State().length == 3 && source.State().capacity == 11 &&
              r.ReadCString(source.State().data) == "abc",
          "non-null initializer stores length and length-plus-eight capacity");
    Check(std::find(r.events.begin(), r.events.end(), "alloc:11") != r.events.end(),
          "non-null source path allocates length plus eight bytes");
}

void CleanupAndReallocation() {
    FakeRuntime r;
    pal4::StringBuffer empty;
    Check(empty.Cleanup(r) == 0 && r.events.empty(), "null cleanup is a no-op");
    auto b = pal4::StringBuffer::Create(0x1000, 3, r);
    const auto old = b.State().data;
    r.events.clear(); b.Reallocate(11, true, r);
    Check(b.State().data != old && b.State().capacity == 19, "growth uses length plus eight capacity");
    Check(r.events == std::vector<std::string>{"alloc:19", "copy:20507:20480:3", "free:20480"}, "growth copies before freeing old allocation");
    r.events.clear(); const auto current = b.State().data; b.Reallocate(11, true, r);
    Check(b.State().data == current && r.events.empty(), "sufficient capacity avoids allocation");
    Check(b.Cleanup(r) == r.free_result && b.State().data == 0, "cleanup returns free result and clears data");
}

void ReallocationUsesSignedCapacityComparison() {
    FakeRuntime runtime;
    pal4::StringBuffer small({0x1000u, 0u, 16u});
    small.Reallocate(0x7fffffffu, false, runtime);
    Check(runtime.events.empty() && small.State().capacity == 16u,
          "signed comparison treats requested length plus one as negative");

    pal4::StringBuffer high_capacity({0x1000u, 0u, 0x80000000u});
    high_capacity.Reallocate(15u, false, runtime);
    Check(high_capacity.State().capacity == 23u &&
              runtime.events == std::vector<std::string>{
                  "alloc:23", "free:4096"},
          "signed comparison grows when stored capacity has the sign bit set");
}

void VariadicFormattingUsesObservedBufferAndLength() {
    struct Formatter final : pal4::IStringBufferFormatRuntime {
        std::vector<std::string>& events;
        std::int32_t result = 7;
        explicit Formatter(std::vector<std::string>& events) : events(events) {}
        std::int32_t Vsprintf(std::uint32_t destination,
                              std::uint32_t format,
                              std::uint32_t arguments) override {
            Check(format == 0x3000u && arguments == 0x4000u,
                  "format and argument-list addresses are forwarded");
            events.push_back("vsprintf:" + std::to_string(destination));
            return result;
        }
    };

    FakeRuntime runtime;
    pal4::StringBuffer buffer({0x2000u, 3u, 16u});
    Formatter formatter(runtime.events);
    Check(&buffer.FormatVariadic(0x3000u, 0x4000u,
                                 runtime, formatter) == &buffer &&
              buffer.State().capacity == 1032u &&
              buffer.State().length == 7u &&
              runtime.events == std::vector<std::string>{
                  "alloc:1032", "free:8192", "vsprintf:20480"},
          "variadic format grows without copying, then stores CRT result");
    runtime.events.clear();
    formatter.result = -1;
    Check(&buffer.FormatVariadic(0x3000u, 0x4000u,
                                 runtime, formatter) == &buffer &&
              buffer.State().length == 0xffffffffu &&
              runtime.events ==
                  std::vector<std::string>{"vsprintf:20480"},
          "sufficient capacity skips growth and preserves signed CRT bits");
}

void ReinitializeStoresAllocatedData() {
    FakeRuntime r;
    auto b = pal4::StringBuffer::Create(0x1000, 1, r);
    const auto old = b.State().data; r.events.clear();
    b.ReallocateAndInitialize(99, r);
    Check(b.State().data == 0x5019u && b.State().data != old &&
              b.State().length == 0 && b.State().capacity == 107,
          "reinitialize stores the new allocation and resets length");
    Check(r.events == std::vector<std::string>{"free:" + std::to_string(old), "alloc:107", "write:20505:0"}, "reinitialize uses its sole stack argument as requested length");
}

void SearchesReturnOffsetAndMinusOne() {
    FakeRuntime r; r.Put(0x1000, "abcabc"); r.Put(0x1100, "bc");
    r.PutDword(0x1200, 0x1100);
    auto b = pal4::StringBuffer::Create(0x1000, 6, r);
    Check(b.FindSubstring(0x1100, 2, r) == 4, "substring search returns offset from original data");
    Check(b.FindSubstringOffset(0x1200, 0, r) == 1, "offset search dereferences the substring holder and is relative to original buffer");
    Check(b.FindSubstringOffset(0x1200, 2, r) == 4,
          "holder substring search starts at the offset but returns an original-data index");
    r.memory[0x4fff] = 'z';
    r.Put(0x1300, "ab");
    r.events.clear();
    Check(b.FindSubstring(0x1300, static_cast<std::uint32_t>(-1), r) == 0 &&
              r.events == std::vector<std::string>{"strstr:20479:4864"},
          "negative start offset is added as a 32-bit address before search");
    r.Put(0x1100, "zz");
    Check(b.FindSubstring(0x1100, 0, r) == -1 && b.FindSubstringOffset(0x1200, 0, r) == -1, "missing substring returns minus one");
    Check(b.FindLastCharacter('b', r) == 4 && b.FindLastCharacter('z', r) == -1, "reverse character lookup returns last offset or minus one");
    r.events.clear();
    Check(b.FindLastCharacter(0x80, r) == -1 &&
              r.events == std::vector<std::string>{"strrchr:20480:-128"},
          "reverse character lookup sign-extends the low input byte");
}

void ComparisonsAndCaseConversion() {
    FakeRuntime r; r.Put(0x1200, "abc"); r.Put(0x1300, "acz");
    r.PutDword(0x1400, 0x1200); r.PutDword(0x1404, 0x1300);
    Check(pal4::StringBuffer::Compare(0x1400, 0x1404, r) == -1,
          "comparison dereferences both records and compares differing bytes");
    r.PutDword(0x1404, 0x1200);
    Check(pal4::StringBuffer::Compare(0x1400, 0x1404, r) == 0,
          "equal strings compare equal regardless of incoming register state");
    r.Put(0x1300, "ab");
    r.PutDword(0x1404, 0x1300);
    Check(pal4::StringBuffer::Compare(0x1400, 0x1404, r) == 1,
          "longer string compares greater after equal prefix");
    Check(pal4::StringBuffer::Compare(0x1404, 0x1400, r) == -1,
          "shorter string compares less after equal prefix");
    r.Put(0x1200, std::string(1, static_cast<char>(0x80)));
    r.Put(0x1300, std::string(1, static_cast<char>(0x7f)));
    Check(pal4::StringBuffer::Compare(0x1400, 0x1404, r) == 1,
          "high-bit bytes use the assembly's unsigned comparison");
    r.Put(0x1200, "abc");
    r.Put(0x1300, "abc");
    auto first = pal4::StringBuffer::Create(0x1200, 3, r);
    auto second = pal4::StringBuffer::Create(0x1300, 3, r);
    Check(pal4::StringBuffer::Compare(first, second, r) == 0,
          "object overload compares owned data");
    static_cast<void>(first.Cleanup(r));
    static_cast<void>(second.Cleanup(r));
    r.Put(0x1000, "MiXeD"); r.Put(0x1100, "mixed");
    auto b = pal4::StringBuffer::Create(0x1000, 5, r);
    r.PutDword(0x1400, 0x1100);
    r.events.clear();
    Check(b.CompareCaseInsensitive(0x1400, r) == 0 &&
              r.events.front() == "strcmpi:" +
                  std::to_string(b.State().data) + ":4352",
          "case insensitive wrapper dereferences the other record");
    Check(&b.ToUpper(r) == &b && r.ReadCString(b.State().data) == "MIXED", "uppercase mutates in place and returns object");
    Check(&b.ToLower(r) == &b && r.ReadCString(b.State().data) == "mixed", "lowercase mutates in place and returns object");
}

void TrimmingPaths() {
    FakeRuntime r; r.Put(0x1000, "word  ");
    auto b = pal4::StringBuffer::Create(0x1000, 6, r);
    r.Put(0x2000, " ");
    Check(&b.TrimRight(0x2000, r) == &b && b.State().length == 4 && r.ReadCString(b.State().data) == "word", "trim-right updates length and terminator and returns object");
    r.Put(0x1200, " x"); r.Put(0x2000, " ");
    auto left = pal4::StringBuffer::Create(0x1200, 3, r);
    r.events.clear();
    Check(&left.TrimLeft(0x2000, r) == &left &&
              left.State().length == 2 &&
              r.ReadCString(left.State().data) == "x" &&
              r.events.back() == "copy:" +
                  std::to_string(left.State().data) + ":" +
                  std::to_string(left.State().data + 1U) + ":3",
          "trim-left moves overlapping content and returns the object");
    r.events.clear();
    Check(&left.TrimLeft(0x2000, r) == &left &&
              left.State().length == 2 &&
              r.events.back() == "copy:" +
                  std::to_string(left.State().data) + ":" +
                  std::to_string(left.State().data) + ":3",
          "trim-left non-match still copies through the original data pointer");
    r.Put(0x1400, "   ");
    auto all = pal4::StringBuffer::Create(0x1400, 3, r);
    r.events.clear();
    Check(&all.TrimLeft(0x2000, r) == &all &&
              all.State().length == 0 &&
              r.ReadCString(all.State().data).empty() &&
              r.events.back() == "copy:" +
                  std::to_string(all.State().data) + ":" +
                  std::to_string(all.State().data + 3U) + ":1",
          "trim-left all-match copies the terminator");
    r.events.clear();
    Check(&all.TrimLeft(0x2000, r) == &all &&
              r.events == std::vector<std::string>{
                  "copy:" + std::to_string(all.State().data) + ":" +
                      std::to_string(all.State().data) + ":1"},
          "trim-left empty input skips scanning but copies the terminator");
    r.Put(0x1300, "abc");
    auto both = pal4::StringBuffer::Create(0x1300, 3, r);
    r.events.clear();
    Check(&both.Trim(0x2000, r) == &both &&
              r.ReadCString(both.State().data) == "abc" &&
              r.events.back() == "copy:" +
                  std::to_string(both.State().data) + ":" +
                  std::to_string(both.State().data) + ":4",
          "combined trim returns the object and invokes left trim after right trim");
}

void TrimRightBoundaryPaths() {
    FakeRuntime runtime;
    runtime.Put(0x2000u, " ");

    runtime.Put(0x1000u, "");
    auto empty = pal4::StringBuffer::Create(0x1000u, 0u, runtime);
    runtime.events.clear();
    Check(&empty.TrimRight(0x2000u, runtime) == &empty &&
              empty.State().length == 0u &&
              runtime.events == std::vector<std::string>{
                  "write:" + std::to_string(empty.State().data) + ":0"},
          "trim-right empty input writes the terminator without searching");

    runtime.Put(0x1100u, "   ");
    auto all = pal4::StringBuffer::Create(0x1100u, 3u, runtime);
    runtime.events.clear();
    Check(&all.TrimRight(0x2000u, runtime) == &all &&
              all.State().length == 0u &&
              runtime.ReadCString(all.State().data).empty() &&
              runtime.events.back() ==
                  "write:" + std::to_string(all.State().data) + ":0",
          "trim-right all-match path reaches zero length");

    runtime.Put(0x1200u, std::string(1, static_cast<char>(0x80)));
    runtime.Put(0x2100u, std::string(1, static_cast<char>(0x80)));
    auto high = pal4::StringBuffer::Create(0x1200u, 1u, runtime);
    runtime.events.clear();
    Check(&high.TrimRight(0x2100u, runtime) == &high &&
              high.State().length == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:" + std::to_string(high.State().data),
                  "strchr:8448:-128",
                  "write:" + std::to_string(high.State().data) + ":0"},
          "trim-right sign-extends high-bit characters before strchr");
}

void InitializeAndAppend() {
    FakeRuntime r; r.Put(0x1000, "ab"); r.Put(0x1100, "cd");
    pal4::StringBuffer b;
    Check(&b.Initialize(0x1000, 2, r) == &b &&
              b.State().length == 2 && b.State().capacity == 10 &&
              r.ReadCString(b.State().data) == "ab" &&
              r.events == std::vector<std::string>{
                  "alloc:10", "copy:20480:4096:2", "write:20482:0"},
          "null-data initialization stores capacity and reuses its first allocation");
    r.events.clear();
    Check(&b.Append(0x1100, 2, r) == &b && b.State().length == 4 &&
              r.events == std::vector<std::string>{
                  "copy:20482:4352:2", "write:20484:0"},
          "append with capacity skips growth and terminates");
    Check(r.ReadCString(b.State().data) == "abcd", "append preserves prefix, adds source and terminates");

    r.Put(0x1200, "efghij");
    r.events.clear();
    Check(&b.Append(0x1200, 6, r) == &b &&
              b.State().length == 10 && b.State().capacity == 18 &&
              r.ReadCString(b.State().data) == "abcdefghij" &&
              r.events == std::vector<std::string>{
                  "alloc:18", "copy:20506:20480:4", "free:20480",
                  "copy:20510:4608:6", "write:20516:0"},
          "append growth copies old content before freeing and appends after it");

    pal4::StringBuffer preallocated({0x3000, 1, 16});
    r.Put(0x3000, "z");
    r.events.clear();
    std::uint32_t length_at_copy = 0;
    r.on_copy = [&] { length_at_copy = preallocated.State().length; };
    Check(&preallocated.Initialize(0x1000, 2, r) == &preallocated &&
              preallocated.State().data == 0x3000 &&
              preallocated.State().capacity == 16 &&
              length_at_copy == 2 &&
              r.ReadCString(0x3000) == "ab" &&
              r.events == std::vector<std::string>{
                  "copy:12288:4096:2", "write:12290:0"},
          "existing capacity stores length before copying into reused storage");
    r.on_copy = {};
}

void ExtractTail() {
    for (const auto& pair : {
             std::pair<std::int32_t, std::string>{0, ""},
             {2, "ef"}, {6, "abcdef"},
             {9, "abcdef"}}) {
        FakeRuntime runtime;
        runtime.Put(0x1000u, "abcdef");
        runtime.PutDword(0x2000u, 0x1000u);
        runtime.PutDword(0x2004u, 6u);
        Check(pal4::StringBuffer::ExtractTailAt(
                  0x2000u, 0x3000u, pair.first,
                  runtime) == 0x3000u,
              "tail extraction returns destination record");
        const auto data = runtime.ReadDword(0x3000u);
        const auto length = runtime.ReadDword(0x3004u);
        Check(length == pair.second.size() &&
                  runtime.ReadDword(0x3008u) == length + 8u &&
                  runtime.ReadCString(data) == pair.second,
              "tail extraction clamps length and copies suffix");
    }
}

void CharacterAddressUsesNativeRecordPointer() {
    FakeRuntime runtime;
    runtime.PutDword(0x2004u, 0u);
    Check(pal4::StringBuffer::CharacterAddressAt(0x2000u, 123,
                                                 runtime) == 0x841964u,
          "null record uses shared empty log message");
    runtime.PutDword(0x2004u, 0x5000u);
    Check(pal4::StringBuffer::CharacterAddressAt(0x2000u, 3,
                                                 runtime) == 0x5003u &&
              pal4::StringBuffer::CharacterAddressAt(0x2000u, -2,
                                                     runtime) == 0x4ffeu,
          "non-null record applies signed offset without bounds check");
}

void CreatePrefixUsesSignedClamp() {
    FakeRuntime runtime;
    runtime.Put(0x1000u, "hello");
    const pal4::StringBufferState source{0x1000u, 5u, 9u};
    const auto short_text = pal4::StringBuffer::CreatePrefix(
        source, 2, runtime);
    Check(short_text.State().length == 2u &&
              short_text.State().capacity == 10u &&
              runtime.ReadCString(short_text.State().data) == "he" &&
              runtime.events[0] == "alloc:10" &&
              runtime.events[1] == "copy:20480:4096:2",
          "short prefix composes native string constructor");
    runtime.events.clear();
    const auto full_text = pal4::StringBuffer::CreatePrefix(
        source, 20, runtime);
    Check(full_text.State().length == 5u &&
              runtime.ReadCString(full_text.State().data) == "hello" &&
              runtime.events[0] == "alloc:13",
          "requested prefix clamps to source length");
    runtime.events.clear();
    const auto empty_text = pal4::StringBuffer::CreatePrefix(
        source, 0, runtime);
    Check(empty_text.State().length == 0u &&
              empty_text.State().capacity == 8u &&
              runtime.ReadCString(empty_text.State().data).empty() &&
              runtime.events[0] == "alloc:8" &&
              source.length == 5u,
          "zero prefix leaves source and creates empty destination");
}

}  // namespace

int main() {
    Check(pal4::StringBuffer::EmptyLogMessageAddress() == 0x841964u,
          "shared empty string address matches IDA");
    CreationAndConcatenation();
    CopyConstructPreservesLayout();
    EmbeddedInitializersPreserveAllocationQuirks();
    CleanupAndReallocation();
    ReallocationUsesSignedCapacityComparison();
    VariadicFormattingUsesObservedBufferAndLength();
    ReinitializeStoresAllocatedData();
    SearchesReturnOffsetAndMinusOne();
    ComparisonsAndCaseConversion();
    TrimmingPaths();
    TrimRightBoundaryPaths();
    InitializeAndAppend();
    ExtractTail();
    CharacterAddressUsesNativeRecordPointer();
    CreatePrefixUsesSignedClamp();
    std::cout << "String buffer tests passed\n";
}
