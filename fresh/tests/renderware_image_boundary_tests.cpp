#include "support/native_test_display.h"
#include "support/native_desktop_observation.h"
#include "pal4/renderware_image_native.h"
#include "pal4/native_game_save_thumbnail.h"
#include "pal4/native_file_texture_cache_key.h"
#include "pal4/native_texture_dictionary_content.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/legacy_crc32.h"
#include "pal4/ui_frame_manager_singleton.h"
#include <windows.h>
#include <rwcore.h>
#include <rtfsyst.h>
#include <rtpng.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {
void Require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
std::unordered_set<void*> allocations;
bool fail_next = false;
bool invalid_free = false;
void* Allocate(size_t size, RwUInt32) {
    if (fail_next) { fail_next = false; return nullptr; }
    void* p = std::malloc(size);
    if (p) allocations.insert(p);
    return p;
}
void Free(void* p) {
    if (p && allocations.erase(p) != 1) invalid_free = true;
    std::free(p);
}
void* Reallocate(void* p, size_t size, RwUInt32 hint) {
    if (!p) return Allocate(size, hint);
    if (fail_next) { fail_next = false; return nullptr; }
    if (!size) { Free(p); return nullptr; }
    const bool known = allocations.erase(p) == 1;
    void* result = std::realloc(p, size);
    if (result) allocations.insert(result); else allocations.insert(p);
    if (!known) invalid_free = true;
    return result;
}
void* Callocate(size_t count, size_t size, RwUInt32 hint) {
    if (size && count > static_cast<size_t>(-1) / size) return nullptr;
    void* result = Allocate(count * size, hint);
    if (result) std::memset(result, 0, count * size);
    return result;
}
class Engine {
public:
    void Start() {
        Require(RwEngineGetVersion() == 0x37002, "unexpected RenderWare runtime version");
        const RwMemoryFunctions memory{Allocate, Free, Reallocate, Callocate};
        Require(RwEngineInit(&memory, 0, 0), "RwEngineInit failed"); initialized_ = true;
        window_ = pal4::test::CreateTestWindow("STATIC", "PAL4 image boundary", WS_OVERLAPPEDWINDOW, 64, 64, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
        Require(window_ != nullptr, "hidden test window creation failed");
        RwEngineOpenParams params{window_};
        Require(RwEngineOpen(&params), "RwEngineOpen failed"); opened_ = true;
        Require(RwEngineStart(), "RwEngineStart failed"); started_ = true;
    }
    void Close() {
        if (started_) { Require(RwEngineStop(), "RwEngineStop failed"); started_ = false; }
        if (opened_) { Require(RwEngineClose(), "RwEngineClose failed"); opened_ = false; }
        if (initialized_) { Require(RwEngineTerm(), "RwEngineTerm failed"); initialized_ = false; }
        if (window_) { DestroyWindow(window_); window_ = nullptr; }
    }
    ~Engine() {
        if (started_) RwEngineStop();
        if (opened_) RwEngineClose();
        if (initialized_) RwEngineTerm();
        if (window_) DestroyWindow(window_);
    }
private:
    HWND window_{};
    bool initialized_{}, opened_{}, started_{};
};
struct DeleteImage { void operator()(RwImage* p) const { RwImageDestroy(p); } };
using Image = std::unique_ptr<RwImage, DeleteImage>;
Image Create(int width, int height, int depth) {
    Image result(RwImageCreate(width, height, depth));
    Require(result != nullptr, "image creation failed");
    Require(RwImageAllocatePixels(result.get()) == result.get(), "pixel allocation failed");
    return result;
}
RwRGBA* Row(RwImage* image, int y) {
    return reinterpret_cast<RwRGBA*>(RwImageGetPixels(image) + y * RwImageGetStride(image));
}
void Allocation() {
    pal4::NativeRenderWareImageApi api;
    for (auto depth : {4u, 8u, 32u}) {
        auto handle = api.Create(3, 2, depth);
        Require(handle != 0, "adapter create returned null");
        Image image(reinterpret_cast<RwImage*>(handle));
        Require(!RwImageGetPixels(image.get()) && !RwImageGetPalette(image.get()), "new image has stale storage");
        Require(api.AllocatePixels(handle) == handle, "adapter allocation result mismatch");
        const auto stride = RwImageGetStride(image.get());
        Require(stride == (depth == 32 ? 12 : 4), "depth/row alignment mismatch");
        const auto palette = RwImageGetPalette(image.get());
        if (depth == 32) Require(palette == nullptr, "32-bit image unexpectedly has palette");
        else Require(reinterpret_cast<RwUInt8*>(palette) == RwImageGetPixels(image.get()) + stride * 2,
                     "indexed palette is not in combined allocation after pixels");
        Require(RwImageFreePixels(image.get()) == image.get() && !RwImageGetPixels(image.get()) &&
                !RwImageGetPalette(image.get()), "free pixels did not clear storage");
        Require(api.AllocatePixels(handle) == handle, "reallocation after free failed");
        api.Destroy(reinterpret_cast<std::uint32_t>(image.release()));
    }
    RwUInt8 borrowed_pixels[12]{};
    const auto borrowed = api.Create(3, 1, 32);
    Require(borrowed != 0, "borrowed-buffer image creation failed");
    RwImageSetPixels(reinterpret_cast<RwImage*>(borrowed), borrowed_pixels);
    RwImageSetStride(reinterpret_cast<RwImage*>(borrowed), 12);
    api.Destroy(borrowed);  // No owns-pixels flag: SDK must not free caller storage.
    borrowed_pixels[0] = 7;
}
void Failure() {
    Image image(RwImageCreate(3, 2, 8));
    Require(image != nullptr, "failure test image create failed");
    const auto before = allocations.size();
    fail_next = true;
    Require(!RwImageAllocatePixels(image.get()) && !fail_next && !RwImageGetPixels(image.get()),
            "allocation failure was not returned to caller");
    Require(allocations.size() == before, "failed allocation changed ownership");
    Require(RwImageAllocatePixels(image.get()) == image.get(), "retry after allocation failure failed");
}
void Masks() {
    auto mask = Create(3, 2, 32);
    auto destination = Create(3, 2, 32);
    for (int y = 0; y < 2; ++y) for (int x = 0; x < 3; ++x) {
        const auto maximum = static_cast<RwUInt8>(40+y);
        Row(mask.get(), y)[x] = x == 0 ? RwRGBA{maximum,10,20,250} :
            (x == 1 ? RwRGBA{10,maximum,20,250} : RwRGBA{10,20,maximum,250});
        Row(destination.get(), y)[x] = RwRGBA{1, 2, 3, 4};
    }
    Require(RwImageMakeMask(mask.get()) == mask.get(), "32-bit make mask result mismatch");
    Require(RwImageApplyMask(destination.get(), mask.get()) == destination.get(), "32-bit apply mask failed");
    for (int y = 0; y < 2; ++y) for (int x = 0; x < 3; ++x) {
        const auto pixel = Row(destination.get(), y)[x];
        Require(pixel.red == 1 && pixel.green == 2 && pixel.blue == 3 && pixel.alpha == 40+y,
                "mask must copy max(R,G,B) alpha and preserve destination RGB");
    }
    for (int depth : {4, 8}) {
        auto indexed_mask = Create(3, 2, depth);
        auto indexed_image = Create(3, 2, depth);
        for (int i = 0; i < (1 << depth); ++i) {
            RwImageGetPalette(indexed_mask.get())[i] = RwRGBA{4, 9, 7, 0};
            RwImageGetPalette(indexed_image.get())[i] = RwRGBA{21, 22, 23, 255};
        }
        std::memset(RwImageGetPixels(indexed_mask.get()), 1, RwImageGetStride(indexed_mask.get()) * 2);
        std::memset(RwImageGetPixels(indexed_image.get()), 1, RwImageGetStride(indexed_image.get()) * 2);
        Require(RwImageMakeMask(indexed_mask.get()) == indexed_mask.get(), "palette make mask failed");
        Require(RwImageApplyMask(indexed_image.get(), indexed_mask.get()) == indexed_image.get() &&
                RwImageGetDepth(indexed_image.get()) == 32 && !RwImageGetPalette(indexed_image.get()),
                "indexed destination did not promote to RGBA");
        for (int y = 0; y < 2; ++y) for (int x = 0; x < 3; ++x) {
            const auto pixel = Row(indexed_image.get(),y)[x];
            Require(pixel.red == 21 && pixel.green == 22 && pixel.blue == 23 && pixel.alpha == 9,
                    "indexed mask/palette conversion lost color or alpha");
        }
    }
    auto mismatch = Create(2, 2, 32);
    const auto before = Row(destination.get(),0)[0];
    Require(!RwImageApplyMask(destination.get(),mismatch.get()) &&
            std::memcmp(&before, &Row(destination.get(),0)[0], sizeof before) == 0,
            "dimension mismatch must fail before pixel mutation");
}
struct CameraRuntime final : pal4::IUiFrameManagerCameraRuntime {
    RwRaster* raster{};
    std::uint32_t cached{};
    std::uint32_t InitializeCameraSubsystem() override { return 0x6000; }
    std::uint32_t ActiveCameraObject(std::uint32_t) override { return 0x7000; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x7060) return reinterpret_cast<std::uint32_t>(raster);
        if (address == 0x5148) return cached;
        std::uint32_t word; std::memcpy(&word, reinterpret_cast<void*>(address), 4); return word;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Require(address == 0x5148, "unexpected manager field write"); cached = value;
    }
    ~CameraRuntime() { if (cached) RwImageDestroy(reinterpret_cast<RwImage*>(cached)); }
};
struct DeleteRaster { void operator()(RwRaster* p) const { RwRasterDestroy(p); } };
void Camera() {
    pal4::NativeRenderWareImageApi api;
    CameraRuntime runtime;
    for (int width : {3, 3, 4}) {
        auto source = Create(width, 2, 32);
        for (int y=0; y<2; ++y) for (int x=0; x<width; ++x) Row(source.get(),y)[x] = RwRGBA{12,34,56,255};
        std::unique_ptr<RwRaster,DeleteRaster> raster(RwRasterCreate(width,2,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888));
        Require(raster && RwRasterSetFromImage(raster.get(),source.get()), "native raster upload failed");
        runtime.raster = raster.get();
        const auto previous = runtime.cached;
        const bool same_size = previous && RwImageGetWidth(reinterpret_cast<RwImage*>(previous)) == width;
        const auto result = pal4::UiFrameManagerSingleton::CopyActiveCameraRaster(0x5000,runtime,api);
        Require(result == runtime.cached && result != 0, "game camera path failed with native image adapter");
        Require(!same_size || previous == result, "same-size camera image was not reused");
        auto* image = reinterpret_cast<RwImage*>(result);
        Require(RwImageGetWidth(image) == width && RwImageGetHeight(image) == 2,
                "game camera path did not create/reuse/resize image");
        const auto pixel = Row(image,1)[width-1];
        Require(pixel.red==12 && pixel.green==34 && pixel.blue==56 && pixel.alpha==255,
                "camera raster copy did not preserve native RGBA");
    }
}

