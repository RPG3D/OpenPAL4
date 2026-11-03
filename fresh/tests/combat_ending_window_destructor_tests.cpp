#include "pal4/combat_ending_window_destructor.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kWindow = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingWindowDestructorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t nil = 0x9000u;
    std::uint32_t references = 2u;

    Runtime() {
        words[kWindow + 48u] = 0x3000u;
        words[0x3000u] = 0x3000u;
        words[kWindow + 460u] = 0x4000u;
        words[0x4000u] = 0x4000u;
        words[kWindow + 444u] = 0x4100u;
        words[0x4100u] = 0x4100u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void DeleteWindow(std::uint32_t object, std::uint32_t flags) override {
        Check(flags == 1u, "deleting-destructor flag");
        calls.emplace_back("delete:" + std::to_string(object));
    }
    void CleanupRecord(std::uint32_t address) override {
        calls.emplace_back("record:" + std::to_string(address));
    }
    void FreeNodes(std::uint32_t tree, std::uint32_t root) override {
        calls.emplace_back("nodes:" + std::to_string(tree) + ":" +
                           std::to_string(root));
    }
    std::uint32_t SharedNil() override { return nil; }
    void FirstElement(std::uint32_t tree) override {
        calls.emplace_back("first:" + std::to_string(tree));
    }
    std::uint32_t Successor(std::uint32_t node) override {
        calls.emplace_back("next:" + std::to_string(node));
        return words[node + 4u];
    }
    void RemoveNode(std::uint32_t tree, std::uint32_t node) override {
        calls.emplace_back("erase:" + std::to_string(tree) + ":" +
                           std::to_string(node));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return 0u;
    }
    std::uint32_t DecrementSharedRef() override {
        calls.emplace_back("decrement");
        return --references;
    }
    std::uint32_t TakeSharedNil() override {
        calls.emplace_back("take-nil");
        const auto old = nil;
        nil = 0u;
        return old;
    }
    std::uint32_t DestructBase(std::uint32_t object) override {
        Check(object == kWindow, "base destructor owner");
        calls.emplace_back("base");
        return 0xbeefu;
    }
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void PopulatedWindow() {
    Runtime runtime;
    runtime.words[kWindow + 684u] = 0x5000u;
    runtime.words[kWindow + 692u] = 0x5200u;
    runtime.words[kWindow + 468u] = 1u;
    runtime.words[0x4004u] = 0x4200u;
    runtime.words[kWindow + 452u] = 1u;
    runtime.words[0x4104u] = 0x4300u;
    runtime.words[kWindow + 52u] = 2u;
    runtime.words[0x3000u] = 0x3100u;
    runtime.words[0x3004u] = 0x3200u;
    runtime.words[0x3100u] = 0x3200u;
    runtime.words[0x3104u] = 0x3000u;
    runtime.words[0x3200u] = 0x3000u;
    runtime.words[0x3204u] = 0x3100u;
    runtime.bytes[0x3108u] = 1u;
    runtime.words[0x310cu] = 0x6000u;
    runtime.bytes[0x3208u] = 0u;
    runtime.words[0x320cu] = 0x6200u;

    Check(pal4::CombatEndingWindowDestructor::Destruct(kWindow, runtime) ==
              0xbeefu,
          "base destructor return forwarded");
    Check(runtime.words[kWindow] == 0x845370u &&
              runtime.words[kWindow + 40u] == 0x845358u,
          "ending window vtables restored");
    Check(Position(runtime, "delete:20480") <
              Position(runtime, "delete:20992") &&
              Position(runtime, "delete:20992") <
                  Position(runtime, "record:4688"),
          "hints precede embedded result records");
    Check(Position(runtime, "record:4688") <
              Position(runtime, "record:4568") &&
              Position(runtime, "record:4568") <
                  Position(runtime, "nodes:4552:16896") &&
              Position(runtime, "nodes:4552:16896") <
                  Position(runtime, "nodes:4536:17152"),
          "records reverse and trees high-to-low");
    Check(Position(runtime, "nodes:4536:17152") <
              Position(runtime, "delete:24576") &&
              Position(runtime, "delete:24576") <
                  Position(runtime, "free:12544") &&
              Position(runtime, "free:12544") <
                  Position(runtime, "free:12800") &&
              runtime.calls.back() == "base",
          "list payload and nodes released before base");
    Check(runtime.words[kWindow + 48u] == 0u &&
              runtime.words[kWindow + 52u] == 0u &&
              runtime.nil == 0u && runtime.references == 0u,
          "list and shared tree state cleared");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "delete:25088") == runtime.calls.end(),
          "unowned list payload left alone");
}

void EmptyWindow() {
    Runtime runtime;
    Check(pal4::CombatEndingWindowDestructor::Destruct(kWindow, runtime) ==
              0xbeefu,
          "empty window result");
    Check(runtime.calls.front() == "write:4096" &&
              runtime.calls[1] == "write:4136" &&
              Position(runtime, "free:12288") <
                  Position(runtime, "base"),
          "empty window still restores vtables and frees list head");
}

void DeletingDestructor() {
    Runtime retained;
    Check(pal4::CombatEndingWindowDestructor::DeletingDestruct(
              kWindow, 2u, retained) == kWindow,
          "retained deletion wrapper returns original object");
    Check(std::find(retained.calls.begin(), retained.calls.end(),
                    "free:4096") == retained.calls.end(),
          "flag without low bit retains object");

    Runtime released;
    Check(pal4::CombatEndingWindowDestructor::DeletingDestruct(
              kWindow, 3u, released) == kWindow,
          "deleting wrapper returns original object");
    Check(released.calls[released.calls.size() - 2] == "base" &&
              released.calls.back() == "free:4096",
          "low flag bit frees object after base destruction");
}

}  // namespace

int main() {
    try {
        PopulatedWindow();
        EmptyWindow();
        DeletingDestructor();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
