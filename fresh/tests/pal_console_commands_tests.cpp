#include "pal4/pal_console_commands.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Expected {
    std::uint32_t callback;
    std::string_view name;
    std::size_t help_bytes;
    std::uint32_t help_hash;
};

constexpr std::array<Expected, pal4::PalConsoleCommands::kCount> kExpected{{
    {0x5bb3d0u, "exit", 14u, 0x91d85700u},
    {0x5bb3e0u, "setPlayerPos", 28u, 0xf92f6b50u},
    {0x5bb8f0u, "showDialog", 27u, 0x6aeafff9u},
    {0x5bb550u, "LA", 24u, 0xdd42c1b7u},
    {0x5bba50u, "setGlobalVar", 21u, 0xbe9945d7u},
    {0x5bbac0u, "setPlayerLevel", 28u, 0x1f543fc4u},
    {0x5bbcd0u, "setPlayerJoin", 26u, 0x722c765cu},
    {0x5bbd50u, "setPlayerLeave", 26u, 0xf165c9a0u},
    {0x5bbdd0u, "showMeTheMoney", 26u, 0xfa3cfa0u},
    {0x5bbe40u, "masterSkill", 24u, 0xcdcaea53u},
    {0x5bbfe0u, "addProp", 18u, 0x1bd1a8f1u},
    {0x5bc0b0u, "addEquip", 13u, 0xc142283du},
    {0x5bc150u, "setPlayerHp", 23u, 0x65c260cau},
    {0x5bc230u, "setPlayerMp", 23u, 0x237ba84u},
    {0x5bc310u, "setPlayerRage", 25u, 0x59a0b798u},
    {0x5bc3f0u, "setGodMode", 30u, 0x453cfe45u},
    {0x5bc420u, "openMovieFlag", 22u, 0xecd57e3eu},
    {0x5bc470u, "resetGlobalData", 20u, 0x64a82cb2u},
    {0x5bc4e0u, "setRoundTimes", 23u, 0x6079d02eu},
    {0x5bc5c0u, "setAutoSeek", 25u, 0x2c43bfcbu},
    {0x5bc650u, "setCameraMode", 32u, 0xacc0f693u},
    {0x5bc7a0u, "setControlPlayer", 16u, 0xf2bfaaaau},
    {0x5bc870u, "setDistOptEnable", 21u, 0x5f93da96u},
    {0x5bc900u, "killAllMst", 18u, 0xd97d8583u},
    {0x5bc920u, "startPuzzle", 26u, 0xc5cc254du},
    {0x5bcb80u, "startJigsaw", 23u, 0xb055445bu},
    {0x5bcde0u, "setMazeVar", 11u, 0x4d0adc1au},
    {0x5bcf20u, "openWeather", 53u, 0xaf6bdde7u},
    {0x5bcf90u, "closeWeather", 6u, 0x687f220cu},
    {0x5d66d0u, "playerGarb2", 22u, 0x8e128df0u},
    {0x5bcfa0u, "playerGarb1", 22u, 0x42eaba87u},
    {0x5bd1b0u, "jumpUnLock", 28u, 0x851d2a3au},
    {0x5bd1c0u, "setControlMode", 31u, 0xc6da7ca7u},
    {0x5bd240u, "openSystemUi", 102u, 0x610fa3d3u},
    {0x5bd300u, "openMagicUi", 39u, 0xc7721ce6u},
    {0x5bd370u, "openStuntUi", 39u, 0xf7087a35u},
    {0x5bd3e0u, "takeonWPN", 14u, 0xb266fba2u},
    {0x5bd420u, "takeoffWPN", 14u, 0xb3ddcd84u},
    {0x5bd460u, "showVar", 14u, 0x14403446u},
    {0x5bdb20u, "setVar", 10u, 0xc01e159cu},
    {0x5be200u, "setForbidenSkill", 41u, 0x9ab07b26u},
    {0x5be470u, "getPlayerAttr", 13u, 0xb3445e03u},
    {0x5be650u, "setPlayerAttr", 22u, 0x8b1f169bu},
    {0x5be870u, "getBasicAttr", 13u, 0xb3445e03u},
    {0x5bea50u, "setBasicAttr", 22u, 0x8b1f169bu},
    {0x5bec70u, "setPetShow", 13u, 0xf141a8b3u},
    {0x5bed00u, "enableSTS", 25u, 0xdab8cefu},
    {0x5bed70u, "resetUiTimer", 32u, 0xbec4dbadu},
}};

std::uint32_t Hash(std::string_view bytes) {
    std::uint32_t value = 2166136261u;
    for (unsigned char byte : bytes)
        value = (value ^ byte) * 16777619u;
    return value;
}

struct AccessRuntime final : pal4::IPalConsoleAccessRuntime {
    std::uint32_t manager_calls = 0u;
    std::uint32_t register_calls = 0u;

    std::uint32_t GetSingletonManager() override {
        ++manager_calls;
        return 0x700u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destroy) override {
        ++register_calls;
        Check(manager == 0x700u && name == "palConsole" && id == 1008u &&
                  factory == 0x418f20u && destroy == 0x418f80u,
              "console singleton operands");
        return 0x1000u;
    }
};

