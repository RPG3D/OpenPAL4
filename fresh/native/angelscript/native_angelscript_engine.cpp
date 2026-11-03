#include "pal4/native_angelscript_engine.h"
#include "scriptstring.h"
#include "pal4/script_global_variable_registry.h"

#include <cmath>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace pal4 {
namespace {

std::mutex callback_mutex;
std::unordered_map<asIScriptEngine*, IGameScriptCallbackRuntime*> callbacks_by_engine;
std::unordered_map<asIScriptContext*,
                   std::pair<asIScriptEngine*, IGameScriptCallbackRuntime*>>
    callbacks_by_context;

template <int SystemId>
void GameCall(asIScriptGeneric* arguments) {
    IGameScriptCallbackRuntime* callbacks = nullptr;
    {
        std::lock_guard lock(callback_mutex);
        const auto context = callbacks_by_context.find(asGetActiveContext());
        if (context != callbacks_by_context.end())
            callbacks = context->second.second;
        else {
            const auto engine = callbacks_by_engine.find(arguments->GetEngine());
            if (engine != callbacks_by_engine.end()) callbacks = engine->second;
        }
    }
    try {
        if (callbacks == nullptr)
            throw std::runtime_error(
                "PAL4 game script callback " + std::to_string(-SystemId) +
                " is not bound");
        callbacks->Invoke(-SystemId, *arguments);
    } catch (const std::exception& error) {
        if (auto* context = asGetActiveContext())
            (void)context->SetException(error.what());
    } catch (...) {
        if (auto* context = asGetActiveContext())
        {
            const auto message=std::string("PAL4 game script callback failed: system_id=")+std::to_string(-SystemId);
            (void)context->SetException(message.c_str());
        }
    }
}

int AbsoluteInt(int value) { return std::abs(value); }
double AbsoluteDouble(double value) { return std::fabs(value); }
float SquareRoot(float value) { return std::sqrt(value); }
float Sine(float value) { return std::sin(value); }
float Cosine(float value) { return std::cos(value); }
float ArcCosine(float value) { return std::acos(value); }
float ArcSine(float value) { return std::asin(value); }
float Tangent(float value) { return std::tan(value); }
float ArcTangent(float value) { return std::atan(value); }

void RegisterMath(asIScriptEngine* engine) {
    const int results[] = {
        engine->RegisterGlobalFunction("int abs( int )",
                                       asFUNCTION(AbsoluteInt), asCALL_CDECL),
        engine->RegisterGlobalFunction("double fabs( double )",
                                       asFUNCTION(AbsoluteDouble), asCALL_CDECL),
        engine->RegisterGlobalFunction("float sqrtf( float )",
                                       asFUNCTION(SquareRoot), asCALL_CDECL),
        engine->RegisterGlobalFunction("float sinf( float )",
                                       asFUNCTION(Sine), asCALL_CDECL),
        engine->RegisterGlobalFunction("float cosf( float )",
                                       asFUNCTION(Cosine), asCALL_CDECL),
        engine->RegisterGlobalFunction("float acosf( float )",
                                       asFUNCTION(ArcCosine), asCALL_CDECL),
        engine->RegisterGlobalFunction("float asinf( float )",
                                       asFUNCTION(ArcSine), asCALL_CDECL),
        engine->RegisterGlobalFunction("float tanf( float )",
                                       asFUNCTION(Tangent), asCALL_CDECL),
        engine->RegisterGlobalFunction("float atanf( float )",
                                       asFUNCTION(ArcTangent), asCALL_CDECL),
    };
    for (const int result : results) {
        if (result < 0)
            throw std::runtime_error("PAL4 math function registration failed");
    }
}

void RegisterGameFunctions(asIScriptEngine* engine) {
#define PAL4_GAME_FUNCTION(system_id, declaration)                         \
    if (engine->RegisterGlobalFunction(                                     \
            declaration, asFUNCTION(GameCall<system_id>), asCALL_GENERIC) < 0) \
        throw std::runtime_error("PAL4 game function registration failed");
#include "game_function_catalog.inc"
#undef PAL4_GAME_FUNCTION
}

class ReadOnlyBytecodeStream final : public asIBinaryStream {
public:
    explicit ReadOnlyBytecodeStream(std::span<const std::byte> bytes)
        : bytes_(bytes) {}

    void Read(void* destination, int size) override {
        if (size < 0 || static_cast<std::size_t>(size) > bytes_.size() - offset_)
            throw std::out_of_range("AngelScript bytecode read");
        if (size != 0)
            std::memcpy(destination, bytes_.data() + offset_, size);
        offset_ += static_cast<std::size_t>(size);
    }

