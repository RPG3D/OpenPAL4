#include "pal4/script_event_router.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kRouter = 0x4000u;
constexpr std::uint32_t kNil = 0x5000u;
constexpr std::uint32_t kHead = 0x6000u;
using Pair = std::pair<std::uint32_t, std::uint32_t>;

struct Runtime final : pal4::IScriptEventRouterRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {pal4::ScriptEventRouter::kSingletonPointer, 0u},
        {pal4::ScriptEventRouter::kNilPointer, 0u},
        {pal4::ScriptEventRouter::kInstanceCount, 0u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {pal4::ScriptEventRouter::kSingletonGuard, 0u}};
    std::deque<std::uint32_t> objects{kRouter};
    std::deque<std::uint32_t> nodes{kNil, kHead};
    std::vector<std::string> calls;
    std::vector<Pair> writes;
    std::vector<Pair> invoked;
    std::uint32_t found_node = 0u;
    std::uint32_t registered_instance = 0x9000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write_byte:" + std::to_string(address));
    }
    std::uint32_t AllocateObject(std::uint32_t bytes) override {
        Check(bytes == 20u, "event router allocates 20-byte object");
        calls.emplace_back("alloc_object");
        if (objects.empty()) return 0u;
        const auto address = objects.front();
        objects.pop_front();
        return address;
    }
    std::uint32_t AllocateNode(std::uint32_t bytes) override {
        Check(bytes == 32u, "event router allocates 32-byte tree nodes");
        calls.emplace_back("alloc_node");
        if (nodes.empty()) return 0u;
        const auto address = nodes.front();
        nodes.pop_front();
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return address + 1u;
    }
    void GetSingletonManager() override {
        calls.emplace_back("manager");
    }
    std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t identifier,
        std::uint32_t create_entry,
        std::uint32_t destroy_entry) override {
        Check(name == "cbEventRouter" && identifier == 1027u &&
                  create_entry == 0x563000u &&
                  destroy_entry == 0x5630f0u,
              "singleton identity and callback entrypoints match IDA");
        calls.emplace_back("register");
        return registered_instance;
    }
    void ResetHeadCursor(std::uint32_t router) override {
        Check(router == kRouter, "head reset uses owning router");
        calls.emplace_back("reset_cursor");
    }
    void ReleaseOwnedCallback(std::uint32_t slot) override {
        calls.emplace_back("release:" + std::to_string(slot));
    }
    std::uint32_t FindNode(std::uint32_t router,
                           std::uint32_t key) override {
        Check(router == kRouter && key == 77u,
              "event lookup receives router and key");
        calls.emplace_back("find");
        return found_node;
    }
    void InvokeCallback(std::uint32_t handler,
                        std::uint32_t payload) override {
        invoked.emplace_back(handler, payload);
        calls.emplace_back("invoke:" + std::to_string(handler));
    }
};

void SingletonGuard() {
    Runtime runtime;
    runtime.bytes[pal4::ScriptEventRouter::kSingletonGuard] = 0x80u;
    Check(pal4::ScriptEventRouter::GetInstance(runtime) == 0x9000u &&
          runtime.bytes[pal4::ScriptEventRouter::kSingletonGuard] == 0x81u &&
          runtime.words[pal4::ScriptEventRouter::kSingletonPointer] == 0x9000u &&
          runtime.calls[1] == "write_byte:9412684" &&
          runtime.calls[2] == "manager" &&
          runtime.calls[3] == "register",
          "singleton registers once after setting guard and preserving high bits");
    runtime.calls.clear();
    Check(pal4::ScriptEventRouter::GetInstance(runtime) == 0x9000u &&
          runtime.calls.size() == 2u &&
          runtime.calls[0] == "read_byte:9412684",
          "repeated singleton lookup reads registered pointer only");
}

