#include "pal4/actor_manager_player_event_dispatch.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerPlayerEventRuntime {
    std::vector<std::string> trace;
    std::int32_t first_result = -3;
    std::int32_t second_result = 17;
    std::int32_t slot_result = 24;
    std::uint32_t routers = 0u;

    std::uint32_t EventRouter() override {
        ++routers;
        trace.emplace_back("router");
        return 0x9000u + routers;
    }
    std::int32_t Dispatch(std::uint32_t router,
                          std::string_view event,
                          std::uint32_t first,
                          std::uint32_t second,
                          std::uint32_t word_count,
                          std::uint32_t mode) override {
        Check(router == 0x9000u + routers && mode == 1u,
              "event router identity and mode");
        if (event == "add_money" || event == "add_picked_money")
            Check(first == 0xabcdef01u && second == 0u &&
                      word_count == 1u,
                  "money event receives one-word payload");
        else
            Check(first == 12u && second == 34u &&
                      word_count == 2u,
                  "property event receives two-word payload");
        trace.emplace_back(std::string(event));
        return routers == 1u ? first_result : second_result;
    }
    std::int32_t DispatchByte(std::uint32_t router,
                              std::string_view event,
                              std::uint8_t payload,
                              std::uint32_t mode) override {
        Check(router == 0x9000u + routers &&
                  event == "pick_up_fail" &&
                  payload == 0x12u && mode == 1u,
              "pick-up failure passes saved ECX high byte");
        trace.emplace_back("pick_up_fail");
        return first_result;
    }
    std::int32_t ActorSlot344(std::uint32_t actor) override {
        Check(actor == 0x1000u, "zero-property actor slot 344");
        trace.emplace_back("slot344");
        return slot_result;
    }
};

}  // namespace

int main() {
    try {
        Runtime money;
        Check(pal4::ActorManagerPlayerEventDispatch::AddMoney(
                  0xabcdef01u, money) == money.second_result &&
                  money.routers == 2u &&
                  money.trace == std::vector<std::string>{
                      "router", "add_money", "router",
                      "add_picked_money"},
              "money events dispatch in order and return second result");

        Runtime property;
        Check(pal4::ActorManagerPlayerEventDispatch::AddProperty(
                  0x1000u, 12u, 34u, property) ==
                  property.second_result &&
                  property.routers == 2u &&
                  property.trace == std::vector<std::string>{
                      "router", "add_property", "router",
                      "add_picked_property"},
              "property events dispatch in order and return second result");

        Runtime absent;
        Check(pal4::ActorManagerPlayerEventDispatch::AddProperty(
                  0x1000u, 0u, 34u, absent) == absent.slot_result &&
                  absent.routers == 0u &&
                  absent.trace == std::vector<std::string>{"slot344"},
              "zero property delegates to actor virtual slot");

        Runtime one_property;
        Check(pal4::ActorManagerPlayerEventDispatch::SendAddProperty(
                  12u, 34u, one_property) ==
                  one_property.first_result &&
                  one_property.trace == std::vector<std::string>{
                      "router", "add_property"},
              "single property event returns its dispatch result");
        Runtime one_money;
        Check(pal4::ActorManagerPlayerEventDispatch::SendAddMoney(
                  0xabcdef01u, one_money) == one_money.first_result &&
                  one_money.trace == std::vector<std::string>{
                      "router", "add_money"},
              "single money event returns its dispatch result");
        Runtime failed_pickup;
        Check(pal4::ActorManagerPlayerEventDispatch::PickUpFail(
                  0x12345678u, failed_pickup) ==
                  failed_pickup.first_result &&
                  failed_pickup.trace == std::vector<std::string>{
                      "router", "pick_up_fail"},
              "pick-up failure sends highest receiver-address byte");
        std::cout << "actor_manager_player_event_dispatch_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
