#include "pal4/shop_ui_registration.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiRegistrationRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b098cu, 0x9010u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t released = 0u;
    bool throw_insert = false;
    std::uint32_t expected_key = 0x8b0980u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x8b098cu, "framework reads shop name pointer");
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 8u, "registration callback size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void InsertOrReplace(const std::uint32_t tree,
                         const std::uint32_t key,
                         const std::uint32_t callback) override {
        Check(tree == 0x8104u && key == expected_key &&
                  callback == allocation,
              "registration key and manager event tree");
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void ReleaseCallback(const std::uint32_t callback,
                         const std::uint32_t flags) override {
        Check(callback == 0x7000u && flags == 1u,
              "temporary callback deleting release");
        calls.emplace_back("release");
        ++released;
    }
};

void TestFrameworkAndTextSetter() {
    Runtime framework;
    Check(pal4::ShopUiRegistration::GetFrameworkHandler(framework) ==
              0x8e7996u &&
              framework.words.at(0x7000u) == 0x843494u &&
              framework.words.at(0x7004u) == 0x9010u &&
              framework.released == 1u &&
              framework.calls == (std::vector<std::string>{
                  "read", "allocate", "write", "write", "manager",
                  "insert", "release"}),
          "framework thunk registers name callback and releases local owner");

    Runtime text;
    text.expected_key = 0x8b0984u;
    Check(pal4::ShopUiRegistration::GetTextSetterOne(text) ==
              0x8e7995u &&
              text.words.at(0x7000u) == 0x8435dcu &&
              text.words.at(0x7004u) == 0x486890u &&
              text.released == 1u &&
              text.calls == (std::vector<std::string>{
                  "allocate", "write", "write", "manager", "insert",
                  "release"}),
          "text-setter thunk registers function callback without name read");

    Runtime trade_task;
    trade_task.expected_key = 0x8b0988u;
    Check(pal4::ShopUiRegistration::GetTextSetterTwo(trade_task) ==
              0x8e7994u &&
              trade_task.words.at(0x7000u) == 0x8435dcu &&
              trade_task.words.at(0x7004u) == 0x486980u &&
              trade_task.released == 1u &&
              trade_task.calls == (std::vector<std::string>{
                  "allocate", "write", "write", "manager", "insert",
                  "release"}),
          "trade-task thunk registers second function callback");
}

void TestNullAllocationAndExceptionCleanup() {
    for (const int variant : {0, 1, 2}) {
        Runtime runtime;
        runtime.allocation = 0u;
        runtime.expected_key = variant == 0 ? 0x8b0980u :
                               variant == 1 ? 0x8b0984u : 0x8b0988u;
        const auto result = variant == 0 ?
            pal4::ShopUiRegistration::RegisterFramework(runtime) :
            variant == 1 ?
            pal4::ShopUiRegistration::RegisterTextSetterOne(runtime) :
            pal4::ShopUiRegistration::RegisterTextSetterTwo(runtime);
        Check(result == (variant == 0 ? 0x8e7996u :
                         variant == 1 ? 0x8e7995u : 0x8e7994u) &&
                  runtime.released == 0u &&
                  runtime.calls.back() == "insert" &&
                  std::count(runtime.calls.begin(), runtime.calls.end(),
                             "write") == 0,
              "null callback still inserts empty owner without release");

        runtime = Runtime{};
        runtime.expected_key = variant == 0 ? 0x8b0980u :
                               variant == 1 ? 0x8b0984u : 0x8b0988u;
        runtime.throw_insert = true;
        bool caught = false;
        try {
            if (variant == 1) {
                static_cast<void>(
                    pal4::ShopUiRegistration::RegisterTextSetterOne(runtime));
            } else if (variant == 0) {
                static_cast<void>(
                    pal4::ShopUiRegistration::RegisterFramework(runtime));
            } else {
                static_cast<void>(
                    pal4::ShopUiRegistration::RegisterTextSetterTwo(runtime));
            }
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.released == 1u &&
                  runtime.calls.back() == "release",
              "tree insertion failure releases callback owner");
    }
}

}  // namespace

int main() {
    TestFrameworkAndTextSetter();
    TestNullAllocationAndExceptionCleanup();
    return EXIT_SUCCESS;
}
