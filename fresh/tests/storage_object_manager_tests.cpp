#include "pal4/storage_object_manager.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IStorageObjectRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::set<std::uint32_t> freed;
    std::uint32_t next = 0x1000u;
    std::uint32_t nil = 0u;
    std::uint32_t users = 0u;
    std::uint32_t allocation_count = 0u;
    std::uint32_t fail_allocation = 0u;
    std::uint32_t registrations = 0u;
    std::uint32_t invoked = 0u;
    std::uint32_t invoked_head = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return static_cast<std::uint8_t>(
            words[address & ~3u] >> ((address & 3u) * 8u));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        const auto slot = address & ~3u;
        const auto shift = (address & 3u) * 8u;
        words[slot] = (words[slot] & ~(0xffu << shift)) |
                      (std::uint32_t{value} << shift);
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t value) override { nil = value; }
    std::uint32_t NilUsers() override { return users; }
    void SetNilUsers(std::uint32_t value) override { users = value; }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 20u || size == 24u, "allocation size");
        if (++allocation_count == fail_allocation) return 0u;
        const auto address = next;
        next += 0x100u;
        return address;
    }
    void FreeMemory(std::uint32_t address) override {
        Check(address != 0u, "free zero");
        Check(freed.insert(address).second, "double free");
    }
    std::uint32_t SingletonManager() override { return 0x1234u; }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) override {
        Check(manager == 0x1234u && name == "storageObjectMgr" &&
                  id == 1050u && factory == 0x5b4f30u &&
                  destructor == 0x5b4f90u, "singleton registration");
        ++registrations;
        return 0x9876u;
    }
    std::uint8_t ConstructorStackByte(std::uint32_t offset) override {
        Check(offset == 0u, "constructor stack byte offset");
        return 0x7eu;
    }
    std::uint32_t InvokeCallback(std::uint32_t callback,
                                  std::uint32_t head) override {
        invoked = callback;
        invoked_head = head;
        return callback + 1u;
    }
};

int BlackHeight(std::uint32_t node, std::uint32_t parent,
                const std::set<int>& expected, std::vector<int>& keys,
                Runtime& runtime) {
    if (node == runtime.nil) return 1;
    Check(runtime.ReadWord(node + 4u) == parent, "parent link");
    const auto left = runtime.ReadWord(node);
    const auto right = runtime.ReadWord(node + 8u);
    const auto colour = runtime.ReadWord(node + 20u);
    Check(colour <= 1u, "node colour");
    if (colour == 0u) {
        Check(runtime.ReadWord(left + 20u) == 1u &&
                  runtime.ReadWord(right + 20u) == 1u, "red child");
    }
    const auto left_height = BlackHeight(left, node, expected, keys, runtime);
    keys.push_back(static_cast<int>(runtime.ReadWord(node + 12u)));
    const auto right_height = BlackHeight(right, node, expected, keys, runtime);
    Check(left_height == right_height, "black height");
    return left_height + (colour == 1u ? 1 : 0);
}

void CheckTree(std::uint32_t object, const std::set<int>& expected,
               Runtime& runtime) {
    const auto tree = object + 4u;
    const auto head = runtime.ReadWord(object + 8u);
    Check(runtime.ReadWord(object + 16u) == expected.size(), "tree size");
    std::vector<int> keys;
    const auto root = runtime.ReadWord(head + 4u);
    if (expected.empty()) {
        Check(root == runtime.nil && runtime.ReadWord(head) == head &&
                  runtime.ReadWord(head + 8u) == head, "empty header");
    } else {
        Check(runtime.ReadWord(root + 20u) == 1u, "black root");
        (void)BlackHeight(root, head, expected, keys, runtime);
        Check(runtime.ReadWord(head) != head &&
                  runtime.ReadWord(head + 8u) != head, "header extrema");
        Check(static_cast<int>(runtime.ReadWord(runtime.ReadWord(head) + 12u)) ==
                  *expected.begin(), "minimum key");
        Check(static_cast<int>(runtime.ReadWord(
                  runtime.ReadWord(head + 8u) + 12u)) == *expected.rbegin(),
              "maximum key");
    }
    Check(std::equal(keys.begin(), keys.end(), expected.begin(), expected.end()),
          "sorted keys");
    for (int key : expected) {
        const auto node = pal4::StorageObjectManager::LowerBound(tree, key, runtime);
        Check(node != head && static_cast<int>(runtime.ReadWord(node + 12u)) == key,
              "lower bound present");
    }
}

void RegistrationAndLifetime() {
    Runtime runtime;
    pal4::StorageObjectRegistration registration{0xaau, 0u};
    Check(pal4::StorageObjectManager::Register(registration, runtime) ==
              0x9876u, "registration result");
    Check(registration.guard == 0xabu && runtime.registrations == 1u,
          "registration guard");
    Check(pal4::StorageObjectManager::Register(registration, runtime) ==
              0x9876u && runtime.registrations == 1u, "cached registration");

    const auto first = pal4::StorageObjectManager::Create(runtime);
    const auto nil = runtime.nil;
    const auto second = pal4::StorageObjectManager::Create(runtime);
    Check(first != 0u && second != 0u && runtime.users == 2u &&
              runtime.nil == nil, "shared nil");
    Check(runtime.ReadWord(first) == 0x84941cu &&
              runtime.ReadByte(first + 4u) == 0x7eu &&
              runtime.ReadByte(first + 5u) == 0x7eu &&
              runtime.ReadByte(first + 12u) == 0u, "object layout");
    CheckTree(first, {}, runtime);
    pal4::StorageObjectManager::Destroy(first, runtime);
    Check(runtime.users == 1u && runtime.nil == nil &&
              !runtime.freed.contains(nil), "first release");
    Check(pal4::StorageObjectManager::DestroyWithFlags(second, 0u, runtime) ==
              second && !runtime.freed.contains(second), "nondeleting thunk");
    runtime.FreeMemory(second);
    Check(runtime.users == 0u && runtime.nil == 0u &&
              runtime.freed.contains(nil), "last release");
    const auto third = pal4::StorageObjectManager::Create(runtime);
    Check(pal4::StorageObjectManager::DestroyWithFlags(third, 3u, runtime) ==
              third && runtime.freed.contains(third), "deleting thunk");
    pal4::StorageObjectManager::Destroy(0u, runtime);
}

