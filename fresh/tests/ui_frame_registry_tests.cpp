#include "pal4/ui_frame_registry.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameRegistryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::string> events;
    std::uint32_t factory_node = 0x8100u;
    std::uint32_t factory_end = 0x8200u;
    std::uint32_t created_frame = 0x9000u;
    std::uint32_t next_allocation = 0x7000u;
    std::uint32_t allocated_bytes = 0;
    std::uint32_t freed_address = 0xffffffffu;
    bool throw_register = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.emplace_back("write");
        words[address] = value;
    }
    std::string_view FrameName(std::uint32_t frame) override {
        events.emplace_back("frame name");
        return names.at(frame);
    }
    std::uint32_t SingletonManager() override {
        events.emplace_back("singleton manager");
        return 0x4400u;
    }
    std::uint32_t RegisterFactorySingleton(
        std::uint32_t manager, const char* type_name,
        std::uint32_t id, std::uint32_t constructor,
        std::uint32_t type_info) override {
        events.emplace_back("register singleton");
        Check(manager == 0x4400u &&
                  std::string_view(type_name) ==
                      "fsObjectFactory < template-arg4 >" &&
                  id == 1019u && constructor == 0x4c1c10u &&
                  type_info == 0x4c2210u,
              "factory singleton identity and constructor");
        if (throw_register) throw std::runtime_error("registration failure");
        return 0x5000u;
    }
    std::uint32_t FindFactoryNode(std::uint32_t factory,
                                  const char* name) override {
        events.emplace_back("find factory");
        Check(factory == 0x5000u && std::string_view(name) == "new",
              "factory tree lookup");
        return factory_node;
    }
    std::uint32_t FactoryEndNode(std::uint32_t factory) override {
        events.emplace_back("factory end");
        Check(factory == 0x5000u, "factory end node owner");
        return factory_end;
    }
    std::uint32_t CreateFrame(std::uint32_t node,
                              const char* name) override {
        events.emplace_back("create frame");
        Check(node == factory_node && std::string_view(name) == "new",
              "factory creates requested frame");
        return created_frame;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("allocate");
        allocated_bytes = bytes;
        return next_allocation;
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free");
        freed_address = address;
    }
};

void CheckFind() {
    Runtime runtime;
    runtime.words[0x518cu] = 0u;
    Check(pal4::UiFrameRegistry::Find(0x5000u, "name", runtime) == 0u,
          "null frame collection is empty");

    runtime.words[0x518cu] = 0x6000u;
    runtime.words[0x5190u] = 0x600cu;
    runtime.words[0x6000u] = 0x9000u;
    runtime.words[0x6004u] = 0x9010u;
    runtime.words[0x6008u] = 0x9020u;
    runtime.names[0x9000u] = "name1";
    runtime.names[0x9010u] = "Name";
    runtime.names[0x9020u] = "name";
    Check(pal4::UiFrameRegistry::Find(0x5000u, "name", runtime) ==
              0x9020u, "exact frame name and length");
    Check(pal4::UiFrameRegistry::Find(0x5000u, "Name", runtime) ==
              0x9010u, "comparison is case-sensitive");
    Check(pal4::UiFrameRegistry::Find(0x5000u, "nam", runtime) == 0u,
          "prefix does not match");
}

