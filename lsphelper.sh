#!/bin/sh

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Prerequisite checks ---
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is not installed or not in PATH. Please install jq." >&2
    exit 1
fi

if ! command -v g++ >/dev/null 2>&1; then
    echo "Error: g++ is not installed or not in PATH." >&2
    exit 1
fi

# --- Configuration ---
COMPILE_COMMANDS_FILE="build/compile_commands.json"
OUTPUT_FILE="compile_commands.json" # Output to current directory as per original script's cp command

# Check if compile_commands.json exists
if [ ! -f "$COMPILE_COMMANDS_FILE" ]; then
    echo "Error: $COMPILE_COMMANDS_FILE not found!" >&2
    exit 1
fi

# --- Get system includes ---
# The awk script extracts lines between "#include <...>" and "End of search list",
# trims them, and prepends "-isystem " to each non-empty line,
# then joins them into a single space-separated string.
SYSTEM_INCLUDES=$(echo | g++ -E -xc++ -v - 2>&1 | \
    awk '
        BEGIN {
            p=0; # p is true when we are in the desired section
            result="";
        }
        /#include <\.\.\.>/ {
            p=1;
            next; # Skip the "#include <...>" line itself
        }
        /End of search list/ {
            p=0;
        }
        p {
            # Trim leading and trailing whitespace
            gsub(/^ +/, "", $0);
            gsub(/ +$/, "", $0);
            if ($0 != "") { # If line is not empty after trim
                if (result != "") {
                    result = result " ";
                }
                result = result "-isystem " $0;
            }
        }
        END {
            if (result != "") {
                print result;
            }
        }
    ')

if [ -z "$SYSTEM_INCLUDES" ]; then
    echo "Warning: Could not determine system includes, or none found. Proceeding without adding them." >&2
fi

# Create a temporary file for building the new JSON
# Using a more robust mktemp usage if available, or a simpler one.
# TMP_FILE=$(mktemp /tmp/compilepatch.XXXXXX)
TMP_FILE=$(mktemp) # Simpler, works on most systems with mktemp

# Trap to clean up TMP_FILE on exit or interrupt
trap 'rm -f "$TMP_FILE" "$TMP_FILE.sedout"' EXIT HUP INT QUIT TERM

# --- Process compile_commands.json ---
echo "[" > "$TMP_FILE"

# Read the compile_commands.json line by line (each line is a JSON object from 'jq -c')
jq -c '.[]' "$COMPILE_COMMANDS_FILE" | while IFS= read -r line; do
    NEW_LINE="$line" # Default to original line if no changes

    # Check if the entry has a 'command' field (string)
    if echo "$line" | jq -e 'has("command")' >/dev/null; then
        FILE=$(echo "$line" | jq -r '.file')
        COMMAND=$(echo "$line" | jq -r '.command')

        # Escape FILE for sed pattern (characters: \, /, ., *, ^, $, [, ])
        ESCAPED_FILE_PATTERN=$(printf '%s\n' "$FILE" | sed 's:[][\\/.^$*]:\\&:g')

        INSERT_PREFIX=""
        if [ -n "$SYSTEM_INCLUDES" ]; then
            # Escape SYSTEM_INCLUDES for sed replacement string (characters: &, \, /)
            ESCAPED_SYSTEM_INCLUDES=$(printf '%s\n' "$SYSTEM_INCLUDES" | sed 's:[&/\\]:\\&:g')
            INSERT_PREFIX="$ESCAPED_SYSTEM_INCLUDES "
        fi

        # Insert system includes before the file. Replaces first occurrence.
        # Using ~ as sed delimiter as paths might contain /
        # This assumes $FILE appears as a distinct word in the command.
        NEW_COMMAND=$(echo "$COMMAND" | sed "s~$ESCAPED_FILE_PATTERN~$INSERT_PREFIX$ESCAPED_FILE_PATTERN~")
        NEW_LINE=$(echo "$line" | jq --arg cmd "$NEW_COMMAND" '.command = $cmd')

    # Check if the entry has an 'arguments' field (array)
    elif echo "$line" | jq -e 'has("arguments")' >/dev/null; then
        FILE=$(echo "$line" | jq -r '.file')
        # ARGS_JSON contains the JSON array string of arguments
        ARGS_JSON=$(echo "$line" | jq '.arguments')

        # Find the index of the file in the arguments array
        # jq 'map(. == "value") | index(true)' finds the index of "value"
        FILE_INDEX=$(echo "$ARGS_JSON" | jq -r --arg f "$FILE" 'map(. == $f) | index(true)')

        if [ "$FILE_INDEX" != "null" ] && [ -n "$SYSTEM_INCLUDES" ]; then
            # Convert space-separated SYSTEM_INCLUDES into a comma-separated list of JSON strings
            # e.g., "-isystem /path1 -isystem /path2" becomes "\"-isystem\",\"/path1\",\"-isystem\",\"/path2\""
            JQ_SYSTEM_INCLUDE_ARGS_STR=$(echo $SYSTEM_INCLUDES | awk '
                BEGIN { ORS=""; first=1; }
                {
                    for(i=1; i<=NF; i++) {
                        if (!first) { print ","; }
                        printf "\"%s\"", $i;
                        first=0;
                    }
                }
            ')

            if [ -n "$JQ_SYSTEM_INCLUDE_ARGS_STR" ]; then
                 # Inject the system include arguments before the file
                NEW_ARGS=$(echo "$ARGS_JSON" | jq ".[0:$FILE_INDEX] + [$JQ_SYSTEM_INCLUDE_ARGS_STR] + .[$FILE_INDEX:]")
                NEW_LINE=$(echo "$line" | jq --argjson args "$NEW_ARGS" '.arguments = $args')
            fi
        # else: FILE_INDEX is null (file not found in args) or SYSTEM_INCLUDES is empty, so no change to args needed.
        # NEW_LINE remains as original $line.
        fi
    fi

    # Append the (potentially modified) line to temporary file with a comma
    echo "$NEW_LINE," >> "$TMP_FILE"
done

# Remove trailing comma from the last line, if any lines were processed
# Check if TMP_FILE has more than just "["
if [ $(wc -l < "$TMP_FILE") -gt 1 ]; then
    sed '$ s/,$//' "$TMP_FILE" > "$TMP_FILE.sedout" && mv "$TMP_FILE.sedout" "$TMP_FILE"
fi

# Close the JSON array
echo "]" >> "$TMP_FILE"

# Replace original target file (or create it in current dir)
cp "$TMP_FILE" "$OUTPUT_FILE"
# rm is handled by trap

echo "$OUTPUT_FILE has been patched with system includes"