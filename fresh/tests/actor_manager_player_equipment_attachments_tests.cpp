#include "pal4/actor_manager_player_equipment_attachments.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Attachments = pal4::ActorManagerPlayerEquipmentAttachments;
using Request = pal4::ActorManagerAttachmentRequest;
constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t animation = 0x6000u;
constexpr std::uint32_t equipment = 0x5000u;

struct Call {
    bool left;
    Request request;
};

struct Runtime final : pal4::IActorManagerPlayerEquipmentRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::vector<Call> calls;
    std::uint32_t found = equipment;
    bool differs = false;
    bool change_slot_after_first = false;

    Runtime() {
        words[actor + 896u] = 0x42u;
        words[equipment + 220u] = 0x7001u;
        words[equipment + 236u] = 0x7002u;
        words[equipment + 204u] = 0x7003u;
        words[equipment + 252u] = 0x7004u;
        words[equipment + 316u] = 0x7005u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t InventorySubsystem() override {
        trace.emplace_back("inventory");
        return 0x9000u;
    }
    std::uint32_t FindEquipment(std::uint32_t collection,
                                std::uint32_t id,
                                std::uint32_t type) override {
        Check(collection == 0x907cu && id == 0x42u && type == 7u,
              "inventory collection, item and record type");
        trace.emplace_back("find");
        return found;
    }
    bool TextDiffers(std::uint32_t first,
                     std::uint32_t second) override {
        Check(first == animation + 496u && second == actor + 1144u,
              "slot zero compares animation and actor text records");
        trace.emplace_back("text_check");
        return differs;
    }
    std::uint32_t LoadLeft(std::uint32_t actual_animation,
                           const Request& request) override {
        Check(actual_animation == animation && request.active == 1u,
              "left attachment receiver and active flag");
        calls.push_back({true, request});
        trace.emplace_back("left");
        if (change_slot_after_first && calls.size() == 1u)
            words[actor + 1192u] = 9u;
        return 0x11u;
    }
    std::uint32_t LoadRight(std::uint32_t actual_animation,
                            const Request& request) override {
        Check(actual_animation == animation && request.active == 1u,
              "right attachment receiver and active flag");
        calls.push_back({false, request});
        trace.emplace_back("right");
        if (change_slot_after_first && calls.size() == 1u)
            words[actor + 1192u] = 9u;
        return 0x22u;
    }
};

void Expect(const Call& call, bool left,
            const std::array<std::uint32_t, 5>& text,
            std::uint32_t slot, const char* message) {
    Check(call.left == left && call.request.text == text &&
              call.request.slot == slot && call.request.active == 1u,
          message);
}

std::array<std::uint32_t, 5> Dynamic(
    const std::uint32_t primary) {
    return {primary, 0x7002u, 0x7003u, 0x7004u, 0x7005u};
}

std::array<std::uint32_t, 5> Single(
    const std::uint32_t primary) {
    return {primary, Attachments::kEmptyError, Attachments::kEmptyError,
            Attachments::kEmptyError, Attachments::kEmptyError};
}

void UnknownSlotAndFoundSlotZero() {
    Runtime unknown;
    unknown.words[actor + 1192u] = 4u;
    Check(Attachments::Refresh(actor, animation, unknown) == 4u &&
              unknown.calls.empty() &&
              unknown.trace[1] == "inventory" &&
              unknown.trace[2] == "find",
          "unknown slot still queries inventory then returns slot");

    Runtime dynamic;
    dynamic.words[actor + 1192u] = 0u;
    dynamic.words[equipment + 236u] = 0u;
    Check(Attachments::Refresh(actor, animation, dynamic) == 0x11u &&
              dynamic.calls.size() == 1u &&
              dynamic.trace[4] == "text_check",
          "found slot zero checks animation text before loading");
    auto expected = Dynamic(0x7001u);
    expected[1] = Attachments::kEmptyText;
    Expect(dynamic.calls[0], true, expected, 0u,
           "found slot zero uses record fields and empty-text fallback");
}

