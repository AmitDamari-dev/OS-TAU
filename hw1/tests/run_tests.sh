#!/usr/bin/env bash
# ==============================================================================
# run_tests.sh - Automated Black-Box & Valgrind Verification Suite for hw1shell
# Tel Aviv University - Operating Systems (0512.4402)
# ==============================================================================

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(cd "${SCRIPT_DIR}/../src" && pwd)"
SHELL_BIN="${SRC_DIR}/hw1shell"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PASSED_COUNT=0
TOTAL_COUNT=0

function assert_contains() {
    local label="$1"
    local output="$2"
    local pattern="$3"
    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    if echo "$output" | grep -F -q "$pattern" || echo "$output" | grep -E -q "$pattern"; then
        echo -e "  [${GREEN}PASS${NC}] ${label}"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "  [${RED}FAIL${NC}] ${label}"
        echo -e "    Expected pattern: ${YELLOW}${pattern}${NC}"
        echo -e "    Actual output:\n${output}"
    fi
}

function assert_not_contains() {
    local label="$1"
    local output="$2"
    local pattern="$3"
    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    if ! echo "$output" | grep -E -q "$pattern"; then
        echo -e "  [${GREEN}PASS${NC}] ${label}"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "  [${RED}FAIL${NC}] ${label}"
        echo -e "    Pattern should NOT be present: ${YELLOW}${pattern}${NC}"
        echo -e "    Actual output:\n${output}"
    fi
}

echo -e "${BLUE}================================================================${NC}"
echo -e "${BLUE}          hw1shell Automated Test Suite (All 14 Guidelines)     ${NC}"
echo -e "${BLUE}================================================================${NC}"

# Ensure binary is built
echo -e "\n[*] Building hw1shell..."
(cd "${SRC_DIR}" && make clean && make)
if [ ! -x "${SHELL_BIN}" ]; then
    echo -e "${RED}[ERROR] hw1shell executable not found or failed to compile!${NC}"
    exit 1
fi
echo -e "${GREEN}[OK] Binary built successfully.${NC}\n"

# ------------------------------------------------------------------------------
# Test 1: Empty Lines & Whitespace Handling (Guideline 1 & 10)
# ------------------------------------------------------------------------------
echo -e "${YELLOW}Test 1: Empty Lines & Whitespace (Guideline 1 & 10)${NC}"
OUT=$(printf "\n    \t    \n\nexit\n" | "${SHELL_BIN}")
assert_contains "Prompt displayed" "$OUT" "hw1shell\$"
assert_not_contains "No invalid command error on empty lines" "$OUT" "hw1shell: invalid command"
assert_not_contains "No syscall error on empty lines" "$OUT" "errno is"

# ------------------------------------------------------------------------------
# Test 2: 'cd' Command Variations & Error Handling (Guideline 3 & 13)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 2: 'cd' Internal Command Variants (Guideline 3 & 13)${NC}"
# 0 args
OUT=$(printf "cd\nexit\n" | "${SHELL_BIN}")
assert_contains "cd with 0 args triggers invalid command" "$OUT" "hw1shell: invalid command"

# 2+ args
OUT=$(printf "cd foo bar\nexit\n" | "${SHELL_BIN}")
assert_contains "cd with 2 args triggers invalid command" "$OUT" "hw1shell: invalid command"

# Non-existent dir (ENOENT is errno 2 on Linux)
OUT=$(printf "cd /non_existent_dir_tau_12345\nexit\n" | "${SHELL_BIN}")
assert_contains "cd non-existent dir prints invalid command" "$OUT" "hw1shell: invalid command"
assert_contains "cd non-existent dir prints chdir failed with errno 2" "$OUT" "hw1shell: chdir failed, errno is 2"

# Valid cd and cd .. verified via /bin/pwd
OUT=$(printf "cd /tmp\n/bin/pwd\ncd ..\n/bin/pwd\nexit\n" | "${SHELL_BIN}")
assert_contains "cd to /tmp changes CWD" "$OUT" "/tmp"
assert_contains "cd .. changes CWD to root" "$OUT" "hw1shell$ /"

