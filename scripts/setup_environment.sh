#!/bin/bash
# TrinityCore Playerbot - Environment Setup Script
# For Claude Code Web Interface

set -e  # Exit on error

echo "=== TrinityCore Playerbot Environment Setup ==="
echo ""

# 1. Verify we're in the correct directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "src/modules/Playerbot" ]; then
    echo "ERROR: Not in TrinityCore root directory!"
    exit 1
fi

echo "✓ TrinityCore root directory verified"

# 2. Check git branch
CURRENT_BRANCH=$(git branch --show-current)
if [ "$CURRENT_BRANCH" != "playerbot-dev" ]; then
    echo "WARNING: Current branch is '$CURRENT_BRANCH', expected 'playerbot-dev'"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
else
    echo "✓ On playerbot-dev branch"
fi

# 3. Update submodules
echo ""
echo "Updating submodules (TBB)..."
git submodule update --init --recursive
echo "✓ Submodules updated"

# 4. Verify build directory
if [ ! -d "build" ]; then
    echo ""
    echo "WARNING: Build directory does not exist!"
    echo "You may need to run CMake configuration first:"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo"
    exit 1
fi

echo "✓ Build directory exists"

# 5. Display current status
echo ""
echo "=== Current Status ==="
git log --oneline -1
echo ""
git status --short | head -10
echo ""

# 6. Display project info
if [ -f ".claude/project.json" ]; then
    echo "=== Project Configuration ==="
    cat .claude/project.json
    echo ""
fi

echo "=== Environment Setup Complete ==="
echo ""
echo "Next steps:"
echo "  1. Check current error baseline: cmake --build build --config RelWithDebInfo --target worldserver"
echo "  2. Continue error elimination cycle"
echo ""
