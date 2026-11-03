#include "pal4/portable/combat_numeric_damage_action.h"

#include "pal4/portable/combat_formula_catalog.h"

#include "pal4/combat_damage_action_handler.h"
#include "pal4/combat_normal_damage_calculator.h"
#include "pal4/native_skill_numeric.h"
#include "pal4/random_number_generator.h"

#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
constexpr std::uint32_t kSourceToken = 0x10000001u;
constexpr std::uint32_t kTargetToken = 0x10000002u;
constexpr std::uint32_t kRelatedTokenBase = 0x20000000u;
constexpr std::uint32_t kActionBase = 0x70000000u;
constexpr std::uint32_t kVectorHandle = 1u;
constexpr std::uint32_t kVectorAddress = 0x50000000u;
constexpr std::uint32_t kMaxRelatedTargets = 100000u;

std::uint32_t FloatWord(float value) {
    return std::bit_cast<std::uint32_t>(value);
}
} // namespace

class CombatNumericDamageAction::Impl final : public ICombatDamageActionRuntime {
public:
    const CombatFormulaCatalog& formulas;
    RandomNumberGenerator& random;
    ICombatNumericActorAccess& actors;
    CombatNumericSkillAction action{};
    CombatNumericActorHandle source{}, target{};
    std::vector<CombatNumericActorHandle> related;
    bool active{}, busy{}, vector_live{};
    std::uint32_t random_draws{};

    Impl(const CombatFormulaCatalog& f, RandomNumberGenerator& r,
         ICombatNumericActorAccess& a)
        : formulas(f), random(r), actors(a) {}

    CombatNumericActorHandle Actor(std::uint32_t token) const {
        if (token == kSourceToken) return source;
        if (token == kTargetToken) return target;
        if (token >= kRelatedTokenBase &&
            token - kRelatedTokenBase < related.size())
            return related[token - kRelatedTokenBase];
        throw std::invalid_argument("foreign Combat numeric actor token");
    }

    std::uint32_t ActionAddress() const {
        return kActionBase | action.target_selector;
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (!active) throw std::logic_error("Combat numeric action is not active");
        const auto action_address = ActionAddress();
        if (address >= action_address && address - action_address <= 36u &&
            (address - action_address) % 4u == 0u) {
            switch (address - action_address) {
            case 20u: return FloatWord(action.primary_chance);
            case 24u: return FloatWord(action.secondary_chance);
            case 28u: return FloatWord(action.splash_multiplier);
            case 32u: return FloatWord(action.skill_multiplier);
            case 36u: return FloatWord(action.element);
            default: break;
            }
        }
        if (address == kSourceToken + 660u)
            return actors.HalfDamage(source) ? 1u : 0u;
        if (address == kTargetToken + 660u)
            return actors.HalfDamage(target) ? 1u : 0u;
        if (vector_live && address >= kVectorAddress &&
            (address - kVectorAddress) % 4u == 0u) {
            const auto index = (address - kVectorAddress) / 4u;
            if (index < related.size()) return kRelatedTokenBase + index;
        }
        if (address >= kRelatedTokenBase + 660u) {
            const auto token = address - 660u;
            if (token >= kRelatedTokenBase &&
                token - kRelatedTokenBase < related.size()) {
                const auto index = token - kRelatedTokenBase;
                return actors.HalfDamage(related[index]) ? 1u : 0u;
            }
        }
        throw std::invalid_argument("unexpected Combat numeric logical read");
    }

