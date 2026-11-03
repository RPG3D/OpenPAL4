#include "pal4/actor_manager_data_events.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t candidate = 0x2000u;

struct Runtime final : pal4::IActorManagerDataEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, bool> skipped;
    std::vector<std::uint32_t> items;
    std::vector<std::string> trace;
    std::uint32_t index = 0;
    std::uint32_t property_result = 0x101u;
    std::int32_t metric8 = 3;
    std::int32_t metric9 = -2;
    bool ready = true;
    bool fail_property{},fail_state{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.push_back("read:" + std::to_string(address));
        return words[address];
    }
    bool IsActorReady(std::uint32_t target) override {
        Check(target == actor, "readiness actor");
        trace.push_back("ready");
        return ready;
    }
    std::uint32_t CreateIterator(std::uint32_t target) override {
        Check(target == actor, "iterator actor");
        trace.push_back("iterator");
        return 0x9000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator end target");
        trace.push_back("at-end");
        return index >= items.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator current target");
        trace.push_back("current");
        return items[index];
    }
    bool ObjectCheckState3(std::uint32_t target) override {
        trace.push_back("state:" + std::to_string(target));
        if(fail_state)throw std::runtime_error("state query failure");
        return skipped[target];
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator advance target");
        trace.push_back("next");
        ++index;
    }
    void DestroyIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check(iterator == 0x9000u && flags == 1u,
              "iterator deleting destructor arguments");
        trace.push_back("destroy");
    }
    std::uint32_t InvokePropertySlot(
        std::uint32_t property, std::uint32_t slot,
        std::uint32_t target, std::uint32_t argument) override {
        Check(property == 0xa000u && target == actor,
              "property object and actor argument");
        trace.push_back("slot:" + std::to_string(slot) + ":" +
                        std::to_string(argument));
        if(fail_property)throw std::runtime_error("property callback failure");
        return property_result;
    }
    std::int32_t ActorMetric(std::uint32_t target,
                             std::uint32_t selector) override {
        Check(target == actor && (selector == 8u || selector == 9u),
              "metric virtual slot and selector");
        trace.push_back("metric:" + std::to_string(selector));
        return selector == 8u ? metric8 : metric9;
    }
};

void ProcessPaths() {
    Runtime runtime;
    runtime.words[actor + 800u] = 0xa000u;
    runtime.ready = false;
    Check(!pal4::ActorManagerDataEvents::Process(actor, runtime) &&
          runtime.trace == std::vector<std::string>{"ready"},
          "not-ready actor skips iterator construction");
    runtime.ready = true;
    runtime.trace.clear();
    Check(!pal4::ActorManagerDataEvents::Process(actor, runtime) &&
          runtime.trace == std::vector<std::string>{
              "ready", "iterator", "at-end", "destroy"},
          "empty iterator is released without property callback");

    runtime.trace.clear();
    runtime.items = {0x9100u, 0x9200u};
    runtime.skipped[0x9100u] = true;
    runtime.skipped[0x9200u] = false;
    Check(pal4::ActorManagerDataEvents::Process(actor, runtime) &&
          runtime.trace == std::vector<std::string>{
              "ready", "iterator", "at-end", "current", "state:37120",
              "next", "at-end", "current", "state:37376",
              "read:4896", "slot:4:0", "destroy"},
          "matching second object invokes property and releases iterator");
    runtime.index = 0;
    runtime.trace.clear();
    runtime.property_result = 0x100u;
    Check(!pal4::ActorManagerDataEvents::Process(actor, runtime) &&
          runtime.trace.back() == "destroy",
          "process return uses low byte of property callback");
    runtime.index = 0;
    runtime.trace.clear();
    runtime.skipped[0x9200u] = true;
    Check(!pal4::ActorManagerDataEvents::Process(actor, runtime) &&
          runtime.trace.back() == "destroy" &&
          runtime.trace[runtime.trace.size() - 2] == "at-end",
          "all skipped objects end without property callback");
}

void ExceptionCleanup() {
    for(bool property:{false,true}){
        Runtime runtime;runtime.words[actor+800u]=0xa000u;runtime.items={candidate};
        runtime.fail_property=property;runtime.fail_state=!property;bool failed{};
        try{(void)pal4::ActorManagerDataEvents::Process(actor,runtime);}catch(const std::runtime_error&){failed=true;}
        Check(failed && runtime.trace.back()=="destroy","candidate exception destroys acquired iterator");
        unsigned destroys{};for(const auto& event:runtime.trace)if(event=="destroy")++destroys;
        Check(destroys==1u,"exception releases iterator exactly once");
        runtime.fail_property=runtime.fail_state=false;runtime.trace.clear();
        Check(pal4::ActorManagerDataEvents::Process(actor,runtime),"same candidate runtime retries after exception");
    }
}

void PropertyAndSignedMetrics() {
    Runtime runtime;
    runtime.words[actor + 800u] = 0xa000u;
    Check(pal4::ActorManagerDataEvents::CallPropertySlot8(
              actor, 77u, runtime) == 0x101u &&
          runtime.trace == std::vector<std::string>{
              "read:4896", "slot:8:77"},
          "first wrapper uses property slot +8");
    runtime.trace.clear();
    Check(pal4::ActorManagerDataEvents::CallPropertySlot12(
              actor, 88u, runtime) == 0x101u &&
          runtime.trace == std::vector<std::string>{
              "read:4896", "slot:12:88"},
          "second wrapper uses property slot +12");

    runtime.trace.clear();
    runtime.words[candidate + 116u] = 3u;
    runtime.words[candidate + 76u] = 0xfffffffeu;
    Check(pal4::ActorManagerDataEvents::CheckMetric8(
              actor, candidate, runtime) &&
          pal4::ActorManagerDataEvents::CheckMetric9(
              actor, candidate, runtime),
          "equal signed thresholds pass selectors 8 and 9");
    runtime.words[candidate + 116u] = 4u;
    runtime.words[candidate + 76u] = 0xffffffffu;
    Check(!pal4::ActorManagerDataEvents::CheckMetric8(
              actor, candidate, runtime) &&
          !pal4::ActorManagerDataEvents::CheckMetric9(
              actor, candidate, runtime),
          "greater signed thresholds fail, including negative one");
    runtime.words[candidate + 116u] = 0xffffffffu;
    Check(pal4::ActorManagerDataEvents::CheckMetric8(
              actor, candidate, runtime),
          "negative candidate compares as signed integer");
    runtime.words[actor + 804u] = 0x1234u;
    runtime.words[candidate + 804u] = 0x1234u;
    Check(pal4::ActorManagerDataEvents::SharesOwner(
              actor, candidate, runtime),
          "same owner field compares equal");
    runtime.words[candidate + 804u] = 0x5678u;
    Check(!pal4::ActorManagerDataEvents::SharesOwner(
              actor, candidate, runtime),
          "different owner field compares unequal");
}

}  // namespace

int main() {
    try {
        ProcessPaths();
        ExceptionCleanup();
        PropertyAndSignedMetrics();
        std::cout << "actor_manager_data_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
