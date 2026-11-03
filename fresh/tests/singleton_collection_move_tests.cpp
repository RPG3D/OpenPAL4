#include "pal4/singleton_collection_move.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ISingletonCollectionMoveRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.emplace_back("write-byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t size) override {
        events.emplace_back("copy:" + std::to_string(size));
        std::vector<std::uint8_t> content;
        for (std::uint32_t index = 0u; index < size; ++index) {
            content.push_back(bytes.at(source + index));
        }
        for (std::uint32_t index = 0u; index < size; ++index) {
            bytes[destination + index] = content[index];
        }
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    void ThrowRange() override {
        throw std::out_of_range("range");
    }
    void DetachAndReset(std::uint32_t element) override {
        events.emplace_back("detach:" + std::to_string(element));
        ++detach_calls;
        if (detach_calls == 1u && first_detach_length != 0xffffffffu) {
            words[element + 8u] = first_detach_length;
        }
    }
    bool AllocateText(std::uint32_t element, std::uint32_t length,
                      std::uint8_t mode) override {
        events.emplace_back("allocate:" + std::to_string(element) + ":" +
                            std::to_string(length) + ":" +
                            std::to_string(mode));
        if (!allocate_succeeds) {
            return false;
        }
        words[element + 4u] = element < 0x2000u
            ? 0x6001u : 0x6001u + (element - 0x2000u) * 16u;
        words[element + 12u] = length + 8u;
        return true;
    }
    void SetTextSize(std::uint32_t element,
                     std::uint32_t length) override {
        events.emplace_back("size:" + std::to_string(element));
        words[element + 8u] = length;
        bytes[words.at(element + 4u) + length] = 0u;
    }
    void Put(std::uint32_t address, const std::string& content) {
        for (std::size_t index = 0u; index < content.size(); ++index) {
            bytes[address + static_cast<std::uint32_t>(index)] =
                static_cast<std::uint8_t>(content[index]);
        }
    }
    void SeedMetadata(std::uint32_t element, std::uint32_t seed) {
        words[element + 16u] = seed;
        words[element + 20u] = seed + 1u;
        words[element + 24u] = seed + 2u;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t detach_calls = 0u;
    std::uint32_t first_detach_length = 0xffffffffu;
    bool allocate_succeeds = true;
};

void CheckEmptyAndOwnRange() {
    Runtime runtime;
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x1000u, 0x2000u, runtime) == 0x2000u &&
              runtime.events.empty(),
          "empty range returns destination end without touching memory");

    runtime.words[0x1008u] = 3u;
    runtime.SeedMetadata(0x1000u, 7u);
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x101cu, 0x101cu, runtime) == 0x1000u &&
              runtime.detach_calls == 2u &&
              runtime.words[0x1010u] == 7u &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "detach:4096") == 2,
          "same-position move calls detach twice and preserves metadata");
}

void CheckSharedTextAndMetadata() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x3001u;
    runtime.words[0x1008u] = 3u;
    runtime.words[0x100cu] = 16u;
    runtime.SeedMetadata(0x1000u, 42u);
    runtime.words[0x2004u] = 0x4001u;
    runtime.bytes[0x3000u] = 1u;
    runtime.bytes[0x4000u] = 0u;
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x101cu, 0x201cu, runtime) == 0x2000u &&
              runtime.words[0x2004u] == 0x3001u &&
              runtime.words[0x2008u] == 3u &&
              runtime.words[0x200cu] == 16u &&
              runtime.bytes[0x3000u] == 2u &&
              runtime.words[0x2010u] == 42u &&
              runtime.words[0x2014u] == 43u &&
              runtime.words[0x2018u] == 44u,
          "shareable source transfers three string fields and three metadata fields");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "free:16384") != runtime.events.end(),
          "sharing releases destination's prior zero-reference header");
}