    std::int32_t ActorStat(std::uint32_t actor,
                           std::uint32_t index) override {
        return actors.Stat(Actor(actor), index);
    }
    std::int32_t CurrentStat(std::uint32_t actor,
                            std::uint32_t index) override {
        return actors.CurrentStat(Actor(actor), index);
    }
    std::int32_t ActorBonus(std::uint32_t actor,
                            std::uint32_t index) override {
        return actors.Bonus(Actor(actor), index);
    }
    std::int32_t ActorElement(std::uint32_t actor,
                              std::uint32_t index) override {
        return actors.Element(Actor(actor), index);
    }
    double ActorDamageFactor(std::uint32_t actor,
                             std::uint32_t mode) override {
        return actors.DamageFactor(Actor(actor), mode);
    }
    double ActorTargetFactor(std::uint32_t actor,
                             std::uint32_t element) override {
        return actors.TargetFactor(Actor(actor), element);
    }
    double DefenderSecondaryFactor(std::uint32_t actor,
                                   std::uint32_t mode) override {
        return actors.SecondaryFactor(Actor(actor), mode);
    }
    double DefenderReduction(std::uint32_t actor,
                             std::uint32_t mode) override {
        return actors.Reduction(Actor(actor), mode);
    }
    std::uint32_t GameDatabase() override {
        throw std::logic_error("Combat numeric damage uses a typed formula catalog");
    }
    float FloatArrayElement(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("Combat numeric damage has no legacy GameDB image");
    }
    float DatabaseFactor(std::uint32_t index) override {
        return formulas.FloatAt(static_cast<std::int32_t>(index));
    }
    std::uint32_t RandomNumber() override {
        ++random_draws;
        return random.Next();
    }
    double HalfValue(std::uint32_t attacker,
                     std::uint32_t defender) override {
        (void)Actor(attacker);
        (void)Actor(defender);
        return CombatDamageConstants::HalfValue();
    }
    std::int64_t FloatToInt(float value) override {
        return TruncateSkillFloat(value);
    }
    std::int32_t AddStat(std::uint32_t actor, std::uint32_t stat,
                         std::int64_t delta) override {
        return actors.AddCurrentStat(Actor(actor), stat, delta);
    }
    void TargetFallback(std::uint32_t target_actor,
                        std::uint32_t source_actor) override {
        actors.TargetFallback(Actor(target_actor), Actor(source_actor));
    }
    std::uint32_t EnumerateTargets(std::uint32_t actor,
                                    std::uint8_t selector) override {
        if (vector_live)
            throw std::logic_error("Combat numeric target enumeration reentered");
        const auto values = actors.RelatedTargets(Actor(actor), selector);
        if (values.size() > kMaxRelatedTargets ||
            values.size() >= std::numeric_limits<std::uint32_t>::max() -
                                 kRelatedTokenBase)
            throw std::length_error("Combat numeric related-target limit");
        for (const auto value : values) {
            actors.ValidateActor(value);
        }
        related = values;
        vector_live = true;
        return kVectorHandle;
    }
    std::uint32_t VectorBegin(std::uint32_t vector) override {
        if (vector != kVectorHandle || !vector_live)
            throw std::invalid_argument("Combat numeric related-target handle");
        return related.empty() ? 0u : kVectorAddress;
    }
    std::uint32_t VectorEnd(std::uint32_t vector) override {
        const auto begin = VectorBegin(vector);
        return begin + static_cast<std::uint32_t>(related.size() * 4u);
    }
    std::uint32_t CandidateStatus(std::uint32_t candidate) override {
        return actors.CombatMode(Actor(candidate));
    }
    bool CheckCandidateActive(std::uint32_t candidate) override {
        return actors.IsNotStateThree(Actor(candidate));
    }
    std::uint32_t FreeVector(std::uint32_t vector) override {
        if (vector != kVectorHandle || !vector_live)
            throw std::invalid_argument("Combat numeric related-target release");
        related.clear();
        vector_live = false;
        return 0u;
    }

    void Begin(const CombatNumericSkillAction& action_value,
               CombatNumericActorHandle source_actor,
               CombatNumericActorHandle target_actor) {
        if (busy) throw std::logic_error("Combat numeric damage action reentered");
        actors.ValidateActor(source_actor);
        actors.ValidateActor(target_actor);
        if (!std::isfinite(action_value.primary_chance) ||
            !std::isfinite(action_value.secondary_chance) ||
            !std::isfinite(action_value.splash_multiplier) ||
            !std::isfinite(action_value.skill_multiplier) ||
            !std::isfinite(action_value.element))
            throw std::invalid_argument("Combat numeric skill action contains non-finite values");
        busy = true;
        active = true;
        action = action_value;
        source = source_actor;
        target = target_actor;
    }

    void End() noexcept {
        related.clear();
        vector_live = false;
        active = false;
        busy = false;
        source = target = 0u;
        action = {};
    }
};

CombatNumericDamageAction::CombatNumericDamageAction(
    const CombatFormulaCatalog& formulas, RandomNumberGenerator& random,
    ICombatNumericActorAccess& actors)
    : impl_(std::make_unique<Impl>(formulas, random, actors)) {}

CombatNumericDamageAction::~CombatNumericDamageAction() = default;

std::int32_t CombatNumericDamageAction::ApplySkill(
    const CombatNumericSkillAction& action,
    CombatNumericActorHandle source, CombatNumericActorHandle target) {
    auto& runtime = *impl_;
    runtime.Begin(action, source, target);
    struct Reset final {
        Impl& runtime;
        ~Reset() { runtime.End(); }
    } reset{runtime};
    const auto action_token = runtime.ActionAddress();
    return CombatDamageActionHandler::Skill(action_token, kSourceToken,
                                             kTargetToken, runtime);
}

std::uint32_t CombatNumericDamageAction::RandomDraws() const noexcept {
    return impl_->random_draws;
}

} // namespace pal4::portable
