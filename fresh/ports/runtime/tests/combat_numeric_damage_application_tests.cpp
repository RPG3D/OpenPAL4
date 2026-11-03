#include "pal4/combat_actor_status.h"
#include "pal4/combat_event_property.h"
#include "pal4/native_skill_numeric.h"
#include "pal4/portable/combat_formula_catalog.h"
#include "pal4/portable/combat_numeric_damage_action.h"
#include "pal4/random_number_generator.h"

#include <array>
#include <algorithm>
#include <bit>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using pal4::portable::CombatNumericActorHandle;
using pal4::portable::ICombatNumericActorAccess;
constexpr CombatNumericActorHandle kSource = 0x20000u;
constexpr CombatNumericActorHandle kTarget = 0x30000u;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

template<class T> T ParseNumber(std::string_view text, const char* name) {
    if constexpr (std::is_floating_point_v<T>) {
        const std::string input(text);
        char* end{};
        errno = 0;
        if constexpr (std::is_same_v<T, float>) {
            const auto value = std::strtof(input.c_str(), &end);
            if (errno == ERANGE || end == input.c_str() || *end != '\0' ||
                !std::isfinite(value))
                throw std::invalid_argument(std::string("invalid ") + name);
            return value;
        } else {
            const auto value = std::strtod(input.c_str(), &end);
            if (errno == ERANGE || end == input.c_str() || *end != '\0' ||
                !std::isfinite(value))
                throw std::invalid_argument(std::string("invalid ") + name);
            return value;
        }
    } else {
        T value{};
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
            throw std::invalid_argument(std::string("invalid ") + name);
        return value;
    }
}

struct Inputs final {
    std::string game_data;
    std::uint32_t seed{}, target_selector{};
    std::int32_t source_hp{}, source_max_hp{}, source_stat7{}, source_element{};
    double source_damage_factor{};
    std::int32_t target_hp{}, target_max_hp{}, target_stat7{}, target_element{};
    double target_reduction{}, target_secondary_factor{};
    std::uint32_t target_half{}, element{};
    float skill_multiplier{}, primary_chance{}, secondary_chance{}, splash_multiplier{};
    std::vector<std::string> provided;

    void Parse(int argc, char** argv) {
        const auto assign = [&](int& index, const char* key, auto& output) {
            if (index + 1 >= argc) throw std::invalid_argument(std::string("missing ") + key);
            output = ParseNumber<std::remove_reference_t<decltype(output)>>(argv[++index], key);
            provided.emplace_back(key);
        };
        for (int index = 1; index < argc; ++index) {
            const std::string_view key(argv[index]);
            if (key == "--original-gamedata") {
                if (index + 1 >= argc) throw std::invalid_argument("missing original gamedata path");
                game_data = argv[++index];
            } else if (key == "--seed") assign(index, "seed", seed);
            else if (key == "--target-selector") assign(index, "target-selector", target_selector);
            else if (key == "--source-hp") assign(index, "source-hp", source_hp);
            else if (key == "--source-max-hp") assign(index, "source-max-hp", source_max_hp);
            else if (key == "--source-stat7") assign(index, "source-stat7", source_stat7);
            else if (key == "--source-element") assign(index, "source-element", source_element);
            else if (key == "--source-damage-factor") assign(index, "source-damage-factor", source_damage_factor);
            else if (key == "--target-hp") assign(index, "target-hp", target_hp);
            else if (key == "--target-max-hp") assign(index, "target-max-hp", target_max_hp);
            else if (key == "--target-stat7") assign(index, "target-stat7", target_stat7);
            else if (key == "--target-element") assign(index, "target-element", target_element);
            else if (key == "--target-reduction") assign(index, "target-reduction", target_reduction);
            else if (key == "--target-secondary-factor") assign(index, "target-secondary-factor", target_secondary_factor);
            else if (key == "--target-half") assign(index, "target-half", target_half);
            else if (key == "--element") assign(index, "element", element);
            else if (key == "--skill-multiplier") assign(index, "skill-multiplier", skill_multiplier);
            else if (key == "--primary-chance") assign(index, "primary-chance", primary_chance);
            else if (key == "--secondary-chance") assign(index, "secondary-chance", secondary_chance);
            else if (key == "--splash-multiplier") assign(index, "splash-multiplier", splash_multiplier);
            else throw std::invalid_argument("unknown numeric contract argument");
        }

        constexpr std::string_view required[]{
            "seed", "target-selector", "source-hp", "source-max-hp",
            "source-stat7", "source-element", "source-damage-factor",
            "target-hp", "target-max-hp", "target-stat7", "target-element",
            "target-reduction", "target-secondary-factor", "target-half",
            "element", "skill-multiplier", "primary-chance",
            "secondary-chance", "splash-multiplier"};
        if (game_data.empty())
            throw std::invalid_argument("missing --original-gamedata <dir>");
        for (const auto key : required)
            if (std::find(provided.begin(), provided.end(), key) == provided.end())
                throw std::invalid_argument(std::string("missing --") + std::string(key));
        if (target_half > 1u || element > 4u || target_selector > 255u)
            throw std::invalid_argument("target-half must be 0/1; element 0..4; selector 0..255");
    }
};

