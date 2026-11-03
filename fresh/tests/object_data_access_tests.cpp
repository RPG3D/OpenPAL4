#include "pal4/object_data_access.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ObjectMethodRuntimeStub {
    std::uint32_t receiver{};
    std::uint32_t slot{};
    std::uint32_t context{};
    std::int32_t result{-31};

    std::int32_t CallVirtual(std::uint32_t received,
                             std::uint32_t offset,
                             std::uint32_t argument) override {
        receiver = received;
        slot = offset;
        context = argument;
        return result;
    }
};

void TestForwarders() {
    Runtime runtime;
    Check(pal4::ObjectMethodForwarder::CallSlot10(
              0x4000u, 0x5000u, runtime) == -31 &&
              runtime.receiver == 0x5000u &&
              runtime.slot == 0x10u && runtime.context == 0x4000u,
          "first forwarder calls receiver slot +0x10 with context");
    runtime.result = 17;
    Check(pal4::ObjectMethodForwarder::CallSlot28(
              0x6000u, 0x7000u, runtime) == 17 &&
              runtime.receiver == 0x7000u &&
              runtime.slot == 0x28u && runtime.context == 0x6000u,
          "second forwarder calls receiver slot +0x28 with context");
}

void TestIndexedData() {
    pal4::IndexedDataRecord record{};
    record.vtable = 0xdeadbeefu;
    for (std::uint32_t index = 0u; index < 8u; ++index)
        record.values[index] = 100u + index;
    record.ResetVtable();
    Check(record.vtable == 0x842bd4u,
          "record vtable is restored");
    for (std::uint32_t index = 0u; index < 8u; ++index)
        Check(record.GetElement(index) == 100u + index,
              "valid index reads corresponding inline value");
    Check(record.GetElement(8u) == 8u &&
              record.GetElement(0xffffffffu) == 0xffffffffu,
          "invalid unsigned and negative-bit-pattern indexes return themselves");
}

}  // namespace

int main() {
    TestForwarders();
    TestIndexedData();
}
