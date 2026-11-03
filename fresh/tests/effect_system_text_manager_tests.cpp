#include "pal4/effect_system_text_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
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

struct Runtime final : pal4::IEffectSystemTextManagerRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t nil = 0u;
    std::uint32_t references = 0u;
    std::uint32_t next_allocation = 0x4000u;
    unsigned allocations{}, fail_at{};

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        if (++allocations == fail_at) throw std::bad_alloc{};
        Check(size == 36u, "nil and head use 36-byte allocation");
        const auto result = next_allocation;
        next_allocation += 0x100u;
        calls.push_back("allocate:" + std::to_string(result));
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override { nil = address; }
    std::uint32_t SharedNilReferences() override { return references; }
    void SetSharedNilReferences(std::uint32_t count) override {
        references = count;
        calls.push_back("references:" + std::to_string(count));
    }
};

void TestNewSharedNil() {
    Runtime runtime;
    Check(pal4::EffectSystemTextManager::Construct(
              0x2000u, 0x5au, runtime) == 0x2000u &&
              runtime.bytes.at(0x2000u) == 0x5au &&
              runtime.bytes.at(0x2001u) == 0x5au &&
              runtime.bytes.at(0x2008u) == 0u &&
              runtime.nil == 0x4000u && runtime.references == 1u &&
              runtime.words.at(0x4000u) == 0u &&
              runtime.words.at(0x4004u) == 0u &&
              runtime.words.at(0x4008u) == 0u &&
              runtime.words.at(0x4020u) == 1u &&
              runtime.words.at(0x2004u) == 0x4100u &&
              runtime.words.at(0x200cu) == 0u &&
              runtime.words.at(0x4100u) == 0x4100u &&
              runtime.words.at(0x4104u) == 0x4000u &&
              runtime.words.at(0x4108u) == 0x4100u &&
              runtime.words.at(0x4120u) == 0u,
          "first manager creates shared nil and self-linked head");
    Check(runtime.calls == std::vector<std::string>{
              "allocate:16384", "references:1", "allocate:16640"},
          "first manager retains both allocations");
}

void TestReusedSharedNil() {
    Runtime runtime;
    runtime.nil = 0x9000u;
    runtime.references = 2u;
    runtime.next_allocation = 0x5000u;
    Check(pal4::EffectSystemTextManager::Construct(
              0x3000u, 0xa5u, runtime) == 0x3000u &&
              runtime.nil == 0x9000u && runtime.references == 3u &&
              runtime.words.at(0x3004u) == 0x5100u &&
              runtime.words.at(0x5104u) == 0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:20480", "references:3", "free:20480",
                  "allocate:20736"},
          "reused nil frees candidate and still allocates a fresh head");
}

void TestConstructionFailure() {
    for (const bool survivor : {false,true}) {
        for (const unsigned failure : {1u,2u}) {
            Runtime runtime;
            runtime.nil=survivor?0x9000u:0u;
            runtime.references=survivor?2u:0u;
            runtime.fail_at=failure;
            bool threw{};
            try {(void)pal4::EffectSystemTextManager::Construct(0x2000u,0u,runtime);}
            catch (const std::bad_alloc&) {threw=true;}
            Check(threw && runtime.nil==(survivor?0x9000u:0u) &&
                runtime.references==(survivor?2u:0u),
                "candidate/head failure preserves surviving domain references");
            if (failure==2u)
                Check(runtime.calls.back()==(survivor?"references:2":"free:16384"),
                    "failed head releases only an unshared candidate");
            runtime.fail_at=0u;
            Check(pal4::EffectSystemTextManager::Construct(0x2000u,0u,runtime)==0x2000u &&
                runtime.references==(survivor?3u:1u),"construction can retry after failure");
        }
    }
}

