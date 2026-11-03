#include "pal4/ui_event_tree.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiEventTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> deleted_handlers;
    std::vector<std::uint32_t> cloned_inputs;
    std::uint32_t next_allocation = 0x5000u;
    std::uint32_t shared_nil = 0u;
    std::uint32_t references = 0u;
    std::uint32_t cloned_handler = 0x7000u;
    std::uint32_t cloned_from = 0u;
    std::uint32_t next_created_node = 0x6200u;
    std::uint32_t deleted_handler = 0u;
    std::uint32_t invoked_handler = 0u;
    std::uint32_t invoked_argument = 0u;
    bool fail_insert = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 0x18u, "24-byte tree node allocation");
        if (fail_insert) throw std::runtime_error("insert");
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
        return address + 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write-byte");
        bytes[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read-byte");
        return bytes.at(address);
    }
    std::uint32_t SharedNil() override {
        calls.emplace_back("nil");
        return shared_nil;
    }
    void SetSharedNil(std::uint32_t address) override {
        calls.emplace_back("set-nil");
        shared_nil = address;
    }
    std::uint32_t ReferenceCount() override {
        calls.emplace_back("references");
        return references;
    }
    void SetReferenceCount(std::uint32_t count) override {
        calls.emplace_back("set-references");
        references = count;
    }
    std::uint32_t FirstElement(std::uint32_t tree) override {
        calls.emplace_back("first-element");
        return words.at(words.at(tree + 4u));
    }
    std::uint32_t CloneHandler(std::uint32_t handler) override {
        calls.emplace_back("clone-handler");
        cloned_from = handler;
        cloned_inputs.push_back(handler);
        return cloned_handler;
    }
    std::uint32_t CreateNode(std::uint32_t parent) override {
        calls.emplace_back("create-node");
        const auto node = next_created_node;
        next_created_node += 0x100u;
        words[node + 4u] = parent;
        words[node + 0x14u] = 0u;
        return node;
    }
    std::uint32_t DeleteHandler(std::uint32_t handler,
                                std::uint32_t flags) override {
        calls.emplace_back("delete-handler");
        Check(flags == 1u, "virtual deleting destructor flag");
        deleted_handler = handler;
        deleted_handlers.push_back(handler);
        return 0xdeadu;
    }
    std::uint8_t InvokeHandler(std::uint32_t handler,
                               std::uint32_t argument) override {
        calls.emplace_back("invoke-handler");
        invoked_handler = handler;
        invoked_argument = argument;
        return 9u;
    }
};

void CheckConstructionAndRelease() {
    using Tree = pal4::UiEventTree;
    Runtime runtime;
    Check(Tree::Construct(0x1000u, runtime) == 0x1000u &&
              runtime.shared_nil == 0x5000u &&
              runtime.references == 1u &&
              runtime.bytes.at(0x1008u) == 0u &&
              runtime.words.at(0x5000u) == 0u &&
              runtime.words.at(0x5004u) == 0u &&
              runtime.words.at(0x5008u) == 0u &&
              runtime.words.at(0x5014u) == 1u &&
              runtime.words.at(0x1004u) == 0x5100u &&
              runtime.words.at(0x100cu) == 0u &&
              runtime.words.at(0x5100u) == 0x5100u &&
              runtime.words.at(0x5104u) == 0x5000u &&
              runtime.words.at(0x5108u) == 0x5100u &&
              runtime.words.at(0x5114u) == 0u,
          "first tree establishes shared nil and empty header");
    runtime.calls.clear();
    Check(Tree::Construct(0x2000u, runtime) == 0x2000u &&
              runtime.shared_nil == 0x5000u &&
              runtime.references == 2u &&
              runtime.freed == (std::vector<std::uint32_t>{0x5200u}) &&
              runtime.words.at(0x2004u) == 0x5300u &&
              runtime.words.at(0x5304u) == 0x5000u,
          "second tree frees candidate nil and reuses shared node");
    const auto ref_update = std::find(runtime.calls.begin(),
                                      runtime.calls.end(), "set-references");
    const auto candidate_free = std::find(runtime.calls.begin(),
                                          runtime.calls.end(), "free");
    Check(ref_update < candidate_free,
          "shared reference increment precedes candidate free");
    Check(Tree::ClearAndRelease(0x1000u, runtime) == 1u &&
              runtime.references == 1u && runtime.shared_nil == 0x5000u &&
              runtime.words.at(0x1004u) == 0u &&
              runtime.freed.back() == 0x5100u,
          "first release preserves shared nil");
    Check(Tree::ClearAndRelease(0x2000u, runtime) == 0x5001u &&
              runtime.references == 0u && runtime.shared_nil == 0u &&
              runtime.freed.at(runtime.freed.size() - 2u) == 0x5300u &&
              runtime.freed.back() == 0x5000u,
          "last release frees header then shared nil");
}

void CheckClearWithNodes() {
    Runtime runtime;
    (void)pal4::UiEventTree::Construct(0x1000u, runtime);
    runtime.words[0x100cu] = 2u;
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x6000u] = 0x5000u;
    runtime.words[0x6008u] = 0x6100u;
    runtime.words[0x6010u] = 0x7000u;
    runtime.words[0x6100u] = 0x5000u;
    runtime.words[0x6108u] = 0x5000u;
    runtime.words[0x6110u] = 0x7100u;
    Check(pal4::UiEventTree::ClearAndRelease(0x1000u, runtime) ==
              0x5001u &&
              runtime.deleted_handlers ==
                  (std::vector<std::uint32_t>{0x7100u, 0x7000u}) &&
              runtime.freed ==
                  (std::vector<std::uint32_t>{0x6100u, 0x6000u,
                                              0x5100u, 0x5000u}) &&
              runtime.words.at(0x5104u) == 0x5000u &&
              runtime.words.at(0x5100u) == 0x5100u &&
              runtime.words.at(0x5108u) == 0x5100u,
          "nonempty clear releases subtree and resets header");
}