void CheckCopyAndAllocationFailure() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x3001u;
    runtime.words[0x1008u] = 3u;
    runtime.SeedMetadata(0x1000u, 5u);
    runtime.bytes[0x3000u] = 0xfeu;
    runtime.Put(0x3001u, "abc");
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x101cu, 0x201cu, runtime) == 0x2000u &&
              runtime.words[0x2004u] == 0x6001u &&
              runtime.words[0x2008u] == 3u &&
              runtime.bytes[0x6001u] == 'a' &&
              runtime.bytes[0x6003u] == 'c' &&
              runtime.bytes[0x6004u] == 0u &&
              runtime.words[0x2010u] == 5u,
          "unshareable source allocates, copies text and moves metadata");

    Runtime failed;
    failed.allocate_succeeds = false;
    failed.words[0x1004u] = 0x3001u;
    failed.words[0x1008u] = 3u;
    failed.words[0x2004u] = 0x4001u;
    failed.words[0x2008u] = 9u;
    failed.SeedMetadata(0x1000u, 99u);
    failed.bytes[0x3000u] = 0xfeu;
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x101cu, 0x201cu, failed) == 0x2000u &&
              failed.words[0x2004u] == 0x4001u &&
              failed.words[0x2008u] == 9u &&
              failed.words[0x2010u] == 99u,
          "failed text allocation still moves trailing metadata");
}

void CheckReverseOrder() {
    Runtime runtime;
    for (std::uint32_t i = 0u; i < 2u; ++i) {
        const std::uint32_t source = 0x1000u + i * 28u;
        runtime.words[source + 4u] = 0x3001u + i * 0x100u;
        runtime.words[source + 8u] = 1u;
        runtime.SeedMetadata(source, 10u + i * 10u);
        runtime.bytes[0x3000u + i * 0x100u] = 0xfeu;
        runtime.bytes[0x3001u + i * 0x100u] =
            static_cast<std::uint8_t>('a' + i);
    }
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x1038u, 0x2038u, runtime) == 0x2000u &&
              runtime.words[0x2010u] == 10u &&
              runtime.words[0x202cu] == 20u &&
              runtime.bytes[0x6001u] == 'a' &&
              runtime.bytes[0x61c1u] == 'b',
          "two elements are moved backward into their corresponding slots");
    const auto first = std::find(runtime.events.begin(), runtime.events.end(),
                                 "allocate:8220:1:1");
    const auto second = std::find(runtime.events.begin(), runtime.events.end(),
                                  "allocate:8192:1:1");
    Check(first < second, "higher-address element moves first");
}

void CheckOwnLengthChange() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x3001u;
    runtime.words[0x1008u] = 3u;
    runtime.SeedMetadata(0x1000u, 8u);
    runtime.first_detach_length = 4u;
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x101cu, 0x101cu, runtime) == 0x1000u &&
              runtime.detach_calls == 2u &&
              runtime.words[0x1008u] == 3u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "copy:0") != runtime.events.end() &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "allocate:4096:3:0") != runtime.events.end(),
          "self path keeps the observed zero-byte copy and mode-zero allocation");
}

void CheckOrdinaryDestinationReferenceAndEmptySource() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x3001u;
    runtime.words[0x1008u] = 1u;
    runtime.words[0x100cu] = 8u;
    runtime.SeedMetadata(0x1000u, 12u);
    runtime.words[0x2004u] = 0x4001u;
    runtime.bytes[0x3000u] = 1u;
    runtime.bytes[0x4000u] = 3u;
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x101cu, 0x201cu, runtime) == 0x2000u &&
              runtime.bytes[0x4000u] == 2u &&
              runtime.bytes[0x3000u] == 2u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "free:16384") == runtime.events.end(),
          "ordinary destination reference decrements without free");

    Runtime empty;
    empty.words[0x1004u] = 0u;
    empty.words[0x1008u] = 0u;
    empty.SeedMetadata(0x1000u, 55u);
    Check(pal4::SingletonCollectionMove::Backward(
              0x1000u, 0x101cu, 0x201cu, empty) == 0x2000u &&
              empty.words[0x2008u] == 0u &&
              empty.bytes[0x6001u] == 0u &&
              empty.words[0x2010u] == 55u &&
              std::find(empty.events.begin(), empty.events.end(),
                        "allocate:8192:0:1") != empty.events.end(),
          "zero-length source uses mode-one allocation and still moves metadata");
}