void Paths() {
    std::string input = "first\\;second\\";
    Require(RwImageSetPath(input.c_str()) == input.c_str(), "SetPath must return its input pointer");
    Require(std::string_view(RwImageGetPath()) == input, "image path copy mismatch");
    input[0] = 'X';
    Require(RwImageGetPath()[0] == 'f', "image path retained caller storage");
    const std::string saved = RwImageGetPath();
    const std::string longer(700, 'p');
    fail_next = true;
    Require(!RwImageSetPath(longer.c_str()) && !fail_next && RwImageGetPath() == saved,
            "failed path growth did not preserve previous value");
    Require(RwImageSetPath(longer.c_str()) == longer.c_str() && RwImageGetPath() == longer,
            "path growth/retry failed");
    Require(RwImageSetPath("") && std::string_view(RwImageGetPath()).empty(), "empty path reset failed");
}

std::filesystem::path format_root;
std::vector<std::string> read_paths;
std::string written_path;
int reader_version = 1;
RwImage* ReadFixture(const char* filename) {
    read_paths.emplace_back(filename);
    std::ifstream input(format_root / filename, std::ios::binary);
    char marker = 0; input.get(marker);
    if (!input || marker == '!') return nullptr;
    auto image = Create(marker == 'X' ? 2 : 1, 1, 32);
    for (int x=0; x<RwImageGetWidth(image.get()); ++x)
        Row(image.get(),0)[x] = marker == 'M' ? RwRGBA{64,32,16,255} : RwRGBA{10,20,30,40};
    return image.release();
}
RwImage* ReadFixtureReplacement(const char* filename) {
    reader_version = 2; return ReadFixture(filename);
}
RwImage* WriteFixture(RwImage* image, const char* filename) {
    written_path = filename;
    std::ofstream output(format_root / filename, std::ios::binary);
    output.put('B');
    return output ? image : nullptr;
}
class FixtureFileSystem {
public:
    void Open(std::uint32_t filesystem_count=1u) {
        format_root = std::filesystem::current_path() /
            ("rw_image_fixture_" + std::to_string(GetCurrentProcessId()) + "_" + std::to_string(GetTickCount64()));
        Require(std::filesystem::create_directory(format_root), "fixture directory already exists"); created_ = true;
        std::filesystem::create_directory(format_root / "one");
        std::filesystem::create_directory(format_root / "two");
        Require(RtFSManagerOpen(filesystem_count), "SDK filesystem manager open failed"); opened_ = true;
        auto directory = format_root.string() + "\\";
        char name[] = "img";
        static_assert(sizeof(name) <= RTFS_MAX_NAME_LENGTH);
        auto* fs = RtWinFSystemInit(8, directory.data(), name);
        Require(fs != nullptr, "SDK Windows filesystem init failed");
        Require(RtFSManagerRegister(fs), "SDK filesystem registration failed");
        RtFSManagerSetDefaultFileSystem(fs);
    }
    void Put(const char* filename, char marker) {
        std::ofstream output(format_root / filename, std::ios::binary);
        output.put(marker); Require(static_cast<bool>(output), "fixture file creation failed");
    }
    ~FixtureFileSystem() {
        if (opened_) RtFSManagerClose();
        // Only this uniquely named, successfully created test directory is removed.
        if (created_) { std::error_code error; std::filesystem::remove_all(format_root, error); }
    }
private:
    bool opened_{}, created_{};
};
void Formats() {
    FixtureFileSystem files; files.Open();
    files.Put("one/tile.tst", '!'); files.Put("two/tile.tst", 'B');
    files.Put("two/mask.tst", 'M'); files.Put("two/wrong.tst", 'X');
    Require(RwImageRegisterImageFormat("tst", ReadFixture, WriteFixture), "format registration failed");
    Require(RwImageSetPath("one\\;two\\"), "search path setup failed");
    Image image(RwImageRead("tile.tst"));
    Require(image && read_paths == std::vector<std::string>{"one\\tile.tst", "two\\tile.tst"},
            "image search did not continue after existing file failed to decode");
    Require(std::string_view(RwImageFindFileType("mask")) == ".tst", "file type extension probe failed");
    Require(!RwImageFindFileType("missing"), "missing image incorrectly acquired extension");
    Require(!RwImageRead("tile.TsT"), "mixed-case extension unexpectedly matched lower/upper registrations");
    Require(RwImageRegisterImageFormat("TST", ReadFixtureReplacement, nullptr), "format update failed");
    image.reset(RwImageRead("tile.TST"));
    Require(image && reader_version == 2, "uppercase extension/replacement reader not selected");
    pal4::NativeRenderWareImageApi api;
    Require(api.Write(reinterpret_cast<std::uint32_t>(image.get()), "written.tst") ==
                reinterpret_cast<std::uint32_t>(image.get()) && written_path == "written.tst" &&
            std::filesystem::exists(format_root / "written.tst"),
            "null update erased writer or Write unexpectedly prefixed image search path");
    Require(!api.Write(reinterpret_cast<std::uint32_t>(image.get()), "no_extension") &&
            !api.Write(reinterpret_cast<std::uint32_t>(image.get()), "wrong.xyz"),
            "unregistered writer accepted a filename");
    Image masked(RwImageReadMaskedImage("tile.tst", "mask.tst"));
    Require(masked && Row(masked.get(),0)[0].alpha == 64 && Row(masked.get(),0)[0].red == 10,
            "read/make/apply masked-image pipeline failed");
    Require(!RwImageReadMaskedImage("tile.tst", "missing.tst") &&
            !RwImageReadMaskedImage("tile.tst", "wrong.tst"), "masked read failure not propagated");
    Image no_mask(RwImageReadMaskedImage("tile.tst", ""));
    Require(no_mask && Row(no_mask.get(),0)[0].alpha == 40, "empty mask name changed original alpha");
}

