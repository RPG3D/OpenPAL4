#include "pal4/nil_string_data_object.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::INilStringDataRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t size) override {
        events.push_back("alloc:" + std::to_string(size));
        return allocation_result;
    }
    void Free(const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        return memory[address];
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
        memory[address] = value;
    }
    std::uint32_t InvokeFactory(const std::uint32_t table_address) override {
        events.push_back("factory:" + std::to_string(table_address));
        if (throw_from_factory) throw std::runtime_error("factory");
        return payload_result;
    }
    std::uint32_t InvokeInsert(std::uint32_t, std::uint32_t,
                               std::uint32_t, std::uint32_t) override {
        return 0;
    }
    void InvokeObject(const std::uint32_t table_address,
                      const std::uint32_t object) override {
        events.push_back("invoke:" + std::to_string(table_address) + ":" +
                         std::to_string(object));
    }
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> events;
    std::uint32_t allocation_result = 0x6000;
    std::uint32_t payload_result = 0x7000;
    bool throw_from_factory = false;
};

void InitializeRegistersCallbacksAndCreatesCircularSentinel() {
    pal4::NilStringDataObject object;
    object.Collection().fields.fill(0xffffffffU);
    object.SetTrailingOwnedObject(0x8888);
    FakeRuntime runtime;
    Check(&object.Initialize(0x3000, runtime) == &object,
          "constructor returns its object");
    const auto& collection = object.Collection();
    Check(collection.Read(0) == pal4::NilStringDataObject::kAllocateCallback &&
              collection.Read(4) == pal4::NilStringDataObject::kDestroyCallback &&
              collection.Read(8) == pal4::NilStringDataObject::kInitializeBuffersCallback,
          "constructor registers allocator, destructor, and string initializer callbacks");
    Check(collection.Read(0x0c) == 0 && collection.Read(0x10) == 0 &&
              collection.Read(0x14) == 0 && collection.Read(0x18) == 0 &&
              collection.Read(0x1c) == 0 && collection.Read(0x20) == 0x6000 &&
              collection.Read(0x24) == 0x6000 &&
              object.TrailingOwnedObject() == 0,
          "constructor clears object state and installs the sentinel");
    Check(runtime.memory[0x6000] == 0x6000 &&
              runtime.memory[0x6004] == 0x6000 &&
              runtime.memory[0x6008] == 0x7000 &&
              runtime.memory[0x600c] == 0x3000,
          "sentinel links and callback payload are initialized");
    Check(runtime.events[0] == "alloc:16" &&
              runtime.events[4] == "factory:12288",
          "empty collection clear leaves sentinel setup order intact");
}

void AllocationFailureSetsCollectionError() {
    pal4::NilStringDataObject object;
    FakeRuntime runtime;
    runtime.allocation_result = 0;
    static_cast<void>(object.Initialize(0x3000, runtime));
    Check(object.Collection().Read(0x0c) == 1 &&
              object.Collection().Read(0x20) == 0 &&
              object.Collection().Read(0x24) == 0,
          "sentinel allocation failure sets error and leaves links empty");
    Check(runtime.events == std::vector<std::string>{"alloc:16"},
          "allocation failure does not invoke the object factory");
}

void ExistingSentinelAvoidsReinitialization() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x24, 0x5000);
    FakeRuntime runtime;
    Check(pal4::NilStringDataObject::InitializeSentinel(collection, 0x3000,
                                                        runtime),
          "existing sentinel is already initialized");
    Check(runtime.events.empty() && collection.Read(0x24) == 0x5000,
          "existing sentinel returns without allocations or callbacks");
}

void FactoryExceptionFreesTemporarySentinelAndPropagates() {
    pal4::CallbackCollectionState collection;
    FakeRuntime runtime;
    runtime.throw_from_factory = true;
    bool propagated = false;
    try {
        static_cast<void>(pal4::NilStringDataObject::InitializeSentinel(
            collection, 0x3000, runtime));
    } catch (const std::runtime_error&) {
        propagated = true;
    }
    Check(propagated && runtime.events.back() == "free:24576",
          "factory exception frees the allocated node and propagates");
    Check(collection.Read(0x24) == 0,
          "failed construction does not publish a partial sentinel");
}

void DestroyComposesCollectionAndStorageDestruction() {
    pal4::NilStringDataObject object;
    auto& collection = object.Collection();
    collection.Write(0x20, 0x7777);
    collection.Write(0x24, 0x5000);
    collection.Write(0x18, 1);
    object.SetTrailingOwnedObject(0x8000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x6000;
    runtime.memory[0x6004] = 0x5000;
    runtime.memory[0x6008] = 0xaaaa;
    runtime.memory[0x600c] = 0x7000;
    runtime.memory[0x5008] = 0xbbbb;
    runtime.memory[0x500c] = 0x7100;
    object.Destroy(runtime);

    Check(runtime.events == std::vector<std::string>{
              "invoke:28676:43690", "free:24576",
              "write:20480:20480", "write:20484:20480",
              "invoke:28932:48059", "free:20480", "free:32768"},
          "destructor clears elements before sentinel payload and trailing allocation");
    Check(object.TrailingOwnedObject() == 0,
          "destructor clears trailing field");
}

}  // namespace

int main() {
    InitializeRegistersCallbacksAndCreatesCircularSentinel();
    AllocationFailureSetsCollectionError();
    ExistingSentinelAvoidsReinitialization();
    FactoryExceptionFreesTemporarySentinelAndPropagates();
    DestroyComposesCollectionAndStorageDestruction();
    std::cout << "5 nil-string-data object tests passed\n";
}
