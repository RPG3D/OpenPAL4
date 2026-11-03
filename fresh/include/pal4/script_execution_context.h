#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

struct ScriptWaitHandlerBaseState;

struct ScriptExecutionContextState {
    const void* vtable;
    void* script_engine;
    std::uint32_t word_8;
    std::uint32_t current_stack_record;
    std::uint8_t address_byte_16;
    std::uint8_t reserved_17[3];
    void* buffer_20;
    void* buffer_24;
    void* buffer_28;
    std::uint8_t address_byte_32;
    std::uint8_t reserved_33[3];
    void* buffer_36;
    void* buffer_40;
    void* buffer_44;
};

class IScriptExecutionContextRuntime {
public:
    virtual ~IScriptExecutionContextRuntime() = default;
    virtual void Cleanup(ScriptExecutionContextState& context) = 0;
    virtual void Free(void* pointer) = 0;
};

using ScriptEngineRegistrationCallback = int (*)(void* engine);

class IScriptEngineBoundary {
public:
    virtual ~IScriptEngineBoundary() = default;
    virtual void* CreateEngine(std::uint32_t version) = 0;
    virtual void RegisterStringFunctions(void* engine) = 0;
    virtual void RegisterMathFunctions(void* engine) = 0;
    virtual void RegisterGameFunctions(void* engine) = 0;
    virtual int InvokeRegistration(
        ScriptEngineRegistrationCallback callback, void* engine) = 0;
    virtual int CompleteCacheCleanup(ScriptExecutionContextState& context) = 0;
    virtual int ReleaseEngine(void* engine) = 0;
};

class IScriptModuleBoundary {
public:
    virtual ~IScriptModuleBoundary() = default;
    virtual void ReportRaw(std::uint32_t logger, std::uintptr_t message) = 0;
    virtual void RaiseError(const char* message) = 0;
    virtual const char* Name(void* interpreter) = 0;
    virtual const char* FullPath(void* interpreter) = 0;
    virtual const char* SectionPath(void* interpreter) = 0;
    virtual int Kind(void* interpreter) = 0;
    virtual std::span<const std::byte> Bytecode(void* interpreter) = 0;
    virtual std::string_view Source(void* interpreter) = 0;
    virtual void EmitLog(std::uint32_t logger, std::string_view text) = 0;
    virtual int LoadBytecode(void* engine, const char* path,
                             std::span<const std::byte> bytecode) = 0;
    virtual int AddSection(void* engine, const char* path,
                           const char* section, std::string_view source) = 0;
    virtual int Build(void* engine, const char* path,
                      std::uint32_t logger) = 0;
    virtual void CheckResult(int result, const char* operation,
                             std::uint32_t logger) = 0;
};

struct ScriptStackModuleRecord {
    char module_name[256];
    std::uint32_t script_context;
    std::int32_t function_index;
    std::uint8_t execution_flag;
    std::uint8_t reserved[3];
    std::uint32_t word_268;
    std::uint32_t word_272;
};

static_assert(sizeof(ScriptStackModuleRecord) == 276);

class IScriptStackModuleRuntime {
public:
    virtual ~IScriptStackModuleRuntime() = default;
    virtual void Report(std::uint32_t logger, const char* message) = 0;
    virtual void RaiseError(const char* message) = 0;
    virtual void CopyModuleName(char (&destination)[256],
                                const char* source) = 0;
    virtual void AcquireContext(void* engine,
                                std::uint32_t& context) = 0;
    virtual int FindFunction(void* engine, const char* module,
                             const char* function) = 0;
    virtual int TranslateFunction(void* engine, int index) = 0;
    virtual std::uint32_t ResolveFunction(void* engine,
                                          const char* module,
                                          int translated_index) = 0;
    virtual int Prepare(std::uint32_t context,
                        std::uint32_t function) = 0;
    virtual void ReleaseContext(std::uint32_t context) = 0;
    virtual void StopPrevious(std::uint32_t context) = 0;
    virtual void CleanupHandlers(ScriptExecutionContextState& state) = 0;
    virtual void* AllocateRecords(std::size_t bytes) = 0;
    virtual void FreeRecords(void* records) = 0;
};

class IScriptExecutionCleanupRuntime {
public:
    virtual ~IScriptExecutionCleanupRuntime() = default;
    virtual void StopScriptContext(std::uint32_t context) = 0;
    virtual void ReleaseScriptContext(std::uint32_t context) = 0;
    virtual void ResetHandler(void* handler) = 0;
    virtual void DeleteHandler(void* handler, unsigned flags) = 0;
};

