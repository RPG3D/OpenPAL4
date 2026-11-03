#pragma once
#include <windows.h>
#include <dbghelp.h>
#include <iostream>
#include <cwchar>
#pragma comment(lib,"dbghelp.lib")
namespace pal4::test {
// Uses the Windows dump writer; no private exception or stack serialization.
inline bool WriteNativeExceptionDump(const wchar_t* directory,EXCEPTION_POINTERS* exception,
    wchar_t* path,std::size_t capacity) noexcept {
    if(!directory || !*directory || !path || !capacity)return false;
    if(swprintf_s(path,capacity,L"%ls\\pal4-native-crash-%lu-%llu.dmp",directory,
        GetCurrentProcessId(),static_cast<unsigned long long>(GetTickCount64()))<0)return false;
    const auto file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(),exception,FALSE};
    const auto flags=static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs |
        MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo);
    const bool written=MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),file,
        flags,exception?&info:nullptr,nullptr,nullptr)!=FALSE;
    const auto error=written?ERROR_SUCCESS:GetLastError();
    CloseHandle(file);SetLastError(error);return written;
}
// Scoped diagnostics for original DLL/native fixtures; do not dereference an
// unknown crashing game/window object while attempting to print the trace.
class NativeExceptionTrace final {
public:
    explicit NativeExceptionTrace(bool enabled=true,const wchar_t* dump_directory=nullptr)
        :enabled_(enabled),previous_directory_(directory_) {
        if(enabled_){directory_=dump_directory;previous_=SetUnhandledExceptionFilter(Trace);}
    }
    ~NativeExceptionTrace(){if(enabled_){SetUnhandledExceptionFilter(previous_);directory_=previous_directory_;}}
    NativeExceptionTrace(const NativeExceptionTrace&)=delete;
    NativeExceptionTrace& operator=(const NativeExceptionTrace&)=delete;
private:
    bool enabled_{};
    LPTOP_LEVEL_EXCEPTION_FILTER previous_{};
    const wchar_t* previous_directory_{};
    inline static const wchar_t* directory_{};
    static LONG WINAPI Trace(EXCEPTION_POINTERS* e){
        if(directory_){
            wchar_t dump[MAX_PATH]{};const bool ok=WriteNativeExceptionDump(directory_,e,dump,MAX_PATH);
            std::cerr<<"Native exception dump written="<<ok<<" error="<<(ok?0:GetLastError())<<'\n';
        }
        HMODULE module{};char path[MAX_PATH]{};const auto ip=e->ExceptionRecord->ExceptionAddress;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<const char*>(ip),&module);
        GetModuleFileNameA(module,path,MAX_PATH);
        std::cerr<<"Native fixture exception code="<<std::hex<<e->ExceptionRecord->ExceptionCode<<" ip="<<ip<<" module="<<path<<" offset="<<(reinterpret_cast<unsigned>(ip)-reinterpret_cast<unsigned>(module))<<std::endl;
        if(directory_)std::cerr<<"Native exception registers eax="<<e->ContextRecord->Eax<<" ebx="<<e->ContextRecord->Ebx
            <<" ecx="<<e->ContextRecord->Ecx<<" edx="<<e->ContextRecord->Edx<<" esi="<<e->ContextRecord->Esi
            <<" edi="<<e->ContextRecord->Edi<<" ebp="<<e->ContextRecord->Ebp<<" esp="<<e->ContextRecord->Esp<<std::endl;
        SymSetOptions(SYMOPT_UNDNAME|SYMOPT_LOAD_LINES);SymInitialize(GetCurrentProcess(),nullptr,TRUE);
        CONTEXT context=*e->ContextRecord;STACKFRAME64 frame{};frame.AddrPC={context.Eip,0,AddrModeFlat};frame.AddrFrame={context.Ebp,0,AddrModeFlat};frame.AddrStack={context.Esp,0,AddrModeFlat};
        for(unsigned i=0;i<24u && StackWalk64(IMAGE_FILE_MACHINE_I386,GetCurrentProcess(),GetCurrentThread(),&frame,&context,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr);++i){
            alignas(SYMBOL_INFO) char storage[sizeof(SYMBOL_INFO)+512]{};auto* symbol=reinterpret_cast<SYMBOL_INFO*>(storage);symbol->SizeOfStruct=sizeof(SYMBOL_INFO);symbol->MaxNameLen=511;DWORD64 displacement{};
            std::cerr<<"frame "<<frame.AddrPC.Offset;if(SymFromAddr(GetCurrentProcess(),frame.AddrPC.Offset,&displacement,symbol))std::cerr<<' '<<symbol->Name<<'+'<<displacement;std::cerr<<std::endl;
        }
        SymCleanup(GetCurrentProcess());return directory_?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;
    }
};
}
