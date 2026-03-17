#!/bin/bash
# Iterative error fixing helper for PlayerBot module
# Analyzes compilation errors and suggests fixes

set -e

BUILD_DIR="/home/user/TrinityCore/build-playerbot"
BUILD_LOG="${BUILD_DIR}/build.log"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
echo_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
echo_error() { echo -e "${RED}[ERROR]${NC} $1"; }
echo_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
echo_fix() { echo -e "${CYAN}[FIX]${NC} $1"; }

if [ ! -f "$BUILD_LOG" ]; then
    echo_error "Build log not found: $BUILD_LOG"
    echo_info "Run ./build-playerbot.sh first!"
    exit 1
fi

echo_info "============================================"
echo_info "PlayerBot Compilation Error Analyzer"
echo_info "============================================"
echo ""

# Extract unique error types
echo_info "Analyzing compilation errors..."
echo ""

# Count different error types
UNDEFINED_REF=$(grep -c "undefined reference to" "$BUILD_LOG" 2>/dev/null || echo 0)
NO_MEMBER=$(grep -c "has no member named" "$BUILD_LOG" 2>/dev/null || echo 0)
NOT_DECLARED=$(grep -c "was not declared" "$BUILD_LOG" 2>/dev/null || echo 0)
INCLUDE_ERROR=$(grep -c "No such file or directory" "$BUILD_LOG" 2>/dev/null || echo 0)
TYPE_ERROR=$(grep -c "no type named" "$BUILD_LOG" 2>/dev/null || echo 0)
AMBIGUOUS=$(grep -c "ambiguous" "$BUILD_LOG" 2>/dev/null || echo 0)

echo_info "Error Summary:"
echo_info "  Undefined references: $UNDEFINED_REF"
echo_info "  Missing members: $NO_MEMBER"
echo_info "  Undeclared identifiers: $NOT_DECLARED"
echo_info "  Missing includes: $INCLUDE_ERROR"
echo_info "  Type errors: $TYPE_ERROR"
echo_info "  Ambiguous references: $AMBIGUOUS"
echo ""

# ============================================================================
# Error Analysis & Fix Suggestions
# ============================================================================

if [ $INCLUDE_ERROR -gt 0 ]; then
    echo_error "════════════════════════════════════════"
    echo_error "MISSING INCLUDE FILES ($INCLUDE_ERROR errors)"
    echo_error "════════════════════════════════════════"
    echo ""
    grep "No such file or directory" "$BUILD_LOG" | head -10 | while read -r line; do
        FILE=$(echo "$line" | grep -oP '[\w/]+\.h' || echo "unknown")
        echo_error "  Missing: $FILE"
        echo_fix "  Add: #include \"$FILE\""
    done
    echo ""
fi

if [ $NOT_DECLARED -gt 0 ]; then
    echo_error "════════════════════════════════════════"
    echo_error "UNDECLARED IDENTIFIERS ($NOT_DECLARED errors)"
    echo_error "════════════════════════════════════════"
    echo ""
    grep "was not declared" "$BUILD_LOG" | head -10 | while read -r line; do
        IDENTIFIER=$(echo "$line" | grep -oP "'\\K[^']+" | head -1 || echo "unknown")
        echo_error "  Undeclared: $IDENTIFIER"
        case "$IDENTIFIER" in
            *Coordinator*)
                echo_fix "  Add: #include \"../Advanced/GroupCoordinator.h\""
                echo_fix "  Or: #include \"../Advanced/TacticalCoordinator.h\""
                ;;
            GetBotAI|GetGameSystems)
                echo_fix "  Add: #include \"Core/PlayerBotHelpers.h\""
                ;;
            urand)
                echo_fix "  Add: #include \"Random.h\""
                ;;
            ObjectAccessor*)
                echo_fix "  Add: #include \"ObjectAccessor.h\""
                ;;
            *)
                echo_fix "  Check if forward declaration or include is needed"
                ;;
        esac
    done
    echo ""
fi

if [ $NO_MEMBER -gt 0 ]; then
    echo_error "════════════════════════════════════════"
    echo_error "MISSING MEMBERS ($NO_MEMBER errors)"
    echo_error "════════════════════════════════════════"
    echo ""
    grep "has no member named" "$BUILD_LOG" | head -10 | while read -r line; do
        CLASS=$(echo "$line" | grep -oP "'\\K[^']+" | head -1 || echo "unknown")
        MEMBER=$(echo "$line" | grep -oP "'\\K[^']+" | tail -1 || echo "unknown")
        echo_error "  Class '$CLASS' missing member '$MEMBER'"
        echo_fix "  Check if method signature or class name is correct"
    done
    echo ""
fi

if [ $TYPE_ERROR -gt 0 ]; then
    echo_error "════════════════════════════════════════"
    echo_error "TYPE ERRORS ($TYPE_ERROR errors)"
    echo_error "════════════════════════════════════════"
    echo ""
    grep "no type named" "$BUILD_LOG" | head -10 | while read -r line; do
        TYPE=$(echo "$line" | grep -oP "'\\K[^']+" | head -1 || echo "unknown")
        echo_error "  Missing type: $TYPE"
        echo_fix "  Add forward declaration or include for $TYPE"
    done
    echo ""
fi

if [ $AMBIGUOUS -gt 0 ]; then
    echo_warning "════════════════════════════════════════"
    echo_warning "AMBIGUOUS REFERENCES ($AMBIGUOUS errors)"
    echo_warning "════════════════════════════════════════"
    echo ""
    grep "ambiguous" "$BUILD_LOG" | head -5 | while read -r line; do
        echo_warning "  $line"
    done
    echo_fix "  Use fully qualified names (e.g., Playerbot::ClassName)"
    echo ""
fi

if [ $UNDEFINED_REF -gt 0 ]; then
    echo_error "════════════════════════════════════════"
    echo_error "UNDEFINED REFERENCES ($UNDEFINED_REF errors)"
    echo_error "════════════════════════════════════════"
    echo ""
    echo_warning "These are linker errors - check for:"
    echo_fix "  1. Missing .cpp file in CMakeLists.txt"
    echo_fix "  2. Method declared but not implemented"
    echo_fix "  3. Library not linked"
    echo ""
    grep "undefined reference to" "$BUILD_LOG" | head -5 | while read -r line; do
        SYMBOL=$(echo "$line" | grep -oP "'\\K[^']+" || echo "unknown")
        echo_error "  Missing: $SYMBOL"
    done
    echo ""
fi

# ============================================================================
# File-Specific Error Report
# ============================================================================

echo_info "════════════════════════════════════════"
echo_info "FILES WITH MOST ERRORS"
echo_info "════════════════════════════════════════"
echo ""

# Extract files with errors and count
grep "error:" "$BUILD_LOG" | \
    grep -oP '/home/user/TrinityCore/[^:]+' | \
    sort | uniq -c | sort -rn | head -10 | \
    while read -r count file; do
        BASENAME=$(basename "$file")
        echo_error "  $count errors: $BASENAME"
        echo_info "    Full path: $file"
    done

echo ""
echo_info "════════════════════════════════════════"
echo_info "NEXT STEPS"
echo_info "════════════════════════════════════════"
echo ""
echo_fix "1. Fix the most common error type first"
echo_fix "2. Re-run: ./build-playerbot.sh"
echo_fix "3. Repeat until all errors are resolved"
echo ""
echo_info "To view full error context:"
echo_info "  cat $BUILD_LOG | grep -A 5 'error:' | less"
echo ""
echo_info "To fix a specific file:"
echo_info "  Use Claude Code to edit the file based on error messages"