void CreateAndDestroy() {
    Runtime runtime;
    Check(pal4::ScriptEventRouter::Create(runtime) == kRouter &&
          runtime.words[pal4::ScriptEventRouter::kNilPointer] == kNil &&
          runtime.words[pal4::ScriptEventRouter::kInstanceCount] == 1u &&
          runtime.words[kRouter + 4u] == kHead &&
          runtime.words[kRouter + 12u] == 0u &&
          runtime.words[kNil] == 0u &&
          runtime.words[kNil + 4u] == 0u &&
          runtime.words[kNil + 8u] == 0u &&
          runtime.words[kNil + 28u] == 1u &&
          runtime.words[kHead] == kHead &&
          runtime.words[kHead + 4u] == kNil &&
          runtime.words[kHead + 8u] == kHead &&
          runtime.words[kHead + 28u] == 0u &&
          runtime.bytes[kRouter] == 0u &&
          runtime.bytes[kRouter + 1u] == 0u &&
          runtime.bytes[kRouter + 8u] == 0u &&
          runtime.bytes[kRouter + 16u] == 0u,
          "first router initializes nil, head, count and observable fields");
    runtime.calls.clear();
    Check(pal4::ScriptEventRouter::Destroy(kRouter, runtime) ==
              kRouter + 1u &&
          runtime.words[pal4::ScriptEventRouter::kNilPointer] == 0u &&
          runtime.words[pal4::ScriptEventRouter::kInstanceCount] == 0u &&
          runtime.words[kRouter + 4u] == 0u &&
          runtime.calls[runtime.calls.size() - 2u] ==
              "free:" + std::to_string(kNil),
          "last router frees head, shared nil and object in order");
    runtime.calls.clear();
    Check(pal4::ScriptEventRouter::Destroy(0u, runtime) == 0u &&
          runtime.calls.empty(),
          "null destroy has deterministic no-op return");
}

void SharedNilAndAllocationFailures() {
    Runtime runtime;
    runtime.objects.push_back(kRouter + 0x100u);
    runtime.nodes.push_back(kNil + 0x100u);
    runtime.nodes.push_back(kHead + 0x100u);
    Check(pal4::ScriptEventRouter::Create(runtime) == kRouter,
          "first router constructs");
    Check(pal4::ScriptEventRouter::Create(runtime) == kRouter + 0x100u &&
          runtime.words[pal4::ScriptEventRouter::kInstanceCount] == 2u &&
          runtime.words[pal4::ScriptEventRouter::kNilPointer] == kNil &&
          runtime.words[kHead + 0x100u + 4u] == kNil,
          "second router reuses first shared nil");
    Check(pal4::ScriptEventRouter::Destroy(kRouter + 0x100u,
                                           runtime) ==
              kRouter + 0x101u &&
          runtime.words[pal4::ScriptEventRouter::kNilPointer] == kNil &&
          runtime.words[pal4::ScriptEventRouter::kInstanceCount] == 1u,
          "destroying one router retains shared nil");

    runtime = Runtime{};
    runtime.objects.clear();
    Check(pal4::ScriptEventRouter::Create(runtime) == 0u &&
          runtime.words[pal4::ScriptEventRouter::kInstanceCount] == 0u,
          "object allocation failure does not touch globals");
    runtime = Runtime{};
    runtime.nodes.clear();
    Check(pal4::ScriptEventRouter::Create(runtime) == 0u &&
          runtime.words[pal4::ScriptEventRouter::kInstanceCount] == 0u &&
          runtime.calls.back() == "free:" + std::to_string(kRouter),
          "nil allocation failure releases object");
    runtime = Runtime{};
    runtime.nodes.pop_back();
    Check(pal4::ScriptEventRouter::Create(runtime) == 0u &&
          runtime.words[pal4::ScriptEventRouter::kInstanceCount] == 0u &&
          runtime.words[pal4::ScriptEventRouter::kNilPointer] == 0u &&
          runtime.calls.back() == "free:" + std::to_string(kRouter),
          "head allocation failure unwinds nil and object");
}

