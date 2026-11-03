#include "pal4/dialog_object.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
std::uint8_t Byte(const pal4::DialogObjectState& state, std::size_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}
struct Runtime final : pal4::IDialogObjectRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> sizes;
    std::map<std::uint32_t, std::map<std::uint32_t, std::uint32_t>> heap;
    std::uint32_t next = 0x1000u;
    std::uint32_t next_first_node = 0u;
    std::uint32_t next_second_node = 0u;
    bool fail_object = false;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate:" + std::to_string(bytes));
        if (fail_object && bytes == 0xdcu) return 0u;
        const auto address = next;
        next += 0x100u;
        sizes[address] = bytes;
        return address;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        if (address != 0u && sizes.contains(address))
            sizes.erase(address);
    }
    void WriteHeapWord(std::uint32_t address, std::uint32_t offset,
                       std::uint32_t value) override {
        Check(address != 0u && sizes.contains(address) &&
              offset < sizes.at(address), "write allocated tree node");
        heap[address][offset] = value;
    }
    std::uint32_t ReadHeapWord(std::uint32_t address,
                               std::uint32_t offset) override {
        Check(sizes.contains(address) && offset < sizes.at(address),
              "read allocated tree node");
        return heap[address][offset];
    }
    std::uint32_t InitializeTextManager(std::uint32_t address) override {
        Check(sizes.at(address) == 16u, "text manager allocation size");
        calls.emplace_back("init-text"); return address;
    }
    void DestroyTextManager(std::uint32_t address) override {
        Check(sizes.contains(address), "live text manager");
        calls.emplace_back("destroy-text");
    }
    void InitializeXmlDocument(pal4::DialogObjectState& state) override {
        calls.emplace_back("init-xml"); state.words[5u] = 0xaaaa5555u;
    }
    void DestroyXmlDocument(pal4::DialogObjectState& state) override {
        Check(state.words[0u] == pal4::DialogObject::kVtable,
              "derived vtable before XML cleanup");
        calls.emplace_back("destroy-xml");
    }
    std::uint8_t ConstructorStackByte() override { return 0x5au; }
    void DestroyFirstTreeBranch(std::uint32_t root) override {
        Check(root == 0xd000u, "first tree root");
        calls.emplace_back("destroy-first-branch");
    }
    void DestroySecondTreeBranch(std::uint32_t root) override {
        Check(root == 0xe000u, "second tree root");
        calls.emplace_back("destroy-second-branch");
    }
    std::uint32_t NextFirstTreeNode(std::uint32_t node) override {
        Check(node == 0xc000u, "first tree iterator");
        calls.emplace_back("next-first"); return next_first_node;
    }
    std::uint32_t NextSecondTreeNode(std::uint32_t node) override {
        Check(node == 0xc100u, "second tree iterator");
        calls.emplace_back("next-second"); return next_second_node;
    }
    void EraseFirstTreeNode(std::uint32_t node) override {
        Check(node == 0xc000u, "first tree erased node");
        calls.emplace_back("erase-first");
    }
    void EraseSecondTreeNode(std::uint32_t node) override {
        Check(node == 0xc100u, "second tree erased node");
        calls.emplace_back("erase-second");
    }
    void DestroyStringRecord(std::uint32_t address) override {
        calls.emplace_back("string:" + std::to_string(address));
    }
    void CleanupBeforeDelete(std::uint32_t object, std::uint32_t first,
                             std::uint32_t second) override {
        Check(object == 0x9000u && first == 0xa000u && second == 0xb000u,
              "pre-delete native arguments");
        calls.emplace_back("pre-delete");
    }
    std::uint32_t VirtualDelete(std::uint32_t object,
                                std::uint8_t flags) override {
        Check(object == 0x9000u && flags == 1u,
              "virtual deleting destructor arguments");
        calls.emplace_back("virtual-delete"); return 0x1234u;
    }
    int RefreshTransform(pal4::DialogObjectState&) override { return 1; }
    std::uint32_t FrameParent(std::uint32_t) override { return 0u; }
    int RemoveFrameChild(std::uint32_t) override { return 1; }
    int AddFrameChild(std::uint32_t, std::uint32_t) override { return 1; }
};
}  // namespace

