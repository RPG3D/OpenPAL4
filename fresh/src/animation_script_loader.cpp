#include "pal4/animation_script_loader.h"

#include <algorithm>

namespace pal4 {

int AnimationScriptLoader::Load(AnimationScriptRecordArrayState& state,
                                std::string_view source,
                                IAnimationScriptLoaderRuntime& runtime) {
    (void)AnimationScriptRecordArray::ClearRecords(state, runtime);
    const auto file = runtime.Open(source, "rt");
    if (file == 0) return 0;

    int mode = 0;
    int mode_before_block = 0;
    while (!runtime.EndOfFile(file)) {
        std::string line = runtime.ReadLine(file, 1024u);
        if (line.empty()) continue;

        std::string first;
        std::string second;
        std::size_t length = std::min({runtime.EffectiveLength(line),
                                       line.size(), std::size_t{1023}});
        for (std::size_t index = 0; index < length; ++index) {
            const char token = line[index];
            if (token == '{') {
                mode = 0;
                ++index;
            } else if (token == '$') {
                mode = 1;
                ++index;
            } else if (token == '}') {
                mode = 2;
                ++index;
            }

            const char current = index < line.size() ? line[index] : '\0';
            const char next = index + 1u < line.size() ? line[index + 1u] : '\0';
            if (current == '/') {
                if (next == '/') mode = 3;
                if (next == '*') {
                    mode_before_block = mode;
                    mode = 4;
                    ++index;
                }
            }

            switch (mode) {
                case 0:
                    first.push_back(index < line.size() ? line[index] : '\0');
                    break;
                case 1:
                    second.push_back(index < line.size() ? line[index] : '\0');
                    break;
                case 2:
                    runtime.Insert(state, state.end, 1u, first, second);
                    index = length;
                    mode = 0;
                    break;
                case 3:
                    index = length;
                    mode = 0;
                    break;
                case 4:
                    if (current == '*' && next == '/') {
                        ++index;
                        mode = mode_before_block;
                    } else if (current == '\r' || current == '\n') {
                        line = runtime.ReadLine(file, 1024u);
                        if (!runtime.EndOfFile(file)) {
                            length = std::min(line.size(), std::size_t{1023});
                            index = 0;
                            if (line.starts_with("*/")) {
                                index = length;
                                mode = mode_before_block;
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
    runtime.Close(file);
    return 1;
}

}  // namespace pal4
