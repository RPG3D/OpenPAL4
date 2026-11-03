#include "pal4/logger_object.h"

#include <string>

namespace pal4 {

std::uint32_t LoggerObject::Create(ILoggerObjectRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectSize);
    if (object == 0) return 0;

    try {
        return InitializeDefaultPath(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t LoggerObject::InitializeDefaultPath(
    const std::uint32_t object, ILoggerObjectRuntime& runtime) {
    runtime.InitializeEmptyPathBuffer(object);
    try {
        runtime.AssignPathBuffer(object, kInitialPath);
    } catch (...) {
        runtime.CleanupPathBuffer(object);
        throw;
    }
    return object;
}

void LoggerObject::SetFilePathAndWriteHeader(
    const std::uint32_t object, const std::uint32_t path_address,
    ILoggerFileSetupRuntime& runtime) {
    const auto path = runtime.ReadCString(path_address);
    static_cast<void>(StringBuffer::InitializeAt(
        object, path_address, static_cast<std::uint32_t>(path.size()), runtime));

    const auto stream = runtime.OpenWriteFile(runtime.ReadDword(object), "wt");
    if (stream == 0) return;

    const auto module_path = runtime.GetModuleFileNameA(0x200);
    const auto separator_position = module_path.find_last_of('\\');
    // With no backslash, the original copy starts at the zeroed byte before
    // Filename, making the label empty even though it copies path bytes.
    const auto module_label = separator_position == std::string::npos
                                  ? std::string{}
                                  : module_path.substr(separator_position + 1);

    const std::string separator(100, ':');
    runtime.Fprintf(stream, separator + "\n", {});
    runtime.Fprintf(stream, "$>\t", {});
    runtime.Fprintf(stream, "[%s Log at ", module_label);
    runtime.Fprintf(stream, runtime.GetDateText(), {});
    runtime.Fprintf(stream, " %s]\n", runtime.GetTimeText());
    runtime.Fprintf(stream, separator + "\n", {});
    runtime.Fprintf(stream, "\n\n", {});
    runtime.CloseFile(stream);
}

int LoggerObject::AppendFormatted(
    const std::uint32_t object, const std::uint32_t format_address,
    const std::uint32_t arguments_address, ILoggerAppendRuntime& runtime) {
    const auto filename = runtime.ReadDword(object);
    const auto stream = runtime.OpenFile(filename, "at");
    const auto formatted = runtime.Vsprintf(format_address, arguments_address);
    if (stream != 0) runtime.Fprintf(stream, formatted);
    return runtime.CloseFile(stream);
}

void LoggerObject::Destroy(const std::uint32_t object,
                           ILoggerObjectRuntime& runtime) {
    if (object == 0) return;
    runtime.CleanupPathBuffer(object);
    runtime.Free(object);
}

}  // namespace pal4
