#pragma once
#include "../../native/renderware/cpk_png_texture_native.h"
#include "../../native/renderware/sdk_memory_image_file_native.h"
#include "pal4/native_package_archive.h"
#include <rtpng.h>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace pal4::test {
inline void VerifyNativePackagePng(const std::filesystem::path& root) {
    const auto check=[](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
    };
    NativePackageArchive archive;
    check(archive.Open((root/"gamedata/PALActor.cpk").string().c_str()), "PNG original actor archive");
    const auto rgba=archive.ReadMember("101\\101_3.png");
    const auto rgb=archive.ReadMember("101\\101_4.png");
    check(rgba && rgb, "PNG original actor members");
    const auto fs_open=_rtGetFSManager()->isOpened;
    const auto fs_count=_rtGetFSManager()->curNbFS;
    auto* const prior_fs=RtFSManagerGetDefaultFileSystem();
    const std::string path=RwImageGetPath()?RwImageGetPath():"";
    const auto mip=RwTextureGetMipmapping(), automatic=RwTextureGetAutoMipmapping();
    auto* const reader=RwTextureGetReadCallBack();
    struct Restore {
        RwBool mip,automatic;
        ~Restore(){RwTextureSetMipmapping(mip);RwTextureSetAutoMipmapping(automatic);}
    } restore{mip,automatic};
    const auto drop=[](RwTexture* t){if(t)RwTextureDestroy(t);};
    const auto verify=[&](const auto& bytes, const char* name, int format) {
        std::unique_ptr<RwTexture,decltype(drop)> texture(ReadCpkPngTexture(bytes,name),drop);
        check(texture && RwTextureGetRaster(texture.get()), "SDK package PNG decode");
        check(RwRasterGetFormat(texture->raster)==format, "SDK package PNG matches original observed raster format");
        check(std::string_view(RwTextureGetName(texture.get()))==name, "SDK package PNG retains resource name");
        check(RwTextureGetReadCallBack()==reader && RtFSManagerGetDefaultFileSystem()==prior_fs &&
            std::string_view(RwImageGetPath()?RwImageGetPath():"")==path &&
            _rtGetFSManager()->isOpened==fs_open && _rtGetFSManager()->curNbFS==fs_count,
            "SDK package PNG restores reader/path/filesystem ownership");
    };
    RwTextureSetMipmapping(TRUE);RwTextureSetAutoMipmapping(TRUE);
    verify(*rgba,"101_3",0x9500); verify(*rgb,"101_4",0x9600);
    RwTextureSetMipmapping(FALSE);
    verify(*rgb,"opaque",0x0600);
    {
        // Host filesystem already active, including a full one-node manager.
        NativeSdkMemoryImageFile host(*rgb);
        auto* const host_fs=RtFSManagerGetDefaultFileSystem();
        const auto count=_rtGetFSManager()->curNbFS;
        std::unique_ptr<RwTexture,decltype(drop)> texture(ReadCpkPngTexture(*rgba,"nested"),drop);
        check(texture && RtFSManagerGetDefaultFileSystem()==host_fs &&
            _rtGetFSManager()->curNbFS==count, "SDK package PNG preserves active host filesystem");
        auto* image=RtPNGImageRead(NativeSdkMemoryImageFile::Name());
        check(image!=nullptr,"SDK package PNG host remains readable after nested scope");
        RwImageDestroy(image);
    }
    check(RtFSManagerGetDefaultFileSystem()==prior_fs && _rtGetFSManager()->isOpened==fs_open &&
        _rtGetFSManager()->curNbFS==fs_count,"SDK package PNG nested filesystem retires cleanly");
    std::cout << "Package PNG: original RGB/RGBA and automatic mip formats, resource names, reader/path and cold/active filesystem ownership verified\n";
}
}
