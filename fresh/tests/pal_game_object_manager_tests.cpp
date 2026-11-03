#include "pal4/pal_game_object_manager.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameObjectManagerRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::int32_t, std::uint32_t> registered_factories;
    std::uint32_t allocation = object;
    std::uint32_t registration_result = 0x2000u;
    bool fail_first = false;
    bool fail_second = false;

    std::uint32_t SingletonManager() override {
        calls.emplace_back("singleton manager");
        return 0x3000u;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) override {
        Check(manager == 0x3000u && name == "palGameObjectManager" &&
              id == 1045u && factory == 0x5a6d40u &&
              destructor == 0x5a6da0u, "singleton registration arguments");
        calls.emplace_back("register");
        return registration_result;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::PalGameObjectManager::kSize,
              "manager allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t Free(std::uint32_t address) override {
        Check(address == object, "manager free address");
        calls.emplace_back("free");
        return 0x77u;
    }
    void InitializeTree(std::uint32_t tree,
                        std::uint32_t node_size) override {
        if (tree == object && node_size == 0x24u) {
            calls.emplace_back("init object tree");
            if (fail_first) throw std::runtime_error("first tree failed");
        } else if (tree == object + 0x14u && node_size == 0x18u) {
            calls.emplace_back("init dialog tree");
            if (fail_second) throw std::runtime_error("second tree failed");
        } else {
            throw std::runtime_error("tree initialization arguments");
        }
    }
    std::uint32_t DestroyTree(std::uint32_t tree,
                              std::uint32_t node_size) override {
        if (tree == object && node_size == 0x24u) {
            calls.emplace_back("destroy object tree");
            return 0x44u;
        }
        if (tree == object + 0x14u && node_size == 0x18u) {
            calls.emplace_back("destroy dialog tree");
            return 0x55u;
        }
        throw std::runtime_error("tree destruction arguments");
    }
    bool InsertFactory(std::uint32_t tree, std::int32_t type,
                       std::uint32_t factory) override {
        Check(tree == object + 0x14u, "factory tree offset");
        calls.emplace_back("insert factory " + std::to_string(type));
        return registered_factories.emplace(type, factory).second;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == object + 0x10u &&
              value == pal4::PalGameObjectManager::kVtable,
              "manager vtable address");
        words[address] = value;
        calls.emplace_back("vtable");
    }
};

void CheckSingleton() {
    Runtime runtime;
    pal4::PalGameObjectManagerSingletonState state;
    Check(pal4::PalGameObjectManager::Get(state, runtime) == 0x2000u &&
          state.guard == 1u && state.instance == 0x2000u &&
          runtime.calls == (std::vector<std::string>{
              "singleton manager", "register"}), "singleton registration");
    runtime.registration_result = 0x4000u;
    Check(pal4::PalGameObjectManager::Get(state, runtime) == 0x2000u &&
          runtime.calls.size() == 2u, "singleton guard caches result");
    pal4::PalGameObjectManagerSingletonState guarded{0x80u, 0u};
    runtime.calls.clear();
    Check(pal4::PalGameObjectManager::Get(guarded, runtime) == 0x4000u &&
          guarded.guard == 0x81u && runtime.calls.size() == 2u,
          "singleton guard preserves other bits");
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::PalGameObjectManager::Create(runtime) == Runtime::object &&
          runtime.words[Runtime::object + 0x10u] ==
              pal4::PalGameObjectManager::kVtable &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "init object tree", "init dialog tree",
              "vtable"}), "manager constructor order");
    runtime.calls.clear();
    Check(pal4::PalGameObjectManager::Destroy(
              Runtime::object, runtime) == 0x77u &&
          runtime.calls == (std::vector<std::string>{
              "vtable", "destroy dialog tree",
              "destroy object tree", "free"}),
          "manager destructor order and free result");
    runtime.calls.clear();
    Check(pal4::PalGameObjectManager::CleanupObjectTree(
              Runtime::object, runtime) == 0x44u &&
          runtime.calls == (std::vector<std::string>{
              "destroy object tree"}), "object tree cleanup entry");
    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::PalGameObjectManager::Create(empty) == 0u &&
          empty.calls == (std::vector<std::string>{"allocate"}),
          "manager allocation failure");
    Check(pal4::PalGameObjectManager::Destroy(0u, empty) == 0u &&
          empty.calls.size() == 1u, "null manager destruction");
}

void CheckConstructionFailures() {
    Runtime first;
    first.fail_first = true;
    try {
        static_cast<void>(pal4::PalGameObjectManager::Create(first));
        throw std::runtime_error("missing first failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "first tree failed" &&
              first.calls == (std::vector<std::string>{
                  "allocate", "init object tree", "free"}),
              "first tree exception cleanup");
    }
    Runtime second;
    second.fail_second = true;
    try {
        static_cast<void>(pal4::PalGameObjectManager::Create(second));
        throw std::runtime_error("missing second failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "second tree failed" &&
              second.calls == (std::vector<std::string>{
                  "allocate", "init object tree", "init dialog tree",
                  "destroy object tree", "free"}),
              "second tree exception cleanup");
    }
}

void CheckFactoryTree() {
    Runtime runtime;
    Check(pal4::PalGameObjectManager::RegisterFactory(
              Runtime::object, -1, 0x3100u, runtime) &&
          pal4::PalGameObjectManager::RegisterFactory(
              Runtime::object, 5, 0x3200u, runtime) &&
          !pal4::PalGameObjectManager::RegisterFactory(
              Runtime::object, 5, 0x3300u, runtime) &&
          runtime.registered_factories[-1] == 0x3100u &&
          runtime.registered_factories[5] == 0x3200u &&
          runtime.calls == (std::vector<std::string>{
              "insert factory -1", "insert factory 5",
              "insert factory 5"}),
          "signed factory key and duplicate rejection");
    runtime.calls.clear();
    Check(pal4::PalGameObjectManager::CleanupFactoryTree(
              Runtime::object, runtime) == 0x55u &&
          runtime.calls == (std::vector<std::string>{
              "vtable", "destroy dialog tree"}),
          "independent factory-tree destructor");
}

}  // namespace

int main() {
    try {
        CheckSingleton();
        CheckLifecycle();
        CheckConstructionFailures();
        CheckFactoryTree();
        std::cout << "pal_game_object_manager_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
