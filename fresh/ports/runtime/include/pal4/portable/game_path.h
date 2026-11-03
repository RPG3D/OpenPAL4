#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>

namespace pal4::portable {
// Original Windows data contains mixed-case paths. Resolve filesystem spelling
// without renaming assets; package-member lookup remains PackageReader's job.
inline std::filesystem::path ResolveOriginalPath(const std::filesystem::path& path){
    namespace fs=std::filesystem;
    if(fs::exists(path))return path;
    const auto lower=[](std::string text){for(auto& c:text)if(c>='A' && c<='Z')c=static_cast<char>(c-'A'+'a');return text;};
    auto result=path.is_absolute()?path.root_path():fs::current_path();
    for(const auto& component:path.relative_path()){
        const auto direct=result/component;
        if(fs::exists(direct)){result=direct;continue;}
        if(!fs::is_directory(result))return path;
        const auto key=lower(component.string());fs::path matched;
        for(const auto& entry:fs::directory_iterator(result))if(lower(entry.path().filename().string())==key){
            if(!matched.empty())throw std::runtime_error("ambiguous original game path: "+path.string());
            matched=entry.path();
        }
        if(matched.empty())return path;
        result=std::move(matched);
    }
    return result;
}
}