struct Actor final {
    std::int32_t current_hp{}, maximum_hp{}, stat7{}, element{};
    std::int32_t health_state{-1};
    double damage_factor{}, reduction{}, secondary_factor{};
    bool half{};
};

class NumericActors final : public ICombatNumericActorAccess,
                            public pal4::ICombatActorStatusRuntime {
public:
    explicit NumericActors(const Inputs& inputs)
        : source_{inputs.source_hp, inputs.source_max_hp, inputs.source_stat7,
                  inputs.source_element, -1, inputs.source_damage_factor},
          target_{inputs.target_hp, inputs.target_max_hp, inputs.target_stat7,
                  inputs.target_element, -1, 0.0, inputs.target_reduction,
                  inputs.target_secondary_factor, inputs.target_half != 0u},
          element_(inputs.element) {}

    void ValidateActor(CombatNumericActorHandle actor) const override {
        static_cast<void>(Get(actor));
    }
    std::int32_t Stat(CombatNumericActorHandle actor,
                      std::uint32_t index) override {
        if (index != 7u) throw std::out_of_range("numeric actor stat index");
        return Get(actor).stat7;
    }
    std::int32_t CurrentStat(CombatNumericActorHandle actor,
                             std::uint32_t index) override {
        if (index != 7u) throw std::out_of_range("numeric current stat index");
        return Get(actor).current_hp;
    }
    std::int32_t Bonus(CombatNumericActorHandle,
                       std::uint32_t) override {
        throw std::logic_error("9116 skill damage does not read actor bonus");
    }
    std::int32_t Element(CombatNumericActorHandle actor,
                         std::uint32_t index) override {
        if ((actor == kSource && index != element_) ||
            (actor == kTarget && index != pal4::CombatDamageState::Next(element_)))
            throw std::out_of_range("numeric actor element index");
        return Get(actor).element;
    }
    double DamageFactor(CombatNumericActorHandle actor,
                        std::uint32_t mode) override {
        if (actor != kSource || mode != 0u)
            throw std::out_of_range("numeric actor damage-factor owner");
        return source_.damage_factor;
    }
    double TargetFactor(CombatNumericActorHandle,
                       std::uint32_t) override {
        throw std::logic_error("9116 skill damage does not read physical target factor");
    }
    double SecondaryFactor(CombatNumericActorHandle actor,
                           std::uint32_t mode) override {
        if (actor != kTarget || mode != element_ + 1u)
            throw std::out_of_range("numeric actor secondary-factor owner");
        return target_.secondary_factor;
    }
    double Reduction(CombatNumericActorHandle actor,
                     std::uint32_t mode) override {
        if (actor != kTarget || mode != element_ + 1u)
            throw std::out_of_range("numeric actor reduction owner");
        return target_.reduction;
    }
    bool HalfDamage(CombatNumericActorHandle actor) override {
        return Get(actor).half;
    }
    std::int32_t AddCurrentStat(CombatNumericActorHandle actor,
                                std::uint32_t stat,
                                std::int64_t delta) override {
        if (stat != 7u || delta < std::numeric_limits<std::int32_t>::min() ||
            delta > std::numeric_limits<std::int32_t>::max())
            throw std::out_of_range("numeric current-stat delta");
        auto& state = Get(actor);
        const auto next = static_cast<std::int64_t>(state.current_hp) + delta;
        if (next < std::numeric_limits<std::int32_t>::min() ||
            next > std::numeric_limits<std::int32_t>::max())
            throw std::overflow_error("numeric current HP overflow");
        state.current_hp = static_cast<std::int32_t>(next);
        writes_.emplace_back(actor, delta);
        static_cast<void>(pal4::CombatActorStatus::UpdateHealthStatus(
            static_cast<std::uint32_t>(actor), *this));
        return state.current_hp;
    }
    void TargetFallback(CombatNumericActorHandle target,
                        CombatNumericActorHandle source) override {
        if (target != kTarget || source != kSource)
            throw std::invalid_argument("numeric fallback actors");
        ++fallbacks_;
    }
    std::vector<CombatNumericActorHandle> RelatedTargets(
        CombatNumericActorHandle, std::uint8_t) override {
        throw std::logic_error("numeric contract does not bind a related-target roster");
    }
    std::uint32_t CombatMode(CombatNumericActorHandle) override {
        throw std::logic_error("numeric contract does not bind candidate Combat modes");
    }
    bool IsNotStateThree(CombatNumericActorHandle) override {
        throw std::logic_error("numeric contract does not bind candidate active state");
    }

    std::int32_t Query(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("health status uses indexed properties");
    }
    std::int32_t QueryIndexed(std::uint32_t actor, std::uint32_t slot,
                              std::uint32_t index) override {
        auto& state = Get(actor);
        if (slot == 0x34u && index == 7u) return state.current_hp;
        if (slot == 0x30u && index == 0u) return state.maximum_hp;
        throw std::out_of_range("numeric health property");
    }
    std::int32_t Set(std::uint32_t actor, std::uint32_t slot,
                     std::int32_t value) override {
        if (slot != 0xacu) throw std::out_of_range("numeric health-state slot");
        Get(actor).health_state = value;
        return value;
    }

    const Actor& Source() const noexcept { return source_; }
    const Actor& Target() const noexcept { return target_; }
    const std::vector<std::pair<CombatNumericActorHandle, std::int64_t>>&
    Writes() const noexcept { return writes_; }
    std::uint32_t Fallbacks() const noexcept { return fallbacks_; }
    void SetElement(std::uint32_t element) noexcept { element_ = element; }

private:
    Actor& Get(CombatNumericActorHandle actor) {
        if (actor == kSource) return source_;
        if (actor == kTarget) return target_;
        throw std::invalid_argument("numeric contract foreign actor handle");
    }
    const Actor& Get(CombatNumericActorHandle actor) const {
        if (actor == kSource) return source_;
        if (actor == kTarget) return target_;
        throw std::invalid_argument("numeric contract foreign actor handle");
    }

    Actor source_, target_;
    std::uint32_t element_{}, fallbacks_{};
    std::vector<std::pair<CombatNumericActorHandle, std::int64_t>> writes_;
};