class IScriptHandlerDispatchRuntime {
public:
    virtual ~IScriptHandlerDispatchRuntime() = default;
    virtual void Process(ScriptWaitHandlerBaseState& handler,
                         int first, int second) = 0;
    virtual void Delete(ScriptWaitHandlerBaseState& handler,
                        unsigned flags) = 0;
    virtual int ExecuteCurrentModule(ScriptExecutionContextState& state) = 0;
};

class IScriptHandlerRegistrationRuntime {
public:
    virtual ~IScriptHandlerRegistrationRuntime() = default;
    virtual bool SameHandlerKey(const ScriptWaitHandlerBaseState& incoming,
                                const ScriptWaitHandlerBaseState& existing) = 0;
    virtual void ResetHandler(ScriptWaitHandlerBaseState& handler) = 0;
    virtual void DeleteHandler(ScriptWaitHandlerBaseState& handler,
                               unsigned flags) = 0;
    virtual void* AllocatePointers(std::size_t bytes) = 0;
    virtual void FreePointers(void* pointers) = 0;
};

class IScriptCurrentModuleRuntime {
public:
    virtual ~IScriptCurrentModuleRuntime() = default;
    virtual void StopContext(std::uint32_t context) = 0;
    virtual void ReleaseContext(std::uint32_t context) = 0;
    virtual void RemoveModule(void* engine,
                              ScriptStackModuleRecord& record) = 0;
    virtual void CompleteCleanup(ScriptExecutionContextState& state) = 0;
    virtual void AbortContext(std::uint32_t context) = 0;
    virtual int ExecuteContext(std::uint32_t context, int option) = 0;
    virtual const char* ExceptionFunctionName(std::uint32_t context) = 0;
    virtual int ExceptionLine(std::uint32_t context, int option) = 0;
    virtual const char* ExceptionReason(std::uint32_t context,
                                        int option) = 0;
    virtual const char* FunctionDeclaration(void* engine,
                                            const char* function_name,
                                            int option) = 0;
    virtual void ReportException(std::uint32_t logger,
                                 std::string_view text) = 0;
    virtual void RaiseException(std::string_view text) = 0;
};

class ScriptExecutionContext final {
public:
    static ScriptExecutionContextState* Initialize(
        ScriptExecutionContextState& state, const void* base_vtable);
    static ScriptExecutionContextState* DestroyAt(
        ScriptExecutionContextState* state, unsigned flags,
        const void* base_vtable, IScriptExecutionContextRuntime& runtime);
    static void ReleaseFields(ScriptExecutionContextState& state,
                              const void* base_vtable,
                              IScriptExecutionContextRuntime& runtime);
    static int InitializeEngine(
        ScriptExecutionContextState& state,
        ScriptEngineRegistrationCallback callback, std::uint32_t context_word,
        IScriptEngineBoundary& runtime);
    static int CleanupEngine(ScriptExecutionContextState& state,
                             IScriptEngineBoundary& runtime);
    static void* GetEngine(const ScriptExecutionContextState& state);
    static bool AddScriptModule(const ScriptExecutionContextState& state,
                                void* interpreter,
                                IScriptModuleBoundary& runtime);
    static int PushStackModule(ScriptExecutionContextState& state,
                               const char* module, const char* function,
                               IScriptStackModuleRuntime& runtime);
    static bool HasStackModule(const ScriptExecutionContextState& state,
                               const char* module);
    static int FindStackModuleStatus(const ScriptExecutionContextState& state,
                                     const char* module);
    static int StackEntryCount(const ScriptExecutionContextState& state);
    static int CompleteStackCleanup(ScriptExecutionContextState& state,
                                    IScriptExecutionCleanupRuntime& runtime);
    static int ClearStack(ScriptExecutionContextState& state,
                          IScriptExecutionCleanupRuntime& runtime);
    static int RemoveOtherStackModules(ScriptExecutionContextState& state,
                                       IScriptExecutionCleanupRuntime& runtime);
    static int CleanupHandlers(ScriptExecutionContextState& state,
                               IScriptExecutionCleanupRuntime& runtime);
    static ScriptStackModuleRecord* SetDebuggerMode(
        ScriptExecutionContextState& state, int mode);
    static int ProcessWaitHandlers(ScriptExecutionContextState& state,
                                   int first, int second,
                                   IScriptHandlerDispatchRuntime& runtime);
    static int ExecuteCurrentModule(ScriptExecutionContextState& state,
                                    IScriptCurrentModuleRuntime& runtime);
    static ScriptStackModuleRecord* SetExecutionFlag(
        ScriptExecutionContextState& state);
    static ScriptStackModuleRecord* ClearExecutionFlag(
        ScriptExecutionContextState& state);
    static std::uint32_t AddWaitHandler(
        ScriptExecutionContextState& state,
        ScriptWaitHandlerBaseState& handler,
        IScriptHandlerRegistrationRuntime& runtime);
};

}  // namespace pal4
