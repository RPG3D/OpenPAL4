#include "pal4/actor_manager_data.h"

#include <algorithm>
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

constexpr std::uint32_t object = 0x1000u;

struct Runtime final : pal4::IActorManagerDataRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> trace;
    std::uint32_t next = 0x9000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 28u, "shared nil and tree head are 28 bytes");
        const auto address = next;
        next += 0x100u;
        allocations.push_back(address);
        trace.push_back("allocate:" + std::to_string(address));
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        trace.push_back("free:" + std::to_string(address));
        return address + 7u;
    }
    void InitializeScriptBase(std::uint32_t target) override {
        Check(target == object || target == object + 0x1000u,
              "script base object");
        trace.push_back("script-init");
    }
    void InitializeLocaleFacet(std::uint32_t facet) override {
        trace.push_back("locale-init:" + std::to_string(facet));
    }
    void InitializeComplex(std::uint32_t complex) override {
        trace.push_back("complex-init:" + std::to_string(complex));
    }
    void ConstructNameNode(std::uint32_t node) override {
        trace.push_back("name-init:" + std::to_string(node));
    }
    void DestroyTreeRoot(std::uint32_t tree,
                         std::uint32_t root) override {
        trace.push_back("root:" + std::to_string(tree) + ":" +
                        std::to_string(root));
    }
    void GetFirstTreeElement(std::uint32_t tree) override {
        trace.push_back("first:" + std::to_string(tree));
    }
    std::uint32_t AdvanceTreeCursor(std::uint32_t cursor) override {
        trace.push_back("advance:" + std::to_string(cursor));
        return successors[cursor];
    }
    void EraseTreeNode(std::uint32_t tree,
                       std::uint32_t node) override {
        trace.push_back("erase:" + std::to_string(tree) + ":" +
                        std::to_string(node));
    }
    void HandleEffect(std::uint32_t handle) override {
        trace.push_back("effect:" + std::to_string(handle));
    }
    void ClearTreeRange(std::uint32_t tree, std::uint32_t first,
                        std::uint32_t head) override {
        trace.push_back("clear-range:" + std::to_string(tree) + ":" +
                        std::to_string(first) + ":" +
                        std::to_string(head));
    }
    void ResetNameNode(std::uint32_t node) override {
        trace.push_back("name-reset:" + std::to_string(node));
    }
    void CleanupOtherTree(std::uint32_t tree) override {
        trace.push_back("other-tree:" + std::to_string(tree));
    }
    void DestroyOwnedProperty(std::uint32_t property,
                              std::uint32_t flags) override {
        Check(flags == 1u, "owned property deleting destructor flag");
        trace.push_back("owned:" + std::to_string(property));
    }
    void CleanupComplex(std::uint32_t complex) override {
        trace.push_back("complex-clean:" + std::to_string(complex));
    }
    void CleanupLocaleFacet(std::uint32_t facet) override {
        trace.push_back("locale-clean:" + std::to_string(facet));
    }
    void ResetScriptBase(std::uint32_t target) override {
        trace.push_back("script-clean:" + std::to_string(target));
    }
};

void Before(const Runtime& runtime, const std::string& left,
            const std::string& right, const char* message) {
    const auto first = std::find(runtime.trace.begin(), runtime.trace.end(),
                                 left);
    const auto second = std::find(runtime.trace.begin(), runtime.trace.end(),
                                  right);
    Check(first != runtime.trace.end() && second != runtime.trace.end() &&
          first < second, message);
}

