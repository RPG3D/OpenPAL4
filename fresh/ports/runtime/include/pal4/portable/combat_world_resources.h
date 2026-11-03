#pragma once

#include "pal4/portable/scene.h"

#include <array>
#include <filesystem>
#include <memory>
#include <string_view>

namespace pal4::portable {

// Loads the original CombatWorld/<area>.cpk BSP into the portable Scene model.
// game_data is the directory containing database.cpk and PALWorld/.
class CombatWorldResources final {
    struct State;
    struct Control;

public:
    class Lease final {
    public:
        Lease() = default;
        ~Lease();
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Lease(Lease&&) noexcept;
        Lease& operator=(Lease&&) noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;
        [[nodiscard]] const Scene& World() const;
        [[nodiscard]] std::string_view Area() const;
        [[nodiscard]] const std::array<std::uint8_t, 16>& LightmapConfig() const;

    private:
        friend class CombatWorldResources;
        explicit Lease(std::shared_ptr<Control>);
        void Release() noexcept;
        std::shared_ptr<Control> control_;
    };

    explicit CombatWorldResources(std::filesystem::path game_data);
    CombatWorldResources(const CombatWorldResources&) = delete;
    CombatWorldResources& operator=(const CombatWorldResources&) = delete;
    CombatWorldResources(CombatWorldResources&&) = delete;
    CombatWorldResources& operator=(CombatWorldResources&&) = delete;
    ~CombatWorldResources();

    // Loads CombatWorld/<area>.cpk transactionally. Repeated same-area loads
    // are no-ops; replacement requires Reset after all leases are returned.
    void Load(std::string_view area);
    void Reset();
    [[nodiscard]] bool Loaded() const noexcept;
    [[nodiscard]] Lease Borrow();

private:
    std::filesystem::path game_data_;
    std::shared_ptr<Control> control_;
};

} // namespace pal4::portable