struct FormulaInput final {
    std::string game_data;
    std::uint32_t seed{}, target_selector{};
    std::int32_t source_hp{}, source_max_hp{}, source_stat7{}, source_element{};
    double source_damage_factor{};
    std::int32_t target_hp{}, target_max_hp{}, target_stat7{}, target_element{};
    double target_reduction{}, target_secondary_factor{};
    std::uint32_t target_half{}, element{};
    float skill_multiplier{}, primary_chance{}, secondary_chance{}, splash_multiplier{};
};

template<class NumericInput>
void Run(const NumericInput& input,
         const pal4::portable::CombatFormulaCatalog& formulas,
         std::string_view formula_source,
         std::optional<std::int32_t> expected_target_state = std::nullopt) {
    Inputs actor_inputs;
    actor_inputs.source_hp = input.source_hp;
    actor_inputs.source_max_hp = input.source_max_hp;
    actor_inputs.source_stat7 = input.source_stat7;
    actor_inputs.source_element = input.source_element;
    actor_inputs.source_damage_factor = input.source_damage_factor;
    actor_inputs.target_hp = input.target_hp;
    actor_inputs.target_max_hp = input.target_max_hp;
    actor_inputs.target_stat7 = input.target_stat7;
    actor_inputs.target_element = input.target_element;
    actor_inputs.target_reduction = input.target_reduction;
    actor_inputs.target_secondary_factor = input.target_secondary_factor;
    actor_inputs.target_half = input.target_half;
    actor_inputs.element = input.element;

    NumericActors actors(actor_inputs);
    static_cast<void>(pal4::CombatActorStatus::UpdateHealthStatus(
        static_cast<std::uint32_t>(kSource), actors));
    static_cast<void>(pal4::CombatActorStatus::UpdateHealthStatus(
        static_cast<std::uint32_t>(kTarget), actors));
    const auto source_before = actors.Source().current_hp;
    const auto target_before = actors.Target().current_hp;
    Check(actors.CurrentStat(kSource, 7u) == input.source_hp &&
              actors.Stat(kSource, 7u) == input.source_stat7,
          "current HP uses the actor current-stat path, separate from formula stat");

    pal4::RandomNumberGenerator random;
    random.Seed(input.seed);
    pal4::portable::CombatNumericDamageAction damage(formulas, random, actors);
    const pal4::portable::CombatNumericSkillAction action{
        input.primary_chance, input.secondary_chance, input.splash_multiplier,
        input.skill_multiplier, static_cast<float>(input.element),
        static_cast<std::uint8_t>(input.target_selector)};
    const auto draws_before = damage.RandomDraws();
    const auto result = damage.ApplySkill(action, kSource, kTarget);
    const auto random_draws = damage.RandomDraws() - draws_before;

    if (expected_target_state) {
        Check(random_draws == 3u,
              "shared 9116 and skill formula consume three original RNG draws");
        Check(actors.Writes().size() == 2u &&
                  actors.Target().current_hp < target_before &&
                  actors.Target().health_state == *expected_target_state,
              "production numeric action writes HP and shared health state");
    }
    std::cout << "formula_source=" << formula_source
              << " formula[5..7]=" << formulas.FloatAt(5) << ','
              << formulas.FloatAt(6) << ',' << formulas.FloatAt(7)
              << " seed=" << input.seed
              << " random_draws=" << random_draws
              << " action_result=" << result << '\n'
              << "stat7_writes=";
    for (std::size_t index = 0; index < actors.Writes().size(); ++index) {
        if (index) std::cout << ',';
        const auto [actor, delta] = actors.Writes()[index];
        std::cout << (actor == kTarget ? "target:" : "source:") << delta;
    }
    std::cout << '\n'
              << "source_hp=" << source_before << "->" << actors.Source().current_hp
              << " source_health_state=" << actors.Source().health_state << '\n'
              << "target_hp=" << target_before << "->" << actors.Target().current_hp
              << " target_health_state=" << actors.Target().health_state
              << " zero_damage_fallbacks=" << actors.Fallbacks() << '\n'
              << "scope=numeric contract operands; no actor roster or battle result\n";
}