void CheckAlternateDestructor() {
    Runtime runtime;
    (void)pal4::UiEventTree::Construct(0x1000u, runtime);
    runtime.words[0x100cu] = 1u;
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x6000u] = 0x5000u;
    runtime.words[0x6008u] = 0x6100u;
    runtime.words[0x6010u] = 0x7000u;
    runtime.words[0x6100u] = 0x5000u;
    runtime.words[0x6108u] = 0x5000u;
    runtime.words[0x6110u] = 0u;
    Check(pal4::UiEventTree::DestroyAlt(0x1000u, runtime) ==
              0x5001u &&
              runtime.deleted_handlers ==
                  (std::vector<std::uint32_t>{0x7000u}) &&
              runtime.freed ==
                  (std::vector<std::uint32_t>{0x6100u, 0x6000u,
                                              0x5100u, 0x5000u}),
          "alternate destructor releases node children, value, header and nil");

    Runtime orphan;
    (void)pal4::UiEventTree::Construct(0x1000u, orphan);
    orphan.words[0x5100u] = 0x6000u;
    orphan.words[0x5104u] = 0x6000u;
    orphan.words[0x5108u] = 0x6000u;
    orphan.words[0x6000u] = 0x5000u;
    orphan.words[0x6004u] = 0x5100u;
    orphan.words[0x6008u] = 0x5000u;
    orphan.words[0x6010u] = 0u;
    orphan.words[0x6014u] = 1u;
    Check(pal4::UiEventTree::DestroyAlt(0x1000u, orphan) ==
              0x5001u &&
              orphan.freed ==
                  (std::vector<std::uint32_t>{0x6000u, 0x5100u,
                                              0x5000u}) &&
              orphan.words.at(0x1004u) == 0u,
          "orphaned first element uses the rebuilt erase before release");
}

void CheckInsertionAndLookup() {
    using Tree = pal4::UiEventTree;
    Runtime runtime;
    (void)Tree::Construct(0x1000u, runtime);
    runtime.next_allocation = 0x6200u;
    runtime.words[0x8100u] = 0x80000000u;
    runtime.words[0x8200u] = 0x7100u;
    runtime.calls.clear();
    Check(Tree::InsertOrReplace(0x1000u, 0x8100u, 0x8200u,
                                runtime) &&
              runtime.freed.empty() &&
              runtime.cloned_inputs ==
                  (std::vector<std::uint32_t>{0x7100u, 0x7000u}) &&
              runtime.words.at(0x620cu) == 0x80000000u &&
              runtime.words.at(0x6210u) == 0x7000u &&
              runtime.words.at(0x6204u) == 0x5100u &&
              runtime.words.at(0x5104u) == 0x6200u &&
              runtime.deleted_handler == 0x7000u &&
              runtime.calls.back() == "delete-handler",
          "missing key inserts cloned handler and releases local reference");

    runtime.calls.clear();
    runtime.words[0x5100u] = 0x6000u;
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x5108u] = 0x6000u;
    runtime.words[0x100cu] = 1u;
    runtime.words[0x6000u] = 0x5000u;
    runtime.words[0x6004u] = 0x5100u;
    runtime.words[0x6008u] = 0x5000u;
    runtime.words[0x600cu] = 0x80000000u;
    runtime.words[0x6010u] = 0u;
    runtime.words[0x6014u] = 1u;
    Check(Tree::InsertOrReplace(0x1000u, 0x8100u, 0x8200u,
                                runtime) &&
              runtime.freed == (std::vector<std::uint32_t>{0x6000u}) &&
              runtime.words.at(0x100cu) == 1u &&
              runtime.calls.back() == "delete-handler",
          "equal unsigned key erases old node before insertion");

    runtime.freed.clear();
    runtime.words[0x8100u] = 1u;
    runtime.words[0x5100u] = 0x6000u;
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x5108u] = 0x6000u;
    runtime.words[0x100cu] = 1u;
    runtime.words[0x600cu] = 0xffffffffu;
    Check(Tree::InsertOrReplace(0x1000u, 0x8100u, 0x8200u,
                                runtime) && runtime.freed.empty() &&
              runtime.words.at(0x6000u) != 0x5000u,
          "smaller unsigned key does not erase candidate");

    runtime.words[0x8100u] = 2u;
    runtime.fail_insert = true;
    runtime.calls.clear();
    bool caught = false;
    try { (void)Tree::InsertOrReplace(0x1000u, 0x8100u, 0x8200u,
                                      runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "delete-handler" &&
              runtime.deleted_handler == 0x7000u,
          "failed insertion releases cloned handler");
    runtime.fail_insert = false;
    runtime.cloned_handler = 0u;
    runtime.deleted_handler = 0u;
    runtime.calls.clear();
    Check(Tree::InsertOrReplace(0x1000u, 0x8100u, 0x8200u,
                                runtime) &&
              runtime.deleted_handler == 0u &&
              runtime.words.at(0x650cu) == 2u,
          "null clone bypasses deleting destructor");

    runtime.words[0x8304u] = 0x7000u;
    Check(Tree::ReleaseClone(0x8300u, runtime) == 0xdeadu &&
              runtime.deleted_handler == 0x7000u,
          "temporary record destroys non-null clone with flag one");
    runtime.words[0x8304u] = 0u;
    runtime.deleted_handler = 0u;
    Check(Tree::ReleaseClone(0x8300u, runtime) == 0u &&
              runtime.deleted_handler == 0u,
          "null temporary clone has no virtual destructor call");

    runtime.words[0x8100u] = 0x80000000u;
    runtime.words[0x5104u] = 0x5000u;
    Check(Tree::Process(0x1000u, 0x8100u, 0x9000u,
                        runtime) == 0u,
          "missing event key returns zero");
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x600cu] = 0xffffffffu;
    Check(Tree::Process(0x1000u, 0x8100u, 0x9000u,
                        runtime) == 0u,
          "smaller unsigned key does not dispatch candidate");
    runtime.words[0x600cu] = 0x80000000u;
    runtime.words[0x6010u] = 0x7200u;
    Check(Tree::Process(0x1000u, 0x8100u, 0x9000u,
                        runtime) == 9u &&
              runtime.invoked_handler == 0x7200u &&
              runtime.invoked_argument == 0x9000u,
          "matched event forwards handler and argument return byte");
}

