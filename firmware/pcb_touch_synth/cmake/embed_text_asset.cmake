# embed_text_asset.cmake
#
# Converts a plain-text asset file into a C source + header pair so it can be
# compiled into the firmware binary and accessed at runtime as a C string.
#
# Called by CMake add_custom_command with:
#   -DINPUT=<path to source text file>
#   -DOUTPUT_C=<path for generated .c file>
#   -DOUTPUT_H=<path for generated .h file>
#   -DVAR_NAME=<C identifier for the exported symbol>
#   -P embed_text_asset.cmake
#
# The generated .h declares:
#   extern const char <VAR_NAME>[];        // null-terminated string
#   extern const unsigned int <VAR_NAME>_len; // length in bytes (excl. null)
#
# Cross-platform: uses only CMake built-ins, no shell commands or Python.

file(READ "${INPUT}" CONTENT)

# Escape backslashes first (must be first to avoid double-escaping).
string(REPLACE "\\" "\\\\" CONTENT "${CONTENT}")

# Escape double-quotes.
string(REPLACE "\"" "\\\"" CONTENT "${CONTENT}")

# Replace newlines with \n" (close quote) + newline + "  " (open next line).
# This produces a readable multi-line string literal in the generated C source.
string(REPLACE "\n" "\\n\"\n    \"" CONTENT "${CONTENT}")

get_filename_component(HEADER_NAME "${OUTPUT_H}" NAME)

file(WRITE "${OUTPUT_H}"
"#pragma once\n"
"/* Auto-generated from ${INPUT} — do not edit.\n"
"   Update the source file in assets/ and recompile to change the content. */\n"
"extern const char ${VAR_NAME}[];\n"
"extern const unsigned int ${VAR_NAME}_len;\n"
)

file(WRITE "${OUTPUT_C}"
"/* Auto-generated from ${INPUT} — do not edit. */\n"
"#include \"${HEADER_NAME}\"\n"
"\n"
"const char ${VAR_NAME}[] =\n"
"    \"${CONTENT}\";\n"
"\n"
"const unsigned int ${VAR_NAME}_len =\n"
"    sizeof(${VAR_NAME}) - 1u; /* exclude null terminator */\n"
)
