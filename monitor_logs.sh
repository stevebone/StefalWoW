#!/bin/bash

# Monitor logs for 10 minutes (600 seconds)
# Focus on Quest and Combat issues

LOG_DIR="/m/Wplayerbot/logs"
DURATION=600
OUTPUT_FILE="/c/TrinityBots/TrinityCore/.claude/LOG_MONITORING_RESULTS_$(date +%Y%m%d_%H%M%S).txt"

echo "=== SERVER LOG MONITORING - 10 MINUTE CAPTURE ===" > "$OUTPUT_FILE"
echo "Start Time: $(date)" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Capture initial state
echo "=== INITIAL LOG SIZES ===" >> "$OUTPUT_FILE"
wc -l "$LOG_DIR"/*.log >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Monitor for specific patterns
echo "=== MONITORING FOR ANOMALIES ===" >> "$OUTPUT_FILE"
echo "Duration: $DURATION seconds (10 minutes)" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Start monitoring in background
(
    timeout $DURATION tail -f "$LOG_DIR/Server.log" "$LOG_DIR/Playerbot.log" 2>/dev/null | \
    grep -E "ERROR|WARN|Quest.*fail|Combat.*fail|Attack.*fail|stuck|timeout|crash|exception|spell.*error|resurrect|ghost|Map\.cpp:686|RemoveFromWorld|LogoutPlayer|BotSession.*destruct|forceExit" | \
    grep -v "AuraEffect::HandleProcTriggerSpellAuraProc: Spell 83470" | \
    head -1000
) >> "$OUTPUT_FILE" 2>&1 &

MONITOR_PID=$!

# Wait for monitoring to complete
sleep $DURATION

# Kill monitoring if still running
kill $MONITOR_PID 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== FINAL LOG SIZES ===" >> "$OUTPUT_FILE"
wc -l "$LOG_DIR"/*.log >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "End Time: $(date)" >> "$OUTPUT_FILE"

# Summary analysis
echo "" >> "$OUTPUT_FILE"
echo "=== SUMMARY ANALYSIS ===" >> "$OUTPUT_FILE"

# Count specific issues
echo "Error counts:" >> "$OUTPUT_FILE"
grep -c "ERROR" "$OUTPUT_FILE" >> "$OUTPUT_FILE" 2>/dev/null || echo "0 ERRORs" >> "$OUTPUT_FILE"
grep -c "WARN" "$OUTPUT_FILE" >> "$OUTPUT_FILE" 2>/dev/null || echo "0 WARNs" >> "$OUTPUT_FILE"
grep -c "Quest" "$OUTPUT_FILE" >> "$OUTPUT_FILE" 2>/dev/null || echo "0 Quest issues" >> "$OUTPUT_FILE"
grep -c "Combat" "$OUTPUT_FILE" >> "$OUTPUT_FILE" 2>/dev/null || echo "0 Combat issues" >> "$OUTPUT_FILE"

echo "" >> "$OUTPUT_FILE"
echo "Monitoring complete. Results saved to: $OUTPUT_FILE"
cat "$OUTPUT_FILE"