void CheckFindNode() {
    using Tree = pal4::UiEventTree;
    Runtime runtime;
    (void)Tree::Construct(0x1000u, runtime);
    Check(Tree::FindNode(0x1000u, 0u, runtime) == 0x5100u,
          "empty tree search returns header");
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x6000u] = 0x6100u;
    runtime.words[0x6008u] = 0x6200u;
    runtime.words[0x600cu] = 0x80000000u;
    runtime.words[0x6100u] = 0x5000u;
    runtime.words[0x6108u] = 0x5000u;
    runtime.words[0x610cu] = 1u;
    runtime.words[0x6200u] = 0x5000u;
    runtime.words[0x6208u] = 0x5000u;
    runtime.words[0x620cu] = 0xfffffffeu;
    Check(Tree::FindNode(0x1000u, 0u, runtime) == 0x6100u &&
              Tree::FindNode(0x1000u, 1u, runtime) == 0x6100u &&
              Tree::FindNode(0x1000u, 2u, runtime) == 0x6000u &&
              Tree::FindNode(0x1000u, 0x80000000u, runtime) == 0x6000u &&
              Tree::FindNode(0x1000u, 0x80000001u, runtime) == 0x6200u &&
              Tree::FindNode(0x1000u, 0xffffffffu, runtime) == 0x5100u,
          "search returns the first unsigned key not less than target");
    Check(Tree::Find(0x1000u, 2u, runtime) == 0x6000u &&
              Tree::Find(0x1000u, 0xffffffffu, runtime) == 0x5100u,
          "direct-return find shares lower-bound semantics");
}

void CheckInsertNode() {
    using Tree = pal4::UiEventTree;
    Runtime runtime;
    (void)Tree::Construct(0x1000u, runtime);
    runtime.next_allocation = 0x6200u;
    const auto nil = runtime.shared_nil;
    const auto header = runtime.words.at(0x1004u);
    Check(Tree::InsertNode(0x1000u, nil, header, 50u, 0x7000u,
                           runtime) == 0x6200u &&
              runtime.words.at(0x6200u) == nil &&
              runtime.words.at(0x6204u) == header &&
              runtime.words.at(0x6208u) == nil &&
              runtime.words.at(0x620cu) == 50u &&
              runtime.words.at(0x6210u) == 0x7000u &&
              runtime.words.at(0x6214u) == 1u &&
              runtime.words.at(0x5100u) == 0x6200u &&
              runtime.words.at(0x5104u) == 0x6200u &&
              runtime.words.at(0x5108u) == 0x6200u &&
              runtime.words.at(0x100cu) == 1u,
          "first native node initializes value, root and extrema");
    Check(Tree::InsertNode(0x1000u, nil, 0x6200u, 70u, 0x7100u,
                           runtime) == 0x6300u &&
              runtime.words.at(0x6208u) == 0x6300u &&
              runtime.words.at(0x6304u) == 0x6200u &&
              runtime.words.at(0x630cu) == 70u &&
              runtime.words.at(0x6310u) == 0x7000u &&
              runtime.words.at(0x6314u) == 0u &&
              runtime.words.at(0x5108u) == 0x6300u &&
              runtime.words.at(0x100cu) == 2u,
          "right insertion links parent and updates maximum");
    Check(Tree::InsertNode(0x1000u, nil, 0x6200u, 30u, 0x7200u,
                           runtime) == 0x6400u &&
              runtime.words.at(0x6200u) == 0x6400u &&
              runtime.words.at(0x5100u) == 0x6400u &&
              runtime.words.at(0x100cu) == 3u,
          "left insertion updates minimum");
}

void CheckFreeNodes() {
    Runtime runtime;
    (void)pal4::UiEventTree::Construct(0x1000u, runtime);
    const auto nil = runtime.shared_nil;
    Check(pal4::UiEventTree::FreeNodes(nil, runtime) == nil &&
              runtime.freed.empty(),
          "shared nil is not freed as a tree node");
    runtime.words[0x6000u] = 0x6100u;
    runtime.words[0x6008u] = 0x6200u;
    runtime.words[0x6010u] = 0x7000u;
    runtime.words[0x6100u] = nil;
    runtime.words[0x6108u] = 0x6300u;
    runtime.words[0x6110u] = 0u;
    runtime.words[0x6200u] = nil;
    runtime.words[0x6208u] = nil;
    runtime.words[0x6210u] = 0x7200u;
    runtime.words[0x6300u] = nil;
    runtime.words[0x6308u] = nil;
    runtime.words[0x6310u] = 0x7300u;
    Check(pal4::UiEventTree::FreeNodes(0x6000u, runtime) == nil,
          "free nodes returns the shared nil");
    Check(runtime.freed ==
              (std::vector<std::uint32_t>{0x6200u, 0x6000u,
                                          0x6300u, 0x6100u}),
          "each right subtree frees before its left-chain node");
    Check(runtime.deleted_handlers ==
              (std::vector<std::uint32_t>{0x7200u, 0x7000u,
                                          0x7300u}),
          "non-null handlers are destroyed before their nodes");
}

