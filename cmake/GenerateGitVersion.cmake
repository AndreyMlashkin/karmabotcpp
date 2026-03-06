if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE is required")
endif()

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

execute_process(
    COMMAND git -C "${SOURCE_DIR}" rev-parse --short=12 HEAD
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_RESULT EQUAL 0 OR GIT_HASH STREQUAL "")
    set(GIT_HASH "unknown")
endif()

set(CONTENT
"const char* gitVersion()
{
    return \"${GIT_HASH}\";
}
")

if(EXISTS "${OUTPUT_FILE}")
    file(READ "${OUTPUT_FILE}" EXISTING_CONTENT)
    if(EXISTING_CONTENT STREQUAL CONTENT)
        return()
    endif()
endif()

file(WRITE "${OUTPUT_FILE}" "${CONTENT}")
