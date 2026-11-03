#pragma once

#include <string>

namespace pal4 {

class IScriptEngineErrorSink {
public:
    virtual ~IScriptEngineErrorSink() = default;
    virtual void Write(const char* text) = 0;
};

class ScriptEngineErrorReporter final {
public:
    static std::string Format(int result, const char* message);
    static void Report(int result, const char* message,
                       IScriptEngineErrorSink* sink);
};

}  // namespace pal4