int main() {
    Runtime runtime;
    pal4::DialogObjectState first{};
    first.words.fill(0xdeadbeefu);
    const auto object = pal4::DialogObject::CreateInstance(first, runtime);
    Check(object == 0x1000u && sizeof(first) == 220u &&
          first.words[0u] == 0x84b194u && Byte(first, 4u) == 0u &&
          first.words[2u] == 0u && first.words[3u] == 0u &&
          first.words[4u] == 0x1100u && first.words[5u] == 0xaaaa5555u &&
          first.words[20u] == 0u &&
          std::bit_cast<float>(first.words[21u]) == 1.0f &&
          Byte(first, 92u) == 0x5au && Byte(first, 108u) == 0x5au &&
          Byte(first, 124u) == 0x5au && Byte(first, 140u) == 0x5au &&
          Byte(first, 141u) == 0x5au && Byte(first, 156u) == 0x5au &&
          Byte(first, 157u) == 0x5au &&
          first.words[24u] == 0u && first.words[28u] == 0u &&
          first.words[32u] == 0u && first.words[38u] == 0u &&
          first.words[42u] == 0u && first.words[43u] == 0u &&
          first.words[44u] == 0u && first.words[50u] == 0u &&
          std::bit_cast<float>(first.words[47u]) == 1.0f &&
          std::bit_cast<float>(first.words[48u]) == 1.0f &&
          std::bit_cast<float>(first.words[49u]) == 1.0f &&
          Byte(first, 212u) == 0u && first.words[54u] == 0u,
          "factory and 220-byte constructor fields");
    const auto first_head = first.words[36u];
    const auto second_head = first.words[40u];
    const auto first_nil = runtime.heap[first_head][4u];
    const auto second_nil = runtime.heap[second_head][4u];
    Check(runtime.sizes.at(first_head) == 36u &&
          runtime.sizes.at(second_head) == 44u &&
          runtime.heap[first_head][0u] == first_head &&
          runtime.heap[first_head][8u] == first_head &&
          runtime.heap[first_head][32u] == 0u &&
          runtime.heap[second_head][0u] == second_head &&
          runtime.heap[second_head][8u] == second_head &&
          runtime.heap[second_head][40u] == 0u &&
          runtime.heap[first_nil][32u] == 1u &&
          runtime.heap[second_nil][40u] == 1u,
          "native shared sentinels and per-object empty heads");

    pal4::DialogObjectState second{};
    second.words.fill(0u);
    pal4::DialogObject::Construct(second, runtime);
    Check(runtime.heap[second.words[36u]][4u] == first_nil &&
          runtime.heap[second.words[40u]][4u] == second_nil,
          "second object reuses shared tree sentinels");
    runtime.calls.clear();
    second.words[24u] = 0xa000u;
    second.words[28u] = 0x9000u;
    second.words[29u] = 0x9028u;
    second.words[32u] = 0xb000u;
    second.words[38u] = 1u;
    second.words[42u] = 1u;
    runtime.heap[second.words[36u]][4u] = 0xd000u;
    runtime.heap[second.words[40u]][4u] = 0xe000u;
    Check(&pal4::DialogObject::Destroy(second, runtime) == &second &&
          second.words[0u] == 0x84b194u && second.words[4u] == 0u &&
          second.words[24u] == 0u && second.words[28u] == 0u &&
          second.words[32u] == 0u && second.words[36u] == 0u &&
          second.words[40u] == 0u &&
          runtime.sizes.contains(first_nil) &&
          runtime.sizes.contains(second_nil),
          "destructor clears owned resources but retains shared sentinels");
    const auto find_call = [&](std::string_view name) {
        for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == name) return i;
        return runtime.calls.size();
    };
    Check(find_call("destroy-text") < find_call("destroy-second-branch") &&
          find_call("destroy-second-branch") <
              find_call("destroy-first-branch") &&
          find_call("destroy-first-branch") < find_call("string:36864") &&
          find_call("string:36864") < find_call("string:36884") &&
          find_call("string:36884") < find_call("destroy-xml"),
          "destructor cleanup order and 20-byte record stride");
    pal4::DialogObjectState sparse{};
    pal4::DialogObject::Construct(sparse, runtime);
    runtime.next_first_node = sparse.words[36u];
    runtime.next_second_node = sparse.words[40u];
    runtime.heap[sparse.words[36u]][0u] = 0xc000u;
    runtime.heap[sparse.words[40u]][0u] = 0xc100u;
    runtime.calls.clear();
    pal4::DialogObject::Destroy(sparse, runtime);
    const auto next_second = find_call("next-second");
    const auto erase_second = find_call("erase-second");
    const auto next_first = find_call("next-first");
    const auto erase_first = find_call("erase-first");
    Check(next_second < erase_second && erase_second < next_first &&
          next_first < erase_first && erase_first < runtime.calls.size(),
          "zero-count linked trees erase nodes in iterator order");
    pal4::DialogObject::Destroy(first, runtime);
    Check(!runtime.sizes.contains(first_nil) &&
          !runtime.sizes.contains(second_nil),
          "last object releases both shared sentinels");

    runtime.calls.clear();
    Check(pal4::DialogObject::DestroyInstance(
              0u, 0xa000u, 0xb000u, runtime) == 0u &&
          runtime.calls.empty(), "null destroy request is inert");
    Check(pal4::DialogObject::DestroyInstance(
              0x9000u, 0xa000u, 0xb000u, runtime) == 0x1234u &&
          runtime.calls == std::vector<std::string>{
              "pre-delete", "virtual-delete"},
          "non-null destroy runs pre-cleanup before virtual delete");

    pal4::DialogObjectState failed{};
    runtime.fail_object = true;
    runtime.calls.clear();
    Check(pal4::DialogObject::CreateInstance(failed, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"allocate:220"} &&
          failed.words[0u] == 0u,
          "allocation failure does not construct object");
}
