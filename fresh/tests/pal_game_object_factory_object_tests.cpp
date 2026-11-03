#include "pal4/pal_game_object_factory_object.h"

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

struct Runtime final : pal4::IPalGameObjectFactoryObjectRuntime {
    std::uint32_t allocation = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate " + std::to_string(size));
        return allocation;
    }
    void FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write " + std::to_string(address));
        words[address] = value;
    }
};

struct ControlRuntime final : pal4::IPalGameObjectFactoryControlRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t virtual_object = 0u;
    std::uint32_t virtual_offset = 0u;
    std::uint32_t virtual_value = 0u;

    std::uint32_t MovementAdapter() override {
        calls.emplace_back("movement");
        return 0x2000u;
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor");
        return 0x3000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void InvokeActorControlProperty(
        std::uint32_t object, std::uint32_t offset,
        std::uint32_t value) override {
        calls.emplace_back("virtual");
        virtual_object = object;
        virtual_offset = offset;
        virtual_value = value;
    }
};

void CheckFactories() {
    Check(pal4::PalGameObjectFactoryObject::ReturnZero(0xffffffffu) == 0u &&
          pal4::PalGameObjectFactoryObject::ReturnOne(0u) == 1u,
          "factory vtable constant callbacks");

    Runtime climb;
    Check(pal4::PalGameObjectFactoryObject::CreateClimb(climb) == 0x1000u &&
          climb.words.at(0x1000u) == 0x849318u &&
          climb.words.at(0x1004u) == 0u &&
          climb.calls == (std::vector<std::string>{
              "allocate 8", "write 4100", "write 4096"}),
          "climb factory allocation and layout");

    Runtime normal;
    Check(pal4::PalGameObjectFactoryObject::CreateNormal(normal) == 0x1000u &&
          normal.words.at(0x1000u) == 0x849340u &&
          normal.words.at(0x1004u) == 0u &&
          normal.calls == climb.calls,
          "normal factory allocation and layout");

    Runtime direct;
    Check(pal4::PalGameObjectFactoryObject::ConstructNormal(
              0x2000u, direct) == 0x2000u &&
          direct.words.at(0x2000u) == 0x849340u &&
          direct.words.at(0x2004u) == 0u &&
          direct.calls == (std::vector<std::string>{
              "write 8196", "write 8192"}),
          "normal direct constructor");

    Runtime failure;
    failure.allocation = 0u;
    Check(pal4::PalGameObjectFactoryObject::CreateClimb(failure) == 0u &&
          pal4::PalGameObjectFactoryObject::CreateNormal(failure) == 0u &&
          failure.words.empty() &&
          failure.calls == (std::vector<std::string>{
              "allocate 8", "allocate 8"}),
          "null allocation never writes object");

    Runtime sound;
    Check(pal4::PalGameObjectFactoryObject::CreateSound(sound) == 0x1000u &&
          sound.words.at(0x1000u) == 0x849354u &&
          sound.words.at(0x1004u) == 0u &&
          sound.calls == climb.calls,
          "sound factory handle layout");
    Runtime local_float;
    Check(pal4::PalGameObjectFactoryObject::CreateLocalFloat(
              local_float) == 0x1000u &&
          local_float.words.at(0x1000u) == 0x849368u &&
          local_float.words.at(0x1004u) == 0u &&
          local_float.calls == climb.calls,
          "local float factory handle layout");
    Check(pal4::PalGameObjectFactoryObject::CreateSound(failure) == 0u &&
          pal4::PalGameObjectFactoryObject::CreateLocalFloat(failure) == 0u &&
          failure.words.empty(),
          "sound and local float allocation failure");

    Runtime floor;
    Check(pal4::PalGameObjectFactoryObject::CreateLocalFloor(floor) ==
              0x1000u &&
          floor.words.at(0x1000u) == 0x8493c0u &&
          floor.words.at(0x1004u) == 0u &&
          floor.calls == climb.calls,
          "local-floor factory handle layout");
    Runtime machine;
    Check(pal4::PalGameObjectFactoryObject::CreateMachine(machine) ==
              0x1000u &&
          machine.words.at(0x1000u) == 0x8493d4u &&
          machine.words.at(0x1004u) == 0u &&
          machine.calls == climb.calls,
          "machine factory handle layout");
    Check(pal4::PalGameObjectFactoryObject::CreateLocalFloor(failure) == 0u &&
          pal4::PalGameObjectFactoryObject::CreateMachine(failure) == 0u &&
          failure.words.empty(),
          "both additional factories preserve null allocation");

    Runtime base;
    pal4::PalGameObjectFactoryObject::DestructBase(0x1000u, base);
    Check(base.words.at(0x1000u) == 0x84932cu &&
          base.words.at(0x1004u) == 0u &&
          base.calls == (std::vector<std::string>{
              "write 4096", "write 4100"}),
          "factory base destructor layout");
    Runtime nonowning;
    Check(pal4::PalGameObjectFactoryObject::Delete(
              0x1000u, 0u, nonowning) == 0x1000u &&
          nonowning.calls == base.calls,
          "nonowning deleting destructor");
    Runtime owning;
    Check(pal4::PalGameObjectFactoryObject::Delete(
              0x1000u, 3u, owning) == 0x1000u &&
          owning.calls == (std::vector<std::string>{
              "write 4096", "write 4100", "free 4096"}),
          "owning deleting destructor frees after base cleanup");
}

void CheckControlCallbacks() {
    ControlRuntime movement;
    Check(pal4::PalGameObjectFactoryObject::SetMovementFlagOne(
              0xffffffffu, movement) == 0u &&
          movement.words.at(0x2010u) == 1u &&
          movement.calls ==
              (std::vector<std::string>{"movement", "write"}),
          "movement enable callback and unused argument");
    movement.calls.clear();
    Check(pal4::PalGameObjectFactoryObject::SetMovementFlagZero(
              movement) == 1u &&
          movement.words.at(0x2010u) == 0u &&
          movement.calls ==
              (std::vector<std::string>{"movement", "write"}),
          "movement disable callback");

    ControlRuntime actor;
    Check(pal4::PalGameObjectFactoryObject::SetActorPropertyZero(
              0xffffffffu, actor) == 0u &&
          actor.virtual_object == 0x308cu &&
          actor.virtual_offset == 32u &&
          actor.virtual_value == 0u &&
          actor.calls == (std::vector<std::string>{"actor", "virtual"}),
          "actor property zero virtual dispatch");
    actor.calls.clear();
    Check(pal4::PalGameObjectFactoryObject::SetActorPropertyOne(
              actor) == 1u &&
          actor.virtual_object == 0x308cu &&
          actor.virtual_offset == 32u &&
          actor.virtual_value == 1u &&
          actor.calls == (std::vector<std::string>{"actor", "virtual"}),
          "actor property one virtual dispatch");
}

}  // namespace

int main() {
    try {
        CheckFactories();
        CheckControlCallbacks();
        std::cout << "pal_game_object_factory_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