void SaveThumbnail() {
    FixtureFileSystem files;files.Open(2u);
    std::filesystem::create_directory(format_root/"save");
    Require(RwImageRegisterImageFormat("png",RtPNGImageRead,RtPNGImageWrite),"register actual SDK PNG codec");
    pal4::NativePkFileManagerRuntime packages;
    const auto manager=packages.InitializePackFileManager();
    auto* fs=RtFSManagerGetDefaultFileSystem();
    packages.ProcessPackFiles(manager,reinterpret_cast<std::uint32_t>(fs));
    Require(packages.AddPrimaryScriptAndProcess()!=0u,"install real native CPK callbacks before thumbnail");
    constexpr std::array fields{40u,44u,48u,68u,56u,76u};
    const auto callbacks=[&]{std::array<std::uint32_t,6> result{};
        for(unsigned i=0;i<fields.size();++i)std::memcpy(&result[i],reinterpret_cast<const char*>(fs)+fields[i],4u);
        return result;};
    const auto script_callbacks=callbacks();
    auto source=Create(800,600,32);
    for(int y=0;y<600;++y)for(int x=0;x<800;++x)
        Row(source.get(),y)[x]=RwRGBA{static_cast<RwUInt8>(x),static_cast<RwUInt8>(y),static_cast<RwUInt8>(x+y),17u};
    const std::vector<RwUInt8> original_pixels(RwImageGetPixels(source.get()),
        RwImageGetPixels(source.get())+RwImageGetStride(source.get())*600);
    std::array<std::uint32_t,0x224u/4u> ui{};ui[0x148u/4u]=reinterpret_cast<std::uint32_t>(source.get());
    pal4::RwTextureDictionaryState dictionary{};
    (void)pal4::RwTextureDictionary::Construct(dictionary);
    pal4::NativeTextureDictionaryContent content([](const char* n){return reinterpret_cast<std::uint32_t>(RwTextureRead(n,nullptr));});
    Require(pal4::RwTextureDictionary::Initialize(dictionary,content),"initialize actual native thumbnail cache");
    pal4::LegacyCrc32 crc;
    const auto path=pal4::NativeFileTextureCacheKey(format_root,"save\\4.png");
    std::array<std::uint8_t,256> normalized{};
    content.CopyName(normalized,path.c_str(),256u);
    content.NormalizeName(normalized);
    const auto crc_name=std::to_string(crc.ComputeAndStore(normalized.data(),content.NameLength(normalized)));
    auto* raster=RwRasterCreate(1,1,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
    Require(raster!=nullptr,"create cached thumbnail raster");
    auto* cached=RwTextureCreate(raster);Require(cached!=nullptr,"create cached thumbnail texture");
    (void)RwTextureSetName(cached,crc_name.c_str());
    (void)RwTexDictionaryAddTexture(reinterpret_cast<RwTexDictionary*>(dictionary.dictionary),cached);cached->refCount=7;
    pal4::NativeGameSaveThumbnailServices services{reinterpret_cast<std::uint32_t>(ui.data()),dictionary,
        [&]{packages.PushFilesystemAccess();},[&]{packages.PopFilesystemAccess();}};
    const auto result=pal4::NativeGameSaveThumbnail::Capture(format_root,4,services);
    Require(result.original_status==1u && result.image_written && result.cache_removed && cached->refCount==1 &&
                cached->dict==nullptr && !packages.FilesystemAccessDepth() && callbacks()==script_callbacks,
        "real thumbnail writes PNG, detaches/clamps actual CRC cache and restores native CPK callbacks");
    Require(!std::memcmp(original_pixels.data(),RwImageGetPixels(source.get()),original_pixels.size()) &&
                ui[0x148u/4u]==reinterpret_cast<std::uint32_t>(source.get()),"thumbnail preserves borrowed cached scene image");
    // Original removal detaches; the observing owner still owns that reference.
    Require(RwTextureDestroy(cached),"release detached test-owned thumbnail texture");
    const auto read=[&]{packages.PushFilesystemAccess();
        auto* image=RwImageRead("save\\4.png");
        packages.PopFilesystemAccess();return Image(image);};
    auto decoded=read();Require(decoded && RwImageGetWidth(decoded.get())==98 && RwImageGetHeight(decoded.get())==75 &&
        RwImageGetDepth(decoded.get())==32,"decode actual saved98x75 PNG through SDK");
    for(int y=0;y<75;++y)for(int x=0;x<98;++x){const auto pixel=Row(decoded.get(),y)[x];
        Require(pixel.red==static_cast<RwUInt8>(x*8) && pixel.green==static_cast<RwUInt8>(y*8) &&
                    pixel.blue==static_cast<RwUInt8>((x+y)*8) && pixel.alpha==255u,
            "PNG has original integer-step scene samples and forced opaque alpha");}
    Row(source.get(),0)[0]=RwRGBA{9u,8u,7u,1u};
    const auto repeated=pal4::NativeGameSaveThumbnail::Capture(format_root,4,services);
    auto changed=read();Require(repeated.original_status==1u && repeated.image_written && !repeated.cache_removed &&
        changed && Row(changed.get(),0)[0].red==9u && Row(changed.get(),0)[0].green==8u &&
        Row(changed.get(),0)[0].blue==7u && Row(changed.get(),0)[0].alpha==255u,
        "repeated thumbnail replaces existing PNG with new cached scene pixels on CRC miss");
    auto device=format_root.root_name().string();auto drive_name=device.substr(0u,1u);
    auto* drive=RtWinFSystemInit(8,device.data(),drive_name.data());
    Require(drive && RtFSManagerRegister(drive) && RtFSManagerGetDefaultFileSystem()==fs,
        "register actual startup-style drive filesystem without replacing the default scoped root");
    const auto absolute=(format_root/"save/4.png").string();
    Require(RtFSManagerGetFileSystemFromFileName(absolute.c_str())==drive,
        "SDK routes absolute filename to its registered drive");
    Row(source.get(),0)[0]=RwRGBA{6u,5u,4u,1u};
    const auto routed=pal4::NativeGameSaveThumbnail::Capture(format_root,4,services);auto drive_decoded=read();
    Require(routed.image_written && drive_decoded && Row(drive_decoded.get(),0)[0].red==6u &&
        Row(drive_decoded.get(),0)[0].green==5u && Row(drive_decoded.get(),0)[0].blue==4u &&
        RtFSManagerGetDefaultFileSystem()==fs && callbacks()==script_callbacks,
        "thumbnail uses registered drive root and preserves unrelated default filesystem/CPK callbacks");
    const auto baseline=allocations.size();ui[0x148u/4u]=0u;bool missing_rejected=false;
    try{(void)pal4::NativeGameSaveThumbnail::Capture(format_root,4,services);}catch(const std::invalid_argument&){missing_rejected=true;}
    ui[0x148u/4u]=reinterpret_cast<std::uint32_t>(source.get());
    Require(missing_rejected && allocations.size()==baseline && !packages.FilesystemAccessDepth() && callbacks()==script_callbacks,
        "missing source cleans temporary SDK image and restores native callbacks on exception");
    const auto failed=pal4::NativeGameSaveThumbnail::Capture(format_root/"missing-directory",4,services);
    Require(failed.original_status==1u && !failed.image_written && !packages.FilesystemAccessDepth() && callbacks()==script_callbacks,
        "original status remains1 on PNG failure, separately reported without leaking images or filesystem scope");
    (void)pal4::RwTextureDictionary::Cleanup(dictionary,content);
    Require(packages.CleanupAllScripts()!=0u,"release retained primary CPK scope");packages.Shutdown();
    Require(!packages.AllocationCount(),"thumbnail package owner releases all allocations");
    std::puts("Native thumbnail: real PNG pixels/replacement, CRC detach, nested filesystem/CPK restoration and failure cleanup pass");
}

void Copy() {
    auto source = Create(3,2,4);
    auto same = Create(3,2,4);
    for (int i=0; i<16; ++i) RwImageGetPalette(source.get())[i] = RwRGBA{static_cast<RwUInt8>(i),20,30,40};
    std::memset(RwImageGetPixels(source.get()), 0xee, 8);
    std::memset(RwImageGetPixels(same.get()), 0xaa, 8);
    for (int y=0; y<2; ++y) for (int x=0; x<3; ++x) RwImageGetPixels(source.get())[y*4+x] = static_cast<RwUInt8>(1+x+y);
    Require(RwImageCopy(same.get(),source.get()) == same.get(), "same-depth copy failed");
    Require(std::memcmp(RwImageGetPalette(source.get()),RwImageGetPalette(same.get()),64)==0 &&
            RwImageGetPixels(same.get())[3]==0xaa && RwImageGetPixels(same.get())[7]==0xaa,
            "copy lost palette or overwrote destination padding");
    auto expanded = Create(3,2,32);
    Require(RwImageCopy(expanded.get(),source.get()) == expanded.get(), "4-to-32 expansion failed");
    Require(Row(expanded.get(),1)[2].red==4 && Row(expanded.get(),1)[2].alpha==40,
            "palette expansion lost RGBA values");
    auto eight = Create(3,2,8);
    std::memset(RwImageGetPalette(eight.get()), 0x5a, 256*sizeof(RwRGBA));
    Require(RwImageCopy(eight.get(),source.get()) == eight.get(), "4-to-8 index copy failed");
    Require(RwImageGetPixels(eight.get())[6]==4 && RwImageGetPalette(eight.get())[1].red==0x5a,
            "4-to-8 copy must retain destination palette while copying byte indexes");
    // Unsupported conversion can dereference null in the original public wrapper;
    // do not treat its internal false result as a safe public failure contract.
}
void Gamma() {
    Require(RwImageSetGamma(2.0f), "gamma table setup failed");
    for (int depth : {4,8,32}) {
        auto image = Create(1,1,depth);
        auto* colors = depth==32 ? Row(image.get(),0) : RwImageGetPalette(image.get());
        const int count = depth==32 ? 1 : 1<<depth;
        for (int i=0; i<count; ++i) colors[i] = RwRGBA{64,128,255,77};
        Require(RwImageGammaCorrect(image.get()) == image.get() && colors[0].red==128 &&
                colors[0].green==181 && colors[0].blue==255 && colors[0].alpha==77,
                "gamma transform differs for palette/RGBA or changes alpha");
        Require(RwImageGammaCorrect(image.get()) == image.get() && colors[0].red==181 &&
                colors[0].green==215, "gamma flag must not suppress a second correction");
    }
    Image missing(RwImageCreate(1,1,32));
    Require(!RwImageGammaCorrect(missing.get()), "missing pixel storage must fail gamma correction");
    Require(RwImageSetGamma(1.0f), "identity gamma setup failed");
    auto source = Create(1,1,32); auto target = Create(1,1,32);
    Row(source.get(),0)[0] = RwRGBA{0,64,128,43};
    Require(RwImageGammaCorrect(source.get()) && Row(source.get(),0)[0].green==64,
            "gamma one should preserve RGB");
    Require(RwImageCopy(target.get(),source.get()) && (target->flags & 2) && (target->flags & 1),
            "copy must propagate gamma flag without losing owned-pixels flag");
}
}  // namespace
int main(int argc, char** argv) {
    try {
        Require(argc == 2, "expected one image boundary case");
        pal4::test::NativeDesktopObservation desktop;
        Engine engine; engine.Start();
        const std::string_view name(argv[1]);
        if (name == "allocation") Allocation();
        else if (name == "failure") Failure();
        else if (name == "masks") Masks();
        else if (name == "camera") Camera();
        else if (name == "paths") Paths();
        else if (name == "formats") Formats();
        else if (name == "copy") Copy();
        else if (name == "gamma") Gamma();
        else if (name == "save_thumbnail") SaveThumbnail();
        else throw std::runtime_error("unknown image case");
        engine.Close();
        desktop.Check("native image boundary preserves desktop focus and cursor");
        Require(!invalid_free && allocations.empty(), "native engine/image allocation ownership leaked or double-freed");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
