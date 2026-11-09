#!/usr/bin/env bash
# ==============================================================================
# package_submission.sh - Tel Aviv University Moodle Submission Packaging Script
# Course: Operating Systems (0512.4402) - Homework 1
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_HW1="$(cd "${SCRIPT_DIR}/.." && pwd)"
SRC_DIR="${ROOT_HW1}/src"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo -e "${RED}[ERROR] Invalid number of arguments.${NC}"
    echo -e "Usage:   ${YELLOW}$0 <id1> [id2]${NC}"
    echo -e "Example: ${YELLOW}$0 123456789 987654321${NC}"
    exit 1
fi

ID1="$1"
if [ $# -eq 2 ]; then
    ID2="$2"
    SUBMISSION_NAME="hw1_${ID1}_${ID2}"
else
    SUBMISSION_NAME="hw1_${ID1}"
fi

TAR_FILE="${SUBMISSION_NAME}.tgz"
WORK_DIR=$(mktemp -d)

trap 'rm -rf "${WORK_DIR}"' EXIT

echo -e "${BLUE}================================================================${NC}"
echo -e "${BLUE}         TAU OS 0512.4402 - HW1 Submission Packaging Tool       ${NC}"
echo -e "${BLUE}================================================================${NC}"
echo -e "Target Archive: ${YELLOW}${TAR_FILE}${NC}"
echo -e "Subdirectory:   ${YELLOW}${SUBMISSION_NAME}${NC}\n"

# Verify required source files exist
for file in Makefile hw1shell.c hw1shell.h; do
    if [ ! -f "${SRC_DIR}/${file}" ]; then
        echo -e "${RED}[ERROR] Required source file ${SRC_DIR}/${file} not found!${NC}"
        exit 1
    fi
done

# Prepare packaging staging directory
STAGING_DIR="${WORK_DIR}/${SUBMISSION_NAME}"
mkdir -p "${STAGING_DIR}"
cp "${SRC_DIR}/Makefile" "${STAGING_DIR}/"
cp "${SRC_DIR}/hw1shell.c" "${STAGING_DIR}/"
cp "${SRC_DIR}/hw1shell.h" "${STAGING_DIR}/"

# Clean any artifacts if present in staging
(cd "${STAGING_DIR}" && rm -f *.o hw1shell)

# Create compressed tarball from parent directory
echo -e "[*] Creating compressed tar archive: ${TAR_FILE}..."
(cd "${WORK_DIR}" && tar -zcvf "${TAR_FILE}" "${SUBMISSION_NAME}")

# Copy final tarball to script output location
OUTPUT_TAR="${ROOT_HW1}/${TAR_FILE}"
mv "${WORK_DIR}/${TAR_FILE}" "${OUTPUT_TAR}"

echo -e "\n[*] Archive Contents Inspection (tar -tzvf):"
tar -tzvf "${OUTPUT_TAR}"

# Verification Step: Extract into /tmp, compile with make, and make clean
echo -e "\n[*] Simulating TAU Moodle Automated Grading Harness in /tmp..."
VERIFY_DIR=$(mktemp -d "/tmp/tau_verify_hw1_XXXXXX")
tar -zxvf "${OUTPUT_TAR}" -C "${VERIFY_DIR}" > /dev/null

if [ ! -d "${VERIFY_DIR}/${SUBMISSION_NAME}" ]; then
    echo -e "${RED}[FAIL] Archive root directory mismatch!${NC}"
    rm -rf "${VERIFY_DIR}"
    exit 1
fi

echo -e "[*] Testing compilation inside extracted folder..."
(cd "${VERIFY_DIR}/${SUBMISSION_NAME}" && make clean > /dev/null 2>&1 || true)
(cd "${VERIFY_DIR}/${SUBMISSION_NAME}" && make)

if [ ! -x "${VERIFY_DIR}/${SUBMISSION_NAME}/hw1shell" ]; then
    echo -e "${RED}[FAIL] 'make' did not produce executable 'hw1shell'!${NC}"
    rm -rf "${VERIFY_DIR}"
    exit 1
fi
echo -e "${GREEN}[PASS] Executable 'hw1shell' compiled cleanly.${NC}"

echo -e "[*] Testing 'make clean'..."
(cd "${VERIFY_DIR}/${SUBMISSION_NAME}" && make clean)

if [ -f "${VERIFY_DIR}/${SUBMISSION_NAME}/hw1shell" ] || ls "${VERIFY_DIR}/${SUBMISSION_NAME}"/*.o > /dev/null 2>&1; then
    echo -e "${RED}[FAIL] 'make clean' did not remove build artifacts!${NC}"
    rm -rf "${VERIFY_DIR}"
    exit 1
fi
echo -e "${GREEN}[PASS] 'make clean' removed all build artifacts.${NC}"

rm -rf "${VERIFY_DIR}"

echo -e "\n${BLUE}================================================================${NC}"
echo -e "${GREEN}[SUCCESS] Package ${TAR_FILE} is 100% compliant with TAU Moodle rules!${NC}"
echo -e "Saved to: ${YELLOW}${OUTPUT_TAR}${NC}"
echo -e "Size:     $(du -h "${OUTPUT_TAR}" | cut -f1)"
echo -e "${BLUE}================================================================${NC}"
