#pragma once
#include "pal4/native_texture_dictionary_content.h"
#include "pal4/legacy_crc32.h"
#include "../../native/renderware/cpk_texture_read_scope_native.h"
#include "../../native/renderware/cpk_clump_reader_native.h"
#include <filesystem>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeTextureDictionaryContent(const std::filesystem::path& root) {
    const auto check=[](bool ok){if(!ok) throw std::runtime_error("native effect texture dictionary");};
    NativePackageArchive archive;
    check(archive.Open((root/"gamedata/Effect.cpk").string().c_str()));
    auto* previous=RwTexDictionaryGetCurrent();
    NativeTextureDictionaryContent runtime([](const char* name){
        return reinterpret_cast<std::uint32_t>(RwTextureRead(name,""));
    });
    RwTextureDictionaryState state;LegacyCrc32 crc;
    (void)RwTextureDictionary::Construct(state);
    check(RwTextureDictionary::Initialize(state,runtime));
    try {
        NativeImagePathScope image_path("");
        std::uint32_t decoded{};
        NativeCpkTextureReadScope read(archive,"tex\\HY\\",decoded);
        check(read.Installed());
        constexpr auto name="gamedata\\effect\\tex\\HY\\smoke 1.png";
        auto t=RwTextureDictionary::AddTexture(state,name,runtime,crc);
        check(t && decoded==1 && !read.Missing() && !read.DecodeFailures());
        check(runtime.TextureReferences(t)==1 && runtime.RasterWidth(runtime.TextureRaster(t))>0);
        auto same=RwTextureDictionary::AddTexture(state,"GAMEDATA\\EFFECT\\TEX\\hy\\SMOKE 1.PNG",runtime,crc);
        check(same==t && decoded==1 && runtime.TextureReferences(t)==2);
        check(RwTextureDestroy(reinterpret_cast<RwTexture*>(same)) && runtime.TextureReferences(t)==1);
        check(!RwTextureDictionary::FindTextureByCrc(state,"absent",runtime,crc));
        bool rejected=false;
        const std::string too_long(256,'a');
        try{(void)RwTextureDictionary::FindTextureByCrc(state,too_long.c_str(),runtime,crc);}
        catch(const std::length_error&){rejected=true;}
        check(rejected);
        // No material users remain before the original force-release policy.
        (void)RwTextureDictionary::Cleanup(state,runtime);
        check(!state.dictionary);
    } catch(...) {
        RwTexDictionarySetCurrent(previous);
        (void)RwTextureDictionary::Cleanup(state,runtime);
        throw;
    }
    RwTexDictionarySetCurrent(previous);
}
}