# ------------------------------------------------------------------------------
# Test 3: External Foreground Commands (Guideline 7 & 8)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 3: External Foreground Commands (Guideline 7 & 8)${NC}"
OUT=$(printf "echo TAU_OS_TEST_123\nexit\n" | "${SHELL_BIN}")
assert_contains "Foreground echo works" "$OUT" "TAU_OS_TEST_123"

# ------------------------------------------------------------------------------
# Test 4: Invalid External Commands & execvp Error (Guideline 11 & 13)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 4: Invalid External Commands (Guideline 11 & 13)${NC}"
OUT=$(printf "non_existent_binary_xyz_987\nexit\n" | "${SHELL_BIN}")
assert_contains "Invalid command message printed" "$OUT" "hw1shell: invalid command"
assert_contains "execvp failed message with errno 2 (ENOENT)" "$OUT" "hw1shell: execvp failed, errno is 2"

# ------------------------------------------------------------------------------
# Test 5: Background Commands & 'jobs' Command (Guideline 4, 5, 9, 12)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 5: Background Jobs (&) and 'jobs' Listing (Guidelines 4, 5, 9, 12)${NC}"
FIFO_IN=$(mktemp -u)
mkfifo "$FIFO_IN"

"${SHELL_BIN}" < "$FIFO_IN" > "${SCRIPT_DIR}/test5_out.tmp" 2>&1 &
SHELL_PID=$!

exec 3> "$FIFO_IN"
echo "sleep 1 &" >&3
sleep 0.1
echo "jobs" >&3
sleep 1.2
echo "" >&3 # Trigger end-of-iteration reaping
sleep 0.1
echo "exit" >&3
exec 3>&-

wait "$SHELL_PID" 2>/dev/null || true
rm -f "$FIFO_IN"

OUT=$(cat "${SCRIPT_DIR}/test5_out.tmp")
rm -f "${SCRIPT_DIR}/test5_out.tmp"

assert_contains "Background job started message" "$OUT" "hw1shell: pid [0-9]+ started"
assert_contains "jobs output format (<pid>\\tsleep 1 &)" "$OUT" "[0-9]+[[:space:]]+sleep 1 &"
assert_contains "Background job finished message reaped" "$OUT" "hw1shell: pid [0-9]+ finished"

# ------------------------------------------------------------------------------
# Test 6: Attached '&' Background Syntax (e.g. 'sleep 1&') (Guideline 5)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 6: Attached '&' Syntax ('sleep 1&') (Guideline 5)${NC}"
FIFO_IN=$(mktemp -u)
mkfifo "$FIFO_IN"
"${SHELL_BIN}" < "$FIFO_IN" > "${SCRIPT_DIR}/test6_out.tmp" 2>&1 &
SHELL_PID=$!

exec 3> "$FIFO_IN"
echo "sleep 1&" >&3
sleep 1.2
echo "" >&3
sleep 0.1
echo "exit" >&3
exec 3>&-

wait "$SHELL_PID" 2>/dev/null || true
rm -f "$FIFO_IN"

OUT=$(cat "${SCRIPT_DIR}/test6_out.tmp")
rm -f "${SCRIPT_DIR}/test6_out.tmp"

assert_contains "Attached '&' job started" "$OUT" "hw1shell: pid [0-9]+ started"
assert_contains "Attached '&' job finished" "$OUT" "hw1shell: pid [0-9]+ finished"

# ------------------------------------------------------------------------------
# Test 7: Max 4 Concurrent Background Commands (Guideline 6)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 7: 4 Background Commands Limit (Guideline 6)${NC}"
FIFO_IN=$(mktemp -u)
mkfifo "$FIFO_IN"
"${SHELL_BIN}" < "$FIFO_IN" > "${SCRIPT_DIR}/test7_out.tmp" 2>&1 &
SHELL_PID=$!

exec 3> "$FIFO_IN"
echo "sleep 2 &" >&3
echo "sleep 2 &" >&3
echo "sleep 2 &" >&3
echo "sleep 2 &" >&3
sleep 0.1
echo "sleep 2 &" >&3 # 5th background job: must fail
sleep 2.2
echo "" >&3 # Reap completed jobs
sleep 0.1
echo "exit" >&3
exec 3>&-