void DestructorBranches() {
    Runtime runtime;
    (void)pal4::ScriptEventRouter::Create(runtime);
    runtime.words[kRouter + 12u] = 2u;
    runtime.words[kHead + 4u] = 0xa000u;
    runtime.words[kHead] = 0xa000u;
    runtime.words[kHead + 8u] = 0xa000u;
    runtime.words[0xa000u] = kNil;
    runtime.words[0xa000u + 4u] = kHead;
    runtime.words[0xa000u + 8u] = kNil;
    runtime.words[0xa000u + 20u] = 0xb000u;
    runtime.words[0xa000u + 24u] = 0u;
    runtime.words[0xa000u + 28u] = 1u;
    runtime.words[0xb000u] = 0xb000u;
    runtime.calls.clear();
    Check(pal4::ScriptEventRouter::Destruct(kRouter, runtime) ==
              kNil + 1u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "free:45056") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "free:40960") != runtime.calls.end() &&
          runtime.words[kHead] == kHead &&
          runtime.words[kHead + 4u] == kNil &&
          runtime.words[kHead + 8u] == kHead &&
          runtime.words[kRouter + 12u] == 0u &&
          runtime.calls.end() !=
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "reset_cursor"),
          "nonempty tree recursively erases then resets head and cursor");

    runtime = Runtime{};
    (void)pal4::ScriptEventRouter::Create(runtime);
    runtime.words[kHead] = 0xa000u;
    runtime.words[kHead + 4u] = 0xa000u;
    runtime.words[kHead + 8u] = 0xa000u;
    runtime.words[0xa000u] = kNil;
    runtime.words[0xa000u + 4u] = kHead;
    runtime.words[0xa000u + 8u] = kNil;
    runtime.words[0xa000u + 20u] = 0xb000u;
    runtime.words[0xa000u + 24u] = 0u;
    runtime.words[0xa000u + 28u] = 0u;
    runtime.words[0xb000u] = 0xb000u;
    runtime.calls.clear();
    Check(pal4::ScriptEventRouter::Destruct(kRouter, runtime) ==
              kNil + 1u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "free:40960") != runtime.calls.end() &&
          runtime.words[kHead] == kHead &&
          runtime.words[kHead + 4u] == kNil,
          "zero-count tree erases stray node using the reconstructed tree");
}

void EventDispatch() {
    Runtime runtime;
    runtime.words[kRouter + 4u] = kHead;
    runtime.bytes[kRouter + 16u] = 1u;
    Check(pal4::ScriptEventRouter::ProcessEvent(
              kRouter, 77u, 0x1234u, 99u, runtime) == 0u &&
          runtime.calls.size() == 1u,
          "suppressed router returns before tree lookup");

    runtime.calls.clear();
    runtime.bytes[kRouter + 16u] = 0u;
    runtime.found_node = kHead;
    Check(pal4::ScriptEventRouter::ProcessEvent(
              kRouter, 77u, 0x1234u, 99u, runtime) == 0u &&
          runtime.invoked.empty(),
          "missing event key does not call handlers");

    runtime.calls.clear();
    runtime.found_node = 0xa000u;
    runtime.words[0xa000u + 20u] = 0xb000u;
    runtime.words[0xa000u + 24u] = 3u;
    runtime.words[0xb000u] = 0xb100u;
    runtime.words[0xb100u] = 0xb200u;
    runtime.words[0xb100u + 12u] = 0xc100u;
    runtime.words[0xb200u] = 0xb000u;
    runtime.words[0xb200u + 12u] = 0xc200u;
    Check(pal4::ScriptEventRouter::ProcessEvent(
              kRouter, 77u, 0x1234u, 99u, runtime) == 3u &&
          runtime.invoked == std::vector<Pair>{
              {0xc100u, 0x1234u}, {0xc200u, 0x1234u}},
          "event router invokes linked callbacks in order and returns count");
}

void SetTreeNode(Runtime& runtime, const std::uint32_t node,
                 const std::uint32_t parent,
                 const std::uint32_t list_head,
                 const std::uint32_t color) {
    runtime.words[node] = kNil;
    runtime.words[node + 4u] = parent;
    runtime.words[node + 8u] = kNil;
    runtime.words[node + 20u] = list_head;
    runtime.words[node + 24u] = 0u;
    runtime.words[node + 28u] = color;
    runtime.words[list_head] = list_head;
    runtime.words[list_head + 4u] = list_head;
}

void TreeIteratorAndListRemoval() {
    Runtime runtime;
    (void)pal4::ScriptEventRouter::Create(runtime);
    constexpr std::uint32_t root = 0xa000u;
    constexpr std::uint32_t left = 0xa100u;
    constexpr std::uint32_t right = 0xa200u;
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 0u);
    SetTreeNode(runtime, right, root, 0xc200u, 0u);
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead] = left;
    runtime.words[kHead + 8u] = right;
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[0xd000u] = left;
    Check(pal4::ScriptEventRouterTree::GetEvent(0xb100u) == 0xb100u &&
          pal4::ScriptEventRouterTree::NextEvent(
              0xd000u, runtime) == root &&
          runtime.words[0xd000u] == root &&
          pal4::ScriptEventRouterTree::NextEvent(
              0xd000u, runtime) == kNil &&
          runtime.words[0xd000u] == right &&
          pal4::ScriptEventRouterTree::NextNode(root, runtime) == right &&
          pal4::ScriptEventRouterTree::NextNode(right, runtime) == kHead,
          "event iterator advances left, root, right, then head");

    runtime.words[0xc000u] = 0xb100u;
    runtime.words[0xc000u + 4u] = 0xb100u;
    runtime.words[0xb100u] = 0xc000u;
    runtime.words[0xb100u + 4u] = 0xc000u;
    runtime.words[root + 24u] = 1u;
    runtime.calls.clear();
    Check(pal4::ScriptEventRouterTree::RemoveEvent(
              root + 16u, 0xd100u, 0xb100u, runtime) == 0xd100u &&
          runtime.words[0xd100u] == 0xc000u &&
          runtime.words[0xc000u] == 0xc000u &&
          runtime.words[0xc000u + 4u] == 0xc000u &&
          runtime.words[root + 24u] == 0u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "release:45320") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "free:45312") != runtime.calls.end(),
          "callback removal unlinks, releases owned slot and writes successor");
}