void CheckCopyValue() {
    Runtime runtime;
    pal4::UiEventTree::CopyValue(0u, 12u, 0x7100u, runtime);
    Check(runtime.words.empty() && runtime.cloned_inputs.empty(),
          "null value destination performs no copy or clone");
    pal4::UiEventTree::CopyValue(0x9000u, 0xffffffffu,
                                 0x7100u, runtime);
    Check(runtime.words.at(0x9000u) == 0xffffffffu &&
              runtime.words.at(0x9004u) == 0x7000u &&
              runtime.cloned_inputs ==
                  (std::vector<std::uint32_t>{0x7100u}),
          "value copy preserves unsigned key and clones handler");
}

void CheckDestroyValue() {
    Runtime runtime;
    runtime.words[0x9004u] = 0x7000u;
    Check(pal4::UiEventTree::DestroyValue(0x9000u, 0u, runtime) ==
              0x9000u &&
              runtime.deleted_handlers ==
                  (std::vector<std::uint32_t>{0x7000u}) &&
              runtime.freed.empty(),
          "nondeleting value destructor releases only the clone");
    runtime.words[0x9004u] = 0u;
    Check(pal4::UiEventTree::DestroyValue(0x9000u, 2u, runtime) ==
              0x9000u && runtime.freed.empty() &&
              runtime.deleted_handlers.size() == 1u,
          "higher flag bits neither delete nor free a null clone");
    Check(pal4::UiEventTree::DestroyValue(0x9000u, 3u, runtime) ==
              0x9000u &&
              runtime.freed == (std::vector<std::uint32_t>{0x9000u}),
          "low flag bit additionally frees the value record");
}

