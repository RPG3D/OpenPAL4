#include "pal4/actor_manager_player_forwarders.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerPlayerForwarderRuntime {
    std::vector<std::string> trace;
    std::uint32_t manager = 0x9000u;
    std::uint32_t actor_result = 0x12345678u;

    std::uint32_t ActorSlot48(std::uint32_t actor,
                              std::uint32_t mode) override {
        Check(actor == 0x1000u && mode == 1u,
              "virtual slot 48 forces mode one");
        trace.emplace_back("actor");
        return actor_result;
    }
    std::uint32_t SingletonManager() override {
        trace.emplace_back("singleton");
        return manager;
    }
    void Query37(std::uint32_t collection,
                 std::uint32_t output) override {
        Check(collection == manager + 32u && output == 0x2000u,
              "query 37 uses embedded collection");
        trace.emplace_back("query37");
    }
    void Query38(std::uint32_t collection,
                 std::uint32_t output) override {
        Check(collection == manager + 32u && output == 0x2000u,
              "query 38 uses embedded collection");
        trace.emplace_back("query38");
    }
    void FirstIterator(std::uint32_t actual_manager,
                       std::uint32_t output) override {
        Check(actual_manager == manager && output == 0x2000u,
              "first iterator uses manager base");
        trace.emplace_back("first");
    }
    void SecondIterator(std::uint32_t actual_manager,
                        std::uint32_t output) override {
        Check(actual_manager == manager && output == 0x2000u,
              "second iterator uses manager base");
        trace.emplace_back("second");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        using Access = pal4::ActorManagerPlayerForwarders;
        Check(Access::SharedTextRecord(0u) == 0x8fa36cu &&
                  Access::SharedTextRecord(0x1234u) == 0x8fa36cu &&
                  Access::EmbeddedProperty144(0x1000u) == 0x1090u &&
                  Access::EmbeddedProperty144(0xfffffff0u) == 0x80u,
              "fixed record ignores input and embedded property wraps at 32 bits");
        Check(Access::ForceActorModeOne(0x1000u, runtime) ==
                  runtime.actor_result &&
                  runtime.trace == std::vector<std::string>{"actor"},
              "actor virtual return is forwarded without singleton lookup");
        runtime.trace.clear();
        Check(Access::Query37(0x2000u, runtime) == 0x2000u &&
                  runtime.trace == std::vector<std::string>{
                      "singleton", "query37"},
              "first candidate query call order and output return");
        runtime.trace.clear();
        Check(Access::Query38(0x2000u, runtime) == 0x2000u &&
                  runtime.trace == std::vector<std::string>{
                      "singleton", "query38"},
              "second candidate query call order and output return");
        runtime.trace.clear();
        Check(Access::FirstIterator(0x2000u, runtime) == 0x2000u &&
                  runtime.trace == std::vector<std::string>{
                      "singleton", "first"},
              "first iterator call order and output return");
        runtime.trace.clear();
        Check(Access::SecondIterator(0x2000u, runtime) == 0x2000u &&
                  runtime.trace == std::vector<std::string>{
                      "singleton", "second"},
              "second iterator call order and output return");
        std::cout << "actor_manager_player_forwarders_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
