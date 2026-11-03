#include "pal4/temperature_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITemperatureWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b1360u, 0x8b1364u}};
    std::vector<std::uint32_t> allocations;
    std::uint32_t freed = 0u;
    std::uint32_t constructed_name = 0u;
    std::uint32_t registered_name = 0u;
    std::uint32_t registered_factory = 0u;
    std::uint32_t tree = 0u;
    std::uint32_t key = 0u;
    std::uint32_t handler = 0u;
    std::uint32_t destroyed = 0u;
    bool fail_allocation = false;
    bool throw_construct = false;
    bool throw_insert = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        return fail_allocation ? 0u : 0x2000u;
    }
    void Free(std::uint32_t address) override { freed = address; }
    std::uint32_t Construct(std::uint32_t object,
                            std::uint32_t layout_name) override {
        Check(object == 0x2000u, "constructor object");
        constructed_name = layout_name;
        if (throw_construct) throw std::runtime_error("construct");
        return 0x2004u;
    }
    std::uint32_t FrameManager() override { return 0x3000u; }
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                   std::uint32_t name,
                                   std::uint32_t factory) override {
        Check(manager == 0x3000u, "manager");
        registered_name = name;
        registered_factory = factory;
        return 0x55u;
    }
    void InsertOrReplace(std::uint32_t target, std::uint32_t event_key,
                         std::uint32_t value) override {
        tree = target;
        key = event_key;
        handler = value;
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DestroyHandler(std::uint32_t value,
                        std::uint32_t flags) override {
        Check(flags == 1u, "deleting flag");
        destroyed = value;
    }
};

void CheckRegistration() {
    using Registration = pal4::TemperatureWindowRegistration;
    Runtime runtime;
    Check(Registration::InitializeNameThunk(runtime) == 0x8b1364u &&
              runtime.words.at(0x8eac98u) == 0x8b1364u,
          "TemperatureWindow name pointer");
    Check(Registration::RegisterFactoryThunk(runtime) == 0x55u &&
              runtime.registered_name == 0x8b1364u &&
              runtime.registered_factory == 0x4a2220u,
          "temperature factory address");
    Check(Registration::Create(0x9000u, runtime) == 0x2004u &&
              runtime.allocations.back() == 0x38u &&
              runtime.constructed_name == 0x9000u,
          "factory allocation and constructor forwarding");
    Check(Registration::RegisterHandlerThunk(runtime) == 0x8eac94u &&
              runtime.allocations.back() == 8u &&
              runtime.words.at(0x2000u) == 0x843494u &&
              runtime.words.at(0x2004u) == 0x8b1364u &&
              runtime.tree == 0x3104u && runtime.key == 0x8b135cu &&
              runtime.handler == 0x2000u && runtime.destroyed == 0x2000u,
          "show_temperature_ui handler registration");
}

void CheckFailures() {
    using Registration = pal4::TemperatureWindowRegistration;
    Runtime empty;
    empty.fail_allocation = true;
    Check(Registration::Create(0x9000u, empty) == 0u &&
              empty.constructed_name == 0u &&
              Registration::RegisterHandler(empty) == 0x8eac94u &&
              empty.handler == 0u && empty.destroyed == 0u,
          "failed allocations skip construction and pass null handler");
    Runtime constructor_failure;
    constructor_failure.throw_construct = true;
    bool caught = false;
    try { (void)Registration::Create(0x9000u, constructor_failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && constructor_failure.freed == 0x2000u,
          "failed constructor frees frame allocation");
    Runtime insertion_failure;
    insertion_failure.throw_insert = true;
    caught = false;
    try { (void)Registration::RegisterHandler(insertion_failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && insertion_failure.destroyed == 0x2000u,
          "failed insertion releases handler");
}

}  // namespace

int main() {
    try {
        CheckRegistration();
        CheckFailures();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