void CheckDuplicateAndFactoryFailures() {
    Runtime runtime;
    runtime.words[0x518cu] = 0x6000u;
    runtime.words[0x5190u] = 0x6004u;
    runtime.words[0x6000u] = 0x9000u;
    runtime.names[0x9000u] = "new";
    std::uint8_t guard = 0;
    std::uint32_t cache = 0;
    Check(pal4::UiFrameRegistry::Register(
              0x5000u, "new", guard, cache, runtime) == 0x9000u &&
              guard == 0 && cache == 0,
          "existing frame bypasses factory singleton");

    runtime.words[0x518cu] = 0u;
    runtime.factory_node = runtime.factory_end;
    Check(pal4::UiFrameRegistry::Register(
              0x5000u, "new", guard, cache, runtime) == 0u &&
              guard == 1u && cache == 0x5000u,
          "factory miss registers singleton but creates nothing");
    Check(runtime.allocated_bytes == 0u, "factory miss avoids collection");

    runtime.factory_node = 0x8100u;
    runtime.created_frame = 0u;
    runtime.events.clear();
    Check(pal4::UiFrameRegistry::Register(
              0x5000u, "new", guard, cache, runtime) == 0u &&
              runtime.allocated_bytes == 0u,
          "null factory result is not appended");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "register singleton") == runtime.events.end(),
          "cached singleton reused");

    Runtime throwing;
    throwing.throw_register = true;
    guard = 0;
    cache = 0;
    bool threw = false;
    try {
        (void)pal4::UiFrameRegistry::Register(
            0x5000u, "new", guard, cache, throwing);
    } catch (const std::runtime_error& error) {
        threw = std::string_view(error.what()) == "registration failure";
    }
    Check(threw && guard == 1u && cache == 0u,
          "guard is set before singleton registration call");
}

void CheckSpareCapacity() {
    Runtime runtime;
    runtime.words[0x518cu] = 0x6000u;
    runtime.words[0x5190u] = 0x6008u;
    runtime.words[0x5194u] = 0x6010u;
    runtime.words[0x6000u] = 0x9100u;
    runtime.words[0x6004u] = 0x9200u;
    runtime.names[0x9100u] = "first";
    runtime.names[0x9200u] = "second";
    std::uint8_t guard = 1u;
    std::uint32_t cache = 0x5000u;
    Check(pal4::UiFrameRegistry::Register(
              0x5000u, "new", guard, cache, runtime) == 0x6008u,
          "spare-capacity return is original end pointer");
    Check(runtime.words[0x6008u] == 0x9000u &&
              runtime.words[0x5190u] == 0x600cu &&
              runtime.words[0x5194u] == 0x6010u &&
              runtime.allocated_bytes == 0u,
          "spare slot appended without allocation");
}

void CheckReallocation(std::uint32_t old_size,
                       std::uint32_t expected_capacity) {
    Runtime runtime;
    runtime.words[0x518cu] = old_size == 0u ? 0u : 0x6000u;
    runtime.words[0x5190u] = old_size == 0u ? 0u :
        0x6000u + old_size * 4u;
    runtime.words[0x5194u] = runtime.words[0x5190u];
    for (std::uint32_t i = 0; i < old_size; ++i) {
        runtime.words[0x6000u + i * 4u] = 0x9100u + i * 0x10u;
        runtime.names[0x9100u + i * 0x10u] = "old" + std::to_string(i);
    }
    std::uint8_t guard = 1u;
    std::uint32_t cache = 0x5000u;
    Check(pal4::UiFrameRegistry::Register(
              0x5000u, "new", guard, cache, runtime) == old_size,
          "reallocation return is prior element count");
    Check(runtime.allocated_bytes == expected_capacity * 4u &&
              runtime.freed_address == (old_size == 0u ? 0u : 0x6000u),
          "doubling/minimum-one allocation and old-storage release");
    Check(runtime.words[0x518cu] == 0x7000u &&
              runtime.words[0x5190u] == 0x7000u + (old_size + 1u) * 4u &&
              runtime.words[0x5194u] ==
                  0x7000u + expected_capacity * 4u &&
              runtime.words[0x7000u + old_size * 4u] == 0x9000u,
          "new collection bounds and appended frame");
    for (std::uint32_t i = 0; i < old_size; ++i)
        Check(runtime.words[0x7000u + i * 4u] ==
                  0x9100u + i * 0x10u,
              "old frame order preserved");
}

}  // namespace

int main() {
    try {
        CheckFind();
        CheckDuplicateAndFactoryFailures();
        CheckSpareCapacity();
        CheckReallocation(0u, 1u);
        CheckReallocation(1u, 2u);
        CheckReallocation(3u, 6u);
        std::cout << "ui_frame_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
