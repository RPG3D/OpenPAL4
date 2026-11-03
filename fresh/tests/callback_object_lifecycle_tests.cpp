#include "pal4/callback_object_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void StoreDword(pal4::CallbackState::Storage& storage, const std::size_t offset,
                const std::uint32_t value) {
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        storage[offset + index] = static_cast<std::byte>(
            (value >> (index * 8U)) & 0xffU);
    }
}

std::uint32_t LoadDword(const pal4::CallbackState::Storage& storage,
                        const std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        value |= std::to_integer<std::uint32_t>(storage[offset + index])
                 << (index * 8U);
    }
    return value;
}

class FakeRuntime final : public pal4::ICallbackObjectRuntime {
public:
    [[nodiscard]] pal4::BinaryTreeNode* ResolveNode(
        const std::uint32_t address) override {
        Check(address == root_address && root != nullptr,
              "resolves the original collection root address");
        events.emplace_back("resolve");
        return root;
    }

    void HandleStatusCheck(pal4::BinaryTreeNode* node) override {
        events.emplace_back("status:" + Name(node));
        if (owned_address_after_status != 0) {
            StoreDword(*object_to_update, 0x1c, owned_address_after_status);
        }
        if (unlink_on_status) node->parent = nullptr;
    }

    void FreeNode(pal4::BinaryTreeNode* node) override {
        events.emplace_back("free-node:" + Name(node));
    }

    [[nodiscard]] int Free(const std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
        return free_result;
    }

    [[nodiscard]] std::string Name(pal4::BinaryTreeNode* node) const {
        for (const auto& named : nodes) {
            if (named.first == node) return named.second;
        }
        Check(false, "node belongs to the test tree");
        return {};
    }

    std::uint32_t root_address = 0x1000;
    pal4::BinaryTreeNode* root = nullptr;
    pal4::CallbackState::Storage* object_to_update = nullptr;
    int free_result = 29;
    std::uint32_t owned_address_after_status = 0;
    bool unlink_on_status = false;
    std::vector<std::pair<pal4::BinaryTreeNode*, std::string>> nodes;
    std::vector<std::string> events;
};

void EmptyOwnedFieldOnlyCleansCollection() {
    pal4::CallbackState::Storage object{};
    FakeRuntime runtime;
    Check(pal4::CallbackObjectLifecycle::Destroy(object, runtime) == 0,
          "null owned field returns zero after discarding cleanup result");
    Check(runtime.events.empty(), "null collection and owner need no runtime calls");
}

void OwnedFieldIsReadAfterCollectionCleanup() {
    pal4::CallbackState::Storage object{};
    pal4::BinaryTreeNode node;
    StoreDword(object, 0x10, 0x1000);
    FakeRuntime runtime;
    runtime.root = &node;
    runtime.nodes.emplace_back(&node, "root");
    runtime.object_to_update = &object;
    runtime.owned_address_after_status = 0x1234;
    Check(pal4::CallbackObjectLifecycle::Destroy(object, runtime) == 29,
          "newly published owned field returns the free result");
    Check(runtime.events ==
              std::vector<std::string>{"resolve", "status:root",
                                            "free-node:root", "free:4660"},
          "destructor reads owned field after collection cleanup");
    Check(LoadDword(object, 0x1c) == 0,
          "destructor clears field published during cleanup");
}

void NonemptyOwnedFieldIsFreedAfterCollectionCleanup() {
    pal4::CallbackState::Storage object{};
    StoreDword(object, 0x1c, 0x12345678);
    FakeRuntime runtime;
    Check(pal4::CallbackObjectLifecycle::Destroy(object, runtime) == 29,
          "returns the free result");
    Check(runtime.events == std::vector<std::string>{"free:305419896"},
          "without a collection only the owned field is freed");
    Check(LoadDword(object, 0x1c) == 0,
          "clears the owned field after freeing it");
}

void DestructorThunkForwardsToDestructor() {
    pal4::CallbackState::Storage object{};
    StoreDword(object, 0x1c, 7);
    FakeRuntime runtime;
    Check(pal4::CallbackObjectLifecycle::DestroyThunk(object, runtime) == 29,
          "thunk preserves the destructor return value");
    Check(runtime.events == std::vector<std::string>{"free:7"},
          "thunk forwards to the destructor behavior");
}

void CollectionCleanupVisitsPreferredLeavesBeforeParents() {
    pal4::CallbackState::Storage empty{};
    StoreDword(empty, 0x14, 0x55);
    FakeRuntime empty_runtime;
    Check(pal4::CallbackObjectLifecycle::CleanupCollection(empty, empty_runtime) == 0,
          "null collection root returns the original zero result");
    Check(LoadDword(empty, 0x14) == 0x55,
          "null root skips both state resets");
    Check(empty_runtime.events.empty(), "null root never resolves a node");

    pal4::CallbackState::Storage populated{};
    pal4::BinaryTreeNode root;
    pal4::BinaryTreeNode left;
    pal4::BinaryTreeNode right;
    root.left = &left;
    root.right = &right;
    left.parent = &root;
    right.parent = &root;
    FakeRuntime runtime;
    runtime.root = &root;
    runtime.nodes = {{&root, "root"}, {&left, "left"}, {&right, "right"}};
    runtime.unlink_on_status = true;
    StoreDword(populated, 0x10, runtime.root_address);
    StoreDword(populated, 0x14, 0x5678);
    Check(pal4::CallbackObjectLifecycle::CleanupCollection(populated, runtime) == 1,
          "non-null root reaches the end condition and returns true");
    Check(runtime.events == std::vector<std::string>{
              "resolve", "status:left", "free-node:left", "status:right",
              "free-node:right", "status:root", "free-node:root"},
          "iterator advances before callbacks and frees leaves before root");
    Check(LoadDword(populated, 0x10) == 0 &&
              LoadDword(populated, 0x14) == 0,
          "non-null path clears both collection fields");
}

}  // namespace

int main() {
    EmptyOwnedFieldOnlyCleansCollection();
    OwnedFieldIsReadAfterCollectionCleanup();
    NonemptyOwnedFieldIsFreedAfterCollectionCleanup();
    DestructorThunkForwardsToDestructor();
    CollectionCleanupVisitsPreferredLeavesBeforeParents();
    std::cout << "5 callback-object lifecycle tests passed\n";
}
