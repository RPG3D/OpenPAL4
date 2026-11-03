#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace pal4 {

enum class AiActionKind : std::uint8_t {
    Cure,
    MagicAttack,
    Other,
    NormalAttack,
    Flee,
    Summon,
    ReviveMate,
    CastSkill,
};

struct AiActionBranch {
    AiActionKind kind;
    std::uint32_t weight_offset;
    std::uint32_t virtual_slot;
    std::string_view message;
};

class IAiActionLifecycleRuntime {
public:
    virtual ~IAiActionLifecycleRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IAiActionTaskRuntime : public IAiActionLifecycleRuntime {
public:
    ~IAiActionTaskRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    virtual void PrepareActor(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SelectTarget(
        std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    virtual void InitializeErrorString() = 0;
    virtual void RegisterErrorStringCleanup() = 0;
    [[nodiscard]] virtual std::uint8_t AnimationId(
        std::uint32_t task) = 0;
    virtual void LogAction(std::uint32_t task,
                           std::string_view source,
                           std::string_view message,
                           std::uint8_t animation_id) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchAction(
        std::uint32_t task, std::uint32_t virtual_slot,
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool PostEvent(
        std::uint32_t event) = 0;
};

class AiActionLocaleFacet final {
public:
    static constexpr std::uint32_t kVtable = 0x845864u;

    // IDA 0x55f760.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t facet, std::uint32_t references,
        IAiActionLifecycleRuntime& runtime);
};

class AiActionDerivedLocaleFacet final {
public:
    static constexpr std::uint32_t kVtable = 0x845898u;

    // IDA 0x562580/0x5625a0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t facet, std::uint32_t references,
        IAiActionLifecycleRuntime& runtime);
    static void Destruct(std::uint32_t facet,
                         IAiActionLifecycleRuntime& runtime);
};

class AiActionSecondaryLocaleFacet final {
public:
    static constexpr std::uint32_t kVtable = 0x8458ccu;

    // IDA 0x562920/0x562940.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t facet, std::uint32_t references,
        IAiActionLifecycleRuntime& runtime);
    static void Destruct(std::uint32_t facet,
                         IAiActionLifecycleRuntime& runtime);
};

class AiActionTask final {
public:
    static constexpr std::array<AiActionBranch, 8> kBranches{{
        {AiActionKind::Cure, 24u, 0u,
         "doCureAction AiType:%d, TeamState:%s, EnemyState:%s"},
        {AiActionKind::MagicAttack, 28u, 4u,
         "doMagicAttackAction AiType:%d, TeamState:%s, EnemyState:%s"},
        {AiActionKind::Other, 32u, 8u,
         "doOtherAction AiType:%d, TeamState:%s, EnemyState:%s"},
        {AiActionKind::NormalAttack, 36u, 12u,
         "doNormalAttackAction AiType:%d, TeamState:%s, EnemyState:%s"},
        {AiActionKind::Flee, 40u, 20u,
         "doFleeAction AiType:%d, TeamState:%s, EnemyState:%s"},
        {AiActionKind::Summon, 48u, 16u,
         "doSummonAction AiType:%d, TeamState:%s, EnemyState:%s"},
        {AiActionKind::ReviveMate, 52u, 24u,
         "doReviveMateAction AiType:%d, TeamState:%s, EnemyState:%s"},
        {AiActionKind::CastSkill, 44u, 28u,
         "doCastSkillAction AiType:%d, TeamState:%s, EnemyState:%s"},
    }};

