#include "pal4/timer_callback.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITimerCallbackRuntime {
    std::uint32_t allocated = 0x3000u;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "clone allocates 24 bytes");
        calls.emplace_back("allocate");
        return allocated;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
        words[address] = value;
    }
};

}  // namespace

int main() {
    pal4::TimerCallbackState first{};
    first.reserved_04 = 0x11111111u;
    first.reserved_14 = 0x22222222u;
    Check(&pal4::TimerCallback::ConstructFirst(
              first, 0x1000u, 0x2000u, 0x3000u) == &first &&
              first.vtable == pal4::TimerCallback::kFirstVtable &&
              first.owner == 0x1000u && first.function == 0x2000u &&
              first.argument == 0x3000u &&
              first.reserved_04 == 0x11111111u &&
              first.reserved_14 == 0x22222222u,
          "first constructor writes only vtable and three payload fields");

    pal4::TimerCallbackState second{};
    second.reserved_04 = 0x33333333u;
    second.reserved_14 = 0x44444444u;
    Check(&pal4::TimerCallback::ConstructSecond(
              second, 0x4000u, 0x5000u, 0x6000u) == &second &&
              second.vtable == pal4::TimerCallback::kSecondVtable &&
              second.owner == 0x4000u && second.function == 0x5000u &&
              second.argument == 0x6000u &&
              second.reserved_04 == 0x33333333u &&
              second.reserved_14 == 0x44444444u,
          "second constructor preserves untouched words");

    Runtime runtime;
    Check(pal4::TimerCallback::Clone(first, runtime) == 0x3000u &&
              runtime.words[0x3000u] ==
                  pal4::TimerCallback::kSecondVtable &&
              runtime.words[0x3008u] == 0x1000u &&
              runtime.words[0x300cu] == 0x2000u &&
              runtime.words[0x3010u] == 0x3000u &&
              runtime.words.count(0x3004u) == 0u &&
              runtime.words.count(0x3014u) == 0u,
          "clone copies only three payload words and final vtable");
    Check(runtime.calls == std::vector<std::string>{
                               "allocate", "write:12288:8664320",
                               "write:12296:4096", "write:12300:8192",
                               "write:12304:12288", "write:12288:8665908"},
          "clone writes base vtable before payload and final vtable last");

    Runtime failed;
    failed.allocated = 0u;
    Check(pal4::TimerCallback::Clone(second, failed) == 0u &&
              failed.calls == std::vector<std::string>{"allocate"},
          "failed clone allocation makes no writes");
}
