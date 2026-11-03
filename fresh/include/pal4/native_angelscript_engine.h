#pragma once

#include <angelscript.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <array>
#include <functional>

namespace pal4 {

struct BytecodeLoadResult {
    int code{};
    std::size_t bytes_read{};
};

class IGameScriptCallbackRuntime {
public:
    virtual ~IGameScriptCallbackRuntime() = default;
    virtual void Invoke(int system_id, asIScriptGeneric& arguments) = 0;
};

class NativeAngelScriptEngine final {
public:
    // Borrowed story storage must outlive the engine. Standalone probes own it.
    using GlobalResolver = std::function<std::int32_t*(std::uint32_t original_address)>;
    explicit NativeAngelScriptEngine(std::int32_t* story_variable = nullptr, GlobalResolver globals = {});
    ~NativeAngelScriptEngine();
    NativeAngelScriptEngine(const NativeAngelScriptEngine&) = delete;
    NativeAngelScriptEngine& operator=(const NativeAngelScriptEngine&) = delete;

    [[nodiscard]] BytecodeLoadResult LoadBytecode(
        const char* module, std::span<const std::byte> bytes);
    [[nodiscard]] int FunctionId(const char* module, const char* name) const;
    // IDA7DEE60: only asNO_FUNCTION is an optional absent entry. Other SDK
    // lookup failures remain errors; callers must not preempt on absence.
    [[nodiscard]] bool HasFunction(const char* module, const char* name) const;
    void SetCallbacks(IGameScriptCallbackRuntime* callbacks);
    void SetContextCallbacks(asIScriptContext* context,
                             IGameScriptCallbackRuntime* callbacks);
    [[nodiscard]] std::int32_t GlobalStoryVariable() const {
        return *story_variable_;
    }
    [[nodiscard]] asIScriptEngine* Raw() const { return engine_; }

private:
    asIScriptEngine* engine_{};
    std::array<std::int32_t,48> owned_globals_{};
    std::int32_t global_story_variable_{};
    std::int32_t* story_variable_{};
};

}  // namespace pal4
