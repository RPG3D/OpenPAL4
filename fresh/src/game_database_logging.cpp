#include "pal4/game_database_logging.h"

namespace pal4 {
namespace {

constexpr char kSourceFile[] =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game"
    "\\GameDB\\palGameDB.cpp";
constexpr char kDatabaseFormat[] =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  dbLog:\t %s\n\n\n";
constexpr char kAssertionFormat[] =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  AssertLog:\t %s\n\n\n";

}  // namespace

std::uint32_t GameDatabaseLogging::Database(
    const std::uint32_t message, IGameDatabaseLoggingRuntime& runtime) {
    return runtime.Append(runtime.Prepare(kDatabaseFormat, kSourceFile,
                                          154u, message));
}

std::uint32_t GameDatabaseLogging::Assertion(
    const std::uint32_t message, IGameDatabaseLoggingRuntime& runtime) {
    return runtime.Append(runtime.Prepare(kAssertionFormat, kSourceFile,
                                          161u, message));
}

}  // namespace pal4
