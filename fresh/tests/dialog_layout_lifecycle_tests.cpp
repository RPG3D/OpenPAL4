#include "pal4/dialog_layout_base.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IDialogLayoutLifecycleRuntime {
    static constexpr std::uint32_t kObject = 0x1000u;
    static constexpr std::uint32_t kRoot = 0x2000u;
    static constexpr std::uint32_t kDesktop = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::uint32_t> freed;

    Runtime() {
        words[kObject + 36u] = kRoot;
        words[kRoot + 68u] = kDesktop;
        words[kObject + 8u] = 0x4001u;
        words[kObject + 12u] = 4u;
        words[kObject + 16u] = 5u;
        words[kObject + 24u] = 0x4101u;
        words[kObject + 28u] = 3u;
        words[kObject + 32u] = 4u;
        bytes[0x4000u] = 0u;
        bytes[0x4100u] = 2u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kObject)
            events.emplace_back("vtable");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("decrement");
    }
    void BuildUtf8Buffer(std::uint32_t string) override {
        Check(string == kRoot + 0x37cu,
              "root name buffer location");
        events.emplace_back("build-utf8");
    }
    std::uint32_t DesktopWindow() override {
        events.emplace_back("desktop");
        return kDesktop;
    }
    bool IsChild(std::uint32_t parent,
                 std::uint32_t child) override {
        Check(child == kRoot, "child root address");
        events.emplace_back("is-child");
        return words.at(child + 68u) == parent;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        Check(child == kRoot &&
                  words.at(child + 68u) == parent,
              "remove current parent");
        words[child + 68u] = 0u;
        events.emplace_back("remove");
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        Check(child == kRoot && parent != 0u,
              "attach root under parent");
        words[child + 68u] = parent;
        events.emplace_back("add");
    }
    void DestroyWindow(std::uint32_t window) override {
        Check(window == kRoot, "destroy root window");
        events.emplace_back("destroy-window");
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        events.emplace_back("free");
    }
};

void CheckParentOperations() {
    Runtime runtime;
    Check(pal4::DialogLayoutBase::IsVisible(
              Runtime::kObject, runtime),
          "attached root is visible");
    pal4::DialogLayoutBase::SetParent(
        Runtime::kObject, true, 0u, runtime);
    Check(runtime.words.at(Runtime::kRoot + 68u) ==
              Runtime::kDesktop &&
              runtime.events ==
                  std::vector<std::string>{
                      "build-utf8", "desktop", "is-child"},
          "already attached root is untouched");

    runtime.events.clear();
    pal4::DialogLayoutBase::SetParent(
        Runtime::kObject, true, 0x3500u, runtime);
    Check(runtime.words.at(Runtime::kRoot + 68u) ==
              0x3500u &&
              runtime.events ==
                  std::vector<std::string>{
                      "build-utf8", "is-child", "remove", "add"},
          "new parent replaces old parent");
    pal4::DialogLayoutBase::SetParent(
        Runtime::kObject, false, 0x3500u, runtime);
    Check(!pal4::DialogLayoutBase::IsVisible(
              Runtime::kObject, runtime) &&
              runtime.words.at(Runtime::kRoot + 68u) == 0u,
          "detach makes root invisible");
    runtime.events.clear();
    pal4::DialogLayoutBase::SetParent(
        Runtime::kObject, false, 0u, runtime);
    Check(runtime.events ==
              std::vector<std::string>{
                  "build-utf8", "desktop", "is-child"},
          "unattached root is not removed twice");
}

void CheckDestruct() {
    Runtime runtime;
    pal4::DialogLayoutBase::Destruct(
        Runtime::kObject, runtime);
    Check(runtime.words.at(Runtime::kObject) ==
              pal4::DialogLayoutBase::kVtable &&
              runtime.words.at(Runtime::kRoot + 68u) == 0u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x4000u} &&
              runtime.bytes.at(0x4100u) == 1u,
          "destructor restores vtable, detaches and releases buffers");
    for (const auto offset : {8u, 12u, 16u, 24u, 28u, 32u})
        Check(runtime.words.at(Runtime::kObject + offset) == 0u,
              "destructor zeroes string record fields");
    Check(runtime.events ==
              std::vector<std::string>{
                  "vtable", "build-utf8", "desktop",
                  "is-child", "remove", "destroy-window",
                  "decrement", "free"},
          "destructor call and release order");
}

void CheckDeletingFlags() {
    for (std::uint8_t flags = 0u; flags < 4u; ++flags) {
        Runtime runtime;
        Check(pal4::DialogLayoutBase::DeletingDestruct(
                  Runtime::kObject, flags, runtime) ==
                  Runtime::kObject,
              "deleting destructor returns original address");
        const auto expected = flags & 1u ? 2u : 1u;
        Check(runtime.freed.size() == expected &&
                  runtime.freed.front() == 0x4000u,
              "delete flag controls object free");
        if (flags & 1u)
            Check(runtime.freed.back() == Runtime::kObject,
                  "object freed after owned buffers");
    }
}

}  // namespace

int main() {
    try {
        CheckParentOperations();
        CheckDestruct();
        CheckDeletingFlags();
        std::cout << "dialog_layout_lifecycle_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
