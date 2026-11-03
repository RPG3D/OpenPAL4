#include "pal4/pal_script_statics.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Api final : pal4::ICrtAtexitApi {
    std::vector<pal4::CrtExitCallback> callbacks;
    std::vector<int> results{3, -4, 9};
    pal4::PalScriptStaticGuards* guards = nullptr;
    std::vector<pal4::PalScriptStaticGuards> observed_guards;

    int Register(const pal4::CrtExitCallback callback) override {
        Check(callback != nullptr && callbacks.size() < results.size(),
              "register receives a valid callback each time");
        callbacks.push_back(callback);
        if (guards != nullptr) observed_guards.push_back(*guards);
        return results[callbacks.size() - 1u];
    }
};

void TestDirectRegistration() {
    Api first;
    Check(pal4::PalScriptStatics::RegisterFirst(first) == 3 &&
              first.callbacks.size() == 1u,
          "first registration forwards CRT return");
    Api second;
    Check(pal4::PalScriptStatics::RegisterSecond(second) == 3 &&
              second.callbacks.size() == 1u &&
              first.callbacks[0u] == second.callbacks[0u],
          "second registration uses the same null callback");
}

void TestInitialize() {
    Api api;
    pal4::PalScriptStaticGuards guards{0x80u, 0x40u, 0x20u};
    api.guards = &guards;
    Check(pal4::PalScriptStatics::Initialize(guards, api) == 9 &&
              guards.first == 0x81u && guards.second == 0x41u &&
              guards.third == 0x21u && api.callbacks.size() == 3u &&
              api.callbacks[0u] == api.callbacks[1u] &&
              api.callbacks[1u] == api.callbacks[2u] &&
              api.observed_guards.size() == 3u &&
              api.observed_guards[0u].first == 0x81u &&
              api.observed_guards[0u].second == 0x40u &&
              api.observed_guards[1u].second == 0x41u &&
              api.observed_guards[1u].third == 0x20u &&
              api.observed_guards[2u].third == 0x21u,
          "initializer preserves other guard bits and returns final CRT result");
    api.callbacks.clear();
    guards = {0xffu, 0xffu, 0xffu};
    Check(pal4::PalScriptStatics::Initialize(guards, api) == 9 &&
              guards.first == 0xffu && guards.second == 0xffu &&
              guards.third == 0xffu && api.callbacks.size() == 3u,
          "set guards still register all three callbacks");
}

}  // namespace

int main() {
    TestDirectRegistration();
    TestInitialize();
}