void CheckSingleElementCopy() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x3001u;
    runtime.words[0x1008u] = 2u;
    runtime.words[0x100cu] = 12u;
    runtime.SeedMetadata(0x1000u, 70u);
    runtime.words[0x2004u] = 0u;
    runtime.bytes[0x3000u] = 1u;
    Check(pal4::SingletonCollectionMove::CopyElement(
              0x2000u, 0x1000u, runtime) == 0x2000u &&
              runtime.words[0x2004u] == 0x3001u &&
              runtime.words[0x2008u] == 2u &&
              runtime.words[0x2010u] == 70u &&
              runtime.words[0x2018u] == 72u &&
              runtime.bytes[0x3000u] == 2u,
          "single-element helper returns destination and copies text/metadata");

    Runtime own;
    own.words[0x1008u] = 3u;
    own.SeedMetadata(0x1000u, 9u);
    Check(pal4::SingletonCollectionMove::CopyElement(
              0x1000u, 0x1000u, own) == 0x1000u &&
              own.detach_calls == 2u && own.words[0x1010u] == 9u,
          "single-element self-copy follows the two-detach path");
}

void CheckCopyConstruction() {
    Runtime runtime;
    runtime.bytes[0x1000u] = 0x5au;
    runtime.bytes[0x3000u] = 1u;
    runtime.words[0x1004u] = 0x3001u;
    runtime.words[0x1008u] = 2u;
    runtime.words[0x100cu] = 16u;
    runtime.SeedMetadata(0x1000u, 35u);
    Check(pal4::SingletonCollectionMove::CopyConstruct(
              0x2000u, 0x1000u, runtime) == 0x2000u &&
              runtime.bytes[0x2000u] == 0x5au &&
              runtime.words[0x2004u] == 0x3001u &&
              runtime.words[0x2008u] == 2u &&
              runtime.words[0x200cu] == 16u &&
              runtime.bytes[0x3000u] == 2u &&
              runtime.words[0x2010u] == 35u,
          "copy construction initializes prefix and shares source text");
    Check(runtime.events[0] == "read-byte:4096" &&
              runtime.events[1] == "write-byte:8192" &&
              runtime.events[2] == "write:8196" &&
              runtime.events[3] == "write:8200" &&
              runtime.events[4] == "write:8204",
          "prefix byte and three text fields initialize before source length");

    Runtime own;
    own.bytes[0x1000u] = 0x33u;
    own.words[0x1004u] = 0x3001u;
    own.words[0x1008u] = 4u;
    own.words[0x100cu] = 8u;
    own.SeedMetadata(0x1000u, 7u);
    Check(pal4::SingletonCollectionMove::CopyConstruct(
              0x1000u, 0x1000u, own) == 0x1000u &&
              own.bytes[0x1000u] == 0x33u &&
              own.words[0x1004u] == 0u &&
              own.words[0x1008u] == 0u &&
              own.words[0x100cu] == 0u &&
              own.words[0x1010u] == 7u &&
              own.detach_calls == 2u,
          "self construction clears text before self-copy and preserves metadata");
}

}  // namespace

int main() {
    CheckEmptyAndOwnRange();
    CheckSharedTextAndMetadata();
    CheckCopyAndAllocationFailure();
    CheckReverseOrder();
    CheckOwnLengthChange();
    CheckOrdinaryDestinationReferenceAndEmptySource();
    CheckSingleElementCopy();
    CheckCopyConstruction();
    std::cout << "8 singleton collection move/copy test cases passed\n";
}
