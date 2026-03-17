#!/bin/bash
# Script to fix ALL Playerbot sConfigMgr usages to sPlayerbotConfig

echo "=== Fixing ALL Playerbot Config Usages ==="

# Find all .cpp files in Playerbot module
FILES=$(find src/modules/Playerbot -name "*.cpp" -type f)

for file in $FILES; do
    # Check if file contains sConfigMgr
    if grep -q "sConfigMgr" "$file"; then
        echo "Fixing: $file"

        # Replace include directive
        sed -i 's|#include "Config\.h"|#include "Config/PlayerbotConfig.h"|g' "$file"

        # Replace sConfigMgr calls with sPlayerbotConfig
        sed -i 's/sConfigMgr->GetIntDefault/sPlayerbotConfig->GetInt/g' "$file"
        sed -i 's/sConfigMgr->GetBoolDefault/sPlayerbotConfig->GetBool/g' "$file"
        sed -i 's/sConfigMgr->GetFloatDefault/sPlayerbotConfig->GetFloat/g' "$file"
        sed -i 's/sConfigMgr->GetStringDefault/sPlayerbotConfig->GetString/g' "$file"

        # Count remaining sConfigMgr (should be 0)
        REMAINING=$(grep -c "sConfigMgr" "$file" || echo "0")
        if [ "$REMAINING" -eq "0" ]; then
            echo "  ✅ Fixed (0 remaining)"
        else
            echo "  ⚠️  Still has $REMAINING sConfigMgr calls"
        fi
    fi
done

echo ""
echo "=== Summary ==="
echo "Total sConfigMgr calls remaining:"
grep -r "sConfigMgr" src/modules/Playerbot/ --include="*.cpp" 2>/dev/null | wc -l