void Word(std::string& out, std::uint32_t value) {
    for (unsigned shift = 0u; shift < 32u; shift += 8u)
        out.push_back(static_cast<char>(value >> shift));
}

void Text(std::string& out, std::string_view value) {
    Word(out, static_cast<std::uint32_t>(value.size()));
    out.append(value);
}

void AddTextTable(std::string& out, std::string_view name,
                  std::string_view string_field, std::string_view value) {
    Text(out, name); Word(out, 20060225u); Text(out, name); Text(out, "");
    Word(out, 0u); Word(out, 2u); Word(out, 1u);
    Text(out, "ID"); Word(out, 1u); Text(out, ""); Text(out, "");
    Text(out, string_field); Word(out, 2u); Text(out, ""); Text(out, "");
    Word(out, 20060223u); Word(out, 7u); Text(out, value);
    Word(out, 20060224u); Word(out, 20060226u);
}

std::string ControlledPal4db() {
    std::string data;
    Text(data, "GAME_DB_FLAG"); Word(data, 20060228u); Word(data, 0u);
    Word(data, 4u);
    Text(data, "Property"); Word(data, 20060225u); Text(data, "Property");
    Text(data, ""); Word(data, 0u); Word(data, 9u); Word(data, 1u);
    Text(data, "ID"); Word(data, 1u); Text(data, ""); Text(data, "");
    Text(data, "TYPE"); Word(data, 1u); Text(data, ""); Text(data, "");
    Text(data, "PA_NAME"); Word(data, 2u); Text(data, ""); Text(data, "");
    for(const auto* field:{"PA_DESCRIPTION","PA_PROPERTY","PA_ICON","PA_MODEL","PA_TEXTURE","PA_ATTACHED_EFFECT"}){Text(data,field);Word(data,2u);Text(data, "");Text(data, "");}
    Word(data, 20060223u); Word(data, 7u); Word(data, 4u); Text(data, "test");
    for(unsigned i=0;i<6;++i)Text(data, "");
    Word(data, 20060224u); Word(data, 20060226u);
    AddTextTable(data, "Tutorial", "STRING", "tutorial");
    AddTextTable(data, "GameString", "STRING", "string");
    Text(data, "CombatFormulaConfig"); Word(data, 20060225u);
    Text(data, "CombatFormulaConfig"); Text(data, ""); Word(data, 0u);
    Word(data, 2u); Word(data, 32u);
    Text(data, "ID"); Word(data, 1u); Text(data, ""); Text(data, "");
    Text(data, "Value"); Word(data, 3u); Text(data, ""); Text(data, "");
    for (std::uint32_t key = 1u; key <= 32u; ++key) {
        float value = 0.0f;
        if (key == 6u) value = 1.0f;
        if (key == 7u) value = 0.9f;
        if (key == 8u) value = 1.1f;
        Word(data, 20060223u); Word(data, key);
        Word(data, std::bit_cast<std::uint32_t>(value)); Word(data, 20060224u);
    }
    Word(data, 20060226u);
    return data;
}

