#pragma once

#include <string_view>

namespace pal4 {

class IDirectoryPathRuntime {
public:
    virtual ~IDirectoryPathRuntime() = default;
    [[nodiscard]] virtual bool SetCurrentDirectory(std::string_view path) = 0;
    virtual void GetLastError() = 0;
    [[nodiscard]] virtual bool CreateDirectory(std::string_view path) = 0;
};

class DirectoryPathProcessor final {
public:
    [[nodiscard]] static bool Process(std::string_view path,
                                      IDirectoryPathRuntime& runtime);
};

}  // namespace pal4
