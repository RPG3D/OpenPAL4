#include "pal4/ui_red_black_tree_storage.h"

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

struct Runtime final : pal4::IUiCeguiRedBlackTreeStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t nil = 0u;
    std::uint32_t users = 0u;
    std::uint32_t next = 0x5000u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        calls.push_back("alloc " + std::to_string(size));
        const auto result = next;
        next += 0x100u;
        return result;
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void CleanupData(std::uint32_t address) override {
        calls.push_back("cleanup " + std::to_string(address));
    }
    void FreeCeguiString(std::uint32_t address) override {
        calls.push_back("cegui " + std::to_string(address));
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override { nil = address; }
    std::uint32_t SharedNilUsers() override { return users; }
    void SetSharedNilUsers(std::uint32_t value) override {
        users = value;
        calls.push_back("users " + std::to_string(value));
    }
};

void CheckConstruction() {
    Runtime r;
    Check(pal4::UiRedBlackTreeStorage::Construct(0x1000u, r) == 0x5100u,
          "first head returned");
    Check(r.nil == 0x5000u && r.users == 1u,
          "first tree owns new shared nil");
    Check(r.words.at(r.nil) == 0u &&
              r.words.at(r.nil + 4u) == 0u &&
              r.words.at(r.nil + 8u) == 0u &&
              r.words.at(r.nil + 32u) == 1u,
          "shared nil fields");
    Check(r.words.at(0x1004u) == 0x5100u &&
              r.words.at(0x100cu) == 0u &&
              r.words.at(0x5100u) == 0x5100u &&
              r.words.at(0x5104u) == r.nil &&
              r.words.at(0x5108u) == 0x5100u &&
              r.words.at(0x5120u) == 0u,
          "first head fields");
    Check(pal4::UiRedBlackTreeStorage::Construct(0x1100u, r) == 0x5300u,
          "second head returned");
    Check(r.nil == 0x5000u && r.users == 2u &&
              r.words.at(0x5304u) == r.nil,
          "second tree reuses shared nil");
    Check(r.calls == (std::vector<std::string>{
              "alloc 36", "users 1", "alloc 36", "alloc 36",
              "users 2", "free 20992", "alloc 36"}),
          "allocation, reference and unused-candidate order");
}

void CheckFreeOrder() {
    Runtime r;
    r.nil = 0x3000u;
    Check(pal4::UiRedBlackTreeStorage::FreeNodes(r.nil, r) == r.nil &&
              r.calls.empty(), "nil is a no-op");
    // Root, left spine, and right subtree: right is released first at each level.
    r.words[0x5000u] = 0x5100u;
    r.words[0x5008u] = 0x5200u;
    r.words[0x5100u] = r.nil;
    r.words[0x5108u] = r.nil;
    r.words[0x5200u] = r.nil;
    r.words[0x5208u] = r.nil;
    Check(pal4::UiRedBlackTreeStorage::FreeNodes(0x5000u, r) == r.nil,
          "returns shared nil");
    Check(r.calls == (std::vector<std::string>{
              "cleanup 21004", "free 20992", "cleanup 20492",
              "free 20480", "cleanup 20748", "free 20736"}),
          "right subtree, root, then left node release order");
}

void CheckCeguiFreeOrder() {
    Runtime r;
    r.nil = 0x3000u;
    Check(pal4::UiRedBlackTreeStorage::FreeCeguiNodes(r.nil, r) ==
              r.nil && r.calls.empty(),
          "CEGUI nil input is inert");
    r.words[0x5000u] = 0x5100u;
    r.words[0x5008u] = 0x5200u;
    r.words[0x5100u] = r.nil;
    r.words[0x5108u] = r.nil;
    r.words[0x5200u] = r.nil;
    r.words[0x5208u] = r.nil;
    Check(pal4::UiRedBlackTreeStorage::FreeCeguiNodes(0x5000u, r) ==
              r.nil,
          "CEGUI free returns its shared nil");
    Check(r.calls == (std::vector<std::string>{
              "cegui 21004", "free 20992", "cegui 20492",
              "free 20480", "cegui 20748", "free 20736"}),
          "CEGUI cleanup precedes each node free, right then left");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckFreeOrder();
        CheckCeguiFreeOrder();
        std::cout << "ui_red_black_tree_storage_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
