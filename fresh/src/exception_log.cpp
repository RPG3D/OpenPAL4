#include "pal4/exception_log.h"

namespace pal4 {

int ExceptionLog::Create(const char* path, IExceptionLogRuntime& runtime) {
    path_ = path;
    const std::uintptr_t stream = runtime.Open(path_.c_str(), "wt");
    if (stream == 0u) {
        return 0;
    }

    runtime.Print(stream, ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    runtime.Print(stream, "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{SEHException}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n");
    runtime.Print(stream, ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n\n\n");
    return runtime.Close(stream);
}

int ExceptionLog::Append(IExceptionLogRuntime& runtime, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    buffer_ = runtime.Format(format, args);
    va_end(args);

    const std::uintptr_t stream = runtime.Open(path_.c_str(), "at");
    if (stream == 0u) {
        return 0;
    }
    runtime.Print(stream, buffer_.c_str());
    return runtime.Close(stream);
}

}  // namespace pal4