void ConstructionAndSharedNil() {
    Runtime runtime;
    pal4::ActorManagerTreeGlobals globals;
    Check(pal4::ActorManagerDataLifecycle::kMinimumSize == 892u,
          "constructor's final name node ends at byte 892");
    Check(pal4::ActorManagerDataLifecycle::Initialize(
              object, 0xbeefu, 0x123456u, globals, runtime) == object,
          "constructor returns object");
    Check(globals.nil_node == 0x9000u && globals.references == 1u &&
          runtime.words[0x9000u] == 0u &&
          runtime.words[0x9004u] == 0u &&
          runtime.words[0x9008u] == 0u &&
          runtime.words[0x9018u] == 1u,
          "first constructor initializes shared nil node");
    Check(runtime.words[object + 840u] == 0x9100u &&
          runtime.words[0x9100u] == 0x9100u &&
          runtime.words[0x9104u] == 0x9000u &&
          runtime.words[0x9108u] == 0x9100u &&
          runtime.words[0x9118u] == 0u &&
          runtime.words[object + 848u] == 0u,
          "first tree head is self-linked and points at shared nil");
    Check(runtime.words[object + 800u] == 0x123456u &&
          runtime.words[object + 804u] == 0xbeefu &&
          runtime.bytes[object + 796u] == 1u &&
          runtime.bytes[object + 812u] == 0x56u &&
          runtime.bytes[object + 836u] == 0x56u &&
          runtime.bytes[object + 837u] == 0x56u &&
          runtime.bytes[object + 844u] == 0u,
          "constructor stores word and low-byte property fields");
    for (const auto offset : {808u, 816u, 820u, 824u, 828u, 832u})
        Check(runtime.words[object + offset] == 0u,
              "constructor clears value/text fields");
    Check(runtime.words[object] == 0x844a68u &&
          runtime.words[object + 716u] == 0x844cb8u &&
          runtime.words[object + 724u] == 0x844a5cu,
          "constructor commits three vtables after name nodes");
    Before(runtime, "script-init", "locale-init:4812",
           "base initialized before locale facet");
    Before(runtime, "complex-init:4824", "name-init:4948",
           "complex data initialized before name nodes");
    Check(pal4::ActorManagerDataLifecycle::Property(object, runtime) == 0u,
          "property getter observes initialized zero");
    runtime.words[object + 832u] = 31u;
    Check(pal4::ActorManagerDataLifecycle::Property(object, runtime) == 31u,
          "property getter reads word at +832");

    Check(pal4::ActorManagerDataLifecycle::Initialize(
              object + 0x1000u, 9u, 0u, globals, runtime) ==
              object + 0x1000u &&
          globals.nil_node == 0x9000u && globals.references == 2u &&
          runtime.words[object + 0x1000u + 840u] == 0x9300u &&
          runtime.words[0x9304u] == 0x9000u &&
          runtime.bytes[object + 0x1000u + 796u] == 0u &&
          std::find(runtime.freed.begin(), runtime.freed.end(), 0x9200u) !=
              runtime.freed.end(),
          "second constructor reuses nil and frees duplicate candidate");

    runtime.words[object + 848u] = 2u;
    runtime.words[0x9104u] = 0x9400u;
    Check(pal4::ActorManagerDataLifecycle::CleanupTree(
              object + 836u, globals, runtime) == 1u &&
          globals.references == 1u &&
          runtime.words[object + 840u] == 0u &&
          runtime.words[object + 848u] == 0u,
          "nonempty tree releases root/head but retains shared nil");
    Before(runtime, "root:4932:37888", "first:4932",
           "nonempty tree destroys root before first-element refresh");
    runtime.words[0x9300u] = 0x9500u;
    runtime.successors[0x9500u] = 0x9300u;
    Check(pal4::ActorManagerDataLifecycle::CleanupTree(
              object + 0x1000u + 836u, globals, runtime) == 0x9007u &&
          globals.references == 0u && globals.nil_node == 0u &&
          runtime.freed.back() == 0x9000u,
          "last tree frees shared nil and forwards free result");
    Before(runtime, "advance:38144", "erase:9028:38144",
           "zero-count orphan node advances before erase");
}

void DestructionAndDeletingThunk() {
    Runtime runtime;
    const auto head = 0x9000u;
    const auto node = 0x9100u;
    runtime.words[object + 840u] = head;
    runtime.words[head] = node;
    runtime.successors[node] = head;
    runtime.words[object + 816u] = 0xa000u;
    runtime.bytes[0x9fffu] = 2u;
    runtime.bytes[object + 796u] = 1u;
    runtime.words[object + 800u] = 0xb000u;
    runtime.words[object + 696u] = 0xc000u;
    runtime.words[object + 680u] = 0xd000u;
    pal4::ActorManagerDataLifecycle::Destroy(object, runtime);
    Check(runtime.bytes[0x9fffu] == 1u &&
          runtime.words[object + 816u] == 0u &&
          runtime.words[object + 820u] == 0u &&
          runtime.words[object + 824u] == 0u &&
          runtime.words[object + 696u] == 0u &&
          runtime.words[object + 680u] == 0u,
          "destructor decrements referenced text and clears owned ranges");
    Before(runtime, "effect:37136", "clear-range:4932:37120:36864",
           "tree node effects released before range clear");
    Before(runtime, "effect:4948", "name-reset:4980",
           "name effects handled before reverse reset");
    Before(runtime, "name-reset:4948", "other-tree:4932",
           "all name nodes reset before tree cleanup");
    Before(runtime, "owned:45056", "complex-clean:4824",
           "owned property destroyed before complex field");
    Before(runtime, "complex-clean:4824", "locale-clean:4812",
           "complex field cleaned before locale facet");
    Before(runtime, "free:49152", "free:53248",
           "script base buffers freed in reverse order");
    Before(runtime, "free:53248", "script-clean:4096",
           "script base final reset follows its buffers");
    Check(runtime.words[object] == 0x8430e8u &&
          runtime.words[object + 724u] == 0x844cccu,
          "destructor restores base vtables");

    Runtime deleting;
    deleting.words[object + 840u] = head;
    deleting.words[head] = head;
    Check(pal4::ActorManagerDataLifecycle::Delete(object, 0u,
                                                   deleting) == object &&
          std::find(deleting.freed.begin(), deleting.freed.end(), object) ==
              deleting.freed.end(),
          "nondeleting thunk returns same object without freeing it");
    Runtime owning;
    owning.words[object + 840u] = head;
    owning.words[head] = head;
    Check(pal4::ActorManagerDataLifecycle::Delete(object, 3u,
                                                   owning) == object &&
          owning.freed.back() == object,
          "deleting thunk observes flag bit zero after destructor");

    Runtime unshared;
    unshared.words[object + 840u] = head;
    unshared.words[head] = head;
    unshared.words[object + 816u] = 0xa000u;
    unshared.bytes[0x9fffu] = 0xffu;
    pal4::ActorManagerDataLifecycle::Destroy(object, unshared);
    Check(std::find(unshared.freed.begin(), unshared.freed.end(),
                    0x9fffu) != unshared.freed.end(),
          "sentinel text reference byte frees its allocation");
}

}  // namespace

int main() {
    try {
        ConstructionAndSharedNil();
        DestructionAndDeletingThunk();
        std::cout << "actor_manager_data_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