    // IDA 0x55f790.
    [[nodiscard]] static bool Process(std::uint32_t task,
                                      IAiActionTaskRuntime& runtime);
};

enum class AiPipelineKind : std::uint8_t {
    Stunt,
    Magic,
    Property,
};

class IAiSkillRecordCandidateRuntime {
public:
    virtual ~IAiSkillRecordCandidateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ActorCapability(
        std::uint32_t actor, std::uint32_t code) = 0;
    [[nodiscard]] virtual bool HasSkill(
        AiPipelineKind kind, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t FindSkill(
        AiPipelineKind kind, std::uint32_t key) = 0;
    [[nodiscard]] virtual bool CanUseSkill(
        std::uint32_t actor, AiPipelineKind kind,
        std::uint32_t skill) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t ConvertToInt(
        double value) = 0;
};

class AiSkillRecordCandidateSelector final {
public:
    // IDA 0x5625b0/0x562750.
    [[nodiscard]] static std::uint32_t SelectMagic(
        std::uint32_t task, std::uint32_t state,
        IAiSkillRecordCandidateRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectStunt(
        std::uint32_t task, std::uint32_t state,
        IAiSkillRecordCandidateRuntime& runtime);
};

class IAiCollectedSkillCandidateRuntime {
public:
    virtual ~IAiCollectedSkillCandidateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ActorCapability(
        std::uint32_t actor, std::uint32_t code) = 0;
    [[nodiscard]] virtual std::vector<std::uint32_t> CollectRecords(
        std::uint32_t collection, std::uint32_t state,
        AiPipelineKind kind) = 0;
    [[nodiscard]] virtual bool CanUseSkill(
        std::uint32_t actor, AiPipelineKind kind,
        std::uint32_t skill) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t ConvertToInt(
        double value) = 0;
};

class AiCollectedSkillCandidateSelector final {
public:
    // IDA 0x562950/0x562b40.
    [[nodiscard]] static std::uint32_t SelectIndexedMagic(
        std::uint32_t task, std::uint32_t state,
        IAiCollectedSkillCandidateRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectFixedStunt(
        std::uint32_t task, std::uint32_t state,
        IAiCollectedSkillCandidateRuntime& runtime);
};

class IAiActionPipelineRuntime {
public:
    virtual ~IAiActionPipelineRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ActorCapability(
        std::uint32_t actor, std::uint32_t code) = 0;
    [[nodiscard]] virtual std::uint32_t LookupPipeline(
        std::uint32_t task, std::uint32_t virtual_slot,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool HasFirstProperty(
        std::uint32_t pipeline) = 0;
    [[nodiscard]] virtual std::string_view PipelineName(
        std::uint32_t pipeline, AiPipelineKind kind) = 0;
    virtual void LogPipeline(std::uint32_t task,
                             std::string_view source,
                             std::string_view message,
                             std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t SelectCandidate(
        std::uint32_t task, AiPipelineKind kind,
        std::uint32_t pipeline,
        std::uint32_t selected_target) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    virtual void ReleaseCandidate(std::uint32_t candidate) = 0;
    [[nodiscard]] virtual std::string_view CandidateName(
        std::uint32_t candidate) = 0;
    [[nodiscard]] virtual std::uint32_t FallbackNormal(
        std::uint32_t task, std::uint32_t selected_target) = 0;
};

class AiActionPipeline final {
public:
    // IDA 0x560050/0x560280/0x5604b0/0x560830/0x560a00.
    [[nodiscard]] static std::uint32_t SelectCure(
        std::uint32_t task, std::uint32_t selected_target,
        IAiActionPipelineRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectMagicAttack(
        std::uint32_t task, std::uint32_t selected_target,
        IAiActionPipelineRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectOther(
        std::uint32_t task, std::uint32_t selected_target,
        IAiActionPipelineRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectSummon(
        std::uint32_t task, std::uint32_t selected_target,
        IAiActionPipelineRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectReviveMate(
        std::uint32_t task, std::uint32_t selected_target,
        IAiActionPipelineRuntime& runtime);
};

class IAiNormalAttackRuntime {
public:
    virtual ~IAiNormalAttackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ActorCapability(
        std::uint32_t actor, std::uint32_t code) = 0;
    [[nodiscard]] virtual std::uint32_t AcquireActorTargets(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t BuildTargetPool(
        std::uint32_t task, std::uint32_t actor_targets) = 0;
    virtual void ReleaseActorTargets(std::uint32_t actor_targets) = 0;
    [[nodiscard]] virtual std::uint32_t SelectCombatTarget(
        std::uint32_t task, std::uint32_t pool,
        std::uint32_t requested_target) = 0;
    virtual void ReleaseTargetPool(std::uint32_t pool) = 0;
    [[nodiscard]] virtual std::string_view TargetName(
        std::uint32_t target) = 0;
    virtual void LogAction(std::uint32_t task,
                           std::string_view source,
                           std::string_view message,
                           std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCombatAction(
        std::uint32_t actor, std::uint32_t target,
        std::uint32_t flags) = 0;
};
class IAiDirectActionRuntime : public IAiNormalAttackRuntime {
public:
    [[nodiscard]] virtual std::uint32_t CreateCombatFlee(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t FallbackNormal(
        std::uint32_t task, std::uint32_t requested_target) = 0;
};

class AiDirectAction final {
public:
    // IDA 0x5606e0/0x5607e0.
    [[nodiscard]] static std::uint32_t NormalAttack(
        std::uint32_t task, std::uint32_t requested_target,
        IAiNormalAttackRuntime& runtime);
    [[nodiscard]] static std::uint32_t Flee(
        std::uint32_t task, std::uint32_t requested_target,
        IAiDirectActionRuntime& runtime);
};

class IAiFixedSkillRuntime {
public:
    virtual ~IAiFixedSkillRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ConvertSkillId(float value) = 0;
    [[nodiscard]] virtual bool HasSkill(AiPipelineKind kind,
                                        std::int32_t skill_id) = 0;
    [[nodiscard]] virtual bool ActorCapability(
        std::uint32_t actor, std::uint32_t code) = 0;
    [[nodiscard]] virtual std::uint32_t FindSkill(
        AiPipelineKind kind, std::int32_t skill_id) = 0;
    [[nodiscard]] virtual std::string_view SkillName(
        AiPipelineKind kind, std::uint32_t skill) = 0;
    virtual void LogAction(std::uint32_t task,
                           std::string_view source,
                           std::string_view message,
                           std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t SelectSkill(
        AiPipelineKind kind, std::uint32_t task,
        std::uint32_t skill, std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t FallbackNormal(
        std::uint32_t task, std::uint32_t target) = 0;
};

class AiFixedSkillAction final {
public:
    // IDA 0x560bd0.
    [[nodiscard]] static std::uint32_t Select(
        std::uint32_t task, std::uint32_t target,
        IAiFixedSkillRuntime& runtime);
};

struct AiActionTargetKey {
    std::uint32_t animation_id;
    std::uint32_t target_id;
    std::uint32_t selection_id;
};

class IAiActionTargetCatalogRuntime {
public:
    virtual ~IAiActionTargetCatalogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayAnimation(
        std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessTargetSelection(
        std::uint32_t task, std::uint32_t animation) = 0;
    [[nodiscard]] virtual std::uint32_t SelectTarget(
        std::uint32_t task, std::uint32_t animation) = 0;
    [[nodiscard]] virtual std::uint32_t AnimationId(
        std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t RecordBase() = 0;
    [[nodiscard]] virtual std::uint32_t RecordCount() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void ReportMissing(std::uint32_t animation) = 0;
};

class AiActionTargetCatalog final {
public:
    static constexpr std::uint32_t kRecordSize = 68u;

    // IDA 0x560cd0.
    [[nodiscard]] static std::uint32_t ChooseRecord(
        std::uint32_t task, IAiActionTargetCatalogRuntime& runtime);
    // IDA 0x560e60.
    [[nodiscard]] static bool CompareTargets(
        const AiActionTargetKey& key,
        std::uint32_t record, IAiActionTargetCatalogRuntime& runtime);
};

class IAiActionAnimationRuntime {
public:
    virtual ~IAiActionAnimationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ActorAnimationId(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t FindAnimationRecord(
        std::uint32_t animation_id) = 0;
    virtual void ReportMissingActor() = 0;
    virtual void BeginMissingAnimation() = 0;
    virtual void ReportMissingAnimation(
        std::uint32_t animation_id) = 0;
};

class AiActionAnimation final {
public:
    // IDA 0x560ea0.
    [[nodiscard]] static std::uint32_t FindRecord(
        std::uint32_t task, IAiActionAnimationRuntime& runtime);
    // IDA 0x560f20.
    [[nodiscard]] static std::uint32_t GetAnimationId(
        std::uint32_t task, IAiActionAnimationRuntime& runtime);
};

enum class AiTeamState : std::uint32_t {
    SelfInDanger = 0,
    SelfWounded = 1,
    TeammateSlaughtered = 2,
    TeammateInDanger = 3,
    TeammateWounded = 4,
    Normal = 5,
    TurnMatch = 6,
    TurnInterval = 7,
    MissingRecord = 8,
};

enum class AiActorGroup : std::uint8_t {
    Team,
    Enemy,
};

struct AiActorStateLog {
    std::uint32_t state;
    std::uint32_t actor = 0;
    std::int32_t hp = 0;
    std::int32_t max_hp = 0;
    double hp_rate = 0.0;
    float lower_threshold = 0.0f;
    float upper_threshold = 0.0f;
    std::uint32_t dead_count = 0;
    std::uint32_t team_count = 0;
    double dead_rate = 0.0;
};

class IAiActorStateRuntime {
public:
    virtual ~IAiActorStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ConvertToInt(float value) = 0;
    [[nodiscard]] virtual std::int32_t TurnCount(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t CurrentStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t MaxStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::uint32_t CreateActorIterator(
        std::uint32_t actor, AiActorGroup group) = 0;
    [[nodiscard]] virtual std::uint32_t ActorCount(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t ActorCurrent(
        std::uint32_t iterator) = 0;
    virtual void ActorNext(std::uint32_t iterator) = 0;
    virtual void ActorReset(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool IsDead(std::uint32_t actor) = 0;
    virtual void ReleaseActorIterator(std::uint32_t iterator) noexcept = 0;
    virtual void ReportMissingRecord() = 0;
    virtual void LogState(std::uint32_t task,
                          std::string_view source,
                          std::string_view message,
                          const AiActorStateLog& details) = 0;
};

class AiTeamStateClassifier final {
public:
    // IDA 0x560f50.
    [[nodiscard]] static AiTeamState Classify(
        std::uint32_t task, std::uint32_t record,
        IAiActorStateRuntime& runtime);
};

enum class AiEnemyState : std::uint32_t {
    Slaughtered = 0,
    InDanger = 1,
    Wounded = 2,
    Normal = 3,
    MissingRecord = 4,
};

class AiEnemyStateClassifier final {
public:
    // IDA 0x561460.
    [[nodiscard]] static AiEnemyState Classify(
        std::uint32_t task, std::uint32_t record,
        IAiActorStateRuntime& runtime);
};

class IAiActionTargetSelectionRuntime {
public:
    virtual ~IAiActionTargetSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateActorIterator(
        std::uint32_t actor, AiActorGroup group) = 0;
    [[nodiscard]] virtual bool IteratorEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator) noexcept = 0;
    [[nodiscard]] virtual bool IsDead(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::vector<std::uint32_t> BuildPool(
        std::uint32_t task, std::uint32_t iterator,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t SelectTarget(
        std::uint32_t task,
        const std::vector<std::uint32_t>& candidates,
        std::uint32_t requested, std::uint32_t mode,
        std::uint32_t state) = 0;
};

class AiActionTargetSelection final {
public:
    // IDA 0x5617f0.
    [[nodiscard]] static std::uint32_t Select(
        std::uint32_t task, std::uint32_t category,
        std::uint32_t state, std::uint32_t requested,
        std::uint32_t mode,
        IAiActionTargetSelectionRuntime& runtime);
};

class IAiActionPoolRuntime {
public:
    virtual ~IAiActionPoolRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ActorAnimationId(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateActorIterator(
        std::uint32_t actor, AiActorGroup group) = 0;
    virtual void IteratorReset(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool IteratorEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator) noexcept = 0;
    [[nodiscard]] virtual bool IsEligible(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorCapability(
        std::uint32_t actor, std::uint32_t mode) = 0;
};

class AiActionCandidatePool final {
public:
    // IDA 0x561a00.
    [[nodiscard]] static std::vector<std::uint32_t> Build(
        std::uint32_t task, std::uint32_t supplied_iterator,
        std::uint32_t mode, IAiActionPoolRuntime& runtime);
};

class IAiTargetWeightRuntime {
public:
    virtual ~IAiTargetWeightRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t ConvertToInt(
        double value) = 0;
    [[nodiscard]] virtual std::int32_t CurrentStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t MaxStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t ActorSelector(
        std::uint32_t actor) = 0;
};

class AiTargetWeightSelector final {
public:
    // IDA 0x561cd0.
    [[nodiscard]] static std::uint32_t Select(
        std::vector<std::uint32_t>& candidates,
        std::uint32_t skill, std::uint32_t mode,
        IAiTargetWeightRuntime& runtime);
    // IDA 0x562140.
    [[nodiscard]] static double SkillProperty(
        std::uint32_t skill, std::int32_t index,
        IAiTargetWeightRuntime& runtime);
    // IDA 0x562160.
    [[nodiscard]] static double RandomRange(
        float first, float second,
        IAiTargetWeightRuntime& runtime);
};

class IAiSkillActionRuntime {
public:
    virtual ~IAiSkillActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t TargetCategory(
        std::uint32_t skill, AiPipelineKind kind) = 0;
    [[nodiscard]] virtual std::uint32_t SelectTarget(
        std::uint32_t task, std::uint32_t category,
        std::uint32_t state, std::uint32_t requested,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::string_view TargetName(
        std::uint32_t target) = 0;
    virtual void LogAction(std::uint32_t task,
                           std::string_view source,
                           std::string_view message,
                           std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateAction(
        AiPipelineKind kind, std::uint32_t skill,
        std::uint32_t actor, std::uint32_t target) = 0;
};

class AiSkillAction final {
public:
    // IDA 0x5621c0/0x562230/0x5622a0.
    [[nodiscard]] static std::uint32_t SelectStunt(
        std::uint32_t task, std::uint32_t skill,
        std::uint32_t requested, IAiSkillActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectMagic(
        std::uint32_t task, std::uint32_t skill,
        std::uint32_t requested, IAiSkillActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectProperty(
        std::uint32_t task, std::uint32_t property_record,
        std::uint32_t requested, IAiSkillActionRuntime& runtime);
};

class IAiActionEventLoggerRuntime {
public:
    virtual ~IAiActionEventLoggerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void FormatToGlobalBuffer(
        std::uint32_t buffer, const char* format,
        const std::vector<std::uint32_t>& arguments) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchEvent(
        std::uint32_t event_key, std::uint32_t source,
        std::uint32_t message_buffer, std::uint32_t mode) = 0;
};

class AiActionEventLogger final {
public:
    static constexpr std::uint32_t kEventKeyPointer = 0x8b8500u;
    static constexpr std::uint32_t kMessageBuffer = 0x8f9848u;

    // IDA 0x562310.
    [[nodiscard]] static std::uint32_t Log(
        std::uint32_t source, const char* format,
        const std::vector<std::uint32_t>& arguments,
        IAiActionEventLoggerRuntime& runtime);
};

}  // namespace pal4