struct Copy {
    std::uint32_t address;
    std::uint32_t limit;
    std::string text;
};

struct ObjectRuntime final : pal4::IPalConsoleObjectRuntime {
    pal4::PalConsoleState state;
    std::vector<std::string> events;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<Copy> copies;
    std::uint32_t access_calls = 0u;

    std::uint32_t Allocate(std::uint32_t) override {
        throw std::runtime_error("unexpected allocation");
    }
    std::uint32_t Free(std::uint32_t) override {
        throw std::runtime_error("unexpected free");
    }
    pal4::PalConsoleState& Access(std::uint32_t address) override {
        Check(address == 0x1000u, "cached console address");
        ++access_calls;
        events.emplace_back("access");
        return state;
    }
    std::uint32_t ReadDword(std::uint32_t) override {
        throw std::runtime_error("unexpected word read");
    }
    std::uint8_t ReadByte(std::uint32_t) override {
        throw std::runtime_error("unexpected byte read");
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        events.emplace_back("write");
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {
        throw std::runtime_error("unexpected byte write");
    }
    void CopyString(std::uint32_t destination, const char* source,
                    std::uint32_t count) override {
        copies.push_back({destination, count, source});
        events.emplace_back(count == 32u ? "name" : "help");
    }
};

void CatalogMatchesIda() {
    const auto& commands = pal4::PalConsoleCommands::Catalog();
    for (std::size_t i = 0; i < commands.size(); ++i) {
        const auto& command = commands[i];
        const auto& expected = kExpected[i];
        const std::string_view description(command.description);
        Check(command.callback == expected.callback &&
                  command.name == expected.name &&
                  description.size() == expected.help_bytes &&
                  Hash(description) == expected.help_hash,
              "command callback, name and original GBK bytes");
    }
    Check(std::strlen(commands[33].description) == 102u,
          "long system-UI help stays intact before native bounded copy");
}

void RegistrationOrderAndGeometry() {
    AccessRuntime access;
    ObjectRuntime object;
    pal4::PalConsoleAccessState state;
    Check(pal4::PalConsoleCommands::Initialize(
              state, access, object, 1024, 769) == 1u,
          "last command result");
    Check(access.manager_calls == 1u && access.register_calls == 1u &&
              state.instance == 0x1000u && (state.guard & 1u) != 0u,
          "all console accesses reuse one singleton");
    Check(object.access_calls == 49u && object.writes.size() == 48u &&
              object.copies.size() == 96u &&
              object.state.ReadDword(pal4::PalConsoleState::kCommandCount) ==
                  48u,
          "geometry access then 48 command insertions");
    Check(object.state.ReadDword(0x08u) == 386u &&
              object.state.ReadDword(0x10u) == 404u &&
              object.state.ReadDword(0x1cu) == 1024u &&
              object.state.ReadDword(0x20u) == 384u &&
              object.state.ReadDword(0x24u) == 0xff20ee20u &&
              object.state.ReadDword(0x28u) == 0xffeeeeeeu,
          "native rectangle geometry and colors");
    Check(object.events.size() == 193u &&
              object.events.front() == "access",
          "first operation accesses console for geometry");
    for (std::size_t i = 0; i < kExpected.size(); ++i) {
        const auto base = 1u + i * 4u;
        const auto record = 0x1000u + 0x66cu +
                            static_cast<std::uint32_t>(i) * 136u;
        Check(object.events[base] == "access" &&
                  object.events[base + 1u] == "write" &&
                  object.events[base + 2u] == "name" &&
                  object.events[base + 3u] == "help" &&
                  object.writes[i] ==
                      std::pair<std::uint32_t, std::uint32_t>{
                          record, kExpected[i].callback} &&
                  object.copies[2u * i].address == record + 4u &&
                  object.copies[2u * i].limit == 32u &&
                  object.copies[2u * i].text == kExpected[i].name &&
                  object.copies[2u * i + 1u].address == record + 36u &&
                  object.copies[2u * i + 1u].limit == 100u &&
                  Hash(object.copies[2u * i + 1u].text) ==
                      kExpected[i].help_hash,
              "command record fields and registration sequence");
    }
}

void CapacityAndReturn() {
    for (const auto initial_count : {63u, 64u}) {
        AccessRuntime access;
        ObjectRuntime object;
        pal4::PalConsoleAccessState state;
        object.state.WriteDword(
            pal4::PalConsoleState::kCommandCount, initial_count);
        Check(pal4::PalConsoleCommands::Initialize(
                  state, access, object, 640, 480) == 0u &&
                  object.state.ReadDword(
                      pal4::PalConsoleState::kCommandCount) == 64u &&
                  object.writes.size() == 64u - initial_count &&
                  object.copies.size() == 2u * (64u - initial_count) &&
                  object.access_calls == 49u,
              "capacity rejects remaining commands and forwards final zero");
    }
}

}  // namespace

int main() {
    try {
        CatalogMatchesIda();
        RegistrationOrderAndGeometry();
        CapacityAndReturn();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
