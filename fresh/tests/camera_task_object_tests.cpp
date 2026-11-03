#include "pal4/camera_task_object.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICameraTaskFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{0x2034u, 0x87654321u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocation = 0x1000u;
    bool throw_in_construct = false;

    void BaseConstruct(std::uint32_t object) override {
        Check(object == 0x1000u, "base construct address");
        calls.emplace_back("base construct");
        if (throw_in_construct) throw std::runtime_error("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t object) override {
        Check(object == 0x1000u, "base destruct address");
        calls.emplace_back("base destruct");
        return 0x4321u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x2034u, "camera record field13 offset");
        calls.emplace_back("read field13");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte " + std::to_string(address));
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 0x98u, "camera task allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::CameraTaskObject::Construct(0x1000u, runtime) ==
                  0x1000u &&
                  runtime.calls.front() == "base construct" &&
                  runtime.words.at(0x1000u) == 0x8486a0u &&
                  runtime.bytes.at(0x103cu) == 0u &&
                  runtime.words.at(0x1040u) == 0u &&
                  runtime.words.at(0x1044u) == 0u &&
                  runtime.words.at(0x1048u) == 0u &&
                  runtime.words.at(0x104cu) == 0u,
              "constructor initializes camera task fields");
        runtime.calls.clear();
        Check(pal4::CameraTaskObject::Configure(
                  0x1000u, 0xaabbu, 0x2000u, runtime) == 0u &&
                  runtime.words.at(0x1048u) == 0xaabbu &&
                  runtime.words.at(0x104cu) == 0x2000u &&
                  runtime.words.at(0x1044u) == 0x87654321u &&
                  runtime.bytes.at(0x103cu) == 0u &&
                  runtime.words.at(0x1040u) == 0u &&
                  runtime.calls ==
                      (std::vector<std::string>{
                          "write 4168", "read field13", "write 4172",
                          "write 4164", "byte 4156", "write 4160"}),
              "configure copies camera field13 and resets state in order");
        runtime.calls.clear();
        Check(pal4::CameraTaskObject::Destruct(0x1000u, runtime) ==
                  0x4321u &&
                  runtime.calls ==
                      (std::vector<std::string>{"write 4096",
                                                "base destruct"}),
              "destructor restores vtable before base teardown");

        Runtime factory;
        Check(pal4::CameraTaskFactory::Create(factory) == 0x1000u &&
                  factory.calls.front() == "allocate" &&
                  factory.calls[1] == "base construct" &&
                  factory.words.at(0x1000u) == 0x8486a0u,
              "camera task factory allocates and constructs");
        Runtime null_allocation;
        null_allocation.allocation = 0u;
        Check(pal4::CameraTaskFactory::Create(null_allocation) == 0u &&
                  null_allocation.calls ==
                      (std::vector<std::string>{"allocate"}),
              "camera task factory handles null allocation");
        Runtime constructor_failure;
        constructor_failure.throw_in_construct = true;
        bool thrown = false;
        try {
            (void)pal4::CameraTaskFactory::Create(constructor_failure);
        } catch (const std::runtime_error&) {
            thrown = true;
        }
        Check(thrown && constructor_failure.freed ==
                            (std::vector<std::uint32_t>{0x1000u}) &&
                  constructor_failure.calls.back() == "free",
              "camera task factory frees failed construction");
        std::cout << "camera_task_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
