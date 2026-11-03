#include "pal4/concurrency_message_target.h"

#include <cstdlib>
#include <iostream>

int main() {
    pal4::ConcurrencyMessageTargetState state{};
    state.unknown_prefix[87] = 0x5au;
    state.declining = 0xffu;
    pal4::ConcurrencyMessageTarget::DeclineIncomingMessages(state);
    if (state.unknown_prefix[87] != 0x5au || state.declining != 1u) {
        std::cerr << "FAIL: decline must set only the byte at offset 88\n";
        return EXIT_FAILURE;
    }
    state.declining = 0u;
    pal4::ConcurrencyMessageTarget::DeclineIncomingMessages(state);
    if (state.declining != 1u)
        return EXIT_FAILURE;
    std::cout << "Concurrency message target tests passed\n";
}
