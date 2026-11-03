#include "pal4/cegui_ui_helpers.h"
#include "pal4/cegui_system_initializer.h"

#include <cstdlib>
#include <iostream>
#include <map>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class ImeRuntime final : public pal4::ICeguiImeRuntime {
public:
    bool write_output = true;
    int calls = 0;

    int MultiByteToWideChar(
        std::uint32_t code_page, std::uint32_t flags,
        const std::array<std::uint8_t, 2>& bytes,
        std::uint16_t& output,
        std::uint32_t output_capacity) override {
        Check(code_page == 0u && flags == 1u &&
                  bytes == std::array<std::uint8_t, 2>{0x12u, 0x34u} &&
                  output_capacity == 1u,
              "IME conversion uses ACP and a two-byte swapped input");
        ++calls;
        if (write_output) output = 0x4e2du;
        return write_output ? 1 : 0;
    }
};

class KeyRuntime final : public pal4::ICeguiKeyLookupRuntime {
public:
    std::uint32_t candidate = 0x3000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1008u, 0x2000u},
        {0x300cu, 32u},
        {0x3010u, 205u}};
    int key_reads = 0;
    int value_reads = 0;

    std::uint32_t SearchNode(std::uint32_t map,
                             std::uint32_t key) override {
        Check(map == 0x1004u && key <= 32u,
              "key search uses embedded map and input key");
        return candidate;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x300cu) ++key_reads;
        if (address == 0x3010u) ++value_reads;
        return words.at(address);
    }
};

void CaptureFlagPreservesOtherObjectBytes() {
    pal4::CeguiSystemState state;
    state.Bytes().fill(0xaau);
    state.Bytes()[0] = 0u;
    state.EnableMouseCapture();
    Check(state.Bytes()[0] == 1u && state.Bytes()[1] == 0xaau,
          "enable writes one only from zero");
    state.Bytes()[0] = 0xfeu;
    state.EnableMouseCapture();
    Check(state.Bytes()[0] == 0xfeu,
          "enable leaves a nonzero capture byte unchanged");
    state.DisableMouseCapture();
    Check(state.Bytes()[0] == 0u && state.Bytes()[1] == 0xaau,
          "disable clears nonzero flag and preserves neighbor");
    state.DisableMouseCapture();
    Check(state.Bytes()[0] == 0u,
          "disable on zero is a no-op");
}

void ImeConversionForwardsWin32Parameters() {
    ImeRuntime runtime;
    Check(pal4::CeguiUiHelpers::ConvertImeCharacter(
              0x12u, 0x1234u, runtime) == 0x4e2du &&
              runtime.calls == 1,
          "converted wide character is returned");
    runtime.write_output = false;
    Check(pal4::CeguiUiHelpers::ConvertImeCharacter(
              0x12u, 0x1234u, runtime) == 0x1234u &&
              runtime.calls == 2,
          "failed conversion retains initial output word");
}

void MapLookupChecksSentinelAndLowerBound() {
    KeyRuntime runtime;
    runtime.candidate = 0x2000u;
    Check(pal4::CeguiUiHelpers::MapVirtualKey(
              0x1000u, 32u, runtime) == 0xffu &&
              runtime.key_reads == 0,
          "sentinel candidate returns unmapped without reading node key");
    runtime.candidate = 0x3000u;
    Check(pal4::CeguiUiHelpers::MapVirtualKey(
              0x1000u, 31u, runtime) == 0xffu &&
              runtime.key_reads == 1 && runtime.value_reads == 0,
          "candidate greater than query is rejected");
    Check(pal4::CeguiUiHelpers::MapVirtualKey(
              0x1000u, 32u, runtime) == 205u &&
              runtime.key_reads == 2 && runtime.value_reads == 1,
          "matching candidate returns value at node plus sixteen");
}

}  // namespace

int main() {
    CaptureFlagPreservesOtherObjectBytes();
    ImeConversionForwardsWin32Parameters();
    MapLookupChecksSentinelAndLowerBound();
    std::cout << "3 CEGUI UI helper test cases passed\n";
}
