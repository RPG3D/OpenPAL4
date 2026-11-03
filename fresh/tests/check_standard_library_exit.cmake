execute_process(COMMAND "${TEST_EXECUTABLE}" atexit
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
string(REPLACE "\r\n" "\n" output "${output}")
if(NOT result STREQUAL "0" OR NOT error STREQUAL "" OR
   NOT output STREQUAL "registered\nexit:second\nexit:first\n")
    message(FATAL_ERROR "atexit process failed: result=${result}, stdout=[${output}], stderr=[${error}]")
endif()
