#include "pal4/animation_script_record_array.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Call final {
    char kind;
    std::uint32_t address;
    std::uint8_t flag;
    bool operator==(const Call&) const = default;
};

class Runtime final : public pal4::IAnimationScriptRecordArrayRuntime {
public:
    std::vector<Call> calls;

    void SetErrorFlag(std::uint32_t address, std::uint8_t flag) override {
        calls.push_back({'F', address, flag});
    }
    void Free(std::uint32_t address) override {
        calls.push_back({'M', address, 0});
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::AnimationScriptRecordArrayState state{
        0x9876u, 0x2000u, 0x2040u, 0x2080u};
    Check(pal4::AnimationScriptRecordArray::Count(state) == 2u &&
              pal4::AnimationScriptRecordArray::AddressAt(state, 0) == 0x2000u &&
              pal4::AnimationScriptRecordArray::AddressAt(state, 1) == 0x2020u &&
              pal4::AnimationScriptRecordArray::AddressAt(state, 2) == 0x2040u,
          "count and unchecked index use 32-byte records");
    const pal4::AnimationScriptRecordArrayState empty{7u, 0u, 0u, 0u};
    Check(pal4::AnimationScriptRecordArray::Count(empty) == 0u &&
              pal4::AnimationScriptRecordArray::AddressAt(empty, 1u) == 32u,
          "null begin gives zero count but indexing remains unchecked");
    const pal4::AnimationScriptRecordArrayState malformed{0u, 0x2000u,
                                                            0x1fe0u, 0u};
    Check(pal4::AnimationScriptRecordArray::Count(malformed) == -1,
          "native signed shift is retained for malformed ranges");
    Check(pal4::AnimationScriptRecordArray::Release(state, runtime) == 0,
          "release returns zero");
    Check(runtime.calls == std::vector<Call>{{'F', 0x2010u, 1},
                                             {'F', 0x2000u, 1},
                                             {'F', 0x2030u, 1},
                                             {'F', 0x2020u, 1},
                                             {'M', 0x2000u, 0}},
          "release destroys both fields per record before freeing array");
    Check(state.reserved == 0x9876u && state.begin == 0 &&
              state.end == 0 && state.capacity == 0,
          "release preserves owner word and clears vector pointers");

    runtime.calls.clear();
    state = {7u, 0u, 0u, 0u};
    Check(pal4::AnimationScriptRecordArray::Release(state, runtime) == 0 &&
              runtime.calls == std::vector<Call>{{'M', 0u, 0u}} &&
              state.reserved == 7u,
          "empty release still forwards null free");

    runtime.calls.clear();
    state = {9u, 0x3000u, 0x3010u, 0x3040u};
    Check(pal4::AnimationScriptRecordArray::Release(state, runtime) == 0 &&
              runtime.calls == std::vector<Call>{{'M', 0x3000u, 0u}} &&
              state.begin == 0 && state.end == 0 && state.capacity == 0,
          "malformed range terminates safely and releases allocation");

    pal4::AnimationScriptRecordArrayRuntimeStub stub;
    state = {0u, 0u, 0u, 0u};
    Check(pal4::AnimationScriptRecordArray::Release(state, stub) == 0,
          "stub is linkable");
    std::cout << "animation_script_record_array_tests passed\n";
}
