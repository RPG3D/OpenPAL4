#include "pal4/ui_frame_resource_loader.h"

#include <string>

namespace pal4 {
namespace {

constexpr char kSourceFile[] =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp";

struct ResourceSpec {
    std::string_view extension;
    std::string_view directory;
    std::uint32_t log_line;
    const char* log_format;
};

constexpr ResourceSpec kFont{
    ".font", "gamedata\\ui\\fonts\\", 382u,
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager load font %s OK\n\n\n"};
constexpr ResourceSpec kImageset{
    ".imageset", "gamedata\\ui\\imagesets\\", 401u,
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager create Imageset %s OK\n\n\n"};
constexpr ResourceSpec kScheme{
    ".scheme", "gamedata\\ui\\schemes\\", 420u,
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager load scheme %s OK\n\n\n"};
constexpr ResourceSpec kSequenceImage{
    ".seqimage", "gamedata\\ui\\sequence_images\\", 439u,
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager load sequence image %s OK\n\n\n"};

unsigned char FoldAscii(unsigned char value) {
    return value >= 'A' && value <= 'Z'
        ? static_cast<unsigned char>(value + ('a' - 'A')) : value;
}

bool HasExtension(std::string_view filename, std::string_view extension) {
    if (filename.size() <= extension.size()) return false;
    const auto tail = filename.substr(filename.size() - extension.size());
    for (std::size_t i = 0; i < extension.size(); ++i) {
        if (FoldAscii(static_cast<unsigned char>(tail[i])) !=
            static_cast<unsigned char>(extension[i])) return false;
    }
    return true;
}

std::u32string WidenBytes(std::string_view text) {
    std::u32string result;
    result.reserve(text.size());
    for (const unsigned char byte : text) result.push_back(byte);
    return result;
}

struct InitSpec {
    const ResourceSpec* resource;
    const char* package_directory;
    const char* wildcard;
    std::uint32_t package_log_line;
    std::uint32_t local_log_line;
    const char* stage_log_format;
    int (UiFrameResourceLoader::*package_loader)(const char*);
    void (IUiFrameResourceRuntime::*create)(std::u32string_view,
                                            std::u32string_view);
};

constexpr InitSpec kInitSpecs[] = {
    {&kFont, "fonts", "*.font", 459u, 480u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading font...\n\n\n",
     &UiFrameResourceLoader::LoadFontFile,
     &IUiFrameResourceRuntime::CreateFont},
    {&kImageset, "imagesets", "*.imageset", 463u, 500u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading Imageset...\n\n\n",
     &UiFrameResourceLoader::LoadImagesetFile,
     &IUiFrameResourceRuntime::CreateImageset},
    {&kScheme, "schemes", "*.scheme", 467u, 521u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading scheme...\n\n\n",
     &UiFrameResourceLoader::LoadSchemeFile,
     &IUiFrameResourceRuntime::LoadScheme},
    {&kSequenceImage, "sequence_images", "*.seqimage", 471u, 542u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading sequence image...\n\n\n",
     &UiFrameResourceLoader::LoadSequenceImageFile,
     &IUiFrameResourceRuntime::CreateSequenceImage},
};

std::uint8_t EnumerateLocal(const InitSpec& spec,
                            IUiFrameResourceInitRuntime& runtime) {
    const auto enumerator = runtime.ClearEnumerator();
    const std::string pattern =
        std::string(spec.resource->directory) + spec.wildcard;
    try {
        if (runtime.BeginEnumerator(enumerator, pattern.c_str())) {
            do {
                const std::string filename(runtime.CurrentFilename(enumerator));
                if (!runtime.IsSpecialDirectory(enumerator) &&
                    !runtime.IsDirectory(enumerator)) {
                    const auto path = WidenBytes(
                        std::string(spec.resource->directory) + filename);
                    (runtime.*spec.create)(path, std::u32string_view{});
                    runtime.Log(kSourceFile, spec.resource->log_line,
                                spec.resource->log_format, filename.c_str());
                }
            } while (runtime.NextFile(enumerator));
        }
    } catch (...) {
        (void)runtime.EndEnumerator(enumerator);
        throw;
    }
    return runtime.EndEnumerator(enumerator);
}

}  // namespace

int UiFrameResourceLoader::LoadFile(const char* filename, Kind kind) {
    const ResourceSpec* spec = nullptr;
    switch (kind) {
    case Kind::Font: spec = &kFont; break;
    case Kind::Imageset: spec = &kImageset; break;
    case Kind::Scheme: spec = &kScheme; break;
    case Kind::SequenceImage: spec = &kSequenceImage; break;
    }

    const std::string_view name(filename);
    if (!HasExtension(name, spec->extension)) return 1;

    std::string path(spec->directory);
    path.append(name);
    {
        const auto cegui_path = WidenBytes(path);
        const std::u32string_view empty_group;
        switch (kind) {
        case Kind::Font: runtime_.CreateFont(cegui_path, empty_group); break;
        case Kind::Imageset: runtime_.CreateImageset(cegui_path, empty_group); break;
        case Kind::Scheme: runtime_.LoadScheme(cegui_path, empty_group); break;
        case Kind::SequenceImage:
            runtime_.CreateSequenceImage(cegui_path, empty_group); break;
        }
    }
    runtime_.Log(kSourceFile, spec->log_line, spec->log_format, filename);
    return 1;
}

int UiFrameResourceLoader::LoadFontFile(const char* filename) {
    return LoadFile(filename, Kind::Font);
}

int UiFrameResourceLoader::LoadImagesetFile(const char* filename) {
    return LoadFile(filename, Kind::Imageset);
}

int UiFrameResourceLoader::LoadSchemeFile(const char* filename) {
    return LoadFile(filename, Kind::Scheme);
}

int UiFrameResourceLoader::LoadSequenceImageFile(const char* filename) {
    return LoadFile(filename, Kind::SequenceImage);
}

std::uint8_t UiFrameResourceInitializer::Initialize(
    const std::uint32_t manager, IUiFrameResourceInitRuntime& runtime) {
    runtime.SetSkinPath("gamedata\\ui\\skin\\");
    runtime.SetSkinAutoLoad(false);
    runtime.SetWindowManagerResourceProvider(
        runtime.ReadWord(manager + 0x1a4u));

    const bool use_package = runtime.UsePackageResources();
    if (use_package) {
        UiFrameResourceLoader loader(runtime);
        int result = 0;
        for (const auto& spec : kInitSpecs) {
            runtime.LogStage(kSourceFile, spec.package_log_line,
                             spec.stage_log_format);
            runtime.InitializePkFileManager();
            result = runtime.EnumeratePackage(
                spec.package_directory,
                [&loader, &spec](const char* filename) {
                    return (loader.*spec.package_loader)(filename);
                });
        }
        return static_cast<std::uint8_t>(result);
    }

    std::uint8_t result = 0;
    for (const auto& spec : kInitSpecs) {
        runtime.LogStage(kSourceFile, spec.local_log_line,
                         spec.stage_log_format);
        result = EnumerateLocal(spec, runtime);
    }
    return result;
}

}  // namespace pal4
