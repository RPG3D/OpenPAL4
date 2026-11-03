#include "pal4/win_action_task.h"

#include <algorithm>
#include <array>
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

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kActor = 0x2000u;
constexpr std::uint32_t kPositionChild = 0x3000u;
constexpr std::uint32_t kSecondChild = 0x4000u;
constexpr std::uint32_t kThirdChild = 0x5000u;
constexpr std::uint32_t kFirstHeader = 0x8fa2c8u;
constexpr std::uint32_t kSecondHeader = 0x8fa2d8u;
constexpr std::uint32_t kFirstData = 0x6289u;
constexpr std::uint32_t kSecondData = 0x7188u;

struct Runtime final : pal4::IWinActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kActor},
        {0x8f9798u, 0xa000u}, {0x8f9770u, 0xb000u},
        {0x8f95d0u, 0xc000u},
        {kFirstHeader + 4u, kFirstData},
        {kFirstHeader + 8u, 5u}, {kFirstHeader + 12u, 9u},
        {kSecondHeader + 4u, kSecondData},
        {kSecondHeader + 8u, 6u}, {kSecondHeader + 12u, 10u},
        {kThirdChild + 68u, 0x9000u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kFirstData - 1u, 1u}, {kSecondData - 1u, 2u},
        {0x841963u, 1u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> factories;
    std::vector<std::uint32_t> added_texts;
    std::uint32_t random = 499u;
    std::uint32_t fail_index = 0u;
    std::uint32_t third_child = kThirdChild;
    bool allocate_text = true;
    std::uint8_t erase_result = 0x5au;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte");
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
    void RestoreRequestedModel(std::uint32_t actor) override {
        Check(actor == kActor, "actor virtual +568 argument");
        calls.push_back("notify");
    }
    std::array<std::uint32_t, 3> ActorPosition(
        std::uint32_t actor) override {
        Check(actor == kActor, "actor virtual +444 argument");
        calls.push_back("position");
        return {11u, 22u, 33u};
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        factories.push_back(factory);
        calls.push_back("create");
        if (factories.size() == fail_index) return 0u;
        if (factories.size() == 1u) return kPositionChild;
        if (factories.size() == 2u) return kSecondChild;
        return third_child;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u &&
                  (child == kPositionChild || child == kSecondChild ||
                   child == third_child) &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "GOM child, container and timing");
        calls.push_back("add");
        added_texts.push_back(text);
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
    }
    void SetTextErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(header == third_child + 64u && flag == 1u,
              "shared text flag");
        calls.push_back("text_flag");
    }
    bool AllocateText(std::uint32_t header, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(header == third_child + 64u && flag == 1u,
              "allocated text header and flag");
        calls.push_back("allocate_text:" + std::to_string(length));
        if (allocate_text) words[header + 4u] = 0xd000u;
        return allocate_text;
    }
    void CopyText(std::uint32_t header, std::uint32_t source,
                  std::uint32_t length) override {
        Check(header == third_child + 64u &&
                  (source == kFirstData || source == kSecondData ||
                   source == 0x841964u),
              "text copy addresses");
        calls.push_back("copy:" + std::to_string(length));
    }
    std::uint8_t EraseText(std::uint32_t header,
                           std::uint32_t start,
                           std::uint32_t count) override {
        Check(header == third_child + 64u, "alias text destination");
        calls.push_back("erase:" + std::to_string(start) + ":" +
                        std::to_string(count));
        return erase_result;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void FactoryAndSharedTextPaths() {
    pal4::WinActionChildFactorySlot slot{};
    Check(pal4::WinActionSecondChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845a80u,
          "second factory slot and vtable");
    Check(pal4::WinActionThirdChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845a84u,
          "third factory slot and vtable");

    for (std::uint32_t sample : {499u, 500u, 1499u}) {
        Runtime r;
        r.random = sample;
        const auto second = sample % 1000u < 500u;
        Check(pal4::WinActionTask::Start(kTask, r) ==
                  (second ? 0x88u : 0x89u) &&
                  r.factories == std::vector<std::uint32_t>{
                      0x84591cu, 0x845a80u, 0x845a84u} &&
                  r.added_texts == std::vector<std::uint32_t>{
                      0xa000u, 0xb000u, 0xc000u},
              "499/500 boundary and three child factories");
        Check(r.words[kPositionChild + 72u] == kActor &&
                  r.words[kPositionChild + 60u] == 11u &&
                  r.words[kPositionChild + 64u] == 22u &&
                  r.words[kPositionChild + 68u] == 33u &&
                  r.words[kSecondChild + 60u] == kActor &&
                  r.words[kThirdChild + 60u] == kActor,
              "position and source child fields");
        const auto data = second ? kSecondData : kFirstData;
        Check(r.words[kThirdChild + 68u] == data &&
                  r.words[kThirdChild + 72u] == (second ? 6u : 5u) &&
                  r.words[kThirdChild + 76u] == (second ? 10u : 9u) &&
                  r.bytes[data - 1u] == (second ? 3u : 2u) &&
                  Index(r, "notify") < Index(r, "position") &&
                  Index(r, "position") < Index(r, "random") &&
                  Index(r, "random") < Index(r, "text_flag"),
              "shared text fields, reference and call order");
    }
}

void CopyEmptyAliasAndFallback() {
    Runtime r;
    r.bytes[kSecondData - 1u] = 0xfeu;
    Check(pal4::WinActionTask::Start(kTask, r) == 6u &&
              Called(r, "allocate_text:6") && Called(r, "copy:6") &&
              r.words[kThirdChild + 72u] == 6u &&
              r.bytes[0xd006u] == 0u,
          "saturated reference copies and terminates text");

    r = Runtime{};
    r.words[kSecondHeader + 8u] = 0u;
    Check(pal4::WinActionTask::Start(kTask, r) == 0u &&
              Called(r, "allocate_text:0") && Called(r, "copy:0"),
          "zero-length source allocates rather than shares");

    r = Runtime{};
    r.bytes[kSecondData - 1u] = 0xfeu;
    r.allocate_text = false;
    Check(pal4::WinActionTask::Start(kTask, r) == 0u &&
              Called(r, "allocate_text:6") && !Called(r, "copy:6"),
          "allocation failure returns without copy");

    r = Runtime{};
    r.words[kSecondHeader + 4u] = 0u;
    Check(pal4::WinActionTask::Start(kTask, r) == 0x64u &&
              r.words[kThirdChild + 68u] == 0x841964u &&
              r.bytes[0x841963u] == 2u,
          "null text data uses global empty buffer");

    r = Runtime{};
    r.third_child = kSecondHeader - 64u;
    Check(pal4::WinActionTask::Start(kTask, r) == r.erase_result &&
              Index(r, "erase:6:4294967295") < Index(r, "erase:0:0"),
          "alias source uses two erasures");
}

void GlobalsAndNullChildren() {
    Runtime r;
    r.words[0x8f9798u] = 0u;
    r.words[0x8f9770u] = 0u;
    r.words[0x8f95d0u] = 0u;
    Check(pal4::WinActionTask::Start(kTask, r) == 0x88u &&
              r.added_texts == std::vector<std::uint32_t>{
                  0x841964u, 0x841964u, 0x841964u},
          "each child text global has empty fallback");

    for (std::uint32_t failed : {1u, 2u, 3u}) {
        r = Runtime{};
        r.fail_index = failed;
        Check(pal4::WinActionTask::Start(kTask, r) == 0u &&
                  r.factories.size() == failed &&
                  r.added_texts.size() == failed - 1u &&
                  !Called(r, "random"),
              "null child stops safely at its factory stage");
    }
}

}  // namespace

int main() {
    try {
        Check(pal4::WinActionTask::kVtable == 0x845cb8u,
              "WinAction task vtable");
        FactoryAndSharedTextPaths();
        CopyEmptyAliasAndFallback();
        GlobalsAndNullChildren();
        std::cout << "WinAction task tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
