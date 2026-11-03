#include "pal4/callback_state_initializer.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t LoadDword(const pal4::CallbackState::Storage& storage,
                        const std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        value |= std::to_integer<std::uint32_t>(storage[offset + index])
                 << (index * 8U);
    }
    return value;
}

void InitializesOnlyObservedFields() {
    pal4::CallbackState::Storage initial;
    initial.fill(std::byte{0xa5});
    const auto state = pal4::CallbackState::Initialize(initial);

    Check(LoadDword(state, 0x00) == 0 && LoadDword(state, 0x04) == 0,
          "clears the first two observed DWORD fields");
    Check(LoadDword(state, 0x10) == 0 && LoadDword(state, 0x14) == 0,
          "clears the two observed collection fields");
    Check(state[0x18] == std::byte{0} && state[0x19] == std::byte{1} &&
              state[0x1a] == std::byte{1},
          "writes the three observed flag bytes");
    Check(LoadDword(state, 0x1c) == 0,
          "clears the observed DWORD at offset 0x1c");
    Check(state[0x08] == std::byte{0xa5} && state[0x0c] == std::byte{0xa5},
          "preserves offsets not written by the binary initializer");
}

void SettersStoreAndReturnTheirArguments() {
    pal4::CallbackState::Storage state{};
    Check(pal4::CallbackState::SetDword0C(state, 0x89abcdef) == 0x89abcdef,
          "DWORD setter returns its input");
    Check(LoadDword(state, 0x0c) == 0x89abcdef,
          "DWORD setter writes offset 0x0c");
    Check(pal4::CallbackState::SetByte1A(state, 0x7f) == 0x7f,
          "byte setter returns its input");
    Check(state[0x1a] == std::byte{0x7f},
          "byte setter writes offset 0x1a");
    Check(state[0x18] == std::byte{0} && state[0x19] == std::byte{0},
          "setters preserve neighboring bytes");
}

void PropertyEmptyChecksOnlyCollectionRoot() {
    pal4::CallbackState::Storage state{};
    Check(pal4::CallbackState::IsPropertyEmpty(state),
          "zero root field reports empty");
    pal4::CallbackState::WriteDword(state, 0x14, 0x1234);
    Check(pal4::CallbackState::IsPropertyEmpty(state),
          "secondary field does not affect emptiness");
    pal4::CallbackState::WriteDword(state, 0x10, 1);
    Check(!pal4::CallbackState::IsPropertyEmpty(state),
          "nonzero root field reports nonempty");
}

}  // namespace

int main() {
    InitializesOnlyObservedFields();
    SettersStoreAndReturnTheirArguments();
    PropertyEmptyChecksOnlyCollectionRoot();
    std::cout << "3 callback-state tests passed\n";
}
