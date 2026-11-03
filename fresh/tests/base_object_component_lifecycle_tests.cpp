#include "pal4/base_object_component.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectComponentRuntime {
    std::vector<char> events;
    bool load_success{true};
    bool throw_on_load{};
    std::string cleaned_path;

    std::uint32_t GameInstance() override {
        events.push_back('G');
        return 0x7000u;
    }
    bool LoadClump(pal4::BaseObjectState& state,
                   std::uint32_t label, std::string_view path,
                   std::uint32_t model_address, std::uint32_t owner,
                   std::uint32_t flag, std::uint32_t zero,
                   std::uint32_t one) override {
        Check(label == 0x6200u && model_address == 0x6200u &&
                  owner == 0x71acu && flag == 0x5au &&
                  zero == 0u && one == 1u,
              "loader receives matching model pointers and original flags");
        Check(path == "root\\prehero\\" || path == "direct\\hero\\",
              "loader receives composed resource path");
        if (path == "root\\prehero\\") {
            Check(state.Read(0x170u) == 0x4444u,
                  "prefix loader stores attachment before resource load");
        }
        events.push_back('L');
        if (throw_on_load) throw std::runtime_error("load failed");
        return load_success;
    }
    void CleanupClump(pal4::BaseObjectState&,
                      std::uint32_t owner) override {
        Check(owner == 0x71acu,
              "cleanup uses game resource owner +0x1ac");
        events.push_back('K');
    }
    void ConstructResource(pal4::BaseObjectState& state) override {
        Check(state.Read(0u) == 0u,
              "base resource construction runs before derived vtable write");
        events.push_back('C');
    }
    void DestroyResource(pal4::BaseObjectState& state) override {
        Check(state.Read(0u) == 0x843440u,
              "resource destructor sees derived vtable");
        events.push_back('D');
    }
    void RemoveFromWorld(pal4::BaseObjectState& state) override {
        Check(state.Read(0u) == 0x843440u,
              "world cleanup sees derived vtable");
        events.push_back('W');
    }
    void FreeObject(std::uint32_t address) override {
        Check(address == 0x1000u,
              "deleting destructor frees original object address");
        events.push_back('F');
    }
    std::string CreatePath() override {
        events.push_back('P');
        return "root\\";
    }
    void CleanupPath(std::string& path) override {
        cleaned_path = path;
        events.push_back('p');
    }
};

void CheckSetupAndCleanup() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    Check(pal4::BaseObjectComponent::Setup(
              state, 0x6200u, "direct\\hero\\", 0x6200u,
              0x5au, runtime) == 1u &&
              runtime.events == std::vector<char>{'G', 'L'},
          "component setup forwards game owner and loader success");
    runtime.events.clear();
    runtime.load_success = false;
    Check(pal4::BaseObjectComponent::Setup(
              state, 0x6200u, "direct\\hero\\", 0x6200u,
              0x5au, runtime) == 0u &&
              runtime.events == std::vector<char>{'G', 'L'},
          "component setup returns zero on loader failure");
    runtime.events.clear();
    Check(pal4::BaseObjectComponent::Cleanup(state, runtime) == 1u &&
              runtime.events == std::vector<char>{'G', 'K'},
          "component cleanup forwards owner then returns one");
}

void CheckConstructionAndDestruction() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    state.Write(0x174u, 0x12345678u);
    Check(pal4::BaseObjectComponent::Construct(
              state, 0x1000u, runtime) == 0x1000u &&
              state.Read(0x174u) == 0x12345600u &&
              state.Read(0u) == 0x843440u &&
              runtime.events == std::vector<char>{'C'},
          "constructor initializes resource, clears one byte and writes vtable");
    runtime.events.clear();
    Check(pal4::BaseObjectComponent::CleanupAndRelated(
              state, runtime) == 1u &&
              runtime.events == std::vector<char>{'W', 'G', 'K'},
          "combined cleanup removes from world before clump cleanup");
    runtime.events.clear();
    pal4::BaseObjectComponent::Destroy(state, runtime);
    Check(runtime.events == std::vector<char>{'W', 'G', 'K', 'D'},
          "destructor cleans world and clump before base resource");
    runtime.events.clear();
    pal4::BaseObjectComponent::CleanupResourceThunk(state, runtime);
    Check(runtime.events == std::vector<char>{'D'},
          "cleanup thunk calls only the base resource destructor");
    runtime.events.clear();
    Check(pal4::BaseObjectComponent::Delete(
              state, 0x1000u, 0u, runtime) == 0x1000u &&
              runtime.events == std::vector<char>{'W', 'G', 'K', 'D'},
          "deleting destructor skips free when flag bit is clear");
    runtime.events.clear();
    Check(pal4::BaseObjectComponent::Delete(
              state, 0x1000u, 3u, runtime) == 0x1000u &&
              runtime.events ==
                  std::vector<char>{'W', 'G', 'K', 'D', 'F'},
          "deleting destructor frees when low flag bit is set");
}

void CheckPrefixLoad() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    Check(pal4::BaseObjectComponent::LoadFromPrefixAndName(
              state, 0x4444u, "pre", "hero", 0x6200u,
              0x9999u, 0x5au, runtime) == 1u &&
              runtime.events == std::vector<char>{'P', 'G', 'L', 'p'} &&
              runtime.cleaned_path == "root\\prehero\\",
          "prefix loader composes path and cleans temporary string");
    runtime.events.clear();
    runtime.load_success = false;
    Check(pal4::BaseObjectComponent::LoadFromPrefixAndName(
              state, 0x4444u, "pre", "hero", 0x6200u,
              0x9999u, 0x5au, runtime) == 0u &&
              runtime.events == std::vector<char>{'P', 'G', 'L', 'p'},
          "prefix loader forwards clump-load failure and cleans path");
    runtime.events.clear();
    runtime.throw_on_load = true;
    bool threw = false;
    try {
        (void)pal4::BaseObjectComponent::LoadFromPrefixAndName(
            state, 0x4444u, "pre", "hero", 0x6200u,
            0x9999u, 0x5au, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events ==
                       std::vector<char>{'P', 'G', 'L', 'p'},
          "prefix loader releases path once on exception");
}

}  // namespace

int main() {
    CheckSetupAndCleanup();
    CheckConstructionAndDestruction();
    CheckPrefixLoad();
}
