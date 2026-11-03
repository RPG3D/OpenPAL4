#include "pal4/logic_error_record.h"
#include "pal4/std_codecvt_facet_resolver.h"
#include "pal4/crt_atexit_stub.h"
#include "pal4/crt_noop_atexit_registration.h"

#include <cstdio>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <locale>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class TrackedCodecvt final : public pal4::StdCodecvtChar {
public:
    explicit TrackedCodecvt(int& deaths) : deaths_(deaths) {}
    ~TrackedCodecvt() override { ++deaths_; }
private:
    int& deaths_;
};

void Exceptions() {
    // Catch through the public standard base after the source has unwound.
    try {
        pal4::LogicErrorObject original(std::string("missing scene asset"));
        pal4::ExceptionCopyThrower::ThrowLogicErrorCopy(original);
    } catch (const std::logic_error& copy) {
        Require(std::string_view(copy.what()) == "missing scene asset", "logic_error lost owned message");
    }
    bool caught = false;
    try {
        pal4::OutOfRangeErrorObject original("actor index 42");
        pal4::ExceptionCopyThrower::ThrowOutOfRangeCopy(original);
    } catch (const std::out_of_range& copy) {
        caught = true;
        Require(std::string_view(copy.what()) == "actor index 42", "out_of_range lost message");
    }
    Require(caught, "throw adapter did not throw the concrete type");
    const std::bad_alloc allocation_failure;
    const std::bad_alloc copied(allocation_failure);
    caught = false;
    try { throw copied; }
    catch (const std::bad_alloc& error) {
        caught = true;
        Require(std::strcmp(error.what(), allocation_failure.what()) == 0,
                "bad_alloc copy changed its diagnostic");
    }
    Require(caught, "bad_alloc lost its dynamic exception type");
}

void Locale() {
    int deaths = 0;
    {
        std::locale owner(std::locale::classic(), new TrackedCodecvt(deaths));
        std::locale copy = owner;
        const auto& facet = pal4::StdCodecvtFacetResolver::Resolve(owner);
        Require(&facet == &pal4::StdCodecvtFacetResolver::Resolve(copy), "resolver did not retain locale facet identity");
        owner = std::locale::classic();
        Require(deaths == 0, "copied locale failed to own facet");
        std::mbstate_t state{};
        const char source[] = {'a', '\0', static_cast<char>(0xFF)};
        char destination[] = {'x', 'y', 'z'};
        const char* from_next = nullptr;
        char* to_next = nullptr;
        const auto result = facet.in(state, source, source + sizeof(source), from_next,
                                     destination, destination + sizeof(destination), to_next);
        Require(result == std::codecvt_base::noconv, "char codecvt must report noconv");
        Require(from_next == source && to_next == destination, "noconv advanced buffer pointers");
        Require(std::string_view(destination, 3) == "xyz", "noconv overwrote destination");
        Require(facet.always_noconv(), "char codecvt unexpectedly requires conversion");
    }
    Require(deaths == 1, "facet ownership leaked or double-destroyed");
}

void FirstExit() { std::puts("exit:first"); }
void SecondExit() { std::puts("exit:second"); }

void ExitRegistration() {
    pal4::CrtAtexitApi native;
    Require(native.Register(nullptr) != 0, "null callback accepted");
    pal4::CrtNoopAtExitRuntime legacy;
    Require(legacy.RegisterAtExit(0) != 0, "null original address accepted");
    Require(legacy.RegisterAtExit(0x74B3AA) != 0, "untranslated EXE callback accepted");
    Require(legacy.RegisterAtExit(pal4::CrtNoopAtExitRegistration::kCallback) == 0,
            "reviewed no-op callback rejected");
    Require(native.Register(FirstExit) == 0, "first registration failed");
    Require(native.Register(SecondExit) == 0, "second registration failed");
    std::puts("registered");
}