class AliasActors final : public ICombatNumericActorAccess {
public:
    explicit AliasActors(std::vector<CombatNumericActorHandle> related = {})
        : related_(std::move(related)) {}

    void ValidateActor(CombatNumericActorHandle actor) const override {
        if (actor < 1u || actor > 3u)
            throw std::invalid_argument("alias-contract actor");
    }
    std::int32_t Stat(CombatNumericActorHandle actor,
                      std::uint32_t index) override {
        ValidateActor(actor);
        if (index != 7u) throw std::out_of_range("alias-contract stat");
        return 4;
    }
    std::int32_t CurrentStat(CombatNumericActorHandle actor,
                             std::uint32_t index) override {
        ValidateActor(actor);
        if (index != 7u) throw std::out_of_range("alias-contract current stat");
        return current_hp_[actor - 1u];
    }
    std::int32_t Bonus(CombatNumericActorHandle,
                       std::uint32_t) override { return 0; }
    std::int32_t Element(CombatNumericActorHandle actor,
                         std::uint32_t index) override {
        ValidateActor(actor);
        if (index > 1u) throw std::out_of_range("alias-contract element");
        return 0;
    }
    double DamageFactor(CombatNumericActorHandle actor,
                        std::uint32_t mode) override {
        ValidateActor(actor);
        if (actor != 1u || mode != 0u)
            throw std::out_of_range("alias-contract damage factor");
        return 1.0;
    }
    double TargetFactor(CombatNumericActorHandle,
                       std::uint32_t) override { return 1.0; }
    double SecondaryFactor(CombatNumericActorHandle actor,
                           std::uint32_t mode) override {
        ValidateActor(actor);
        if (mode != 1u) throw std::out_of_range("alias-contract secondary factor");
        return 0.2;
    }
    double Reduction(CombatNumericActorHandle actor,
                     std::uint32_t mode) override {
        ValidateActor(actor);
        if (mode != 1u) throw std::out_of_range("alias-contract reduction");
        return 0.1;
    }
    bool HalfDamage(CombatNumericActorHandle actor) override {
        ValidateActor(actor);
        half_reads.push_back(actor);
        return false;
    }
    std::int32_t AddCurrentStat(CombatNumericActorHandle actor,
                                std::uint32_t stat,
                                std::int64_t delta) override {
        ValidateActor(actor);
        if (stat != 7u) throw std::out_of_range("alias-contract stat write");
        writes.emplace_back(actor, delta);
        auto& hp = current_hp_[actor - 1u];
        hp = static_cast<std::int32_t>(static_cast<std::int64_t>(hp) + delta);
        return hp;
    }
    void TargetFallback(CombatNumericActorHandle,
                        CombatNumericActorHandle) override {}
    std::vector<CombatNumericActorHandle> RelatedTargets(
        CombatNumericActorHandle target, std::uint8_t selector) override {
        if (target != 2u || selector != 7u)
            throw std::invalid_argument("alias-contract related target request");
        return related_;
    }
    std::uint32_t CombatMode(CombatNumericActorHandle actor) override {
        ValidateActor(actor);
        candidate_status.push_back(actor);
        return 1u;
    }
    bool IsNotStateThree(CombatNumericActorHandle actor) override {
        ValidateActor(actor);
        candidate_active.push_back(actor);
        return true;
    }

    std::vector<std::pair<CombatNumericActorHandle, std::int64_t>> writes;
    std::vector<CombatNumericActorHandle> half_reads, candidate_status,
        candidate_active;

private:
    std::vector<CombatNumericActorHandle> related_;
    std::array<std::int32_t, 3> current_hp_{100, 80, 60};
};