    void Write(void*, int) override {
        throw std::logic_error("read-only AngelScript bytecode stream");
    }

    [[nodiscard]] std::size_t BytesRead() const { return offset_; }

private:
    std::span<const std::byte> bytes_;
    std::size_t offset_{};
};

}  // namespace

NativeAngelScriptEngine::NativeAngelScriptEngine(std::int32_t* story_variable, GlobalResolver globals)
    : engine_(asCreateScriptEngine(ANGELSCRIPT_VERSION)),
      story_variable_(story_variable ? story_variable : &global_story_variable_) {
    if (engine_ == nullptr)
        throw std::runtime_error("AngelScript 2.2.0 engine creation failed");
    try {
        RegisterScriptString(engine_);
        RegisterMath(engine_);
        RegisterGameFunctions(engine_);
        const auto storage=[&](std::uint32_t original){
            auto* value=globals?globals(original):&owned_globals_.at((original-0x90dec0u)/4u);
            if(!value)throw std::invalid_argument("PAL4 script global storage is missing");return value;};
        if(globals){auto* live=storage(0x90dec0u);if(story_variable && live!=story_variable)throw std::invalid_argument("PAL4 script story/global owner mismatch");story_variable_=live;}
        const auto bind=[&](const std::string& declaration,std::int32_t* value){if(engine_->RegisterGlobalProperty(declaration.c_str(),value)<0)throw std::runtime_error("PAL4 global registration failed: "+declaration);};
        // Live CRT table89D330..89D3EC and7E0670: mvar, maze1..10,
        // then37 named variables. CSB negative indices use this exact order.
        bind("int global_mvar",story_variable_);
        for(unsigned i=1u;i<=10u;++i)bind("int maze_var"+std::to_string(i),storage(0x90dec0u+4u*i));
        for(unsigned i=0u;i<ScriptGlobalVariableRegistry::kNamedVariables.size();++i)
            bind(std::string(ScriptGlobalVariableRegistry::NamedSignature(static_cast<ScriptNamedVariable>(i))),storage(ScriptGlobalVariableRegistry::kNamedVariables[i]));
    } catch (...) {
        (void)engine_->Release();
        engine_ = nullptr;
        throw;
    }
}

NativeAngelScriptEngine::~NativeAngelScriptEngine() {
    SetCallbacks(nullptr);
    {
        std::lock_guard lock(callback_mutex);
        for (auto it = callbacks_by_context.begin();
             it != callbacks_by_context.end();) {
            if (it->second.first == engine_)
                it = callbacks_by_context.erase(it);
            else
                ++it;
        }
    }
    if (engine_ != nullptr) (void)engine_->Release();
}

void NativeAngelScriptEngine::SetCallbacks(
    IGameScriptCallbackRuntime* callbacks) {
    std::lock_guard lock(callback_mutex);
    if (callbacks == nullptr)
        callbacks_by_engine.erase(engine_);
    else
        callbacks_by_engine[engine_] = callbacks;
}

void NativeAngelScriptEngine::SetContextCallbacks(
    asIScriptContext* context, IGameScriptCallbackRuntime* callbacks) {
    if (context == nullptr || context->GetEngine() != engine_)
        throw std::invalid_argument("PAL4 script callback context mismatch");
    std::lock_guard lock(callback_mutex);
    if (callbacks == nullptr)
        callbacks_by_context.erase(context);
    else
        callbacks_by_context[context] = {engine_, callbacks};
}

BytecodeLoadResult NativeAngelScriptEngine::LoadBytecode(
    const char* module, std::span<const std::byte> bytes) {
    if (module == nullptr || bytes.empty())
        throw std::invalid_argument("AngelScript module and bytecode are required");
    ReadOnlyBytecodeStream stream(bytes);
    const int code = engine_->LoadByteCode(module, &stream);
    return {code, stream.BytesRead()};
}

int NativeAngelScriptEngine::FunctionId(const char* module,
                                        const char* name) const {
    return engine_->GetFunctionIDByName(module, name);
}

bool NativeAngelScriptEngine::HasFunction(const char* module, const char* name) const {
    if (!module || !*module || !name || !*name)
        throw std::invalid_argument("PAL4 script module and function are required");
    const auto id = FunctionId(module, name);
    if (id == asNO_FUNCTION) return false;
    if (id < 0) throw std::runtime_error("PAL4 script function lookup failed");
    return true;
}

}  // namespace pal4
