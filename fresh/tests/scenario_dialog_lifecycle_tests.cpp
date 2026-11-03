#include "pal4/scenario_dialog.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScenarioDialogLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> removed;
    std::uint32_t remaining_refs = 2u;
    std::uint32_t nil = 0x5000u;
    bool throw_in_free_nodes = false;

    explicit Runtime(std::uint32_t count, std::uint32_t first = 0x4000u) {
        words[0x1044u] = 0x4000u;
        words[0x104cu] = count;
        words[0x4000u] = first;
        words[0x4004u] = 0x4020u;
        words[0x4008u] = 0x4000u;
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
    void FreeNodes(std::uint32_t tree,
                   std::uint32_t root) override {
        Check(tree == 0x1040u && root == 0x4020u,
              "bulk tree root");
        calls.emplace_back("free-nodes");
        if (throw_in_free_nodes)
            throw std::runtime_error("free-nodes");
    }
    std::uint32_t SharedNil() override {
        calls.emplace_back("shared-nil");
        return nil;
    }
    void FirstElement(std::uint32_t tree) override {
        Check(tree == 0x1040u, "first-element tree");
        calls.emplace_back("first");
    }
    std::uint32_t Successor(std::uint32_t node) override {
        calls.emplace_back("successor");
        return successors.at(node);
    }
    void RemoveNode(std::uint32_t tree,
                    std::uint32_t node) override {
        Check(tree == 0x1040u, "remove tree");
        calls.emplace_back("remove");
        removed.push_back(node);
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
        return 0xbeefu;
    }
    std::uint32_t DecrementSharedRef() override {
        calls.emplace_back("decrement-ref");
        return remaining_refs;
    }
    std::uint32_t TakeSharedNil() override {
        calls.emplace_back("take-nil");
        const auto result = nil;
        nil = 0u;
        return result;
    }
    std::uint32_t DestructBase(std::uint32_t object) override {
        Check(object == 0x1000u, "base owner");
        calls.emplace_back("destroy-base");
        return 0xd00du;
    }
};

void CheckBulkTree() {
    Runtime runtime(3u, 0x4100u);
    Check(pal4::ScenarioDialog::DestructTree(0x1040u, runtime) == 2u,
          "nonzero shared ref result");
    Check(runtime.words.at(0x4004u) == 0x5000u &&
              runtime.words.at(0x4000u) == 0x4000u &&
              runtime.words.at(0x4008u) == 0x4000u &&
              runtime.words.at(0x1044u) == 0u &&
              runtime.words.at(0x104cu) == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x4000u} &&
              runtime.removed.empty(),
          "bulk path clears sentinel and tree fields");
    const auto free_nodes = std::find(runtime.calls.begin(),
                                      runtime.calls.end(), "free-nodes");
    const auto first = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), "first");
    const auto free = std::find(runtime.calls.begin(),
                                runtime.calls.end(), "free");
    Check(free_nodes < first && first < free &&
              runtime.calls.back() == "decrement-ref",
          "bulk release order");
}

void CheckFirstElement() {
    Runtime runtime(2u, 0x4100u);
    Check(pal4::ScenarioDialog::GetFirstElement(
              0x1040u, 0x7000u, runtime) == 0x7000u &&
              runtime.words.at(0x7000u) == 0x4100u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "read", "write"}),
          "tree sentinel first pointer copied to output");
    Runtime empty(0u);
    Check(pal4::ScenarioDialog::GetFirstElement(
              0x1040u, 0x7000u, empty) == 0x7000u &&
              empty.words.at(0x7000u) == 0x4000u,
          "empty tree yields sentinel");
}

void CheckIndividualTree() {
    Runtime runtime(0u, 0x4100u);
    runtime.successors[0x4100u] = 0x4200u;
    runtime.successors[0x4200u] = 0x4000u;
    runtime.remaining_refs = 0u;
    Check(pal4::ScenarioDialog::DestructTree(0x1040u, runtime) ==
              0xbeefu, "last shared ref frees nil");
    Check(runtime.removed ==
              (std::vector<std::uint32_t>{0x4100u, 0x4200u}) &&
              runtime.freed ==
              (std::vector<std::uint32_t>{0x4000u, 0x5000u}) &&
              runtime.nil == 0u,
          "successor captured before each removal and nil released");
    Runtime empty(0u);
    empty.remaining_refs = 0u;
    empty.nil = 0u;
    Check(pal4::ScenarioDialog::DestructTree(0x1040u, empty) ==
              0u && empty.removed.empty() &&
              empty.freed == std::vector<std::uint32_t>{0x4000u},
          "empty tree and empty shared nil");
}

void CheckDestructAndDelete() {
    for (const std::uint8_t flags : {0u, 1u, 2u, 3u}) {
        Runtime runtime(1u);
        Check(pal4::ScenarioDialog::DeletingDestruct(
                  0x1000u, flags, runtime) == 0x1000u,
              "deleting destructor returns original object");
        Check(runtime.words.at(0x1000u) ==
                  pal4::ScenarioDialog::kVtable &&
                  runtime.words.at(0x1028u) ==
                  pal4::ScenarioDialog::kTreeVtable &&
                  runtime.calls[runtime.calls.size() -
                      ((flags & 1u) ? 2u : 1u)] == "destroy-base",
              "vtable restore and base destruction");
        Check(runtime.freed == ((flags & 1u)
                  ? std::vector<std::uint32_t>{0x4000u, 0x1000u}
                  : std::vector<std::uint32_t>{0x4000u}),
              "only flag bit zero frees object");
    }
    Runtime direct(0u);
    Check(pal4::ScenarioDialog::Destruct(0x1000u, direct) ==
              0xd00du, "ordinary destructor forwards base return");
}

void CheckExceptionCleanup() {
    Runtime runtime(1u);
    runtime.throw_in_free_nodes = true;
    bool threw = false;
    try {
        (void)pal4::ScenarioDialog::Destruct(0x1000u, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "free-nodes";
    }
    Check(threw && runtime.calls.back() == "destroy-base",
          "tree failure still destroys base");
}

}  // namespace

int main() {
    try {
        CheckBulkTree();
        CheckFirstElement();
        CheckIndividualTree();
        CheckDestructAndDelete();
        CheckExceptionCleanup();
        std::cout << "scenario_dialog_lifecycle_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
