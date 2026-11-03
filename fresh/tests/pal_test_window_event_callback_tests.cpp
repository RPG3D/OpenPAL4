#include "pal4/pal_test_window_event_callback.h"

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PalTestWindowEventCallbackRuntimeStub {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::PalTestWindowEventCallback::Construct(
              0x5000u, 0x44a150u, 0x6000u, 0x7000u, runtime) == 0x5000u &&
                  runtime.writes ==
                      std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                          {0x5008u, 0x44a150u}, {0x500cu, 0x6000u},
                          {0x5010u, 0x7000u}, {0x5000u, 0x84381cu}},
          "callback stores handler, context, owner and vtable in IDA order");
    std::cout << "Pal test window event callback tests passed\n";
}
