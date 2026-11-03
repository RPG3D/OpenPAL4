#include "pal4/bink_player.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IBinkPlayerRuntime {
public:
    void EnsureSingletonManager() override {
        events.emplace_back("manager");
    }
    std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::function<std::byte*()> create,
        std::function<std::uint32_t(std::byte*)> destroy) override {
        events.emplace_back("register");
        Check(name == "bnkBinkPlayer" && id == 1007u,
              "Bink singleton uses IDA name and class identifier");
        create_callback = std::move(create);
        destroy_callback = std::move(destroy);
        return 0x456u;
    }
    std::byte* Allocate(std::size_t size) override {
        events.emplace_back("allocate");
        observed_size = size;
        if (allocation_fails)
            return nullptr;
        auto* memory = new std::byte[size];
        std::fill_n(memory, size, std::byte{0x5a});
        allocation = memory;
        return memory;
    }
    std::uint32_t Free(std::byte* object) override {
        events.emplace_back("free");
        Check(object == allocation, "free receives original allocation");
        delete[] object;
        allocation = nullptr;
        return 0x88u;
    }
    void InitializePackage(std::byte* package) override {
        events.push_back(PackageEvent("init", package));
        std::fill_n(package, 0x1001bcu, std::byte{0});
    }
    std::uint32_t ClosePackage(std::byte* package) override {
        events.push_back(PackageEvent("close", package));
        return 0x55u;
    }
    std::uint32_t CleanupPackage(std::byte* package) override {
        events.push_back(PackageEvent("cleanup", package));
        return package == allocation + pal4::BinkPlayer::kFirstPackageOffset ?
                   0x71u : 0x72u;
    }
    std::string PackageEvent(const char* name, std::byte* package) {
        if (package == allocation + pal4::BinkPlayer::kFirstPackageOffset)
            return std::string(name) + "-first";
        Check(package == allocation +
                  pal4::BinkPlayer::kSecondPackageOffset,
              "package call uses one of two native offsets");
        return std::string(name) + "-second";
    }

    ~Runtime() override { delete[] allocation; }

    std::vector<std::string> events;
    std::function<std::byte*()> create_callback;
    std::function<std::uint32_t(std::byte*)> destroy_callback;
    std::byte* allocation = nullptr;
    std::size_t observed_size = 0u;
    bool allocation_fails = false;
};

void SingletonAndAllocation() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t registration = 0u;
    Check(pal4::BinkPlayer::InitializeSingleton(
              guard, registration, runtime) == 0x456u &&
              guard == 0x81u && registration == 0x456u &&
              runtime.events == std::vector<std::string>{
                  "manager", "register"} &&
              runtime.create_callback && runtime.destroy_callback,
          "first singleton call sets guard and registers callbacks");
    runtime.events.clear();
    Check(pal4::BinkPlayer::InitializeSingleton(
              guard, registration, runtime) == 0x456u &&
              runtime.events.empty(),
          "repeat singleton call returns cached registration");
    runtime.allocation_fails = true;
    Check(runtime.create_callback() == nullptr &&
              runtime.events == std::vector<std::string>{"allocate"} &&
              runtime.observed_size == pal4::BinkPlayer::kSize &&
              runtime.destroy_callback(nullptr) == 0u,
          "allocation failure returns null without constructing");
}

void ConstructorAndCleanup() {
    Runtime runtime;
    auto* object = pal4::BinkPlayer::Create(runtime);
    Check(object != nullptr && runtime.observed_size ==
              pal4::BinkPlayer::kSize &&
              runtime.events == std::vector<std::string>{
                  "allocate", "init-first", "init-second"} &&
              object[0] == std::byte{0} &&
              object[4] == std::byte{0} &&
              object[8] == std::byte{0} &&
              object[12] == std::byte{0x5a} &&
              object[16] == std::byte{0} &&
              object[20] == std::byte{0} &&
              object[24] == std::byte{0} &&
              object[pal4::BinkPlayer::kSize - 1u] == std::byte{0},
          "constructor initializes nested video and both package slots");
    runtime.events.clear();
    Check(pal4::BinkPlayer::Cleanup(object, runtime) == 0x71u &&
              runtime.events == std::vector<std::string>{
                  "cleanup-second", "cleanup-first"},
          "closed packages are cleaned in reverse order");
    runtime.events.clear();
    object[pal4::BinkPlayer::kFirstPackageOpenFlag] = std::byte{1};
    object[pal4::BinkPlayer::kSecondPackageOpenFlag] = std::byte{2};
    Check(pal4::BinkPlayer::Destroy(object, runtime) == 0x88u &&
              runtime.events == std::vector<std::string>{
                  "close-first", "close-second", "cleanup-second",
                  "cleanup-first", "free"},
          "destroy closes both open packages then cleans and frees");
}

}  // namespace

int main() {
    SingletonAndAllocation();
    ConstructorAndCleanup();
    std::cout << "Bink player tests passed\n";
}
