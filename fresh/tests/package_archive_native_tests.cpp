#include "pal4/native_package_archive.h"
#include "pal4/native_game_database_source.h"
#include "pal4/script_manager_core.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main(const int argc, char** argv) {
    Check(argc == 2, "provide an original PAL4 installation path");
    const std::string root(argv[1]);
    {
        pal4::NativePackageArchive objects;
        const auto package_path = root + "\\gamedata\\palobject.cpk";
        Check(objects.Open(package_path.c_str()) &&
                  objects.Find("JDumy\\JDumy.dff") >= 0 &&
                  objects.Find("JDumy\\JDumy_jg.eff") >= 0,
              "original ActorControl JDumy assets exist in palobject.cpk");
        const auto clump = objects.ReadMember("JDumy\\JDumy.dff");
        std::uint32_t chunk{};
        if (clump && clump->size() >= sizeof(chunk))
            std::memcpy(&chunk, clump->data(), sizeof(chunk));
        Check(clump && clump->size() > 12u && chunk == 0x10u,
              "original JDumy model starts with RenderWare clump chunk");
        const auto marker = objects.ReadMember("M001\\M001.dff");
        chunk = 0u;
        if (marker && marker->size() >= sizeof(chunk))
            std::memcpy(&chunk, marker->data(), sizeof(chunk));
        Check(marker && marker->size() > 12u && chunk == 0x10u,
              "IDA521C00 original M001 signpost is a PALObject RenderWare clump");
        Check(objects.Find("M001\\M001.tab") < 0 && objects.Find("M001\\M001.uva") < 0 &&
                  objects.Find("M001\\M001.eff") < 0 && objects.Find("M001\\M001.scl") < 0,
              "original M001 is static without animation, UV or resource effect/scale sidecars");
        std::cout << "Original signpost M001 members:";
        for (const auto& member : objects.ListFiles("M001")) std::cout << ' ' << member;
        std::cout << '\n';
    }
    {
        pal4::NativePackageArchive world;
        const auto package_path = root + "\\gamedata\\PALWorld\\q01.cpk";
        Check(world.Open(package_path.c_str()),
              "open original Q01 world package");
        Check(world.Find("N01\\N01.bsp") >= 0 &&
                  world.Find("N01\\N01_floor.dff") < 0 &&
                  world.Find("N01\\N01_wall.dff") < 0,
              "world package contains BSP but movement clumps use scene data");
        Check(world.Find("N01\\MiniMap.df") < 0,
              "Q01/N01 world package has no minimap layer file");
        Check(world.Find("N01\\N01_clipNA.dff") >= 0 &&
                  world.Find("N01\\N01_fog") < 0 &&
                  world.Find("N01\\sequence.xml") < 0 &&
                  world.Find("N01\\sky.dff") < 0 &&
                  world.Find("N01\\skyDual.dff") < 0,
              "Q01/N01 provides clipNA but no optional fog, sky or sequence");
        pal4::NativePackageArchive scene_data;
        Check(scene_data.Open((root + "\\gamedata\\scenedata.cpk").c_str()),
              "open original scene data package");
        Check(scene_data.Find("Q01\\N01\\MiniMap.df") < 0,
              "Q01/N01 scene package takes the empty minimap layer branch");
        Check(scene_data.Find("Q01\\Q01\\MiniMap.df") >= 0 &&
                  world.Find("Q01\\MiniMap.df") < 0,
              "Q01/Q01 minimap layer belongs to scene data");
        const auto q01_minimap = scene_data.ReadMember(
            "Q01\\Q01\\MiniMap.df");
        std::uint32_t q01_layer_count = 0u;
        if (q01_minimap && q01_minimap->size() >= 4u)
            std::memcpy(&q01_layer_count, q01_minimap->data(), 4u);
        Check(q01_minimap && q01_minimap->size() == 32u &&
                  q01_layer_count == 1u,
              "Q01/Q01 has one complete minimap layer record");
        std::uint32_t q01_scale = 0u;
        std::memcpy(&q01_scale, q01_minimap->data() + 4u, 4u);
        Check(q01_scale == 18u &&
                  scene_data.Find("Q01\\Q01\\MiniMap_0.png") >= 0,
              "Q01/Q01 minimap layer has scale 18 and its source image");
        const auto scene_files = scene_data.ListFiles("Q01\\N01");
        const auto event_volumes = scene_data.ReadMember(
            "Q01\\N01\\N01.evf");
        const auto pathways = scene_data.ReadMember(
            "Q01\\N01\\pathway.pwy");
        std::uint32_t event_count = 0u;
        std::uint32_t pathway_count = 1u;
        if (event_volumes && event_volumes->size() >= sizeof(event_count))
            std::memcpy(&event_count, event_volumes->data(),
                        sizeof(event_count));
        if (pathways && pathways->size() >= sizeof(pathway_count))
            std::memcpy(&pathway_count, pathways->data(),
                        sizeof(pathway_count));
        Check(scene_files.size() == 12u &&
                  scene_data.Find("Q01\\N01\\N01_floor.dff") >= 0 &&
                  scene_data.Find("Q01\\N01\\N01_wall.dff") >= 0 &&
                  scene_data.Find("Q01\\N01\\N01_intri.dff") < 0 &&
                  scene_data.Find("Q01\\N01\\npcInfo.npc") >= 0,
              "original Q01/N01 scene data provides movement and NPC files");
        Check(event_volumes && event_volumes->size() == 2810u &&
                  event_count == 2u && pathways && pathways->size() == 4u &&
                  pathway_count == 0u,
              "Q01/N01 has two EVF volumes and an empty pathway file");
        const auto npc_info = scene_data.ReadMember(
            "Q01\\N01\\npcInfo.npc");
        std::uint32_t npc_count{};
        if (npc_info && npc_info->size() >= sizeof(npc_count))
            std::memcpy(&npc_count, npc_info->data(), sizeof(npc_count));
        Check(npc_info && npc_info->size() == 225u && npc_count == 1u,
              "Q01/N01 NPC file contains one encoded record");
        const auto scene_objects = scene_data.ReadMember(
            "Q01\\N01\\GameObjs.gob");
        std::uint32_t object_count{};
        if (scene_objects && scene_objects->size() >= sizeof(object_count))
            std::memcpy(&object_count, scene_objects->data(),
                        sizeof(object_count));
        Check(scene_objects && scene_objects->size() == 2736u &&
                  object_count == 4u,
              "Q01/N01 GOB file contains four original object records");
    }
    static_assert(pal4::ScriptManagerCore::kPackageOffset +
                  pal4::PackageArchiveState::kBytes ==
                  pal4::ScriptManagerCore::kTreeOffset);
    for (const auto [name, expected_count] : {
             std::pair{"ui.cpk", 547u},
             std::pair{"script.cpk", 74u},
             std::pair{"VideoA.cpk", 3u}}) {
        pal4::NativePackageArchive archive;
        const auto path = root + "\\gamedata\\" + name;
        if (std::string_view(name) == "ui.cpk") {
            Check(!archive.Open((root + "\\gamedata\\missing.cpk").c_str()) &&
                  archive.LastFailure() ==
                      pal4::PackageArchiveError::CannotOpenFile,
                  "missing original CPK reports open failure");
        }
        Check(archive.Open(path.c_str()), "open original CPK with game sequence");
        Check(archive.EntryCount() == expected_count,
              "decrypted original CPK entry count");
        if (std::string_view(name) == "script.cpk") {
            Check(archive.Find("script.csb") == 40 &&
                      archive.Find("editData\\script\\script.csb") == -1,
                  "original global CSB lives at script package root");
            const auto script = archive.ReadMember("script.csb");
            std::uint32_t declared_bytes = 0u;
            if (script && script->size() >= sizeof(declared_bytes))
                std::memcpy(&declared_bytes, script->data(),
                            sizeof(declared_bytes));
            Check(script && script->size() == 0x280u &&
                      declared_bytes == 0x27cu,
                  "original global CSB decodes with expected length prefix");
        }
        if (std::string_view(name) == "VideoA.cpk") {
            for (const char* movie : {"1A.bik", "PAL4A.bik"}) {
                Check(archive.Find(movie) >= 0,
                      "original NewGame movie is indexed in VideoA.cpk");
                const auto handle = archive.OpenMember(movie);
                Check(handle != 0u, "open original NewGame movie member");
                char header[4]{};
                const auto read_state = archive.ReadMemberData(
                    handle, header, sizeof(header));
                Check(read_state != 0u &&
                          archive.SeekMember(handle, 0, 1u) == 4 &&
                          std::memcmp(header, "BIK", 3) == 0,
                      "original NewGame movie has a Bink header");
                Check(archive.CloseMember(handle),
                      "close original NewGame movie member");
            }
        }
        if (std::string_view(name) == "ui.cpk") {
            Check(archive.Find("fonts") >= 0,
                  "lookup original UI fonts directory");
            const auto loading = archive.ReadMember("layouts\\loading.xml");
            const auto maimai = archive.ReadMember(
                "imagesets\\maimai_0.imageset");
            Check(loading && maimai,
                  "original loading layout and imageset exist in ui.cpk");
            const std::string_view loading_xml(
                reinterpret_cast<const char*>(loading->data()),
                loading->size());
            const std::string_view maimai_xml(
                reinterpret_cast<const char*>(maimai->data()),
                maimai->size());
            Check(loading_xml.find("ImgMask") != std::string_view::npos &&
                      loading_xml.find("ImgPoser") != std::string_view::npos &&
                      loading_xml.find("set:maimai0") != std::string_view::npos &&
                      maimai_xml.find("loadinglight") != std::string_view::npos,
                  "original loading layout references its CPK imageset");
            const auto files = archive.ListFiles("fonts");
            Check(!files.empty(), "enumerate original UI font files");
            std::size_t decoded_bytes = 0u;
            for (const auto& name : files) {
                const auto member_path = std::string("fonts\\") + name;
                const auto member = archive.ReadMember(member_path.c_str());
                Check(member.has_value() && !member->empty(),
                      "decode original UI font member");
                if (name == "fontPuzzle_0.imageset") {
                    const std::string_view xml(
                        reinterpret_cast<const char*>(member->data()),
                        member->size());
                    Check(xml.find("<Imageset") != std::string_view::npos,
                          "decoded original imageset contains XML");
                }
                decoded_bytes += member->size();
            }
            const auto logo_world = archive.ReadMember(
                "uiWorld\\zjm\\ZJM.bsp");
            Check(logo_world && logo_world->size() > 12u,
                  "decode original Logo UIWorld BSP from ui.cpk");
            std::uint32_t logo_chunk = 0;
            std::memcpy(&logo_chunk, logo_world->data(), sizeof(logo_chunk));
            Check(logo_chunk == 0x0bu,
                  "original Logo BSP starts with RenderWare world chunk");
            std::cout << "Logo UIWorld bytes: " << logo_world->size() << '\n';
            Check(archive.Find("uiWorld\\zjm\\zjtai3-1.dds") >= 0 &&
                      archive.Find("uiWorld\\zjm\\ZJM_trans.uva") >= 0,
                  "original Logo texture and animation members exist");
            std::cout << "ui.cpk fonts: " << files.size() << " files\n";
            std::cout << "decoded font bytes: " << decoded_bytes << '\n';
            const auto handle = archive.OpenMember(
                "fonts\\fontPuzzle_0.imageset");
            Check(handle != 0u && archive.OwnsMemberHandle(handle),
                  "leave original UI member open before archive close");
            Check(archive.Close() && !archive.OwnsMemberHandle(handle) &&
                  !archive.CloseMember(handle) &&
                  archive.Open(path.c_str()) &&
                  archive.EntryCount() == expected_count,
                  "archive close releases live member and permits reopen");
        }
        Check(archive.Close() && !archive.Close(),
              "native package close preserves open-state return");
        Check(archive.Find("fonts") == -1, "closed archive rejects lookup");
    }
    pal4::NativeGameDatabaseSource database;
    Check(database.Open(root) && database.Size() == 647273u,
          "decode original pal4db.db from database.cpk");
    Check(database.ReadText() == "GAME_DB_FLAG" &&
              database.ReadWord() == 20060228u,
          "original database signature and version");
    pal4::ArchiveStreamAdapter database_stream(database);
    std::uint32_t property_count = 0u;
    Check(database_stream.Read(database.StreamState(), &property_count,
                               sizeof(property_count)) == 1u &&
              property_count > 0u && property_count < 1000u,
          "rewritten database reader can consume native stream");
    database.Rewind();
    Check(database.ReadText() == "GAME_DB_FLAG" &&
              database.Position() == 16u,
          "database stream can restart for game loader");
    Check(!database.Open(root + "\\missing-install") &&
              database.Size() == 0u && database.Position() == 0u &&
              !database.ReadWord() && !database.ReadText(),
          "missing database archive leaves no stale stream data");
    std::vector<std::uint8_t> script_object(
        pal4::ScriptManagerCore::kTreeOffset + 16u, 0x5au);
    {
        pal4::NativePackageArchive embedded(std::span<std::uint8_t>(
            script_object.data() + pal4::ScriptManagerCore::kPackageOffset,
            pal4::PackageArchiveState::kBytes));
        const auto path = root + "\\gamedata\\script.cpk";
        Check(embedded.Open(path.c_str()) && embedded.EntryCount() == 74u,
              "embedded script package opens original CPK");
        const auto* package = script_object.data() +
                              pal4::ScriptManagerCore::kPackageOffset;
        std::uint32_t count = 0u;
        std::memcpy(&count, package + 40u, sizeof(count));
        Check(count == 74u &&
              package[pal4::PackageArchiveState::kOpenFlag] == 1u,
              "embedded script object receives package index and open flag");
    }
    const auto* package = script_object.data() +
                          pal4::ScriptManagerCore::kPackageOffset;
    Check(package[pal4::PackageArchiveState::kOpenFlag] == 0u,
          "embedded package closes during archive destruction");
    Check(script_object[pal4::ScriptManagerCore::kTreeOffset] == 0x5au &&
          script_object[pal4::ScriptManagerCore::kTreeOffset + 15u] == 0x5au,
          "embedded package preserves adjacent script tree storage");
    std::cout << "3 original CPK archives opened and closed\n";
}