void Streams() {
    std::ios_base::Init first;
    {
        std::ios_base::Init second;
        Require(std::cin.rdbuf() && std::cout.rdbuf() && std::cerr.rdbuf() && std::clog.rdbuf(),
                "standard stream buffer missing");
        Require(std::cin.tie() == &std::cout, "cin is not tied to cout");
        Require((std::cerr.flags() & std::ios_base::unitbuf) != 0, "cerr lacks unitbuf");
    }
    // Destroying one Init must leave the streams usable while another lives.
    std::cout << "streams-ready\n";
    std::cout.flush();
    Require(std::cout.good(), "stream unavailable after nested Init destruction");
}

void WideStreams() {
#ifdef _WIN32
    static_assert(sizeof(wchar_t) == 2);
    Require(std::char_traits<wchar_t>::eof() == 0xffff, "Windows wide EOF differs from reviewed VC6 contract");
#endif
    // Match the reviewed two-byte bulk I/O contract without sharing any
    // unsigned-short VC6 stream object or vtable with the host library.
    const wchar_t text[] = {L'A', L'\0', L'\x4e2d', L'\xfffe'};
    std::wstringbuf buffer(std::ios::in | std::ios::out);
    Require(buffer.sputn(text, 4) == 4, "wide bulk write failed");
    Require(buffer.pubseekpos(0, std::ios::in) != std::wstreampos(-1), "wide seek failed");
    wchar_t result[] = {L'x', L'x', L'x', L'x', L'G', L'G'};
    Require(buffer.sgetn(result, 6) == 4, "wide read did not stop at EOF");
    Require(std::wstring_view(result, 4) == std::wstring_view(text, 4), "wide I/O truncated NUL or BMP data");
    Require(result[4] == L'G' && result[5] == L'G', "wide read overran actual count");
    Require(std::char_traits<wchar_t>::eq_int_type(buffer.sgetc(), std::char_traits<wchar_t>::eof()),
            "wide buffer did not retain EOF");
}

class FailingWideSync final : public std::wstreambuf {
public:
    int calls = 0;
private:
    int sync() override { ++calls; return -1; }
};

void FlushFailure() {
    FailingWideSync buffer;
    std::wostream stream(&buffer);
    stream.flush();
    Require(buffer.calls == 1 && stream.bad(), "sync failure did not set badbit");
    stream.clear();
    stream.exceptions(std::ios::badbit);
    bool caught = false;
    try { stream.flush(); }
    catch (const std::ios_base::failure&) { caught = true; }
    Require(caught && stream.bad() && buffer.calls == 2, "flush exception contract differs");
}

class FileFixture final {
public:
    FileFixture() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 32; ++attempt) {
            path_ = std::filesystem::temp_directory_path() /
                    ("pal4-filebuf-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(path_)) return;
        }
        throw std::runtime_error("could not create exclusive file fixture");
    }
    ~FileFixture() {
        std::error_code ignored;
        std::filesystem::remove(path_ / "narrow.bin", ignored);
        std::filesystem::remove(path_ / "wide.bin", ignored);
        std::filesystem::remove(path_, ignored);
    }
    std::filesystem::path File(const char* name) const { return path_ / name; }
private:
    std::filesystem::path path_;
};