void TreeEraseShapes() {
    Runtime runtime;
    (void)pal4::ScriptEventRouter::Create(runtime);
    constexpr std::uint32_t root = 0xa000u;
    SetTreeNode(runtime, root, kHead, 0xc000u, 0u);
    runtime.words[kHead] = root;
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead + 8u] = root;
    runtime.words[kRouter + 12u] = 1u;
    Check(pal4::ScriptEventRouterTree::EraseToOutput(
              kRouter, 0xd000u, root, runtime) == 0xd000u &&
          runtime.words[0xd000u] == kHead &&
          runtime.words[kHead] == kHead &&
          runtime.words[kHead + 4u] == kNil &&
          runtime.words[kHead + 8u] == kHead &&
          runtime.words[kRouter + 12u] == 0u &&
          runtime.words[root + 20u] == 0u,
          "red root removal resets tree head and returns end iterator");

    runtime = Runtime{};
    (void)pal4::ScriptEventRouter::Create(runtime);
    constexpr std::uint32_t left = 0xa100u;
    constexpr std::uint32_t right = 0xa200u;
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 0u);
    SetTreeNode(runtime, right, root, 0xc200u, 0u);
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[kHead] = left;
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead + 8u] = right;
    runtime.words[kRouter + 12u] = 3u;
    Check(pal4::ScriptEventRouterTree::EraseToOutput(
              kRouter, 0xd000u, root, runtime) == 0xd000u &&
          runtime.words[0xd000u] == right &&
          runtime.words[kHead + 4u] == right &&
          runtime.words[right] == left &&
          runtime.words[right + 4u] == kHead &&
          runtime.words[left + 4u] == right &&
          runtime.words[right + 28u] == 1u &&
          runtime.words[kRouter + 12u] == 2u,
          "two-child erase transplants in-order successor and exchanges colors");

    runtime = Runtime{};
    (void)pal4::ScriptEventRouter::Create(runtime);
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 1u);
    SetTreeNode(runtime, right, root, 0xc200u, 1u);
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[kHead] = left;
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead + 8u] = right;
    runtime.words[kRouter + 12u] = 3u;
    Check(pal4::ScriptEventRouterTree::EraseNode(
              kRouter, left, runtime) == root &&
          runtime.words[root] == kNil &&
          runtime.words[right + 28u] == 0u &&
          runtime.words[root + 28u] == 1u &&
          runtime.words[kRouter + 12u] == 2u,
          "black leaf removal recolors sibling and preserves black root");
}

void RecursiveTreeCleanup() {
    Runtime runtime;
    (void)pal4::ScriptEventRouter::Create(runtime);
    constexpr std::uint32_t root = 0xa000u;
    constexpr std::uint32_t left = 0xa100u;
    constexpr std::uint32_t right = 0xa200u;
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 0u);
    SetTreeNode(runtime, right, root, 0xc200u, 0u);
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.calls.clear();
    Check(pal4::ScriptEventRouterTree::EraseRecursive(
              root, runtime) == kNil,
          "recursive cleanup returns the shared nil node");
    std::vector<std::string> frees;
    for (const auto& call : runtime.calls)
        if (call.rfind("free:", 0u) == 0u) frees.push_back(call);
    Check(frees == std::vector<std::string>{
              "free:49664", "free:41472", "free:49152",
              "free:40960", "free:49408", "free:41216"},
          "recursive cleanup releases right subtree, root, then left subtree");
}

