#include "pal4/shop_ui_script.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiScriptRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kFrame, 0x6000u}, {0x6004u, 0x6100u},
        {kFrame + 0x74u, 0x9001u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::string> appended;
    bool cache_hit = false;
    bool add_result = true;
    bool throw_create = false;
    std::uint32_t interpreter = 0xc000u;
    std::uint32_t next_copy = 0u;
    std::uint32_t empty_lookups = 0u;
    std::uint32_t messages = 0u;
    std::uint32_t pushed = 0u;
    std::uint32_t released = 0u;
    std::uint32_t cleanup_all = 0u;

    void GetData(std::uint32_t, std::uint32_t,
                 std::uint32_t) override {
        Check(false, "task execution does not select shop data");
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void CallVisibilitySlot(const std::uint32_t function,
                            const std::uint32_t frame,
                            const std::uint32_t visible,
                            const std::uint32_t argument) override {
        Check(function == 0x6100u && frame == kFrame &&
                  visible == 0u && argument == 0u,
              "task script hides shop through virtual slot");
        calls.emplace_back("hide");
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade system");
        return 0x6200u;
    }
    std::uint32_t SetTradeState(
        const std::uint32_t system) override {
        Check(system == 0x6200u, "task script clears trade state");
        calls.emplace_back("trade state");
        return 0x1234u;
    }
    std::uint32_t GameArenaManager() override {
        calls.emplace_back("arena");
        return 0x7000u;
    }
    std::uint32_t CopyTextRecord(
        const std::uint32_t source) override {
        calls.emplace_back("copy text");
        if (next_copy++ == 0u) {
            Check(source == 0x70d4u, "arena context source +0xd4");
            return 0xa000u;
        }
        Check(source == 0xa100u, "script path copied to full name");
        return 0xa200u;
    }
    void InitializeScripts() override {
        calls.emplace_back("initialize scripts");
    }
    std::uint32_t TextFromCString(
        const std::string_view text) override {
        Check(text == "shop-script", "script package name");
        calls.emplace_back("script path");
        return 0xa100u;
    }
    void AppendLiteral(const std::uint32_t text,
                       const std::string_view literal) override {
        Check(text == 0xa200u &&
                  (literal == "[" || literal == "]"),
              "bracket append target");
        calls.emplace_back("append literal");
        appended.emplace_back(literal);
    }
    void AppendName(const std::uint32_t text,
                    const std::uint32_t name) override {
        Check(text == 0xa200u &&
                  name == (words.at(kFrame + 0x74u) == 0u ?
                               0xbeefu : 0x9001u),
              "task name append pointer or fallback");
        calls.emplace_back("append name");
        appended.emplace_back("task name");
    }
    std::uint32_t TextData(const std::uint32_t text) override {
        Check(text == 0xa000u || text == 0xa100u ||
                  text == 0xa200u,
              "constructed text record handle");
        calls.emplace_back("text data");
        return text + 1u;
    }
    std::uint32_t EmptyText() override {
        ++empty_lookups;
        calls.emplace_back("empty text");
        return 0xbeefu;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0xb000u;
    }
    bool CacheContains(const std::uint32_t cache,
                       const std::uint32_t full_name) override {
        Check(cache == 0xb054u && full_name == 0xa201u,
              "cache checks bracketed shop task name");
        calls.emplace_back("cache");
        return cache_hit;
    }
    std::uint32_t ScriptMode() override {
        calls.emplace_back("mode");
        return 2u;
    }
    std::uint32_t CreateInterpreter(
        const std::uint32_t mode, const std::uint32_t path,
        const std::uint32_t full_name, const std::uint32_t script_name,
        const std::uint32_t context,
        const std::uint32_t reader) override {
        Check(mode == 2u && path == 0xa101u &&
                  full_name == 0xa201u &&
                  script_name == (words.at(kFrame + 0x74u) == 0u ?
                                      0xbeefu : 0x9001u) &&
                  context == 0xa001u && reader == 0x600d50u,
              "interpreter receives mode, paths, name, context and reader");
        calls.emplace_back("create interpreter");
        if (throw_create) throw std::runtime_error("create");
        return interpreter;
    }
    bool AddScriptModule(const std::uint32_t cache,
                            const std::uint32_t value) override {
        Check(cache == 0xb054u && value == interpreter,
              "module-add context and interpreter");
        calls.emplace_back("add module");
        return add_result;
    }
    std::uint32_t FunctionName(const std::uint32_t value) override {
        Check(value == interpreter, "interpreter function-name receiver");
        calls.emplace_back("function name");
        return 0xd000u;
    }
    std::uint32_t FullPath(const std::uint32_t value) override {
        Check(value == interpreter, "interpreter full-path receiver");
        calls.emplace_back("full path");
        return 0xd100u;
    }
    void PushStackModule(const std::uint32_t cache,
                         const std::uint32_t full_path,
                         const std::uint32_t function_name) override {
        Check(cache == 0xb054u && full_path == 0xd100u &&
                  function_name == 0xd000u,
              "module stack path/name");
        calls.emplace_back("push module");
        ++pushed;
    }
    void ReleaseInterpreter(const std::uint32_t value) override {
        Check(value == interpreter, "interpreter release receiver");
        calls.emplace_back("release interpreter");
        ++released;
    }
    void ShowMissingScriptMessage(
        const std::string_view message, const std::string_view title,
        const std::uint32_t flags) override {
        Check(message == "not found script files" &&
                  title == "no script" && flags == 0u,
              "missing script message box contents");
        calls.emplace_back("missing message");
        ++messages;
    }
    void CleanupAllScripts() override {
        calls.emplace_back("cleanup all scripts");
        ++cleanup_all;
    }
    void CleanupTextRecord(const std::uint32_t text) override {
        calls.emplace_back("cleanup text");
        cleaned.push_back(text);
    }
};

void CheckCommonCompletion(const Runtime& runtime) {
    Check(runtime.appended ==
              std::vector<std::string>{"[", "task name", "]"} &&
              runtime.cleaned ==
                  std::vector<std::uint32_t>{
                      0xa200u, 0xa100u, 0xa000u} &&
              runtime.cleanup_all == 1u &&
              runtime.calls.size() >= 7u &&
              runtime.calls[0] == "read" &&
              runtime.calls[1] == "read" &&
              runtime.calls[2] == "hide" &&
              runtime.calls[3] == "trade system" &&
              runtime.calls[4] == "trade state" &&
              runtime.calls[5] == "arena" &&
              runtime.calls[6] == "copy text" &&
              runtime.calls[runtime.calls.size() - 4u] ==
                  "cleanup all scripts",
          "shop close, context copy and reverse cleanup order");
}

void TestCacheAndInterpreterBranches() {
    Runtime cached;
    cached.cache_hit = true;
    Check(pal4::ShopUiScript::ExecuteTask(
              Runtime::kFrame, 0x12345678u, cached) == 1u &&
              cached.messages == 0u && cached.pushed == 0u &&
              std::find(cached.calls.begin(), cached.calls.end(),
                        "create interpreter") == cached.calls.end(),
          "cached script skips creation and message box");
    CheckCommonCompletion(cached);

    Runtime missing;
    missing.interpreter = 0u;
    Check(pal4::ShopUiScript::ExecuteTask(
              Runtime::kFrame, 0u, missing) == 1u &&
              missing.messages == 1u && missing.pushed == 0u &&
              std::find(missing.calls.begin(), missing.calls.end(),
                        "add module") == missing.calls.end(),
          "null interpreter shows missing-script message");
    CheckCommonCompletion(missing);

    Runtime failed_add;
    failed_add.add_result = false;
    Check(pal4::ShopUiScript::ExecuteTask(
              Runtime::kFrame, 0u, failed_add) == 1u &&
              failed_add.messages == 1u &&
              failed_add.released == 0u,
          "failed module addition shows missing-script message");
    CheckCommonCompletion(failed_add);

    Runtime reported;
    Check(pal4::ShopUiScript::ExecuteTask(
              Runtime::kFrame, 0u, reported) == 1u &&
              reported.messages == 0u && reported.pushed == 1u &&
              reported.released == 1u &&
              std::find(reported.calls.begin(), reported.calls.end(),
                        "function name") <
                  std::find(reported.calls.begin(), reported.calls.end(),
                            "full path"),
          "successful module addition pushes stack and releases interpreter");
    CheckCommonCompletion(reported);
}

void TestNullNameAndExceptionCleanup() {
    Runtime empty_name;
    empty_name.words[Runtime::kFrame + 0x74u] = 0u;
    Check(pal4::ShopUiScript::ExecuteTask(
              Runtime::kFrame, 0u, empty_name) == 1u &&
              empty_name.empty_lookups == 2u,
          "null task name uses empty text for append and interpreter");
    CheckCommonCompletion(empty_name);

    Runtime throwing;
    throwing.throw_create = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ShopUiScript::ExecuteTask(
            Runtime::kFrame, 0u, throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.cleaned ==
              (std::vector<std::uint32_t>{
                  0xa200u, 0xa100u, 0xa000u}) &&
              throwing.cleanup_all == 0u &&
              throwing.messages == 0u,
          "interpreter creation exception unwinds three text records");
}

}  // namespace

int main() {
    TestCacheAndInterpreterBranches();
    TestNullNameAndExceptionCleanup();
    return EXIT_SUCCESS;
}
