#include "pal4/directory_path_processor.h"

#include <string>

namespace pal4 {

bool DirectoryPathProcessor::Process(
    const std::string_view path, IDirectoryPathRuntime& runtime) {
    std::string prefix;
    for (std::size_t index = 0; index < path.size(); ++index) {
        const char current = path[index];
        if (current == ':') {
            ++index;
            continue;
        }

        if (current == '\\' && index + 1 < path.size() &&
            path[index + 1] == '\\') {
            ++index;
            while (index < path.size() && path[index] == '\\')
                ++index;
            while (index < path.size() && path[index] != '\\')
                ++index;
            continue;
        }

        if (current != '\\' && index + 1 != path.size())
            continue;

        prefix.assign(path.substr(0, index + 1));
        if (index + 1 == path.size())
            prefix.assign(path);
        if (!runtime.SetCurrentDirectory(prefix)) {
            runtime.GetLastError();
            if (!runtime.CreateDirectory(prefix))
                return false;
        }
    }
    return true;
}

}  // namespace pal4
