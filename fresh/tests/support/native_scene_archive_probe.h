#pragma once
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <source_location>

namespace pal4::test {
inline void CheckSceneArchive(bool ok,
    const std::source_location location=std::source_location::current()) {
    if(!ok) throw std::runtime_error("native scene archive ownership assertion at line " +
        std::to_string(location.line()));
}
inline void VerifyNativeSceneArchiveOwnership(const std::filesystem::path& root,
    NativePkFileManagerRuntime& packages) {
    const auto allocations=packages.AllocationCount(), archives=packages.ArchiveCount();
    const auto instances=packages.ScriptInstanceCount();
    const auto file=packages.BoundFileSystem();
    const auto word=[](std::uint32_t address) {
        return *reinterpret_cast<const std::uint32_t*>(address);
    };
    CheckSceneArchive(file!=0u);
    constexpr std::array<std::uint32_t,6u> slots{40u,44u,48u,68u,56u,76u};
    std::array<std::uint32_t,6u> callbacks{};
    for (std::size_t i=0;i<slots.size();++i) callbacks[i]=word(file+slots[i]);
    const auto depth=word(packages.ManagerAddress()+120u);
    NativeGameArenaManagerOwner arena;
    const auto image=arena.Construct(); arena.BindSceneArchives(packages);
    CheckSceneArchive(!arena.LoadSceneArchive(root/"gamedata"/"PALWorld"/"missing-scene-fixture.cpk"));
    CheckSceneArchive(packages.AllocationCount()==allocations && packages.ArchiveCount()==archives &&
        packages.ScriptInstanceCount()==instances && !packages.SceneArchiveCount());
    for(unsigned pass=0;pass<2u;++pass) {
        CheckSceneArchive(arena.LoadSceneArchive(root/"gamedata"/"PALWorld"/"Q01.cpk"));
        const auto archive=*reinterpret_cast<const std::uint32_t*>(image+316u);
        CheckSceneArchive(archive && packages.SceneArchiveCount()==1u &&
            packages.ScriptInstanceCount()==instances+1u && packages.ArchiveCount()==archives+1u);
        const auto member=packages.OpenSceneArchiveMember(archive,0x53434e45u,"N01\\N01.bsp");
        CheckSceneArchive(member!=0u && packages.OpenSceneArchiveMember(archive,0x53434e45u,"N01\\N01.bsp")==member);
        std::array<std::uint8_t,12u> bytes{};
        packages.Scripts().ReadPackageData(bytes.data(),bytes.size(),member);
        CheckSceneArchive(std::any_of(bytes.begin(),bytes.end(),[](auto b){return b!=0u;}));
        bool threw=false;
        try {
            CheckSceneArchive(packages.WithSceneArchive(archive,[&](NativePackageArchive& package) {
                CheckSceneArchive(package.Find("N01\\N01.bsp")>=0);
                CheckSceneArchive(word(packages.ManagerAddress()+120u)==depth+1u);
                using Exists=std::uint32_t(__cdecl*)(std::uint32_t,const char*);
                using Open=std::uint32_t(__cdecl*)(std::uint32_t,std::uint32_t,const char*);
                using Read=std::uint32_t(__cdecl*)(std::uint32_t,void*,std::uint32_t);
                using Close=std::uint32_t(__cdecl*)(std::uint32_t);
                constexpr auto path="gamedata\\PALWorld\\Q01\\N01\\N01.bsp";
                const auto exists=reinterpret_cast<Exists>(word(file+76u));
                CheckSceneArchive(exists(0u,path)!=0u);
                CheckSceneArchive(packages.AddEffectScriptAndProcess()!=0u);
                CheckSceneArchive(word(packages.ManagerAddress()+120u)==depth+2u);
                (void)packages.CleanupAllScripts();
                CheckSceneArchive(word(packages.ManagerAddress()+120u)==depth+1u);
                CheckSceneArchive(exists(0u,path)!=0u);
                CheckSceneArchive(exists(0u,"gamedata\\PALWorld\\Q01\\N01\\missing.bsp")==0u);
                CheckSceneArchive(reinterpret_cast<Open>(word(file+40u))(0u,0x53434e46u,path)==1u);
                std::array<std::uint8_t,12u> actual{};
                CheckSceneArchive(reinterpret_cast<Read>(word(file+48u))(
                    0x53434e46u,actual.data(),actual.size())==actual.size());
                CheckSceneArchive(actual==bytes);
                CheckSceneArchive(reinterpret_cast<Close>(word(file+44u))(0x53434e46u)!=0u);
                bool rejected=false;
                try {packages.DestroySceneArchive(archive);}
                catch(const std::logic_error&) {rejected=true;}
                CheckSceneArchive(rejected);
                if (pass) throw std::runtime_error("intentional scene mount rollback");
                return true;
            }));
        } catch(const std::runtime_error& error) {
            if(std::string(error.what())!="intentional scene mount rollback") throw;
            threw=true;
        }
        CheckSceneArchive(threw==(pass!=0u));
        CheckSceneArchive(word(packages.ManagerAddress()+120u)==depth);
        for(std::size_t i=0;i<slots.size();++i)
            CheckSceneArchive(word(file+slots[i])==callbacks[i]);
        if(!pass) {arena.ReleaseSceneArchive();arena.ReleaseSceneArchive();}
        else arena.Destroy(); // populated +316 with no world still releases.
        CheckSceneArchive(packages.SceneArchiveCount()==0u && packages.AllocationCount()==allocations &&
            packages.ArchiveCount()==archives && packages.ScriptInstanceCount()==instances &&
            !packages.Scripts().CloseScriptHandle(archive+ScriptManagerCore::kPackageOffset,member));
        bool retired=false;
        try {packages.DestroySceneArchive(archive);} catch(const std::invalid_argument&) {retired=true;}
        CheckSceneArchive(retired);
    }
    CheckSceneArchive(!arena.Address() && !arena.OutstandingAllocations());
}
}