void CheckInsertModes() {
    using Tree = pal4::UiEventTree;
    Runtime unique;
    (void)Tree::Construct(0x1000u, unique);
    unique.next_allocation = 0x6200u;
    const auto first = Tree::Insert(0x1000u, 50u, 0x7000u, unique);
    Check(first.node == 0x6200u && first.inserted &&
              unique.words.at(0x6204u) == 0x5100u &&
              unique.words.at(0x6200u) == 0x5000u &&
              unique.words.at(0x5104u) == 0x6200u,
          "unique mode inserts into empty header through InsertNode");
    unique.words[0x5104u] = 0x6000u;
    unique.words[0x5100u] = 0x6000u;
    unique.words[0x5108u] = 0x6000u;
    unique.words[0x6000u] = 0x5000u;
    unique.words[0x6004u] = 0x5100u;
    unique.words[0x6008u] = 0x5000u;
    unique.words[0x600cu] = 50u;
    unique.words[0x6014u] = 1u;
    const auto duplicate = Tree::Insert(0x1000u, 50u, 0x7000u, unique);
    Check(duplicate.node == 0x6000u && !duplicate.inserted,
          "unique mode returns existing node for equal key");
    unique.next_allocation = 0x6200u;
    const auto smaller = Tree::Insert(0x1000u, 40u, 0x7000u, unique);
    Check(smaller.node == 0x6200u && smaller.inserted &&
              unique.words.at(0x6204u) == 0x6000u,
          "new minimum bypasses predecessor and inserts left");
    unique.words[0x6000u] = 0x6100u;
    unique.words[0x6008u] = 0x6300u;
    unique.words[0x600cu] = 100u;
    unique.words[0x6100u] = 0x5000u;
    unique.words[0x6108u] = 0x5000u;
    unique.words[0x610cu] = 20u;
    unique.words[0x6300u] = 0x5000u;
    unique.words[0x6308u] = 0x5000u;
    unique.words[0x630cu] = 200u;
    unique.words[0x6304u] = 0x6000u;
    unique.words[0x6314u] = 1u;
    unique.words[0x5100u] = 0x6100u;
    unique.next_allocation = 0x6200u;
    const auto middle = Tree::Insert(0x1000u, 150u, 0x7000u, unique);
    Check(middle.inserted && middle.node == 0x6200u &&
              unique.words.at(0x6204u) == 0x6300u &&
              unique.words.at(0x620cu) == 150u,
          "left search below nonminimum parent checks predecessor");

    Runtime inline_tree;
    (void)Tree::Construct(0x1000u, inline_tree);
    inline_tree.bytes[0x1008u] = 1u;
    const auto root = Tree::Insert(0x1000u, 50u, 0x7000u, inline_tree);
    Check(root.node == 0x6200u && root.inserted &&
              inline_tree.words.at(0x5100u) == 0x6200u &&
              inline_tree.words.at(0x5104u) == 0x6200u &&
              inline_tree.words.at(0x5108u) == 0x6200u &&
              inline_tree.words.at(0x6200u) == 0x5000u &&
              inline_tree.words.at(0x6208u) == 0x5000u &&
              inline_tree.words.at(0x620cu) == 50u &&
              inline_tree.words.at(0x6210u) == 0x7000u &&
              inline_tree.words.at(0x6214u) == 1u &&
              inline_tree.words.at(0x100cu) == 1u,
          "inline mode links and blackens its first node");
    const auto left = Tree::Insert(0x1000u, 10u, 0x7100u, inline_tree);
    const auto right = Tree::Insert(0x1000u, 90u, 0x7200u, inline_tree);
    Check(left.node == 0x6300u && right.node == 0x6400u &&
              inline_tree.words.at(0x6200u) == 0x6300u &&
              inline_tree.words.at(0x6208u) == 0x6400u &&
              inline_tree.words.at(0x5100u) == 0x6300u &&
              inline_tree.words.at(0x5108u) == 0x6400u &&
              inline_tree.words.at(0x100cu) == 3u,
          "inline mode updates leftmost and rightmost header links");

    Runtime recolor;
    (void)Tree::Construct(0x1000u, recolor);
    recolor.bytes[0x1008u] = 1u;
    recolor.words[0x100cu] = 3u;
    recolor.words[0x5100u] = 0x6100u;
    recolor.words[0x5104u] = 0x6000u;
    recolor.words[0x5108u] = 0x6300u;
    recolor.words[0x6000u] = 0x6100u;
    recolor.words[0x6004u] = 0x5100u;
    recolor.words[0x6008u] = 0x6300u;
    recolor.words[0x600cu] = 100u;
    recolor.words[0x6014u] = 1u;
    recolor.words[0x6100u] = 0x5000u;
    recolor.words[0x6104u] = 0x6000u;
    recolor.words[0x6108u] = 0x5000u;
    recolor.words[0x610cu] = 40u;
    recolor.words[0x6114u] = 0u;
    recolor.words[0x6300u] = 0x5000u;
    recolor.words[0x6304u] = 0x6000u;
    recolor.words[0x6308u] = 0x5000u;
    recolor.words[0x630cu] = 150u;
    recolor.words[0x6314u] = 0u;
    const auto inserted = Tree::Insert(0x1000u, 20u, 0x7000u, recolor);
    Check(inserted.node == 0x6200u && inserted.inserted &&
              recolor.words.at(0x6114u) == 1u &&
              recolor.words.at(0x6314u) == 1u &&
              recolor.words.at(0x6014u) == 1u &&
              recolor.words.at(0x100cu) == 4u &&
              recolor.words.at(0x5104u) == 0x6000u,
          "red uncle recolors both children and restores black root");

    Runtime left_rotation;
    (void)Tree::Construct(0x1000u, left_rotation);
    left_rotation.bytes[0x1008u] = 1u;
    left_rotation.words[0x100cu] = 2u;
    left_rotation.words[0x5100u] = 0x6100u;
    left_rotation.words[0x5104u] = 0x6000u;
    left_rotation.words[0x5108u] = 0x6000u;
    left_rotation.words[0x6000u] = 0x6100u;
    left_rotation.words[0x6004u] = 0x5100u;
    left_rotation.words[0x6008u] = 0x5000u;
    left_rotation.words[0x600cu] = 100u;
    left_rotation.words[0x6014u] = 1u;
    left_rotation.words[0x6100u] = 0x5000u;
    left_rotation.words[0x6104u] = 0x6000u;
    left_rotation.words[0x6108u] = 0x5000u;
    left_rotation.words[0x610cu] = 40u;
    left_rotation.words[0x6114u] = 0u;
    (void)Tree::Insert(0x1000u, 20u, 0x7000u, left_rotation);
    Check(left_rotation.words.at(0x5104u) == 0x6100u &&
              left_rotation.words.at(0x6108u) == 0x6000u &&
              left_rotation.words.at(0x6004u) == 0x6100u &&
              left_rotation.words.at(0x6114u) == 1u &&
              left_rotation.words.at(0x6014u) == 0u,
          "black right uncle rotates left-side parent into root");

    Runtime right_rotation;
    (void)Tree::Construct(0x1000u, right_rotation);
    right_rotation.bytes[0x1008u] = 1u;
    right_rotation.words[0x100cu] = 2u;
    right_rotation.words[0x5100u] = 0x6000u;
    right_rotation.words[0x5104u] = 0x6000u;
    right_rotation.words[0x5108u] = 0x6100u;
    right_rotation.words[0x6000u] = 0x5000u;
    right_rotation.words[0x6004u] = 0x5100u;
    right_rotation.words[0x6008u] = 0x6100u;
    right_rotation.words[0x600cu] = 100u;
    right_rotation.words[0x6014u] = 1u;
    right_rotation.words[0x6100u] = 0x5000u;
    right_rotation.words[0x6104u] = 0x6000u;
    right_rotation.words[0x6108u] = 0x5000u;
    right_rotation.words[0x610cu] = 150u;
    right_rotation.words[0x6114u] = 0u;
    (void)Tree::Insert(0x1000u, 180u, 0x7000u, right_rotation);
    Check(right_rotation.words.at(0x5104u) == 0x6100u &&
              right_rotation.words.at(0x6100u) == 0x6000u &&
              right_rotation.words.at(0x6004u) == 0x6100u &&
              right_rotation.words.at(0x6114u) == 1u &&
              right_rotation.words.at(0x6014u) == 0u,
          "black left uncle rotates right-side parent into root");
}

