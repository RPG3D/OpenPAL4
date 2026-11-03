#include "pal4/id_generator_registry.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IIdGeneratorRegistryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::uint32_t registry = 0x2000u;
    std::uint32_t next_address = 0xa000u;
    std::uint32_t initialized_object = 0u;
    bool fail_allocation = false;

    std::uint32_t InitializeRegistry(std::uint32_t object) override {
        initialized_object = object;
        return registry;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (fail_allocation) return 0u;
        const auto result = next_address;
        next_address += 0x100u;
        return result;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
};

void CheckSpareCapacity() {
    Runtime runtime;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2008u] = 0x3000u;
    runtime.words[0x200cu] = 0x3010u;
    Check(pal4::IdGeneratorRegistry::Register(
              0x1100u, runtime) == 0x3008u &&
              runtime.initialized_object == 0x1100u &&
              runtime.words[0x2008u] == 0x3008u &&
              runtime.words[0x200cu] == 0x3010u &&
              runtime.words[0x3000u] ==
                  pal4::IdGeneratorRegistry::kNameAddress &&
              runtime.words[0x3004u] ==
                  pal4::IdGeneratorRegistry::kFactoryAddress &&
              runtime.allocations.empty() && runtime.freed.empty(),
          "spare capacity appends pair and returns new end address");
}

void CheckGrowthAndThunk() {
    Runtime runtime;
    Check(pal4::IdGeneratorRegistry::RegisterViaThunk(
              0x1200u, runtime) == 0u &&
              runtime.initialized_object == 0x1200u &&
              runtime.words[0x2004u] == 0xa000u &&
              runtime.words[0x2008u] == 0xa008u &&
              runtime.words[0x200cu] == 0xa008u &&
              runtime.words[0xa000u] ==
                  pal4::IdGeneratorRegistry::kNameAddress &&
              runtime.words[0xa004u] ==
                  pal4::IdGeneratorRegistry::kFactoryAddress &&
              runtime.allocations == std::vector<std::uint32_t>{8u} &&
              runtime.freed == std::vector<std::uint32_t>{0u},
          "empty registry allocates one pair and returns old count");
    Check(pal4::IdGeneratorRegistry::Register(
              0x1200u, runtime) == 1u &&
              runtime.words[0x2004u] == 0xa100u &&
              runtime.words[0x2008u] == 0xa110u &&
              runtime.words[0x200cu] == 0xa110u &&
              runtime.words[0xa100u] ==
                  pal4::IdGeneratorRegistry::kNameAddress &&
              runtime.words[0xa104u] ==
                  pal4::IdGeneratorRegistry::kFactoryAddress &&
              runtime.words[0xa108u] ==
                  pal4::IdGeneratorRegistry::kNameAddress &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{8u, 16u} &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0u, 0xa000u},
          "one-element growth copies old pair and appends another");
    Check(pal4::IdGeneratorRegistry::Register(
              0x1200u, runtime) == 2u &&
              runtime.words[0x2004u] == 0xa200u &&
              runtime.words[0x2008u] == 0xa218u &&
              runtime.words[0x200cu] == 0xa220u &&
              runtime.words[0xa200u] ==
                  pal4::IdGeneratorRegistry::kNameAddress &&
              runtime.words[0xa208u] ==
                  pal4::IdGeneratorRegistry::kNameAddress &&
              runtime.words[0xa210u] ==
                  pal4::IdGeneratorRegistry::kNameAddress &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{8u, 16u, 32u} &&
              runtime.freed.back() == 0xa100u,
          "two-element growth doubles capacity and returns old count");
    Check(pal4::IdGeneratorRegistry::Register(
              0x1200u, runtime) == 0xa220u &&
              runtime.words[0x2008u] == 0xa220u &&
              runtime.allocations.size() == 3u,
          "new spare slot is filled without another allocation");
}

void CheckAllocationFailure() {
    Runtime runtime;
    runtime.fail_allocation = true;
    bool threw = false;
    try {
        (void)pal4::IdGeneratorRegistry::Register(0x1200u, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.allocations ==
                       std::vector<std::uint32_t>{8u} &&
              runtime.freed.empty() && runtime.words[0x2004u] == 0u &&
              runtime.words[0x2008u] == 0u,
          "allocation failure leaves registry fields unchanged");
}

void CheckFactoryAndSize() {
    Runtime runtime;
    Check(pal4::IdGeneratorRegistry::CreateFactoryObject(runtime) ==
              0xa000u &&
              runtime.allocations == std::vector<std::uint32_t>{8u} &&
              runtime.words[0xa000u] == 0x84346cu &&
              runtime.words[0xa004u] == 1u,
          "factory allocates eight bytes and initializes its two words");
    runtime.fail_allocation = true;
    Check(pal4::IdGeneratorRegistry::CreateFactoryObject(runtime) == 0u &&
              runtime.allocations.size() == 2u,
          "factory returns null without writes on allocation failure");

    Check(pal4::IdGeneratorRegistry::Size(0x2000u, runtime) == 0,
          "null array has zero size");
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2008u] = 0x3018u;
    Check(pal4::IdGeneratorRegistry::Size(0x2000u, runtime) == 3,
          "size divides positive byte span by eight");
    runtime.words[0x2008u] = 0x2ff8u;
    Check(pal4::IdGeneratorRegistry::Size(0x2000u, runtime) == -1,
          "size uses signed arithmetic shift on negative span");
}

}  // namespace

int main() {
    try {
        CheckSpareCapacity();
        CheckGrowthAndThunk();
        CheckAllocationFailure();
        CheckFactoryAndSize();
        std::cout << "id_generator_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