void VerifyLogicalHandleAliases(
    const pal4::portable::CombatFormulaCatalog& formulas) {
    {
        AliasActors actors;
        pal4::RandomNumberGenerator random;
        random.Seed(17u);
        pal4::portable::CombatNumericDamageAction damage(formulas, random, actors);
        const pal4::portable::CombatNumericSkillAction action{
            1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0u};
        static_cast<void>(damage.ApplySkill(action, 1u, 1u));
        Check(actors.writes.size() == 2u && actors.writes[0].first == 1u &&
                  actors.writes[1].first == 1u && actors.half_reads ==
                      std::vector<CombatNumericActorHandle>{1u},
              "self-target retains distinct internal source/target tokens");
    }
    {
        const std::vector<CombatNumericActorHandle> related{1u, 2u, 3u, 3u};
        AliasActors actors(related);
        pal4::RandomNumberGenerator random;
        random.Seed(17u);
        pal4::portable::CombatNumericDamageAction damage(formulas, random, actors);
        const auto before = damage.RandomDraws();
        const pal4::portable::CombatNumericSkillAction action{
            1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 7u};
        static_cast<void>(damage.ApplySkill(action, 1u, 2u));
        Check(damage.RandomDraws() - before == 7u,
              "primary, secondary and all four related formulas consume RNG");
        Check(actors.candidate_status == related &&
                  actors.candidate_active == related,
              "candidate filters preserve aliases, duplicates and roster order");
        const std::vector<CombatNumericActorHandle> expected_half_reads{
            2u, 1u, 2u, 3u, 3u};
        Check(actors.half_reads == expected_half_reads,
              "half-damage reads decode each related token without reverse lookup");
        const std::vector<CombatNumericActorHandle> expected_writes{
            2u, 1u, 2u, 3u, 3u, 1u};
        Check(actors.writes.size() == expected_writes.size(),
              "shared handler applies primary, each candidate and accumulated source stat");
        for (std::size_t index = 0; index < expected_writes.size(); ++index)
            Check(actors.writes[index].first == expected_writes[index],
                  "related-target writes preserve original list order");
    }
}

FormulaInput ControlledInputs(std::int32_t target_hp) {
    FormulaInput input;
    input.seed = 17u;
    input.target_selector = 0u;
    input.source_hp = 100;
    input.source_max_hp = 100;
    input.source_stat7 = 4;
    input.source_element = 0;
    input.source_damage_factor = 1.0;
    input.target_hp = target_hp;
    input.target_max_hp = 100;
    input.target_stat7 = 3;
    input.target_element = 0;
    input.target_reduction = 0.1;
    input.target_secondary_factor = 0.2;
    input.target_half = 1u;
    input.element = 4u;
    input.skill_multiplier = 2.0f;
    input.primary_chance = 1.0f;
    input.secondary_chance = 0.0f;
    input.splash_multiplier = 1.0f;
    return input;
}

void RunControlledFormulaContract() {
    const auto bytes = ControlledPal4db();
    const auto formulas = pal4::portable::CombatFormulaCatalog::ReadDatabase(
        std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()));
    VerifyLogicalHandleAliases(formulas);
    Run(ControlledInputs(20), formulas, "controlled Pal4db fixture", 2);
    Run(ControlledInputs(1), formulas, "controlled Pal4db death-state fixture", 3);
}

void PrintUsage() {
    std::cout << "Usage: pal4_portable_combat_numeric_damage_tests --original-gamedata <dir>"
                 " --seed <u32> --target-selector <u32> --source-hp <i32>"
                 " --source-max-hp <i32> --source-stat7 <i32> --source-element <i32>"
                 " --source-damage-factor <f64> --target-hp <i32>"
                 " --target-max-hp <i32> --target-stat7 <i32>"
                 " --target-element <i32> --target-reduction <f64>"
                 " --target-secondary-factor <f64> --target-half <0|1>"
                 " --element <0..4> --skill-multiplier <f32>"
                 " --primary-chance <f32> --secondary-chance <f32>"
                 " --splash-multiplier <f32>\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            RunControlledFormulaContract();
            std::cout << "SKIP original Pal4db formula branch: pass --original-gamedata <dir> and explicit actor operands\n";
            return 0;
        }
        if (std::string_view(argv[1]) == "--help") {
            PrintUsage();
            return 0;
        }
        Inputs inputs;
        inputs.Parse(argc, argv);
        const auto formulas = pal4::portable::CombatFormulaCatalog::ReadFromGameData(
            inputs.game_data);
        Run(inputs, formulas, "original Pal4db.db");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
