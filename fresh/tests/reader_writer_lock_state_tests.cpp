#include "pal4/reader_writer_lock_state.h"

#include <cstdlib>
#include <iostream>

int main() {
    pal4::ReaderWriterLockState state;
    state.Set(0x12345678u, 0x87654321u);
    if (&state.Initialize() != &state || state.First() != 0u ||
        state.Second() != 0u) {
        std::cerr << "FAIL: lock constructor clears both DWORDs and returns this\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