wait "$SHELL_PID" 2>/dev/null || true
rm -f "$FIFO_IN"

OUT=$(cat "${SCRIPT_DIR}/test7_out.tmp")
rm -f "${SCRIPT_DIR}/test7_out.tmp"

assert_contains "5th background job rejected with too many commands" "$OUT" "hw1shell: too many background commands running"

STARTED_COUNT=$(echo "$OUT" | grep -c "hw1shell: pid [0-9]\+ started" || true)
FINISHED_COUNT=$(echo "$OUT" | grep -c "hw1shell: pid [0-9]\+ finished" || true)

TOTAL_COUNT=$((TOTAL_COUNT + 1))
if [ "$STARTED_COUNT" -eq 4 ]; then
    echo -e "  [${GREEN}PASS${NC}] Exactly 4 background jobs started"
    PASSED_COUNT=$((PASSED_COUNT + 1))
else
    echo -e "  [${RED}FAIL${NC}] Expected 4 started background jobs, got ${STARTED_COUNT}"
fi

TOTAL_COUNT=$((TOTAL_COUNT + 1))
if [ "$FINISHED_COUNT" -eq 4 ]; then
    echo -e "  [${GREEN}PASS${NC}] Exactly 4 background jobs reaped and finished"
    PASSED_COUNT=$((PASSED_COUNT + 1))
else
    echo -e "  [${RED}FAIL${NC}] Expected 4 finished background jobs, got ${FINISHED_COUNT}"
fi

# ------------------------------------------------------------------------------
# Test 8: 'exit' Blocks and Reaps Active Background Jobs (Guideline 2)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 8: 'exit' Waits For Active Background Jobs (Guideline 2)${NC}"
START_TIME=$(date +%s)
printf "sleep 2 &\nexit\n" | "${SHELL_BIN}" > /dev/null 2>&1
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

TOTAL_COUNT=$((TOTAL_COUNT + 1))
if [ "$ELAPSED" -ge 2 ]; then
    echo -e "  [${GREEN}PASS${NC}] 'exit' waited for background job (Elapsed: ${ELAPSED}s >= 2s)"
    PASSED_COUNT=$((PASSED_COUNT + 1))
else
    echo -e "  [${RED}FAIL${NC}] 'exit' did not wait for background job (Elapsed: ${ELAPSED}s < 2s)"
fi

# ------------------------------------------------------------------------------
# Test 9: Valgrind Memory Leak and Error Check
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}Test 9: Valgrind Leak & Memory Error Audit${NC}"
if command -v valgrind > /dev/null 2>&1; then
    VALGRIND_LOG="${SCRIPT_DIR}/valgrind.log"
    printf "echo hello\ncd /tmp\n/bin/pwd\nnonexistent_cmd\nexit\n" | \
        valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
                 --error-exitcode=42 --log-file="${VALGRIND_LOG}" "${SHELL_BIN}" > /dev/null 2>&1
    VAL_EXIT=$?
    
    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    if [ "$VAL_EXIT" -eq 0 ] && grep -q "ERROR SUMMARY: 0 errors from 0 contexts" "${VALGRIND_LOG}"; then
        echo -e "  [${GREEN}PASS${NC}] Valgrind reported 0 errors and 0 leaks"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "  [${RED}FAIL${NC}] Valgrind detected memory errors or leaks (exit code ${VAL_EXIT})"
        cat "${VALGRIND_LOG}"
    fi
    rm -f "${VALGRIND_LOG}"
else
    echo -e "  [${YELLOW}SKIP${NC}] Valgrind not found on PATH."
fi

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
echo -e "\n${BLUE}================================================================${NC}"
if [ "$PASSED_COUNT" -eq "$TOTAL_COUNT" ]; then
    echo -e "${GREEN}ALL TESTS PASSED! (${PASSED_COUNT}/${TOTAL_COUNT})${NC}"
    echo -e "${BLUE}================================================================${NC}"
    exit 0
else
    echo -e "${RED}TESTS FAILED: ${PASSED_COUNT}/${TOTAL_COUNT} passed.${NC}"
    echo -e "${BLUE}================================================================${NC}"
    exit 1
fi
