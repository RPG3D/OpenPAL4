#include "pal4/native_angelscript_engine.h"
#include "as_scriptengine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    std::int32_t story_variable=70201;
    pal4::NativeAngelScriptEngine runtime(&story_variable);
    Check(runtime.GlobalStoryVariable()==70201,
          "borrowed story registration preserves existing loaded progress");
    const auto* engine = static_cast<asCScriptEngine*>(runtime.Raw());
    Check(engine->systemFunctions.GetLength() == 318,
          "string, math and game registrations match PAL4 system ID range");
    Check(engine->systemFunctions[45]->id == -46 &&
              std::strcmp(engine->systemFunctions[45]->name.AddressOf(),
                          "abs") == 0 &&
              engine->systemFunctions[53]->id == -54 &&
              std::strcmp(engine->systemFunctions[53]->name.AddressOf(),
                          "atanf") == 0,
          "math functions occupy original system IDs -46 through -54");
    Check(engine->systemFunctions[54]->id == -55 &&
              std::strcmp(engine->systemFunctions[56]->name.AddressOf(),
                          "giNewGame") == 0 &&
              std::strcmp(engine->systemFunctions[64]->name.AddressOf(),
                          "giArenaLoad") == 0 &&
              std::strcmp(engine->systemFunctions[290]->name.AddressOf(),
                          "giFlashOutBlack") == 0 &&
              std::strcmp(engine->systemFunctions[296]->name.AddressOf(),
                          "giPlayMovie") == 0 &&
              engine->systemFunctions[317]->id == -318,
          "game callback IDs retain the original registration positions");

    constexpr char kSource[] =
        "float Root() { return sqrtf(9.0); } "
        "bool Same() { return \"A\" == \"A\"; } "
        "int Step() { int value = 1; value += 2; return value; } "
        "int Story() { global_mvar += 1; return global_mvar; }";
    Check(runtime.Raw()->AddScriptSection("callbacks", "callbacks.cs", kSource,
                                           sizeof(kSource) - 1) == 0 &&
              runtime.Raw()->Build("callbacks") == 0,
          "build script that calls registered math and string functions");
    asIScriptContext* context = nullptr;
    Check(runtime.Raw()->CreateContext(&context) == 0 && context != nullptr,
          "create callback test context");
    const int root = runtime.FunctionId("callbacks", "Root");
    Check(root >= 0 && context->Prepare(root) == 0 &&
              context->Execute() == asEXECUTION_FINISHED &&
              context->GetReturnFloat() == 3.0f,
          "execute registered square-root callback");
    const int same = runtime.FunctionId("callbacks", "Same");
    Check(same >= 0 && context->Prepare(same) == 0 &&
              context->Execute() == asEXECUTION_FINISHED &&
              context->GetReturnDWord() == 1u,
          "execute registered string factory and comparison callbacks");
    const int step = runtime.FunctionId("callbacks", "Step");
    Check(step >= 0 && context->Prepare(step) == 0,
          "prepare original AngelScript step interface");
    int step_result = asEXECUTION_SUSPENDED;
    int step_count = 0;
    while (step_result == asEXECUTION_SUSPENDED && step_count < 32) {
        step_result = context->ExecuteStep(asEXEC_STEP_INTO);
        ++step_count;
    }
    Check(step_result == asEXECUTION_FINISHED && step_count > 1 &&
              context->GetReturnDWord() == 3u,
          "original ExecuteStep yields between source lines");
    (void)context->Release();
    Check(runtime.Raw()->CreateContext(&context)==0 && context,
          "create story binding context");
    const int story=runtime.FunctionId("callbacks","Story");
    Check(story>=0 && context->Prepare(story)==0 &&
              context->Execute()==asEXECUTION_FINISHED &&
              context->GetReturnDWord()==70202u && story_variable==70202,
          "script writes the borrowed game story word");
    story_variable=10100;
    Check(context->Prepare(story)==0 && context->Execute()==asEXECUTION_FINISHED &&
              context->GetReturnDWord()==10101u && runtime.GlobalStoryVariable()==10101,
          "subsequent external restore is immediately visible to existing script code");
    (void)context->Release();
    return EXIT_SUCCESS;
}
