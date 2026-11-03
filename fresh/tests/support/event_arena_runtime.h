#pragma once

#include "pal4/game_arena_load_handler.h"

#include <optional>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pal4::test {

struct EventArenaRuntime final : IGameArenaLoadRuntime {
    bool loading = false;
    int requests = 0;
    std::string world;
    std::string area;
    std::function<void()> enter_loading;

    std::uint32_t ArenaManager() override { return 0x1000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return address == 0x10c4u ? 1u : 0u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t InitializeXmlDocument() override { return 0x2000u; }
    std::optional<std::uint32_t> FindAttribute(
        std::uint32_t, std::string_view) override { return std::nullopt; }
    void SetInt(std::uint32_t, std::int32_t) override {}
    void SetString(std::uint32_t, std::string_view) override {}
    std::uint32_t InitializeIntAttribute(
        std::string_view, std::int32_t) override { return 0x3000u; }
    std::uint32_t InitializeStringAttribute(
        std::string_view key, std::string_view value) override {
        if (key == "PALGAMEIV_LOADING_ARENA") world = value;
        if (key == "PALGAMEIV_LOADING_AREA") area = value;
        return 0x3000u;
    }
    void AddAttribute(std::uint32_t, std::uint32_t) override {}
    void DestroyAttribute(std::uint32_t) override {}
    std::uint32_t GameInstance() override { return 0x4000u; }
    void InitializeStateAndUi(
        std::uint32_t, std::uint32_t state, std::uint32_t) override {
        if (state != 6u)
            throw std::runtime_error("event did not request Loading");
        loading = true;
        ++requests;
        if(enter_loading)enter_loading();
    }
    std::uint32_t ActorControl() override { return 0x5000u; }
    void ProcessActorInput(std::uint32_t) override {}
    void ProcessActorInputAndClearVector(std::uint32_t) override {}
    std::uint32_t InputManager() override { return 0x6000u; }
    void InitializeInputState(std::uint32_t) override {}
    std::uint32_t Milliseconds() override { return 123u; }
    std::uint32_t GameState() override { return 0x7000u; }
    std::int32_t DestroyXmlDocument(std::uint32_t) override { return 0; }
};

}  // namespace pal4::test