struct CleanupRuntime final : pal4::IEffectSystemTextManagerCleanupRuntime {
    static constexpr std::uint32_t kManager = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNil = 0x9000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kManager + 4u, kHead}, {kManager + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil},
        {kHead + 8u, kHead}};
    std::vector<std::string> calls;
    std::uint32_t nil = kNil;
    std::uint32_t references = 1u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void ClearTextElements(std::uint32_t manager) override {
        Check(manager == kManager, "clear receives manager");
        calls.push_back("clear");
    }
    void DeleteSubtree(std::uint32_t root) override {
        calls.push_back("subtree:" + std::to_string(root));
    }
    std::uint32_t Next(std::uint32_t node) override {
        calls.push_back("next:" + std::to_string(node));
        return words.at(node + 16u);
    }
    void EraseNode(std::uint32_t manager,
                   std::uint32_t node) override {
        Check(manager == kManager, "erase receives manager");
        calls.push_back("erase:" + std::to_string(node));
    }
    void GetFirstElement(std::uint32_t manager) override {
        Check(manager == kManager, "first element receives manager");
        calls.push_back("first");
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override {
        nil = address;
        calls.push_back("nil:" + std::to_string(address));
    }
    std::uint32_t SharedNilReferences() override { return references; }
    void SetSharedNilReferences(std::uint32_t count) override {
        references = count;
        calls.push_back("references:" + std::to_string(count));
    }
};

void TestDestructEmptyAndShared() {
    CleanupRuntime last;
    Check(pal4::EffectSystemTextManager::Destruct(
              CleanupRuntime::kManager, last) == CleanupRuntime::kNil + 1u &&
              last.calls == std::vector<std::string>{
                  "clear", "free:12288", "references:0", "nil:0",
                  "free:36864"} &&
              last.nil == 0u && last.references == 0u &&
              last.words.at(CleanupRuntime::kManager + 4u) == 0u,
          "last empty manager releases head then nil");

    CleanupRuntime shared;
    shared.references = 2u;
    Check(pal4::EffectSystemTextManager::Destruct(
              CleanupRuntime::kManager, shared) == 1u &&
              shared.calls == std::vector<std::string>{
                  "clear", "free:12288", "references:1"} &&
              shared.nil == CleanupRuntime::kNil,
          "shared nil survives non-final manager");
}

void TestDestructPopulatedAndFallback() {
    CleanupRuntime populated;
    populated.words[CleanupRuntime::kManager + 12u] = 2u;
    populated.words[CleanupRuntime::kHead] = 0x4000u;
    populated.words[CleanupRuntime::kHead + 4u] = 0x4000u;
    populated.words[CleanupRuntime::kHead + 8u] = 0x4000u;
    (void)pal4::EffectSystemTextManager::Destruct(
        CleanupRuntime::kManager, populated);
    Check(populated.calls == std::vector<std::string>{
              "clear", "subtree:16384", "first", "free:12288",
              "references:0", "nil:0", "free:36864"} &&
              populated.words.at(CleanupRuntime::kHead) ==
                  CleanupRuntime::kHead &&
              populated.words.at(CleanupRuntime::kHead + 4u) ==
                  CleanupRuntime::kNil,
          "populated manager deletes subtree and resets head before free");

    CleanupRuntime fallback;
    fallback.words[CleanupRuntime::kHead] = 0x4000u;
    fallback.words[0x4010u] = 0x4100u;
    fallback.words[0x4110u] = CleanupRuntime::kHead;
    (void)pal4::EffectSystemTextManager::Destruct(
        CleanupRuntime::kManager, fallback);
    Check(fallback.calls == std::vector<std::string>{
              "clear", "next:16384", "erase:16384", "next:16640",
              "erase:16640", "free:12288", "references:0", "nil:0",
              "free:36864"},
          "zero-count fallback advances before erasing");
}

struct LookupRuntime final : pal4::IEffectSystemTextManagerLookupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x2004u, 0x3000u}, {0x4000u + 28u, 0xdeadbeefu},
        {0x4100u + 28u, 0x12345678u}};
    std::unordered_map<std::uint32_t, std::string> names{
        {0x400cu, "alpha"}, {0x410cu, "gamma"}};
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t LowerBound(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x2000u, "lookup receives manager");
        if (name <= "alpha") return 0x4000u;
        if (name <= "gamma") return 0x4100u;
        return 0x3000u;
    }
    int CompareName(std::string_view name,
                    std::uint32_t node_record) override {
        return name.compare(names.at(node_record));
    }
};

