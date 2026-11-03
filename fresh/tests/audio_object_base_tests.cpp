#include "pal4/audio_object_base.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IAudioObjectBaseRuntime {
public:
    void FreeObject(void* object) override {
        freed = object;
        ++calls;
    }
    void* freed = nullptr;
    unsigned calls = 0u;
};

void BaseAndAccessors() {
    std::array<std::byte, 92u> bytes{};
    bytes.fill(std::byte{0x5a});
    Runtime runtime;
    pal4::AudioObjectBase::Initialize(bytes.data());
    std::uint32_t vtable = 0u;
    std::memcpy(&vtable, bytes.data(), sizeof(vtable));
    Check(vtable == pal4::AudioObjectBase::kVtable &&
              bytes[4] == std::byte{0x5a},
          "base initializer writes only vtable");
    Check(pal4::AudioObjectBase::MemberAt40(bytes.data()) ==
                  bytes.data() + 40u &&
              pal4::AudioObjectBase::MemberAt32(bytes.data()) ==
                  bytes.data() + 32u &&
              pal4::AudioObjectBase::MemberAt80(bytes.data()) ==
                  bytes.data() + 80u,
          "path accessors return exact byte offsets");
    const std::uint64_t first = 0x1122334455667788ull;
    const std::uint64_t second = 0x8877665544332211ull;
    std::memcpy(bytes.data() + 24u, &first, sizeof(first));
    std::memcpy(bytes.data() + 72u, &second, sizeof(second));
    Check(pal4::AudioObjectBase::ReadAt24(bytes.data()) == first &&
              pal4::AudioObjectBase::ReadAt72(bytes.data()) == second,
          "quadword accessors read exact native offsets");
    Check(pal4::AudioObjectBase::WriteAt16(
              bytes.data(), 0xa1b2c3d4u) == 0xa1b2c3d4u,
          "setter forwards input value");
    std::uint32_t written = 0u;
    std::memcpy(&written, bytes.data() + 16u, sizeof(written));
    Check(written == 0xa1b2c3d4u && bytes[20] == std::byte{0x5a},
          "setter writes only offset 16 word");
    Check(pal4::AudioObjectBase::DestroyAt(
              bytes.data(), 2u, runtime) == bytes.data() &&
              runtime.calls == 0u,
          "unrelated delete flag retains allocation");
    Check(pal4::AudioObjectBase::DestroyAt(
              bytes.data(), 1u, runtime) == bytes.data() &&
              runtime.calls == 1u && runtime.freed == bytes.data(),
          "delete flag frees after restoring base vtable");
}

}  // namespace

int main() {
    BaseAndAccessors();
    std::cout << "Audio object base tests passed\n";
}
