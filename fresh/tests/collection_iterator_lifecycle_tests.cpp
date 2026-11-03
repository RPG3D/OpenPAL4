#include "pal4/collection_iterator_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollectionIteratorLifecycleRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation{0x6000u};

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestCreationAndClone() {
    Runtime runtime;
    runtime.words[0x4024u] = 0xaaaau;
    runtime.words[0x4004u] = 0xbbbbu;
    std::uint32_t output = 0x1234u;
    Check(&pal4::CollectionIteratorLifecycle::CreateFromThird(
              0x4000u, output, runtime) == &output &&
              output == 0x6000u &&
              runtime.words.at(0x6000u) == 0x8497d8u &&
              runtime.words.at(0x6004u) == 0xaaaau &&
              runtime.words.at(0x6008u) == 0x4020u &&
              runtime.calls == std::vector<std::string>{
                  "read:16420", "allocate:12", "write:24576:8689712",
                  "write:24580:43690", "write:24584:16416",
                  "write:24576:8689624"},
          "third-array creator copies current pointer and embedded owner");

    runtime.calls.clear();
    output = 0x1234u;
    Check(&pal4::CollectionIteratorLifecycle::CreateFromFirst(
              0x4000u, output, runtime) == &output &&
              output == 0x6000u &&
              runtime.words.at(0x6000u) == 0x849804u &&
              runtime.words.at(0x6004u) == 0xbbbbu &&
              runtime.words.at(0x6008u) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  "read:16388", "allocate:12", "write:24576:8689756",
                  "write:24580:48059", "write:24584:16384",
                  "write:24576:8689668"},
          "first-array creator copies current pointer and group owner");

    runtime.allocation = 0u;
    runtime.calls.clear();
    Check(pal4::CollectionIteratorLifecycle::CreateFromThird(
              0x4000u, output, runtime) == 0u &&
              output == 0u && runtime.calls ==
                  std::vector<std::string>{"read:16420", "allocate:12"},
          "failed clone writes null output without touching iterator memory");
    runtime.calls.clear();
    Check(pal4::CollectionIteratorLifecycle::CloneSecond(
              {0x849804u, 1u, 2u}, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:12"},
          "second clone also returns null on allocation failure");
}

void TestAccessAndAdvance() {
    Runtime runtime;
    runtime.words[0x6004u] = 0x7000u;
    runtime.words[0x7000u] = 0xcafeu;
    runtime.words[0x6008u] = 0x4000u;
    runtime.words[0x4004u] = 0x7004u;
    runtime.words[0x4008u] = 0x7008u;
    Check(pal4::CollectionIteratorLifecycle::GetValue(0x6000u, runtime) ==
              0xcafeu && runtime.calls ==
              std::vector<std::string>{"read:24580", "read:28672"},
          "get value dereferences the current slot");
    runtime.calls.clear();
    Check(pal4::CollectionIteratorLifecycle::NextFromFirst(
              0x6000u, runtime) == 0x4000u &&
              runtime.words.at(0x6004u) == 0x7004u &&
              runtime.calls == std::vector<std::string>{
                  "read:24584", "read:16388", "write:24580:28676"},
          "first advance uses owner offset four");
    runtime.calls.clear();
    Check(pal4::CollectionIteratorLifecycle::NextFromSecond(
              0x6000u, runtime) == 0x4000u &&
              runtime.words.at(0x6004u) == 0x7008u &&
              runtime.calls == std::vector<std::string>{
                  "read:24584", "read:16392", "write:24580:28680"},
          "second advance uses owner offset eight");
}

void TestDestructors() {
    using Method = std::uint32_t (*)(
        std::uint32_t, std::uint32_t,
        pal4::ICollectionIteratorLifecycleRuntime&);
    const std::vector<std::pair<Method, std::uint32_t>> methods{
        {&pal4::CollectionIteratorLifecycle::DestroySecond, 0x84985cu},
        {&pal4::CollectionIteratorLifecycle::DestroyFirst, 0x849830u},
        {&pal4::CollectionIteratorLifecycle::CleanupFirst, 0x849830u},
        {&pal4::CollectionIteratorLifecycle::CleanupSecond, 0x84985cu}};
    for (const auto& [method, base] : methods) {
        Runtime runtime;
        Check(method(0x6000u, 2u, runtime) == 0x6000u &&
                  runtime.words.at(0x6000u) == base &&
                  runtime.calls == std::vector<std::string>{
                      "write:24576:" + std::to_string(base)},
              "destructor without low flag only resets vtable");
        runtime.calls.clear();
        Check(method(0x6000u, 3u, runtime) == 0x6000u &&
                  runtime.calls == std::vector<std::string>{
                      "write:24576:" + std::to_string(base), "free:24576"},
              "destructor with low flag frees after vtable reset");
    }
    pal4::CollectionIteratorLifecycleRuntimeStub stub;
    Check(stub.Allocate(12u) == 0u && stub.ReadWord(0u) == 0u,
          "native iterator lifecycle boundary has a linkable stub");
}

}  // namespace

int main() {
    TestCreationAndClone();
    TestAccessAndAdvance();
    TestDestructors();
}
