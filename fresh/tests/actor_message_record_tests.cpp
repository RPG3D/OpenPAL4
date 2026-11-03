#include "pal4/actor_message_record.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IActorMessageRecordRuntime {
public:
    pal4::ActorMessageRecord record;
    std::uint32_t allocation = 0x6000u;
    std::uint32_t requested = 0u;
    std::uint32_t accessed = 0u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        requested = bytes;
        return allocation;
    }
    pal4::ActorMessageRecord& Access(
        std::uint32_t address) override {
        accessed = address;
        return record;
    }
};

void TestShortAndFullTextCopy() {
    pal4::ActorMessageRecord source;
    pal4::ActorMessageRecord destination;
    source.Bytes().fill(0xa5u);
    destination.Bytes().fill(0xccu);
    source.Bytes()[0] = 'I';
    source.Bytes()[1] = 'n';
    source.Bytes()[2] = 'n';
    source.Bytes()[3] = 0u;
    source.WriteDword(0x40u, 0x12345678u);
    source.WriteDword(0x44u, 0xaabbccddu);
    Check(destination.CopyFrom(source) == 0xaabbccddu &&
              destination.Bytes()[0] == 'I' &&
              destination.Bytes()[1] == 'n' &&
              destination.Bytes()[2] == 'n' &&
              destination.Bytes()[3] == 0u &&
              destination.Bytes()[4] == 0u &&
              destination.Bytes()[0x3fu] == 0u &&
              destination.ReadDword(0x40u) == 0x12345678u &&
              destination.ReadDword(0x44u) == 0xaabbccddu,
          "strncpy pads short text and copies trailing fields");
    for (std::uint32_t index = 0u;
         index < pal4::ActorMessageRecord::kTextBytes; ++index) {
        source.Bytes()[index] = static_cast<std::uint8_t>('A' + index % 26u);
    }
    Check(destination.CopyFrom(source) == 0xaabbccddu &&
              destination.Bytes()[0] == 'A' &&
              destination.Bytes()[0x3fu] ==
                  source.Bytes()[0x3fu],
          "full-width text copies 64 bytes without forcing terminator");
}

void TestFactory() {
    Runtime runtime;
    runtime.record.Bytes().fill(0xa5u);
    Check(pal4::ActorMessageRecordFactory::Create(runtime) == 0x6000u &&
              runtime.requested == 0x48u &&
              runtime.accessed == 0x6000u &&
              runtime.record.Bytes()[0] == 0u &&
              runtime.record.Bytes()[1] == 0xa5u &&
              runtime.record.ReadDword(0x40u) == 0u &&
              runtime.record.ReadDword(0x44u) == 0u,
          "factory allocates 72 bytes and initializes only observed fields");
    Runtime failure;
    failure.allocation = 0u;
    Check(pal4::ActorMessageRecordFactory::Create(failure) == 0u &&
              failure.requested == 0x48u &&
              failure.accessed == 0u,
          "allocation failure returns null without record access");
}

}  // namespace

int main() {
    TestShortAndFullTextCopy();
    TestFactory();
    return EXIT_SUCCESS;
}
