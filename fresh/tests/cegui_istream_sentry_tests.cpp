#include "pal4/cegui_istream_sentry.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiIstreamSentryRuntimeStub {
public:
    std::vector<std::string> calls;
    std::vector<std::int32_t> peek_values;
    std::vector<std::int32_t> consume_values;
    std::uint32_t clear_ios = 0u;
    std::uint32_t clear_flags = 0u;
    bool clear_rethrow = true;
    std::int32_t sync_result = 0;
    std::uint32_t facet = 0xc000u;
    bool classic = false;
    std::uint32_t released = 0u;
    std::uint32_t destroyed = 0u;

    void SeedWord(std::uint32_t address, std::uint32_t value) {
        words_[address] = value;
    }
    void SeedByte(std::uint32_t address, std::uint8_t value) {
        bytes_[address] = value;
    }
    std::uint32_t Word(std::uint32_t address) {
        return words_.at(address);
    }
    void ClearIos(std::uint32_t ios,
                  std::uint32_t flags, bool rethrow) override {
        calls.emplace_back("clear");
        clear_ios = ios;
        clear_flags = flags;
        clear_rethrow = rethrow;
    }
    std::int32_t SyncStreambuf(std::uint32_t streambuf) override {
        Check(streambuf == 0x8000u, "tie sync receives tied streambuf");
        calls.emplace_back("sync");
        return sync_result;
    }
    std::int32_t PeekVirtual(std::uint32_t streambuf) override {
        Check(streambuf == 0x6000u, "peek virtual receives streambuf");
        calls.emplace_back("peek");
        if (peek_values.empty()) {
            return -1;
        }
        const auto result = peek_values.front();
        peek_values.erase(peek_values.begin());
        return result;
    }
    std::int32_t ConsumeVirtual(std::uint32_t streambuf) override {
        Check(streambuf == 0x6000u, "consume virtual receives streambuf");
        calls.emplace_back("consume");
        if (consume_values.empty()) {
            return -1;
        }
        const auto result = consume_values.front();
        consume_values.erase(consume_values.begin());
        return result;
    }
    pal4::CeguiLocaleSnapshot AcquireLocale(
        std::uint32_t ios) override {
        Check(ios == 0x4020u, "locale acquisition receives ios base");
        calls.emplace_back("acquire");
        return {0xa000u, 0xb000u};
    }
    std::uint32_t FindClassificationFacet(
        std::uint32_t locale) override {
        Check(locale == 0xa000u, "facet query receives locale");
        calls.emplace_back("facet");
        return facet;
    }
    bool IsClassicLocale(std::uint32_t locale) override {
        Check(locale == 0xa000u, "classic check receives locale");
        calls.emplace_back("classic");
        return classic;
    }
    std::uint32_t ClassicClassificationFacet() override {
        calls.emplace_back("fallback");
        return 0xc000u;
    }
    std::uint32_t ReleaseLocaleFacet(
        std::uint32_t held) override {
        Check(held == 0xb000u, "locale release receives held facet");
        calls.emplace_back("release");
        return released;
    }
    void DestroyFacet(std::uint32_t object,
                      std::uint8_t flags) override {
        Check(flags == 1u, "facet destructor receives flag one");
        calls.emplace_back("destroy");
        destroyed = object;
    }
    void ThrowMissingFacet() override {
        calls.emplace_back("throw_bad_cast");
        throw std::bad_cast();
    }
};

void SeedStream(Runtime& runtime, std::uint32_t state = 0u,
                std::uint32_t streambuf = 0x6000u,
                std::uint32_t format = 0u) {
    runtime.SeedWord(0x4000u, 0x5000u);
    runtime.SeedWord(0x5004u, 0x20u);
    runtime.SeedWord(0x4024u, state);
    runtime.SeedWord(0x402cu, format);
    runtime.SeedWord(0x4048u, streambuf);
    runtime.SeedWord(0x404cu, 0u);
    runtime.SeedWord(0x601cu, 0xe000u);
    runtime.SeedWord(0xe000u, 0u);
    runtime.SeedWord(0x602cu, 0xe004u);
    runtime.SeedWord(0xe004u, 0u);
    runtime.SeedWord(0xc010u, 0xd000u);
}

void PeekAreaAndVirtualFallback() {
    Runtime runtime;
    SeedStream(runtime);
    runtime.SeedWord(0xe000u, 0xf000u);
    runtime.SeedWord(0xe004u, 1u);
    runtime.SeedByte(0xf000u, 0xe5u);
    Check(pal4::CeguiIstreamSentry::PeekStreamBuffer(
              0x6000u, runtime) == 0xe5 && runtime.calls.empty() &&
              runtime.Word(0xe000u) == 0xf000u,
          "direct peek reads unsigned byte without advancing cursor");
    runtime.SeedWord(0xe004u, 0u);
    runtime.peek_values = {0x42};
    Check(pal4::CeguiIstreamSentry::PeekStreamBuffer(
              0x6000u, runtime) == 0x42 &&
              runtime.calls == std::vector<std::string>{"peek"},
          "empty get area calls virtual peek");
    runtime.calls.clear();
    runtime.SeedWord(0xe000u, 0xfffffffEu);
    runtime.SeedWord(0xe004u, 3u);
    Check(pal4::CeguiIstreamSentry::PeekStreamBuffer(
              0x6000u, runtime) == -1 &&
              runtime.calls == std::vector<std::string>{"peek"},
          "wrapped cursor count also uses virtual peek");
}