void FilebufIntegration() {
    FileFixture fixture;
    std::filebuf narrow;
    const auto failed = std::streampos(-1);
    Require(narrow.pubseekoff(0, std::ios::beg, std::ios::in) == failed,
            "unopened file accepted seek");
    Require(narrow.sgetc() == std::char_traits<char>::eof(), "unopened file yielded data");
    char storage[128]{};
    narrow.pubsetbuf(storage, sizeof(storage));
    const auto mode = std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc;
    Require(narrow.open(fixture.File("narrow.bin"), mode) == &narrow, "file open failed");
    const char bytes[] = {'\0', '\x7f', static_cast<char>(0x80), static_cast<char>(0xff), 'Z'};
    Require(narrow.sputn(bytes, sizeof(bytes)) == sizeof(bytes) && narrow.pubsync() == 0,
            "binary file write/sync failed");
    Require(narrow.pubseekoff(0, std::ios::beg, std::ios::in) != failed, "file seek failed");
    Require(narrow.sgetc() == 0 && narrow.sgetc() == 0 && narrow.sbumpc() == 0,
            "peek consumed data or lost embedded NUL");
    Require(narrow.sputbackc('\0') == 0 && narrow.sbumpc() == 0, "same-character putback failed");
    const auto saved = narrow.pubseekoff(0, std::ios::cur, std::ios::in);
    Require(saved != failed && narrow.sbumpc() == 0x7f, "position save/read failed");
    Require(narrow.pubseekpos(saved, std::ios::in) != failed && narrow.sbumpc() == 0x7f,
            "position token did not restore input");
    Require(narrow.sbumpc() == 0x80 && narrow.sbumpc() == 0xff,
            "high bytes confused with signed EOF");
    Require(narrow.sungetc() != std::char_traits<char>::eof() && narrow.sbumpc() == 0xff,
            "unget failed at high byte");
    Require(narrow.sbumpc() == 'Z' && narrow.sbumpc() == std::char_traits<char>::eof(),
            "file end contract failed");
    Require(narrow.close() == &narrow && !narrow.is_open() && narrow.close() == nullptr,
            "file close ownership contract failed");

    std::wfilebuf wide;
    wide.pubimbue(std::locale::classic());
    Require(wide.open(fixture.File("wide.bin"), mode) == &wide, "wide file open failed");
    const wchar_t text[] = {L'A', L'\0', L'Z'};
    Require(wide.sputn(text, 3) == 3 && wide.pubsync() == 0, "wide file write failed");
    Require(wide.pubseekoff(0, std::ios::beg, std::ios::in) != std::wstreampos(-1), "wide file seek failed");
    Require(wide.sgetc() == L'A' && wide.sbumpc() == L'A', "wide peek consumed data");
    const auto wide_saved = wide.pubseekoff(0, std::ios::cur, std::ios::in);
    Require(wide_saved != std::wstreampos(-1) && wide.sbumpc() == 0, "wide NUL read failed");
    Require(wide.pubseekpos(wide_saved, std::ios::in) != std::wstreampos(-1) && wide.sbumpc() == 0,
            "wide conversion-state position restore failed");
    Require(wide.sbumpc() == L'Z' && wide.close() == &wide, "wide file read/close failed");
}

int CallbackSlot() { static const int slot = std::ios_base::xalloc(); return slot; }
void RecordEvent(std::ios_base::event event, std::ios_base& stream, int tag) {
    auto* events = static_cast<std::vector<int>*>(stream.pword(CallbackSlot()));
    if (events) events->push_back(100 * static_cast<int>(event) + tag);
}

void IosCallbacks() {
    std::vector<int> events;
    events.reserve(8);
    {
        std::ostringstream stream;
        stream.pword(CallbackSlot()) = &events;
        stream.register_callback(RecordEvent, 10);
        stream.register_callback(RecordEvent, 20);
        stream.imbue(std::locale::classic());
        const int imbue = 100 * static_cast<int>(std::ios_base::imbue_event);
        Require(events == std::vector<int>({imbue + 20, imbue + 10}), "ios callback order/arguments differ");
        stream.exceptions(std::ios::failbit);
        bool caught = false;
        try { stream.clear(std::ios::failbit | std::ios::eofbit); }
        catch (const std::ios_base::failure&) { caught = true; }
        Require(caught && stream.fail() && stream.eof(), "clear must store state before raising failure");
        stream.exceptions(std::ios::goodbit);
    }
    const int erase = 100 * static_cast<int>(std::ios_base::erase_event);
    Require(events.size() == 4 && events[2] == erase + 20 && events[3] == erase + 10,
            "destructor did not notify callbacks in reverse registration order");
}

