#!/bin/bash
# Quick syntax check for PlayerBot module files
# Faster than full compilation - useful for rapid iteration

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLAYERBOT_DIR="${SCRIPT_DIR}/src/modules/Playerbot"
BUILD_DIR="${SCRIPT_DIR}/build-playerbot"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
echo_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
echo_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Default: check recently modified files
MODE="${1:-recent}"
FILES_TO_CHECK=()

case "$MODE" in
    "recent")
        echo_info "Checking recently modified PlayerBot files (last 24 hours)..."
        mapfile -t FILES_TO_CHECK < <(find "${PLAYERBOT_DIR}" -name "*.cpp" -mtime -1 -type f)
        ;;
    "all")
        echo_info "Checking all PlayerBot source files..."
        mapfile -t FILES_TO_CHECK < <(find "${PLAYERBOT_DIR}" -name "*.cpp" -type f)
        ;;
    "staged")
        echo_info "Checking staged git files..."
        mapfile -t FILES_TO_CHECK < <(git diff --cached --name-only --diff-filter=ACMR | grep "\.cpp$" || true)
        ;;
    *)
        # Specific file
        if [ -f "$MODE" ]; then
            FILES_TO_CHECK=("$MODE")
            echo_info "Checking specific file: $MODE"
        else
            echo_error "Unknown mode or file not found: $MODE"
            echo_info "Usage: $0 [recent|all|staged|<file.cpp>]"
            exit 1
        fi
        ;;
esac

if [ ${#FILES_TO_CHECK[@]} -eq 0 ]; then
    echo_info "No files to check!"
    exit 0
fi

echo_info "Files to check: ${#FILES_TO_CHECK[@]}"

# Setup include paths from CMake if available
INCLUDE_PATHS=""
if [ -f "${BUILD_DIR}/compile_commands.json" ]; then
    echo_info "Using compile_commands.json for include paths"
    # Extract include paths from compile_commands.json (first entry)
    INCLUDE_PATHS=$(jq -r '.[0].command' "${BUILD_DIR}/compile_commands.json" 2>/dev/null | \
        grep -oP '\-I\S+' | tr '\n' ' ' || echo "")
fi

# Fallback: manual include paths
if [ -z "$INCLUDE_PATHS" ]; then
    echo_info "Using fallback include paths"
    INCLUDE_PATHS="-I${SCRIPT_DIR}/src/server/game \
        -I${SCRIPT_DIR}/src/server/shared \
        -I${SCRIPT_DIR}/src/common \
        -I${SCRIPT_DIR}/src/modules/Playerbot \
        -I${BUILD_DIR} \
        -I/usr/include/mysql"
fi

# Check each file
TOTAL=0
FAILED=0
PASSED=0

for FILE in "${FILES_TO_CHECK[@]}"; do
    TOTAL=$((TOTAL + 1))
    echo ""
    echo_info "[$TOTAL/${#FILES_TO_CHECK[@]}] Checking: $(basename "$FILE")"

    # Run syntax-only compilation
    if g++ -std=c++20 -fsyntax-only \
        -Wall -Wextra -Wno-unused-parameter \
        ${INCLUDE_PATHS} \
        -c "$FILE" 2>&1 | tee /tmp/syntax_check_$$.log; then
        echo_success "  ✓ OK: $(basename "$FILE")"
        PASSED=$((PASSED + 1))
    else
        echo_error "  ✗ FAILED: $(basename "$FILE")"
        FAILED=$((FAILED + 1))
        echo ""
        echo_error "  Errors in $(basename "$FILE"):"
        cat /tmp/syntax_check_$$.log | head -10
    fi
done

# Cleanup
rm -f /tmp/syntax_check_$$.log

echo ""
echo_info "============================================"
echo_info "Syntax Check Summary:"
echo_info "  Total files: $TOTAL"
echo_success "  Passed: $PASSED"
if [ $FAILED -gt 0 ]; then
    echo_error "  Failed: $FAILED"
else
    echo_info "  Failed: $FAILED"
fi
echo_info "============================================"

if [ $FAILED -eq 0 ]; then
    echo_success "All syntax checks passed!"
    exit 0
else
    echo_error "Some files have syntax errors!"
    exit 1
fi