void CheckRotations() {
    using Tree = pal4::UiEventTree;
    Runtime root;
    (void)Tree::Construct(0x1000u, root);
    root.words[0x5104u] = 0x6000u;
    root.words[0x6000u] = 0x5000u;
    root.words[0x6004u] = 0x5100u;
    root.words[0x6008u] = 0x6200u;
    root.words[0x6200u] = 0x6300u;
    root.words[0x6204u] = 0x6000u;
    root.words[0x6208u] = 0x5000u;
    root.words[0x6300u] = 0x5000u;
    root.words[0x6304u] = 0x6200u;
    root.words[0x6308u] = 0x5000u;
    Check(Tree::RotateRight(0x1000u, 0x6000u, root) == 0x6200u &&
              root.words.at(0x5104u) == 0x6200u &&
              root.words.at(0x6204u) == 0x5100u &&
              root.words.at(0x6200u) == 0x6000u &&
              root.words.at(0x6004u) == 0x6200u &&
              root.words.at(0x6008u) == 0x6300u &&
              root.words.at(0x6304u) == 0x6000u,
          "right rotation promotes root's right child and reparents middle");
    Check(Tree::RotateLeft(0x1000u, 0x6200u, root) == 0x6000u &&
              root.words.at(0x5104u) == 0x6000u &&
              root.words.at(0x6008u) == 0x6200u &&
              root.words.at(0x6200u) == 0x6300u &&
              root.words.at(0x6304u) == 0x6200u,
          "left rotation restores root and middle child's parent");

    Runtime nested;
    (void)Tree::Construct(0x1000u, nested);
    nested.words[0x5104u] = 0x6000u;
    nested.words[0x6000u] = 0x6100u;
    nested.words[0x6004u] = 0x5100u;
    nested.words[0x6008u] = 0x5000u;
    nested.words[0x6100u] = 0x5000u;
    nested.words[0x6104u] = 0x6000u;
    nested.words[0x6108u] = 0x6200u;
    nested.words[0x6200u] = 0x6300u;
    nested.words[0x6204u] = 0x6100u;
    nested.words[0x6208u] = 0x5000u;
    nested.words[0x6300u] = 0x5000u;
    nested.words[0x6304u] = 0x6200u;
    nested.words[0x6308u] = 0x5000u;
    Check(Tree::RotateRight(0x1000u, 0x6100u, nested) == 0x6200u &&
              nested.words.at(0x5104u) == 0x6000u &&
              nested.words.at(0x6000u) == 0x6200u &&
              nested.words.at(0x6204u) == 0x6000u &&
              nested.words.at(0x6108u) == 0x6300u &&
              nested.words.at(0x6304u) == 0x6100u,
          "nonroot rotation replaces the correct parent link");
}

void CheckSuccessor() {
    using Tree = pal4::UiEventTree;
    Runtime runtime;
    (void)Tree::Construct(0x1000u, runtime);
    const auto nil = runtime.shared_nil;
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x5108u] = 0x6400u;
    runtime.words[0x6000u] = 0x6100u;
    runtime.words[0x6004u] = 0x5100u;
    runtime.words[0x6008u] = 0x6200u;
    runtime.words[0x6100u] = nil;
    runtime.words[0x6104u] = 0x6000u;
    runtime.words[0x6108u] = nil;
    runtime.words[0x6200u] = 0x6300u;
    runtime.words[0x6204u] = 0x6000u;
    runtime.words[0x6208u] = 0x6400u;
    runtime.words[0x6300u] = nil;
    runtime.words[0x6304u] = 0x6200u;
    runtime.words[0x6308u] = nil;
    runtime.words[0x6400u] = nil;
    runtime.words[0x6404u] = 0x6200u;
    runtime.words[0x6408u] = nil;

    auto iterator = 0x6000u;
    Check(Tree::Successor(iterator, runtime) == 0x6000u &&
              iterator == 0x6300u,
          "successor finds the minimum of the right subtree");
    Check(Tree::Successor(iterator, runtime) == 0x6300u &&
              iterator == 0x6200u,
          "successor ascends from a left child");
    iterator = 0x6100u;
    Check(Tree::Successor(iterator, runtime) == 0x6100u &&
              iterator == 0x6000u,
          "leftmost leaf advances to its parent");
    iterator = 0x6400u;
    Check(Tree::Successor(iterator, runtime) == 0x6400u &&
              iterator == 0x5100u,
          "maximum advances to the header after right-ancestor ascent");
    iterator = 0x6000u;
    Tree::SuccessorAlt(iterator, runtime);
    Check(iterator == 0x6300u,
          "alternate successor advances the iterator in place");
    iterator = 0x6400u;
    Tree::SuccessorAlt(iterator, runtime);
    Check(iterator == 0x5100u,
          "alternate successor reaches the header from maximum");
}

void CheckPredecessor() {
    using Tree = pal4::UiEventTree;
    Runtime runtime;
    (void)Tree::Construct(0x1000u, runtime);
    const auto nil = runtime.shared_nil;
    runtime.words[0x5100u] = 0x6100u;
    runtime.words[0x5104u] = 0x6000u;
    runtime.words[0x5108u] = 0x6200u;
    runtime.words[0x6000u] = 0x6100u;
    runtime.words[0x6004u] = 0x5100u;
    runtime.words[0x6008u] = 0x6200u;
    runtime.words[0x6014u] = 1u;
    runtime.words[0x6100u] = nil;
    runtime.words[0x6104u] = 0x6000u;
    runtime.words[0x6108u] = 0x6300u;
    runtime.words[0x6114u] = 1u;
    runtime.words[0x6200u] = 0x6400u;
    runtime.words[0x6204u] = 0x6000u;
    runtime.words[0x6208u] = nil;
    runtime.words[0x6214u] = 1u;
    runtime.words[0x6300u] = nil;
    runtime.words[0x6304u] = 0x6100u;
    runtime.words[0x6308u] = nil;
    runtime.words[0x6314u] = 1u;
    runtime.words[0x6400u] = nil;
    runtime.words[0x6404u] = 0x6200u;
    runtime.words[0x6408u] = nil;
    runtime.words[0x6414u] = 1u;

    auto iterator = 0x5100u;
    Check(Tree::Predecessor(iterator, runtime) == 0x6200u &&
              iterator == 0x6200u,
          "header predecessor returns maximum");
    iterator = 0x6000u;
    Check(Tree::Predecessor(iterator, runtime) == 0x6300u &&
              iterator == 0x6300u,
          "predecessor finds maximum of left subtree");
    iterator = 0x6400u;
    Check(Tree::Predecessor(iterator, runtime) == 0x6000u &&
              iterator == 0x6000u,
          "predecessor ascends across left ancestors");
    iterator = 0x6100u;
    Check(Tree::Predecessor(iterator, runtime) == 0x5100u &&
              iterator == 0x5100u,
          "minimum advances to header");
}

