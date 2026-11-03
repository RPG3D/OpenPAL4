#include "pal4/callback_string_field.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::ICallbackStringRuntime {
public:
    void Free(const std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }

    [[nodiscard]] std::uint32_t Length(
        const std::uint32_t address) override {
        events.emplace_back("length:" + std::to_string(address));
        return address == source_address ? source_length : result_length;
    }

    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size) override {
        events.emplace_back("allocate:" + std::to_string(size));
        return destination_address;
    }

    void CopyAnsiString(const std::uint32_t destination,
                        const std::uint32_t source) override {
        events.emplace_back("copy:" + std::to_string(destination) + ":" +
                            std::to_string(source));
        if (object_to_update != nullptr) {
            pal4::CallbackState::WriteDword(*object_to_update, 0x1c,
                                            updated_destination);
        }
    }

    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        events.emplace_back("write:" + std::to_string(address) + ":" +
                            std::to_string(value));
    }

    static constexpr std::uint32_t source_address = 0x1000;
    static constexpr std::uint32_t destination_address = 0x2000;
    std::uint32_t source_length = 5;
    std::uint32_t result_length = 5;
    pal4::CallbackState::Storage* object_to_update = nullptr;
    std::uint32_t updated_destination = destination_address;
    std::vector<std::string> events;
};

void ReplacesExistingStringInBinaryCallOrder() {
    pal4::CallbackState::Storage object{};
    pal4::CallbackState::WriteDword(object, 0x1c, 0x3000);
    FakeRuntime runtime;
    const auto result = pal4::CallbackStringField::Set(
        object, FakeRuntime::source_address, runtime);

    Check(result == 5, "returns lstrlenA result for the stored copy");
    Check(pal4::CallbackState::ReadDword(object, 0x1c) ==
              FakeRuntime::destination_address,
          "stores allocated destination in field +0x1c");
    Check(runtime.events == std::vector<std::string>{
                               "free:12288", "length:4096", "allocate:6",
                               "copy:8192:4096", "write:8197:0",
                               "length:8192"},
          "matches free, length, allocation, copy, byte write, and final length order");
}

void NullOldStringSkipsFreeAndReloadsStoredDestination() {
    pal4::CallbackState::Storage object{};
    FakeRuntime runtime;
    runtime.object_to_update = &object;
    runtime.updated_destination = 0xfffffffdu;
    runtime.source_length = 5;
    static_cast<void>(pal4::CallbackStringField::Set(
        object, FakeRuntime::source_address, runtime));
    Check(pal4::CallbackState::ReadDword(object, 0x1c) ==
              runtime.updated_destination,
          "copy callback can update the stored destination");
    Check(runtime.events == std::vector<std::string>{
                               "length:4096", "allocate:6", "copy:8192:4096",
                               "write:2:0", "length:4294967293"},
          "reloads field +0x1c after copy for 32-bit address addition and final length");
}

}  // namespace

int main() {
    ReplacesExistingStringInBinaryCallOrder();
    NullOldStringSkipsFreeAndReloadsStoredDestination();
    std::cout << "2 callback-string tests passed\n";
}
