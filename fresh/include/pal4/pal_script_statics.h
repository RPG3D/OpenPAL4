#pragma once

#include "pal4/crt_atexit_stub.h"

#include <cstdint>

namespace pal4 {

struct PalScriptStaticGuards final {
    std::uint8_t first = 0u;   // IDA 0x978b3c.
    std::uint8_t second = 0u;  // IDA 0x978a96.
    std::uint8_t third = 0u;   // IDA 0x978a97.
};

class PalScriptStatics final {
public:
    // IDA 0x601650, including the 0x6016c0 tail.
    [[nodiscard]] static int Initialize(PalScriptStaticGuards& guards,
                                        ICrtAtexitApi& api);
    // IDA 0x6016a0.
    [[nodiscard]] static int RegisterFirst(ICrtAtexitApi& api);
    // IDA 0x6016b0.
    [[nodiscard]] static int RegisterSecond(ICrtAtexitApi& api);
};

}  // namespace pal4
