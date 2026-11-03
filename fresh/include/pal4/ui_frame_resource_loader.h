#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

class IUiFrameResourceRuntime {
public:
    virtual ~IUiFrameResourceRuntime() = default;

    virtual void CreateFont(std::u32string_view path,
                            std::u32string_view group) = 0;
    virtual void CreateImageset(std::u32string_view path,
                                std::u32string_view group) = 0;
    virtual void LoadScheme(std::u32string_view path,
                            std::u32string_view group) = 0;
    virtual void CreateSequenceImage(std::u32string_view path,
                                     std::u32string_view group) = 0;
    virtual void Log(const char* source, std::uint32_t line,
                     const char* format, const char* filename) = 0;
};

class UiFrameResourceLoader final {
public:
    explicit UiFrameResourceLoader(IUiFrameResourceRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x4bd3b0, 0x4bd680, 0x4bd950 and 0x4bdc20, respectively.
    [[nodiscard]] int LoadFontFile(const char* filename);
    [[nodiscard]] int LoadImagesetFile(const char* filename);
    [[nodiscard]] int LoadSchemeFile(const char* filename);
    [[nodiscard]] int LoadSequenceImageFile(const char* filename);

private:
    enum class Kind { Font, Imageset, Scheme, SequenceImage };
    [[nodiscard]] int LoadFile(const char* filename, Kind kind);

    IUiFrameResourceRuntime& runtime_;
};

class IUiFrameResourceInitRuntime : public IUiFrameResourceRuntime {
public:
    virtual void SetSkinPath(std::string_view path) = 0;
    virtual void SetSkinAutoLoad(bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetWindowManagerResourceProvider(std::uint32_t provider) = 0;
    [[nodiscard]] virtual bool UsePackageResources() = 0;
    virtual void LogStage(const char* source, std::uint32_t line,
                          const char* format) = 0;
    virtual void InitializePkFileManager() = 0;
    [[nodiscard]] virtual int EnumeratePackage(
        const char* directory,
        const std::function<int(const char*)>& callback) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEnumerator() = 0;
    [[nodiscard]] virtual bool BeginEnumerator(
        std::uint32_t enumerator, const char* pattern) = 0;
    [[nodiscard]] virtual const char* CurrentFilename(
        std::uint32_t enumerator) = 0;
    [[nodiscard]] virtual bool IsSpecialDirectory(
        std::uint32_t enumerator) = 0;
    [[nodiscard]] virtual bool IsDirectory(
        std::uint32_t enumerator) = 0;
    [[nodiscard]] virtual bool NextFile(std::uint32_t enumerator) = 0;
    [[nodiscard]] virtual std::uint8_t EndEnumerator(
        std::uint32_t enumerator) = 0;
};

class UiFrameResourceInitializer final {
public:
    // IDA 0x4bdef0. Returns the last package or local enumerator result byte.
    [[nodiscard]] static std::uint8_t Initialize(
        std::uint32_t manager, IUiFrameResourceInitRuntime& runtime);
};

}  // namespace pal4
