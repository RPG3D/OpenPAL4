#include "pal4/native_world_path_planner_owner.h"
#include "pal4/native_package_archive.h"
#include <cstring>
#include "pal4/pathway_grid.h"
#include <bit>
#include <cmath>
#include <string>
#include <vector>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <stdexcept>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Word(const std::uint32_t address) {
    return *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
}

class BorrowedGridRuntime final : public pal4::IPathwayGridRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override {
        throw std::logic_error("query must not allocate");
    }
    void Free(std::uint32_t) override {
        throw std::logic_error("query must not free borrowed data");
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override {
        *reinterpret_cast<std::uint32_t*>(address)=value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override { return Word(address); }
};

}  // namespace

int main(const int argc, char** argv) {
    try {
        pal4::NativeWorldPathPlannerOwner planner;
        Check(planner.Construct() != 0u &&
                  planner.OutstandingAllocations() == 1u &&
                  Word(planner.Address() + 4u) == 0u &&
                  Word(planner.Address() + 8u) == 0u,
              "native world path planner constructs an empty path list");
        Check(!planner.LoadScene("missing-game-root", "Q01", "N01") &&
                  !planner.LoadScene("missing-game-root", "../Q01", "N01"),
              "missing package and invalid scene name do not load pathways");
        if (argc == 2) {
            const auto root = std::filesystem::path(argv[1]);
            pal4::NativePackageArchive archive;
            Check(archive.Open((root / "gamedata/scenedata.cpk").string().c_str()),
                  "original scene archive opens");
            const auto bytes = archive.ReadMember("Q02\\Q02\\pathway.pwy");
            Check(bytes.has_value(), "original Q02/Q02 pathway exists");
            std::size_t offset = 0u;
            const auto read = [&](void* output, const std::size_t count) {
                Check(offset <= bytes->size() && count <= bytes->size()-offset,
                      "original pathway fixture bounds");
                std::memcpy(output, bytes->data()+offset, count);
                offset += count;
            };
            const auto word = [&]() {
                std::uint32_t result = 0u;
                read(&result, sizeof(result));
                return result;
            };
            Check(word() == 5u, "Q02/Q02 has five original paths");
            Check(planner.LoadScene(root, "Q02", "Q02"),
                  "nonempty original pathway loads");
            for (auto index = 0u; index < 5u; ++index) {
                const auto length = word();
                std::string name(length, '\0');
                read(name.data(), length);
                Check(length > 0u, "original path name has terminator storage");
                name.back() = '\0';
                name.resize(std::strlen(name.c_str()));
                Check(word() != 0u, "original path has a grid");
                const auto radius = word();
                std::uint8_t loop = 0u;
                read(&loop, sizeof(loop));
                const auto count = word();
                std::vector<pal4::PathwayPoint> points(count);
                read(points.data(), count*sizeof(pal4::PathwayPoint));
                const auto address = planner.FindPath(name);
                Check(address != 0u, "exact name resolves native grid");
                auto& grid = *reinterpret_cast<pal4::PathwayGridState*>(address);
                Check(grid.point_count == count + (loop != 0u) &&
                          grid.kind == radius && (grid.loop_word & 0xffu) == loop,
                      "native grid preserves radius, loop and point count");
                Check(std::memcmp(reinterpret_cast<void*>(grid.positions),
                                  points.data(), count*sizeof(pal4::PathwayPoint)) == 0,
                      "native grid retains every original coordinate");
                if (loop != 0u) {
                    Check(std::memcmp(reinterpret_cast<void*>(grid.positions+count*12u),
                                      points.data(),12u) == 0,
                          "closed grid appends its first point");
                }
                double expected = 0.0;
                for(auto point=1u; point<grid.point_count; ++point) {
                    const auto& first=points[point-1u];
                    const auto& last=points[point == count ? 0u : point];
                    double squared=0.0;
                    for(auto axis=0u;axis<3u;++axis) {
                        const auto delta=last.coordinates[axis]-first.coordinates[axis];
                        squared+=static_cast<double>(delta)*delta;
                    }
                    expected+=std::sqrt(squared);
                }
                Check(std::fabs(std::bit_cast<float>(grid.total_length_bits)-expected) < 0.02,
                      "native segment lengths reproduce original polyline");
                BorrowedGridRuntime query;
                const auto start=pal4::PathwayGrid::CoordinatesAt(grid,0.0f,query);
                Check(start.coordinates == points.front().coordinates,
                      "existing grid query starts at original first point");
                const auto end = pal4::PathwayGrid::CoordinatesAt(grid,
                    std::bit_cast<float>(grid.total_length_bits), query);
                Check(end.coordinates == (loop ? points.front() : points.back()).coordinates,
                      "native path query reaches original endpoint");
                const auto position = pal4::PathwayGrid::FindClosestPosition(
                    grid, points[count/2u], query);
                const auto recovered = pal4::PathwayGrid::CoordinatesAt(grid,position,query);
                for (auto axis=0u;axis<3u;++axis)
                    Check(std::fabs(recovered.coordinates[axis]-points[count/2u].coordinates[axis])<0.02f,
                          "nearest-position query roundtrips an original waypoint");
                std::printf("path=%s points=%u loop=%u length=%.3f\n",
                    name.c_str(),count,loop,std::bit_cast<float>(grid.total_length_bits));
                Check(planner.FindPath(name+"_missing") == 0u,
                      "path lookup rejects longer unmatched name");
            }
            Check(offset == bytes->size(), "all original records decoded exactly");
            Check(planner.FindPath("missing") == 0u && planner.FindPath("400") == 0u &&
                      planner.FindPath(std::string_view("4001\0x",6u)) == 0u,
                  "lookup rejects absent, prefix and embedded-NUL names");
            const auto stable = planner.OutstandingAllocations();
            const auto retained = planner.FindPath("4001");
            Check(archive.Find("M06\\4\\pathway.pwy") < 0 && retained != 0u &&
                      !planner.LoadScene(root, "M06", "4") &&
                      planner.SceneMemberMissing() &&
                      planner.FindPath("4001") == retained &&
                      planner.OutstandingAllocations() == stable,
                  "absent original pathway retains old list without masking load result");
            Check(!planner.LoadScene(root, "../M06", "4") &&
                      !planner.SceneMemberMissing() &&
                      planner.FindPath("4001") == retained,
                  "invalid input clears missing status and preserves prior paths");
            for(auto cycle=0u;cycle<3u;++cycle)
                Check(planner.LoadScene(root,"Q02","Q02") &&
                          !planner.SceneMemberMissing() &&
                          planner.OutstandingAllocations()==stable,
                      "nonempty original scene reload has stable allocations");
            Check(planner.LoadScene(root,"Q01","N01") &&
                      planner.FindPath("4001")==0u &&
                      planner.OutstandingAllocations()==2u,
                  "nonempty to empty releases records and grids, retaining array capacity");
            planner.Destroy();
            (void)planner.Construct();
            Check(!planner.LoadScene(root, "M06", "4") &&
                      planner.SceneMemberMissing() &&
                      planner.OutstandingAllocations() == 1u &&
                      Word(planner.Address() + 4u) == 0u &&
                      Word(planner.Address() + 8u) == 0u,
                  "new planner stays usable and empty for absent M06/4 pathway");
            Check(planner.LoadScene(root, "Q01", "N01") &&
                      !planner.SceneMemberMissing() &&
                      Word(planner.Address() + 4u) == 0u &&
                      Word(planner.Address() + 8u) == 0u,
                  "original Q01/N01 pathway file loads its zero records");
            Check(planner.LoadScene(root, "Q01", "N01") &&
                      planner.OutstandingAllocations() == 1u,
                  "original empty pathway scene reloads without allocations");
        }
        planner.Destroy();
        Check(planner.Address() == 0u &&
                  planner.OutstandingAllocations() == 0u,
              "native world path planner releases its owned image");
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
        return EXIT_FAILURE;
    }
}
