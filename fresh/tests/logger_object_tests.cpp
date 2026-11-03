#include "pal4/logger_object.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::ILoggerObjectRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t size) override {
        events.emplace_back("allocate:" + std::to_string(size));
        return allocation;
    }
    void InitializeEmptyPathBuffer(const std::uint32_t object) override {
        events.emplace_back("initialize-empty:" + std::to_string(object));
        if (throw_during_empty_initialize) throw std::runtime_error("empty init");
    }
    void AssignPathBuffer(const std::uint32_t object,
                          const std::string_view path) override {
        events.emplace_back("assign:" + std::to_string(object) + ":" +
                            std::string(path));
        if (throw_during_assign) throw std::runtime_error("assign");
    }
    void CleanupPathBuffer(const std::uint32_t object) override {
        events.emplace_back("cleanup:" + std::to_string(object));
    }
    void Free(const std::uint32_t object) override {
        events.emplace_back("free:" + std::to_string(object));
    }

    std::vector<std::string> events;
    std::uint32_t allocation = 0x5000;
    bool throw_during_empty_initialize = false;
    bool throw_during_assign = false;
};

void FactoryAllocatesTwelveBytesAndInitializesDefaultPath() {
    FakeRuntime runtime;
    Check(pal4::LoggerObject::Create(runtime) == 0x5000,
          "factory returns allocated logger object");
    Check(runtime.events == std::vector<std::string>{
              "allocate:12", "initialize-empty:20480",
              "assign:20480:logFile.txt"},
          "factory constructs an empty buffer before assigning its path");
}

void DefaultPathConstructorReturnsTheObject() {
    FakeRuntime runtime;
    Check(pal4::LoggerObject::InitializeDefaultPath(0x6200, runtime) ==
              0x6200 &&
              runtime.events == std::vector<std::string>{
                                    "initialize-empty:25088",
                                    "assign:25088:logFile.txt"},
          "default-path constructor returns its original object address");
}

void AllocationFailureSkipsPathInitialization() {
    FakeRuntime runtime;
    runtime.allocation = 0;
    Check(pal4::LoggerObject::Create(runtime) == 0,
          "failed allocation returns null");
    Check(runtime.events == std::vector<std::string>{"allocate:12"},
          "failed allocation does not initialize or free an object");
}

void EmptyConstructorExceptionFreesAllocationAndPropagates() {
    FakeRuntime runtime;
    runtime.throw_during_empty_initialize = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::LoggerObject::Create(runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw, "construction exception propagates");
    Check(runtime.events == std::vector<std::string>{
              "allocate:12", "initialize-empty:20480", "free:20480"},
          "empty-construction failure frees the unconstructed allocation");
}

void PathAssignmentExceptionCleansBufferThenFreesAllocation() {
    FakeRuntime runtime;
    runtime.throw_during_assign = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::LoggerObject::Create(runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "allocate:12", "initialize-empty:20480",
              "assign:20480:logFile.txt", "cleanup:20480", "free:20480"},
          "assignment failure cleans constructed string before factory free");
}

void DestructorCleansPathBeforeFreeAndAcceptsNull() {
    FakeRuntime runtime;
    pal4::LoggerObject::Destroy(0, runtime);
    Check(runtime.events.empty(), "null destructor does nothing");
    pal4::LoggerObject::Destroy(0x5000, runtime);
    Check(runtime.events == std::vector<std::string>{
              "cleanup:20480", "free:20480"},
          "destructor cleans the embedded string before freeing the object");
}

}  // namespace

int main() {
    FactoryAllocatesTwelveBytesAndInitializesDefaultPath();
    DefaultPathConstructorReturnsTheObject();
    AllocationFailureSkipsPathInitialization();
    EmptyConstructorExceptionFreesAllocationAndPropagates();
    PathAssignmentExceptionCleansBufferThenFreesAllocation();
    DestructorCleansPathBeforeFreeAndAcceptsNull();
    std::cout << "6 logger-object test cases passed\n";
}