void TreeEraseRotationsAndDeepSuccessor() {
    constexpr std::uint32_t root = 0xa000u;
    constexpr std::uint32_t left = 0xa100u;
    constexpr std::uint32_t right = 0xa200u;
    constexpr std::uint32_t near = 0xa300u;
    constexpr std::uint32_t far = 0xa400u;

    Runtime runtime;
    (void)pal4::ScriptEventRouter::Create(runtime);
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 1u);
    SetTreeNode(runtime, right, root, 0xc200u, 0u);
    SetTreeNode(runtime, near, right, 0xc300u, 1u);
    SetTreeNode(runtime, far, right, 0xc400u, 1u);
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[right] = near;
    runtime.words[right + 8u] = far;
    runtime.words[kHead] = left;
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead + 8u] = far;
    runtime.words[kRouter + 12u] = 5u;
    Check(pal4::ScriptEventRouterTree::EraseNode(
              kRouter, left, runtime) == root &&
          runtime.words[kHead + 4u] == right &&
          runtime.words[right] == root &&
          runtime.words[root + 4u] == right &&
          runtime.words[root + 8u] == near &&
          runtime.words[right + 28u] == 1u &&
          runtime.words[kRouter + 12u] == 4u,
          "red sibling repair rotates left and maintains tree links");

    runtime = Runtime{};
    (void)pal4::ScriptEventRouter::Create(runtime);
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 1u);
    SetTreeNode(runtime, right, root, 0xc200u, 1u);
    SetTreeNode(runtime, near, right, 0xc300u, 0u);
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[right] = near;
    runtime.words[kHead] = left;
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead + 8u] = right;
    runtime.words[kRouter + 12u] = 4u;
    Check(pal4::ScriptEventRouterTree::EraseNode(
              kRouter, left, runtime) == root &&
          runtime.words[kHead + 4u] == near &&
          runtime.words[near] == root &&
          runtime.words[near + 8u] == right &&
          runtime.words[root + 4u] == near &&
          runtime.words[right + 4u] == near &&
          runtime.words[near + 28u] == 1u &&
          runtime.words[kRouter + 12u] == 3u,
          "near red child triggers right then left rotation");

    runtime = Runtime{};
    (void)pal4::ScriptEventRouter::Create(runtime);
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 1u);
    SetTreeNode(runtime, right, root, 0xc200u, 1u);
    SetTreeNode(runtime, near, left, 0xc300u, 0u);
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[left + 8u] = near;
    runtime.words[kHead] = left;
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead + 8u] = right;
    runtime.words[kRouter + 12u] = 4u;
    Check(pal4::ScriptEventRouterTree::EraseNode(
              kRouter, right, runtime) == kHead &&
          runtime.words[kHead + 4u] == near &&
          runtime.words[near] == left &&
          runtime.words[near + 8u] == root &&
          runtime.words[left + 4u] == near &&
          runtime.words[root + 4u] == near &&
          runtime.words[near + 28u] == 1u &&
          runtime.words[kRouter + 12u] == 3u,
          "mirror near red child triggers left then right rotation");

    runtime = Runtime{};
    (void)pal4::ScriptEventRouter::Create(runtime);
    SetTreeNode(runtime, root, kHead, 0xc000u, 1u);
    SetTreeNode(runtime, left, root, 0xc100u, 1u);
    SetTreeNode(runtime, right, root, 0xc200u, 1u);
    SetTreeNode(runtime, near, right, 0xc300u, 0u);
    SetTreeNode(runtime, far, right, 0xc400u, 0u);
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[right] = near;
    runtime.words[right + 8u] = far;
    runtime.words[kHead] = left;
    runtime.words[kHead + 4u] = root;
    runtime.words[kHead + 8u] = far;
    runtime.words[kRouter + 12u] = 5u;
    Check(pal4::ScriptEventRouterTree::EraseNode(
              kRouter, root, runtime) == near &&
          runtime.words[kHead + 4u] == near &&
          runtime.words[near] == left &&
          runtime.words[near + 8u] == right &&
          runtime.words[right] == kNil &&
          runtime.words[left + 4u] == near &&
          runtime.words[right + 4u] == near &&
          runtime.words[kRouter + 12u] == 4u,
          "deep successor transplants from right subtree and repairs its parent");
}

}  // namespace

int main() {
    try {
        SingletonGuard();
        CreateAndDestroy();
        SharedNilAndAllocationFailures();
        DestructorBranches();
        EventDispatch();
        TreeIteratorAndListRemoval();
        TreeEraseShapes();
        RecursiveTreeCleanup();
        TreeEraseRotationsAndDeepSuccessor();
        std::cout << "script_event_router_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