void StateAndTiePaths() {
    Runtime runtime;
    SeedStream(runtime);
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, true, runtime) && runtime.calls.empty(),
          "clean state without tie accepts input");
    SeedStream(runtime, 1u, 0u);
    Check(!pal4::CeguiIstreamSentry::Process(
              0x4000u, true, runtime) &&
              runtime.clear_ios == 0x4020u &&
              runtime.clear_flags == 7u && !runtime.clear_rethrow &&
              runtime.calls == std::vector<std::string>{"clear"},
          "preexisting failure adds failbit and badbit for null streambuf");

    runtime.calls.clear();
    SeedStream(runtime);
    runtime.SeedWord(0x404cu, 0x7000u);
    runtime.SeedWord(0x7000u, 0x7100u);
    runtime.SeedWord(0x7104u, 0x20u);
    runtime.SeedByte(0x7024u, 0u);
    runtime.SeedWord(0x7024u, 0u);
    runtime.SeedWord(0x7048u, 0x8000u);
    runtime.sync_result = -1;
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, true, runtime) &&
              runtime.calls == std::vector<std::string>{"sync", "clear"} &&
              runtime.clear_ios == 0x7020u &&
              runtime.clear_flags == 4u,
          "failed tied-stream sync sets tied badbit before accepting input");
    runtime.calls.clear();
    runtime.SeedByte(0x7024u, 2u);
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, true, runtime) && runtime.calls.empty(),
          "tied stream with existing failbit skips sync");
    runtime.calls.clear();
    runtime.SeedByte(0x7024u, 0u);
    runtime.sync_result = 0;
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, true, runtime) &&
              runtime.calls == std::vector<std::string>{"sync"},
          "successful tied-stream sync does not clear ios state");
}

void WhitespaceAndLocalePaths() {
    Runtime runtime;
    SeedStream(runtime, 0u, 0x6000u, 1u);
    runtime.SeedWord(0xe000u, 0xf000u);
    runtime.SeedWord(0xe004u, 3u);
    runtime.SeedByte(0xf000u, ' ');
    runtime.SeedByte(0xf001u, '\t');
    runtime.SeedByte(0xf002u, 'X');
    runtime.SeedByte(0xd000u + 2u * ' ', 0x48u);
    runtime.SeedByte(0xd000u + 2u * '\t', 0x08u);
    runtime.SeedByte(0xd000u + 2u * 'X', 0u);
    runtime.released = 0xb000u;
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, false, runtime) &&
              runtime.Word(0xe000u) == 0xf002u &&
              runtime.Word(0xe004u) == 1u &&
              runtime.destroyed == 0xb000u &&
              runtime.calls == std::vector<std::string>{
                  "acquire", "facet", "release", "destroy"},
          "skipws consumes classified bytes and releases locale first");
    runtime.calls.clear();
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, true, runtime) &&
              runtime.Word(0xe000u) == 0xf002u &&
              runtime.calls.empty(),
          "no-skip mode bypasses locale and leaves next byte untouched");
    runtime.calls.clear();
    runtime.SeedWord(0xe004u, 0u);
    runtime.peek_values = {-1};
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, false, runtime) &&
              runtime.calls.back() == "peek",
          "EOF during peek ends whitespace scan without consuming");
}

void MissingFacetAndClassicFallback() {
    Runtime runtime;
    SeedStream(runtime, 0u, 0x6000u, 1u);
    runtime.facet = 0u;
    bool threw = false;
    try {
        (void)pal4::CeguiIstreamSentry::Process(
            0x4000u, false, runtime);
    } catch (const std::bad_cast&) {
        threw = true;
    }
    Check(threw && runtime.calls == std::vector<std::string>{
              "acquire", "facet", "classic", "throw_bad_cast", "release"},
          "missing nonclassic facet throws after releasing locale");
    runtime.calls.clear();
    runtime.classic = true;
    runtime.peek_values = {-1};
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, false, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "acquire", "facet", "classic", "fallback",
                  "release", "peek"},
          "classic locale obtains fallback facet and processes EOF");
}

void VirtualWhitespaceConsumption() {
    Runtime runtime;
    SeedStream(runtime, 0u, 0x6000u, 1u);
    runtime.peek_values = {' ', 'X'};
    runtime.consume_values = {' '};
    runtime.SeedByte(0xd000u + 2u * ' ', 0x48u);
    runtime.SeedByte(0xd000u + 2u * 'X', 0u);
    Check(pal4::CeguiIstreamSentry::Process(
              0x4000u, false, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "acquire", "facet", "release", "peek",
                  "consume", "peek"},
          "empty get area uses virtual peek and consume in whitespace loop");
}

}  // namespace

int main() {
    PeekAreaAndVirtualFallback();
    StateAndTiePaths();
    WhitespaceAndLocalePaths();
    MissingFacetAndClassicFallback();
    VirtualWhitespaceConsumption();
    std::cout << "5 CEGUI istream sentry test cases passed\n";
}