class AbsentFacet final : public std::locale::facet {
public:
    inline static std::locale::id id;
};

void IteratorsAndFacets() {
    std::string bytes{'A', '\0', static_cast<char>(0xff)};
    std::istringstream input(bytes);
    std::istringstream other("Z");
    std::istreambuf_iterator<char> it(input), other_it(other), end;
    Require(it == other_it, "input iterators compare EOF status, not buffer identity");
    Require(*it == 'A' && *it == 'A' && input.rdbuf()->sgetc() == 'A', "iterator peek consumed input");
    Require(*it++ == 'A' && *it == '\0', "iterator increment skipped NUL");
    ++it;
    Require(static_cast<unsigned char>(*it) == 0xff && it != end, "high byte confused with EOF");
    ++it;
    Require(it == end && it != other_it, "iterator failed to reach EOF");

    const auto& classic = std::locale::classic();
    Require(std::has_facet<std::ctype<char>>(classic) && std::has_facet<std::numpunct<char>>(classic) &&
            std::has_facet<std::num_get<char>>(classic) && std::has_facet<std::num_put<char>>(classic),
            "required classic numeric/ctype facet missing");
    Require(!std::has_facet<AbsentFacet>(classic), "test locale unexpectedly contains custom facet");
    bool caught = false;
    try { (void)std::use_facet<AbsentFacet>(classic); }
    catch (const std::bad_cast&) { caught = true; }
    Require(caught, "missing facet did not throw bad_cast");
    const auto& ctype = std::use_facet<std::ctype<char>>(classic);
    Require(ctype.tolower('A') == 'a' && ctype.toupper('z') == 'Z' && std::isdigit('9', classic),
            "classic character classification/case mapping failed");

    std::ostringstream output;
    output.imbue(classic);
    output << std::setfill('0') << std::setw(2) << 3 << '/' << std::setw(2) << 11
           << '/' << std::setw(4) << 2024;
    Require(output.str() == "03/11/2024" && output.width() == 0 && output.fill() == '0',
            "setw must affect only the next field while fill persists");
}

class CountingSync final : public std::streambuf {
public:
    int calls = 0;
private:
    int sync() override { ++calls; return 0; }
};

void UnwindSync() {
    CountingSync buffer;
    std::ostream stream(&buffer);
    stream.setf(std::ios::unitbuf);
    { std::ostream::sentry normal(stream); }
    Require(buffer.calls == 1, "unitbuf sentry did not synchronize normally");
    int exceptions_seen = 0;
    struct Probe {
        std::ostream& stream;
        int& exceptions_seen;
        ~Probe() {
            exceptions_seen = std::uncaught_exceptions();
            std::ostream::sentry unwinding(stream);
        }
    };
    try {
        Probe probe{stream, exceptions_seen};
        throw std::runtime_error("unwind test");
    } catch (const std::runtime_error&) {}
    // VC6's observed uncaught_exception candidate always returns false.
    // Modern sentry suppresses unitbuf synchronization during stack unwinding.
    Require(exceptions_seen > 0 && buffer.calls == 1 && stream.good(),
            "host unitbuf unwinding contract changed");
}
}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "expected one test case name");
        const std::string_view name(argv[1]);
        if (name == "exceptions") Exceptions();
        else if (name == "locale") Locale();
        else if (name == "atexit") ExitRegistration();
        else if (name == "streams") Streams();
        else if (name == "wide_streams") WideStreams();
        else if (name == "flush_failure") FlushFailure();
        else if (name == "filebuf") FilebufIntegration();
        else if (name == "ios_callbacks") IosCallbacks();
        else if (name == "iterators_facets") IteratorsAndFacets();
        else if (name == "unwind_sync") UnwindSync();
        else throw std::runtime_error("unknown case");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