void AllocationFailures() {
    Runtime body_failure;
    body_failure.fail_allocation = 1u;
    Check(pal4::StorageObjectManager::Create(body_failure) == 0u,
          "body allocation failure");
    Runtime nil_failure;
    nil_failure.fail_allocation = 2u;
    try {
        (void)pal4::StorageObjectManager::Create(nil_failure);
        Check(false, "nil allocation must throw");
    } catch (const std::bad_alloc&) {
        Check(nil_failure.users == 0u && nil_failure.nil == 0u &&
                  nil_failure.freed.contains(0x1000u), "nil rollback");
    }
    Runtime head_failure;
    head_failure.fail_allocation = 3u;
    try {
        (void)pal4::StorageObjectManager::Create(head_failure);
        Check(false, "head allocation must throw");
    } catch (const std::bad_alloc&) {
        Check(head_failure.users == 0u && head_failure.nil == 0u &&
                  head_failure.freed.size() == 2u, "head rollback");
    }
}

void OrderedTreeAndCallbacks() {
    Runtime runtime;
    const auto object = pal4::StorageObjectManager::Create(runtime);
    const auto head = runtime.ReadWord(object + 8u);
    Check(pal4::StorageObjectManager::FindAndInvoke(object, 4, runtime) == 0u,
          "empty lookup");
    std::vector<int> insertion(100);
    std::iota(insertion.begin(), insertion.end(), -50);
    std::mt19937 generator(0x5b5110u);
    std::shuffle(insertion.begin(), insertion.end(), generator);
    std::set<int> expected;
    for (int key : insertion) {
        Check(pal4::StorageObjectManager::Insert(
                  object, key, static_cast<std::uint32_t>(key + 1000), runtime),
              "insert unique");
        expected.insert(key);
        CheckTree(object, expected, runtime);
    }
    Check(!pal4::StorageObjectManager::Insert(object, 0, 9999u, runtime),
          "reject duplicate");
    CheckTree(object, expected, runtime);
    for (int key : expected) {
        const auto callback = static_cast<std::uint32_t>(key + 1000);
        Check(pal4::StorageObjectManager::FindAndInvoke(object, key, runtime) ==
                  callback + 1u && runtime.invoked == callback &&
                  runtime.invoked_head == head, "callback lookup");
    }
    Check(pal4::StorageObjectManager::FindAndInvoke(object, 51, runtime) == 0u,
          "missing callback");
    runtime.WriteByte(object + 12u, 1u);
    Check(pal4::StorageObjectManager::Insert(object, 0, 9000u, runtime),
          "duplicate permitted by flag");
    Check(runtime.ReadWord(object + 16u) == 101u, "duplicate count");
    pal4::StorageObjectManager::Destroy(object, runtime);
    Check(runtime.users == 0u && runtime.nil == 0u, "tree release");
}

void EraseAndIterators() {
    Runtime runtime;
    const auto object = pal4::StorageObjectManager::Create(runtime);
    const auto tree = object + 4u;
    const auto head = runtime.ReadWord(object + 8u);
    std::set<int> expected;
    for (int key : {10, 5, 15, 2, 7, 12, 18, 1, 3, 6, 8, 11, 13, 17, 19}) {
        Check(pal4::StorageObjectManager::Insert(object, key, 100u + key,
                                                 runtime), "setup insert");
        expected.insert(key);
    }
    auto first = runtime.ReadWord(head);
    Check(static_cast<int>(runtime.ReadWord(
              pal4::StorageObjectManager::Successor(first, runtime) + 12u)) == 2,
          "successor");
    Check(pal4::StorageObjectManager::Successor(
              runtime.ReadWord(head + 8u), runtime) == head, "end successor");
    Check(static_cast<int>(runtime.ReadWord(
              pal4::StorageObjectManager::Predecessor(
                  runtime.ReadWord(head + 8u), runtime) + 12u)) == 18,
          "predecessor");
    const auto output = 0x80u;
    for (int key : {10, 1, 19, 7, 5, 15, 12, 6, 8, 2, 3, 11, 13, 17, 18}) {
        const auto node = pal4::StorageObjectManager::LowerBound(tree, key,
                                                                  runtime);
        Check(node != head, "erase target");
        Check(pal4::StorageObjectManager::Erase(tree, output, node, runtime) ==
                  output, "erase output address");
        expected.erase(key);
        CheckTree(object, expected, runtime);
    }
    pal4::StorageObjectManager::Destroy(object, runtime);
}

}  // namespace

int main() {
    try {
        RegistrationAndLifetime();
        AllocationFailures();
        OrderedTreeAndCallbacks();
        EraseAndIterators();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