void CheckErase() {
    using Tree = pal4::UiEventTree;
    const auto make_tree = [] {
        Runtime runtime;
        (void)Tree::Construct(0x1000u, runtime);
        runtime.words[0x6000u] = 0x5000u;
        runtime.words[0x6004u] = 0x5100u;
        runtime.words[0x6008u] = 0x5000u;
        runtime.words[0x600cu] = 50u;
        runtime.words[0x6010u] = 0x7000u;
        runtime.words[0x6014u] = 1u;
        runtime.words[0x5100u] = 0x6000u;
        runtime.words[0x5104u] = 0x6000u;
        runtime.words[0x5108u] = 0x6000u;
        runtime.words[0x100cu] = 1u;
        return runtime;
    };

    auto single = make_tree();
    Check(Tree::Erase(0x1000u, 0x6000u, single) == 0x5100u &&
              single.words.at(0x5100u) == 0x5100u &&
              single.words.at(0x5104u) == 0x5000u &&
              single.words.at(0x5108u) == 0x5100u &&
              single.words.at(0x5004u) == 0x5100u &&
              single.words.at(0x100cu) == 0u &&
              single.deleted_handler == 0x7000u &&
              single.freed.back() == 0x6000u,
          "black singleton erase resets root and extrema then frees value");

    auto red_leaf = make_tree();
    red_leaf.words[0x6100u] = 0x6000u;
    red_leaf.words[0x6104u] = 0x5100u;
    red_leaf.words[0x6108u] = 0x5000u;
    red_leaf.words[0x6110u] = 0u;
    red_leaf.words[0x6114u] = 1u;
    red_leaf.words[0x6004u] = 0x6100u;
    red_leaf.words[0x6010u] = 0u;
    red_leaf.words[0x6014u] = 0u;
    red_leaf.words[0x5104u] = 0x6100u;
    red_leaf.words[0x5108u] = 0x6100u;
    red_leaf.words[0x100cu] = 2u;
    Check(Tree::Erase(0x1000u, 0x6000u, red_leaf) == 0x6100u &&
              red_leaf.words.at(0x5100u) == 0x6100u &&
              red_leaf.words.at(0x6100u) == 0x5000u &&
              red_leaf.words.at(0x6104u) == 0x5100u &&
              red_leaf.words.at(0x100cu) == 1u &&
              red_leaf.deleted_handler == 0u,
          "red leftmost leaf removal updates minimum without black fixup");

    auto one_child = make_tree();
    one_child.words[0x6000u] = 0x6100u;
    one_child.words[0x6100u] = 0x5000u;
    one_child.words[0x6104u] = 0x6000u;
    one_child.words[0x6108u] = 0x5000u;
    one_child.words[0x6110u] = 0u;
    one_child.words[0x6114u] = 0u;
    one_child.words[0x5100u] = 0x6100u;
    one_child.words[0x100cu] = 2u;
    Check(Tree::Erase(0x1000u, 0x6000u, one_child) == 0x5100u &&
              one_child.words.at(0x5104u) == 0x6100u &&
              one_child.words.at(0x5108u) == 0x6100u &&
              one_child.words.at(0x6104u) == 0x5100u &&
              one_child.words.at(0x6114u) == 1u &&
              one_child.words.at(0x100cu) == 1u,
          "red child replaces black root and becomes black");

    auto two_children = make_tree();
    two_children.words[0x6000u] = 0x6100u;
    two_children.words[0x6008u] = 0x6200u;
    two_children.words[0x6100u] = 0x5000u;
    two_children.words[0x6104u] = 0x6000u;
    two_children.words[0x6108u] = 0x5000u;
    two_children.words[0x6114u] = 0u;
    two_children.words[0x6200u] = 0x5000u;
    two_children.words[0x6204u] = 0x6000u;
    two_children.words[0x6208u] = 0x5000u;
    two_children.words[0x6214u] = 0u;
    two_children.words[0x5100u] = 0x6100u;
    two_children.words[0x5108u] = 0x6200u;
    two_children.words[0x100cu] = 3u;
    Check(Tree::Erase(0x1000u, 0x6000u, two_children) ==
              0x6200u &&
              two_children.words.at(0x5104u) == 0x6200u &&
              two_children.words.at(0x6200u) == 0x6100u &&
              two_children.words.at(0x6204u) == 0x5100u &&
              two_children.words.at(0x6104u) == 0x6200u &&
              two_children.words.at(0x6214u) == 1u &&
              two_children.words.at(0x100cu) == 2u &&
              two_children.freed.back() == 0x6000u,
          "direct successor replaces two-child root and swaps colour");

    auto deep = make_tree();
    deep.words[0x6000u] = 0x6100u;
    deep.words[0x6008u] = 0x6200u;
    deep.words[0x6100u] = 0x5000u;
    deep.words[0x6104u] = 0x6000u;
    deep.words[0x6108u] = 0x5000u;
    deep.words[0x6200u] = 0x6300u;
    deep.words[0x6204u] = 0x6000u;
    deep.words[0x6208u] = 0x5000u;
    deep.words[0x6300u] = 0x5000u;
    deep.words[0x6304u] = 0x6200u;
    deep.words[0x6308u] = 0x5000u;
    deep.words[0x6314u] = 0u;
    deep.words[0x5100u] = 0x6100u;
    deep.words[0x5108u] = 0x6200u;
    deep.words[0x100cu] = 4u;
    Check(Tree::Erase(0x1000u, 0x6000u, deep) == 0x6300u &&
              deep.words.at(0x5104u) == 0x6300u &&
              deep.words.at(0x6300u) == 0x6100u &&
              deep.words.at(0x6308u) == 0x6200u &&
              deep.words.at(0x6200u) == 0x5000u &&
              deep.words.at(0x6204u) == 0x6300u &&
              deep.words.at(0x100cu) == 3u,
          "deep successor detaches from old parent and carries right branch");

    auto rotate = make_tree();
    rotate.words[0x6000u] = 0x6100u;
    rotate.words[0x6008u] = 0x6200u;
    rotate.words[0x6100u] = 0x5000u;
    rotate.words[0x6104u] = 0x6000u;
    rotate.words[0x6108u] = 0x5000u;
    rotate.words[0x6110u] = 0u;
    rotate.words[0x6114u] = 1u;
    rotate.words[0x6200u] = 0x5000u;
    rotate.words[0x6204u] = 0x6000u;
    rotate.words[0x6208u] = 0x6300u;
    rotate.words[0x6214u] = 1u;
    rotate.words[0x6300u] = 0x5000u;
    rotate.words[0x6304u] = 0x6200u;
    rotate.words[0x6308u] = 0x5000u;
    rotate.words[0x6314u] = 0u;
    rotate.words[0x5100u] = 0x6100u;
    rotate.words[0x5108u] = 0x6300u;
    rotate.words[0x100cu] = 4u;
    Check(Tree::Erase(0x1000u, 0x6100u, rotate) == 0x6000u &&
              rotate.words.at(0x5104u) == 0x6200u &&
              rotate.words.at(0x6200u) == 0x6000u &&
              rotate.words.at(0x6004u) == 0x6200u &&
              rotate.words.at(0x6214u) == 1u &&
              rotate.words.at(0x6014u) == 1u,
          "black leaf removal repairs right sibling with far red child");

    const auto node = [](Runtime& state, std::uint32_t address,
                         std::uint32_t parent, std::uint32_t left,
                         std::uint32_t right, std::uint32_t colour) {
        state.words[address] = left;
        state.words[address + 4u] = parent;
        state.words[address + 8u] = right;
        state.words[address + 0x10u] = 0u;
        state.words[address + 0x14u] = colour;
    };

    auto near_red = make_tree();
    node(near_red, 0x6000u, 0x5100u, 0x6100u, 0x6200u, 1u);
    node(near_red, 0x6100u, 0x6000u, 0x5000u, 0x5000u, 1u);
    node(near_red, 0x6200u, 0x6000u, 0x6300u, 0x5000u, 1u);
    node(near_red, 0x6300u, 0x6200u, 0x5000u, 0x5000u, 0u);
    near_red.words[0x5100u] = 0x6100u;
    near_red.words[0x5108u] = 0x6200u;
    near_red.words[0x100cu] = 4u;
    Check(Tree::Erase(0x1000u, 0x6100u, near_red) == 0x6000u &&
              near_red.words.at(0x5104u) == 0x6300u &&
              near_red.words.at(0x6300u) == 0x6000u &&
              near_red.words.at(0x6308u) == 0x6200u &&
              near_red.words.at(0x6004u) == 0x6300u &&
              near_red.words.at(0x6204u) == 0x6300u &&
              near_red.words.at(0x6314u) == 1u,
          "near red nephew requires two structural rotations");

    auto mirror = make_tree();
    node(mirror, 0x6000u, 0x5100u, 0x6100u, 0x6200u, 1u);
    node(mirror, 0x6100u, 0x6000u, 0x6300u, 0x5000u, 1u);
    node(mirror, 0x6200u, 0x6000u, 0x5000u, 0x5000u, 1u);
    node(mirror, 0x6300u, 0x6100u, 0x5000u, 0x5000u, 0u);
    mirror.words[0x5100u] = 0x6300u;
    mirror.words[0x5108u] = 0x6200u;
    mirror.words[0x100cu] = 4u;
    Check(Tree::Erase(0x1000u, 0x6200u, mirror) == 0x5100u &&
              mirror.words.at(0x5104u) == 0x6100u &&
              mirror.words.at(0x5108u) == 0x6000u &&
              mirror.words.at(0x6108u) == 0x6000u &&
              mirror.words.at(0x6004u) == 0x6100u,
          "black right leaf uses mirror repair and updates maximum");

    auto red_sibling = make_tree();
    node(red_sibling, 0x6000u, 0x5100u, 0x6100u, 0x6200u, 1u);
    node(red_sibling, 0x6100u, 0x6000u, 0x5000u, 0x5000u, 1u);
    node(red_sibling, 0x6200u, 0x6000u, 0x6300u, 0x6400u, 0u);
    node(red_sibling, 0x6300u, 0x6200u, 0x5000u, 0x5000u, 1u);
    node(red_sibling, 0x6400u, 0x6200u, 0x5000u, 0x5000u, 1u);
    red_sibling.words[0x5100u] = 0x6100u;
    red_sibling.words[0x5108u] = 0x6400u;
    red_sibling.words[0x100cu] = 5u;
    Check(Tree::Erase(0x1000u, 0x6100u, red_sibling) == 0x6000u &&
              red_sibling.words.at(0x5104u) == 0x6200u &&
              red_sibling.words.at(0x6200u) == 0x6000u &&
              red_sibling.words.at(0x6008u) == 0x6300u &&
              red_sibling.words.at(0x6214u) == 1u &&
              red_sibling.words.at(0x6014u) == 1u &&
              red_sibling.words.at(0x6314u) == 0u,
          "red sibling rotates before black sibling recolouring");
}

}  // namespace

int main() {
    try {
        CheckConstructionAndRelease();
        CheckClearWithNodes();
        CheckAlternateDestructor();
        CheckInsertionAndLookup();
        CheckFindNode();
        CheckInsertNode();
        CheckFreeNodes();
        CheckCopyValue();
        CheckDestroyValue();
        CheckInsertModes();
        CheckRotations();
        CheckSuccessor();
        CheckPredecessor();
        CheckErase();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
