#include "pal4/combat_actor_state_queries.h"

namespace pal4 {
namespace {

template<class Runtime> bool HasReadyOrStateZero(std::uint32_t iterator,
                         Runtime& runtime) {
    bool found = false;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorCurrent(iterator);
            if (runtime.IsActorReady(actor) || runtime.IsStateZero(actor)) {
                found = true;
                break;
            }
            runtime.IteratorAdvance(iterator);
        }
    } catch (...) {
        runtime.DestroyIterator(iterator);
        throw;
    }
    runtime.DestroyIterator(iterator);
    return found;
}

bool HasTypeTwo(std::uint32_t iterator,
                ICombatActorReadyRosterRuntime& runtime) {
    bool found = false;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorCurrent(iterator);
            if (runtime.ReadWord(actor + 828u) == 2u) {
                found = true;
                break;
            }
            runtime.IteratorAdvance(iterator);
        }
    } catch (...) {
        runtime.DestroyIterator(iterator);
        throw;
    }
    runtime.DestroyIterator(iterator);
    return found;
}

}  // namespace

std::uint32_t CombatActorStateQueries::CheckExtended(
    ICombatActorStateQueryRuntime& runtime) {
    class Legacy final : public ICombatActorRosterStateQueryRuntime {
        ICombatActorStateQueryRuntime& r;
    public:
        explicit Legacy(ICombatActorStateQueryRuntime& r):r(r){}
        std::uint32_t FirstPlayerIterator() override{return r.FirstEvent37Iterator(r.ActorManager()+32u);}
        std::uint32_t FirstMonsterIterator() override{return r.FirstMonsterIterator(r.ActorManager());}
        bool IteratorAtEnd(std::uint32_t i) override{return r.IteratorAtEnd(i);}
        std::uint32_t IteratorCurrent(std::uint32_t i) override{return r.IteratorCurrent(i);}
        void IteratorAdvance(std::uint32_t i) override{r.IteratorAdvance(i);}
        void DestroyIterator(std::uint32_t i) override{r.DestroyIterator(i);}
        bool IsActorReady(std::uint32_t a) override{return r.IsActorReady(a);}
        bool IsStateZero(std::uint32_t a) override{return r.IsStateZero(a);}
    } legacy(runtime);
    return CheckExtended(legacy);
}
std::uint32_t CombatActorStateQueries::CheckExtended(
    ICombatActorRosterStateQueryRuntime& runtime) {
    const auto first_iterator = runtime.FirstPlayerIterator();
    if (!HasReadyOrStateZero(first_iterator, runtime))
        return 0u;

    const auto second_iterator = runtime.FirstMonsterIterator();
    return HasReadyOrStateZero(second_iterator, runtime) ? 3u : 1u;
}

bool CombatActorStateQueries::CheckActorsReady(
    ICombatActorStateQueryRuntime& runtime) {
    class Legacy final : public ICombatActorReadyRosterRuntime {
        ICombatActorStateQueryRuntime& r;
    public:
        explicit Legacy(ICombatActorStateQueryRuntime& r):r(r){}
        std::uint32_t FirstPlayerIterator()override{return r.FirstEvent37Iterator(r.ActorManager()+32u);}
        std::uint32_t FirstMonsterIterator()override{return r.FirstMonsterIterator(r.ActorManager());}
        bool IteratorAtEnd(std::uint32_t t)override{return r.IteratorAtEnd(t);}
        std::uint32_t IteratorCurrent(std::uint32_t t)override{return r.IteratorCurrent(t);}
        void IteratorAdvance(std::uint32_t t)override{r.IteratorAdvance(t);}
        void DestroyIterator(std::uint32_t t)override{r.DestroyIterator(t);}
        bool IsActorReady(std::uint32_t a)override{return r.IsActorReady(a);}
        bool IsStateZero(std::uint32_t a)override{return r.IsStateZero(a);}
        std::uint32_t ReadWord(std::uint32_t a)override{return r.ReadWord(a);}
    } legacy(runtime);
    return CheckActorsReady(legacy);
}
bool CombatActorStateQueries::CheckActorsReady(ICombatActorReadyRosterRuntime& runtime){
    if(HasTypeTwo(runtime.FirstPlayerIterator(),runtime))return true;
    return HasTypeTwo(runtime.FirstMonsterIterator(),runtime);
}

}  // namespace pal4