void TestLookup() {
    LookupRuntime runtime;
    Check(pal4::EffectSystemTextManager::Contains(
              0x2000u, "alpha", runtime) &&
              pal4::EffectSystemTextManager::Contains(
                  0x2000u, "gamma", runtime),
          "exact names are present");
    Check(!pal4::EffectSystemTextManager::Contains(
              0x2000u, "aardvark", runtime) &&
              !pal4::EffectSystemTextManager::Contains(
                  0x2000u, "beta", runtime) &&
              !pal4::EffectSystemTextManager::Contains(
                  0x2000u, "omega", runtime),
          "lower-bound candidates are not mistaken for exact names");
    Check(pal4::EffectSystemTextManager::GetProperty(
              0x2000u, "alpha", runtime) == 0xdeadbeefu &&
              pal4::EffectSystemTextManager::GetProperty(
                  0x2000u, "gamma", runtime) == 0x12345678u &&
              pal4::EffectSystemTextManager::GetProperty(
                  0x2000u, "beta", runtime) == 0u &&
              pal4::EffectSystemTextManager::GetProperty(
                  0x2000u, "omega", runtime) == 0u,
          "property query returns node +28 only for an exact name");
}

struct ClearRuntime final : pal4::IEffectSystemTextManagerClearRuntime {
    static constexpr std::uint32_t kManager = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNil = 0x8f605cu;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kManager + 4u, kHead}, {kManager + 12u, 3u},
        {kHead, 0x4020u}, {kHead + 4u, 0x4000u},
        {kHead + 8u, 0x4040u},
        {0x4000u, 0x4020u}, {0x4008u, 0x4040u},
        {0x4000u + 16u, 0x4040u}, {0x4000u + 28u, 0x7000u},
        {0x4020u, kNil}, {0x4028u, kNil},
        {0x4020u + 16u, 0x4000u}, {0x4020u + 28u, 0x7100u},
        {0x4040u + 16u, kHead}, {0x4040u + 28u, 0u}};
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SharedNil() override { return kNil; }
    void CleanupProperty(std::uint32_t property) override {
        calls.push_back("property:" + std::to_string(property));
    }
    std::uint32_t Next(std::uint32_t node) override {
        calls.push_back("next:" + std::to_string(node));
        return words.at(node + 16u);
    }
    void DeleteSubtree(std::uint32_t root) override {
        calls.push_back("subtree:" + std::to_string(root));
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back("release:" + std::to_string(record));
    }
    void FreeNode(std::uint32_t node) override {
        calls.push_back("free:" + std::to_string(node));
    }
    void EraseNode(std::uint32_t manager,
                   std::uint32_t node) override {
        Check(manager == kManager, "clear erase receives manager");
        calls.push_back("erase:" + std::to_string(node));
    }
};

void TestClearElements() {
    ClearRuntime runtime;
    Check(pal4::EffectSystemTextManager::ClearElements(
              ClearRuntime::kManager, runtime) == ClearRuntime::kHead &&
              runtime.calls == std::vector<std::string>{
                  "property:28928", "next:16416", "property:28672",
                  "next:16384", "next:16448", "subtree:16448",
                  "release:16396", "free:16384", "subtree:9396316",
                  "release:16428", "free:16416"},
          "property cleanup precedes right-subtree and left-chain release");
    Check(runtime.words.at(0x4000u + 28u) == 0u &&
              runtime.words.at(0x4020u + 28u) == 0u &&
              runtime.words.at(ClearRuntime::kManager + 12u) == 0u &&
              runtime.words.at(ClearRuntime::kHead) == ClearRuntime::kHead &&
              runtime.words.at(ClearRuntime::kHead + 4u) ==
                  ClearRuntime::kNil,
          "clear removes properties and resets retained tree head");

    ClearRuntime empty;
    empty.words[ClearRuntime::kManager + 12u] = 0u;
    Check(pal4::EffectSystemTextManager::ClearElements(
              ClearRuntime::kManager, empty) == 0u &&
              empty.calls.empty(),
          "zero-count manager performs no property traversal");
}

}  // namespace

int main() {
    TestNewSharedNil();
    TestReusedSharedNil();
    TestConstructionFailure();
    TestDestructEmptyAndShared();
    TestDestructPopulatedAndFallback();
    TestLookup();
    TestClearElements();
}
