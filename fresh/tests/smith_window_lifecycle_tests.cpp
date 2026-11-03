#include "pal4/smith_window.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowDestructionRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0xdcu, 0x5001u},
        {object + 0xe0u, 3u}, {object + 0xe4u, 8u},
        {object + 0xccu, 0x6000u},
        {object + 0xd0u, 4u}, {object + 0xd4u, 8u},
        {object + 0xbcu, 0x7000u},
        {object + 0xc0u, 5u}, {object + 0xc4u, 8u},
        {object + 0x30u, 0x8000u},
        {object + 0x34u, 6u}, {object + 0x38u, 8u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{0x5000u, 2u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x5000u, "reference header address");
        calls.emplace_back("read-byte");
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(value == 0u, "destructor clears text state");
        words[address] = value;
        calls.emplace_back("write-word");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x5000u, "reference decrement address");
        bytes[address] = value;
        calls.emplace_back("write-byte");
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
    void ResetEffectHandle(std::uint32_t address) override {
        Check(address == object + 0xa4u, "effect handle offset");
        calls.emplace_back("reset-effect");
    }
    std::uint32_t DestructBase(std::uint32_t owner) override {
        Check(owner == object, "base destructor owner");
        calls.emplace_back("base");
        return 0x55u;
    }
};

void CheckReferenceCases() {
    for (const auto reference : {0u, 0xffu, 2u}) {
        Runtime runtime;
        runtime.bytes[0x5000u] = static_cast<std::uint8_t>(reference);
        Check(pal4::SmithWindow::Destruct(Runtime::object, runtime) ==
                  0x55u && runtime.calls.back() == "base" &&
                  runtime.freed ==
                      (reference == 2u
                          ? std::vector<std::uint32_t>{
                                0x6000u, 0x7000u, 0x8000u}
                          : std::vector<std::uint32_t>{
                                0x5000u, 0x6000u, 0x7000u, 0x8000u}),
              "reference byte controls shared text release");
        if (reference == 2u)
            Check(runtime.bytes.at(0x5000u) == 1u,
                  "shared reference decremented");
        for (const auto offset : {0xdcu, 0xe0u, 0xe4u,
                                  0xccu, 0xd0u, 0xd4u,
                                  0xbcu, 0xc0u, 0xc4u,
                                  0x30u, 0x34u, 0x38u})
            Check(runtime.words.at(Runtime::object + offset) == 0u,
                  "all text and adapter fields cleared");
        const auto reset = std::find(runtime.calls.begin(),
                                     runtime.calls.end(), "reset-effect");
        Check(reset != runtime.calls.end() &&
                  std::find(reset, runtime.calls.end(), "free") !=
                      runtime.calls.end(),
              "effect reset precedes adapter free");
    }
}

void CheckNullFields() {
    Runtime runtime;
    runtime.words[Runtime::object + 0xdcu] = 0u;
    runtime.words[Runtime::object + 0xccu] = 0u;
    runtime.words[Runtime::object + 0xbcu] = 0u;
    runtime.words[Runtime::object + 0x30u] = 0u;
    (void)pal4::SmithWindow::Destruct(Runtime::object, runtime);
    Check(runtime.freed ==
              (std::vector<std::uint32_t>{0u, 0u, 0u}) &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "read-byte") == runtime.calls.end(),
          "null shared text skips header; three ordinary frees still called");
}

}  // namespace

int main() {
    try {
        CheckReferenceCases();
        CheckNullFields();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