void SlotZeroStaticChoices() {
    for (const bool found : {false, true}) {
        for (const std::uint32_t version : {120299u, 120300u}) {
            Runtime runtime;
            runtime.found = found ? equipment : 0u;
            runtime.words[actor + 1192u] = 0u;
            runtime.words[Attachments::kVersion] = version;
            runtime.differs = true;
            Check(Attachments::Refresh(actor, animation, runtime) == 0x11u &&
                      runtime.calls.size() == 1u,
                  "different slot zero loads one fixed left attachment");
            const bool early = version < 120300u;
            Expect(runtime.calls[0], true,
                   {early ? Attachments::kJq02a : Attachments::kJqja,
                    early ? Attachments::kJq02 : Attachments::kJqj1,
                    Attachments::kEmptyError,
                    Attachments::kEmptyError,
                    Attachments::kEmptyError}, 0u,
                   "version threshold selects exact fixed text pair");
        }
    }

    Runtime absent;
    absent.found = 0u;
    absent.words[actor + 1192u] = 0u;
    Check(Attachments::Refresh(actor, animation, absent) == 0x11u &&
              absent.calls.size() == 1u,
          "missing equipment with equal text loads fallback");
    Expect(absent.calls[0], true, Single(Attachments::kWta), 0u,
           "slot zero fallback uses WTA and four empty-error strings");

    Runtime signed_version;
    signed_version.words[actor + 1192u] = 0u;
    signed_version.differs = true;
    signed_version.words[Attachments::kVersion] = 0xffffffffu;
    Check(Attachments::Refresh(actor, animation, signed_version) == 0x11u,
          "negative signed version still loads static attachment");
    Expect(signed_version.calls[0], true,
           {Attachments::kJq02a, Attachments::kJq02,
            Attachments::kEmptyError, Attachments::kEmptyError,
            Attachments::kEmptyError}, 0u,
           "signed version comparison treats negative as early");
}

void FoundSlotOneVariants() {
    Runtime variant;
    variant.words[actor + 1192u] = 1u;
    variant.words[equipment + 288u] = 4u;
    variant.words[equipment + 284u] = 0x7100u;
    variant.words[equipment + 304u] = 4u;
    variant.words[equipment + 300u] = 0x7200u;
    variant.change_slot_after_first = true;
    Check(Attachments::Refresh(actor, animation, variant) == 0x22u &&
              variant.calls.size() == 2u &&
              variant.trace[variant.trace.size() - 1u] == "right",
          "found slot one returns second right-hand result");
    Expect(variant.calls[0], true, Dynamic(0x7100u), 1u,
           "found slot one left uses its nonempty variant");
    Expect(variant.calls[1], false, Dynamic(0x7200u), 9u,
           "found slot one right rereads actor slot after left callback");

    Runtime base;
    base.words[actor + 1192u] = 1u;
    Check(Attachments::Refresh(actor, animation, base) == 0x22u &&
              base.calls.size() == 2u,
          "found slot one loads both hands without variants");
    Expect(base.calls[0], true, Dynamic(0x7001u), 1u,
           "empty left variant falls back to base record");
    Expect(base.calls[1], false, Dynamic(0x7001u), 1u,
           "empty right variant falls back to base record");
}

void RemainingSlots() {
    Runtime two;
    two.words[actor + 1192u] = 2u;
    Check(Attachments::Refresh(actor, animation, two) == 0x11u &&
              two.calls.size() == 1u,
          "found slot two loads left");
    Expect(two.calls[0], true, Dynamic(0x7001u), 2u,
           "found slot two uses equipment index four name");

    Runtime three;
    three.words[actor + 1192u] = 3u;
    Check(Attachments::Refresh(actor, animation, three) == 0x22u &&
              three.calls.size() == 1u,
          "found slot three loads right");
    Expect(three.calls[0], false, Dynamic(0x7001u), 3u,
           "found slot three uses base equipment name");

    Runtime missing_one;
    missing_one.found = 0u;
    missing_one.words[actor + 1192u] = 1u;
    missing_one.change_slot_after_first = true;
    Check(Attachments::Refresh(actor, animation, missing_one) == 0x11u &&
              missing_one.calls.size() == 2u,
          "missing slot one returns left-hand result after right call");
    Expect(missing_one.calls[0], false,
           Single(Attachments::kWl01), 1u,
           "missing slot one first loads fixed right hand");
    Expect(missing_one.calls[1], true,
           Single(Attachments::kWl01), 9u,
           "missing slot one left rereads actor slot");

    Runtime missing_two;
    missing_two.found = 0u;
    missing_two.words[actor + 1192u] = 2u;
    Check(Attachments::Refresh(actor, animation, missing_two) == 0x11u,
          "missing slot two returns left load");
    Expect(missing_two.calls[0], true, Single(Attachments::kWm01), 2u,
           "missing slot two uses WM01");

    Runtime missing_three;
    missing_three.found = 0u;
    missing_three.words[actor + 1192u] = 3u;
    Check(Attachments::Refresh(actor, animation, missing_three) == 0x22u,
          "missing slot three returns right load");
    Expect(missing_three.calls[0], false, Single(Attachments::kWta), 3u,
           "missing slot three uses WTA");
}

}  // namespace

int main() {
    try {
        UnknownSlotAndFoundSlotZero();
        SlotZeroStaticChoices();
        FoundSlotOneVariants();
        RemainingSlots();
        std::cout << "actor_manager_player_equipment_attachments_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
