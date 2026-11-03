#include "pal4/property_window_script_action.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IPropertyWindowScriptRuntime {
public:
    Runtime() {
        Put(0x1000u, "arena");
        Put(0x1100u, "PFNSkipScript");
        Put(0x1200u, "[EarthBall]");
    }

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        auto result = next_address;
        next_address += bytes + 16u;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return memory[address];
    }
    std::uint32_t ReadDword(std::uint32_t) override { return 0u; }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t bytes) override {
        calls.push_back("copy:" + std::to_string(bytes));
        for (std::uint32_t i = 0; i < bytes; ++i)
            memory[destination + i] = memory[source + i];
    }
    void WriteDword(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t address, std::uint8_t byte) override {
        memory[address] = byte;
    }
    std::uint32_t FindSubstring(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t FindCharacter(std::uint32_t, std::int32_t) override {
        return 0u;
    }
    std::uint32_t FindLastCharacter(std::uint32_t, std::int32_t) override {
        return 0u;
    }
    std::int32_t CompareCaseInsensitive(std::uint32_t,
                                        std::uint32_t) override { return 0; }
    void ConvertUppercase(std::uint32_t) override {}
    void ConvertLowercase(std::uint32_t) override {}

    void PlayMusic(std::string_view file, std::uint32_t mode) override {
        Check(file == "WA43.wav" && mode == 0u, "sound and mode");
        calls.emplace_back("play");
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x3000u;
    }
    pal4::StringBufferState ReadStringState(std::uint32_t address) override {
        Check(address == 0x30d4u, "arena context offset");
        calls.emplace_back("context");
        return {0x1000u, 5u, 32u};
    }
    void InitializeScripts() override { calls.emplace_back("initialize"); }
    std::uint32_t LiteralAddress(pal4::EarthballLiteral literal) override {
        return literal == pal4::EarthballLiteral::ScriptPath ? 0x1100u
                                                             : 0x1200u;
    }
    std::uint32_t ScriptEngine() override {
        calls.emplace_back("engine");
        return 0x4000u;
    }
    bool HasStackModule(std::uint32_t cache,
                       std::uint32_t path) override {
        Check(cache == 0x4054u && Text(path) == "PFNSkipScript[EarthBall]",
              "cache key");
        calls.emplace_back("cache");
        return cache_hit;
    }
    std::uint32_t ScriptMode() override {
        calls.emplace_back("mode");
        return 2u;
    }
    std::uint32_t CreateInterpreter(std::uint32_t mode,
                                    std::uint32_t base,
                                    std::uint32_t full,
                                    std::string_view function,
                                    std::uint32_t context,
                                    std::uint32_t reader_callback) override {
        Check(mode == 2u && Text(base) == "PFNSkipScript" &&
                  Text(full) == "PFNSkipScript[EarthBall]" &&
                  function == "EarthBall" && Text(context) == "arena" &&
                  reader_callback ==
                      pal4::PropertyWindowScriptAction::kReadToBufferCallback,
              "interpreter arguments");
        calls.emplace_back("create");
        return interpreter;
    }
    bool AddScriptModule(std::uint32_t cache,
                         std::uint32_t object) override {
        Check(cache == 0x4054u && object == interpreter,
              "registration arguments");
        calls.emplace_back("add-module");
        return module_added;
    }
    std::uint32_t FunctionName(std::uint32_t object) override {
        Check(object == interpreter, "function name interpreter");
        calls.emplace_back("function-name");
        return 0x6000u;
    }
    std::uint32_t FullPath(std::uint32_t object) override {
        Check(object == interpreter, "what interpreter");
        calls.emplace_back("full-path");
        return 0x6100u;
    }
    void PushStackModule(std::uint32_t cache,
                         std::uint32_t what,
                         std::uint32_t name) override {
        Check(cache == 0x4054u && what == 0x6100u && name == 0x6000u,
              "stack path/function order");
        calls.emplace_back("push-module");
    }
    void ReleaseInterpreter(std::uint32_t object) override {
        Check(object == interpreter, "release interpreter");
        calls.emplace_back("release-interpreter");
    }
    void CleanupAllScripts() override {
        calls.emplace_back("cleanup-scripts");
        if (throw_during_cleanup) throw std::runtime_error("cleanup");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui-manager");
        return 0x5000u;
    }
    void ProcessEvents(std::uint32_t table, std::uint32_t id,
                       const std::array<std::uint32_t, 2>& payload) override {
        Check(table == 0x5104u &&
                  id == pal4::PropertyWindowScriptAction::kEventId &&
                  payload == std::array<std::uint32_t, 2>{0u, 0u},
              "UI event ID and zero payload");
        calls.emplace_back("ui-event");
    }

    std::string Text(std::uint32_t address) const {
        std::string result;
        for (std::uint32_t i = 0; i < 128u; ++i) {
            auto found = memory.find(address + i);
            if (found == memory.end() || found->second == 0u) break;
            result.push_back(static_cast<char>(found->second));
        }
        return result;
    }
    void Put(std::uint32_t address, std::string_view text) {
        for (std::size_t i = 0; i < text.size(); ++i)
            memory[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        memory[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }

    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> calls;
    std::uint32_t next_address = 0x7000u;
    std::uint32_t interpreter = 0x8000u;
    bool cache_hit = false;
    bool module_added = false;
    bool throw_during_cleanup = false;
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    return runtime.calls.size();
}

void CheckFinalization(const Runtime& runtime) {
    const auto event = Index(runtime, "ui-event");
    Check(Index(runtime, "cleanup-scripts") < event,
          "script cleanup precedes UI broadcast");
    Check(event + 3u < runtime.calls.size(), "all strings cleaned");
    Check(runtime.calls[event + 1u].find("free:") == 0u &&
              runtime.calls[event + 2u].find("free:") == 0u &&
              runtime.calls[event + 3u].find("free:") == 0u,
          "three strings freed after event");
    Check(runtime.calls.back() == "free:28672",
          "arena context freed last");
}

void RunCases() {
    Runtime cache;
    cache.cache_hit = true;
    Check(pal4::PropertyWindowScriptAction::ExecuteEarthballSkip(cache) ==
              0x7001u, "returns context cleanup result");
    Check(Index(cache, "create") == cache.calls.size() &&
              Index(cache, "add-module") == cache.calls.size(),
          "cache hit skips interpreter");
    CheckFinalization(cache);

    Runtime failed;
    failed.interpreter = 0u;
    static_cast<void>(pal4::PropertyWindowScriptAction::ExecuteEarthballSkip(failed));
    Check(Index(failed, "create") < Index(failed, "cleanup-scripts") &&
              Index(failed, "add-module") == failed.calls.size(),
          "null interpreter skips registration");
    CheckFinalization(failed);

    Runtime normal;
    static_cast<void>(pal4::PropertyWindowScriptAction::ExecuteEarthballSkip(normal));
    Check(Index(normal, "add-module") < Index(normal, "cleanup-scripts") &&
              Index(normal, "push-module") == normal.calls.size(),
          "failed registration does not push a module");
    CheckFinalization(normal);

    Runtime registered;
    registered.module_added = true;
    static_cast<void>(pal4::PropertyWindowScriptAction::ExecuteEarthballSkip(registered));
    Check(Index(registered, "function-name") < Index(registered, "full-path") &&
              Index(registered, "full-path") < Index(registered, "push-module") &&
              Index(registered, "push-module") <
                  Index(registered, "release-interpreter"),
          "registered module push and interpreter release order");
    CheckFinalization(registered);

    Runtime throwing;
    throwing.throw_during_cleanup = true;
    try {
        static_cast<void>(pal4::PropertyWindowScriptAction::ExecuteEarthballSkip(throwing));
        Check(false, "expected cleanup exception");
    } catch (const std::runtime_error&) {
        Check(throwing.calls.back() == "free:28672" &&
                  Index(throwing, "ui-event") == throwing.calls.size(),
              "SEH-style unwind frees strings before UI dispatch");
    }
}

}  // namespace

int main() { RunCases(); }
