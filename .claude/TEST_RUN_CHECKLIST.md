# Overnight Mode Test Run - Checklist

## 📋 Pre-Test Checklist

### Step 1: Clean Up Working Directory
```bash
cd /c/TrinityBots/TrinityCore

# Check current status
git status

# Decision point: What to do with uncommitted files?

# Option A: Commit everything to playerbot-dev
git add -A
git commit -m "chore: pre-overnight test - documentation and scripts"
git push

# Option B: Stash uncommitted changes
git stash save "pre-overnight-test"

# Option C: Discard unwanted changes
git checkout src/modules/Playerbot/AI/BotAI.cpp  # if you don't want this change
git clean -fd  # removes untracked files (BE CAREFUL!)
```

### Step 2: Verify Build is Clean
```bash
cd build
cmake --build . --target worldserver --config RelWithDebInfo -j 8

# Should compile successfully
```

### Step 3: Check Crash Queue Status
```bash
# Check if there are any pending crash requests
ls .claude/crash_analysis_queue/requests/

# If there are old completed requests, archive them
mkdir -p .claude/crash_analysis_queue/archive
mv .claude/crash_analysis_queue/requests/request_d8aef0f7.json .claude/crash_analysis_queue/archive/
```

### Step 4: Verify Deploy Directory
```bash
# Check if M:/Wplayerbot exists
ls M:/Wplayerbot/

# Should see:
# worldserver.exe
# worldserver.pdb
# (and other server files)
```

---

## 🚀 Start Test Run

### Method 1: Using Batch File (Recommended)
```bash
cd C:\TrinityBots\TrinityCore
.\.claude\scripts\start_overnight_mode.bat
```

### Method 2: Direct Python
```bash
cd C:\TrinityBots\TrinityCore
python .claude\scripts\overnight_autonomous_mode.py
```

---

## 👀 What to Watch For

### At Startup (First 30 seconds)
```
✅ Should see:
───────────────────────────────────────────────────────
🌙 OVERNIGHT AUTONOMOUS MODE v1.0
───────────────────────────────────────────────────────
Current branch: playerbot-dev
✅ playerbot-dev is clean (or committed and pushed)
Creating overnight-20251031 from playerbot-dev...
Pushing overnight-20251031 to origin...

✅ OVERNIGHT BRANCH SETUP COMPLETE
   Branch: overnight-20251031
```

❌ If you see errors:
- Check git status is clean
- Check no "NUL" file exists
- Check git push works

### During Test Run (Every 30 seconds)
```
💓 Heartbeat: Iteration X
   Running: HH:MM:SS
   Fixes Applied: 0 (will increase if crashes occur)
   Fixes Failed: 0
```

### If a Crash is Detected
```
🔔 Found 1 pending request(s)
✅ Created auto-process marker for abc123

... (wait ~10 minutes for analysis)

🌙 OVERNIGHT FIX DEPLOYMENT (NO HUMAN APPROVAL)
───────────────────────────────────────────────────────
📝 Step 1/5: Applying fix to source files...
✅ Applied fix to 1 file(s)

🔨 Step 2/5: Compiling worldserver...
  Running: cmake --build...
✅ Compilation SUCCESSFUL

💾 Step 3/5: Creating git commit...
✅ Git commit created

📤 Step 4/5: Pushing to overnight branch...
✅ Pushed to overnight branch

🚀 Step 5/5: Copying worldserver + pdb to Wplayerbot...
✅ Deployed to M:/Wplayerbot

✅ OVERNIGHT DEPLOYMENT COMPLETE
```

---

## ⏱️ Test Duration

**Planned:** 60 minutes

### What Should Happen
- If no crashes: Runs peacefully, showing heartbeats every 5 minutes
- If crashes occur: Processes them automatically, deploys fixes

### To Stop After 60 Minutes
Press `Ctrl+C` in the terminal

You should see:
```
🛑 OVERNIGHT MODE STOPPED BY USER
═══════════════════════════════════════════════════════
Running time: 1:00:15
Fixes Applied: X
Fixes Failed: Y
Overnight Branch: overnight-20251031

Next steps:
1. Review commits in overnight-20251031
2. Test the fixes
3. Merge good fixes to playerbot-dev
```

---

## 🔍 Post-Test Verification

### Step 1: Check the Overnight Branch
```bash
git fetch
git checkout overnight-20251031
git log --oneline

# Should see commits for any fixes that were applied
```

### Step 2: Check Deploy Directory
```bash
ls -lh M:/Wplayerbot/worldserver.exe
ls -lh M:/Wplayerbot/worldserver.pdb

# Check timestamps - should be recent if any fixes were deployed
```

### Step 3: Check Logs
```bash
tail -100 .claude/logs/overnight_autonomous.log

# Review what happened during the test
```

### Step 4: Check Deployment Markers
```bash
ls .claude/crash_analysis_queue/overnight_deployed/

# Should see deployed_*.txt files for each fix
```

---

## ✅ Success Criteria for Test

### Startup Success
- ✅ Git branch setup completes without errors
- ✅ overnight-YYYYMMDD branch created
- ✅ playerbot-dev committed/pushed (if there were changes)
- ✅ Monitoring loop starts

### Runtime Success (If No Crashes)
- ✅ Heartbeats appear every 5 minutes
- ✅ No error messages
- ✅ Runs for full 60 minutes
- ✅ Stops cleanly on Ctrl+C

### Runtime Success (If Crashes Occur)
- ✅ Crash detected within 30 seconds
- ✅ Analysis triggered
- ✅ Fix generated (10 minutes)
- ✅ Fix applied to source
- ✅ Compilation successful
- ✅ Commit created on overnight branch
- ✅ Push to origin successful
- ✅ worldserver deployed to M:/Wplayerbot
- ✅ Continues to next crash

---

## 🚨 Troubleshooting

### Error: "unable to add 'NUL' to index"
```bash
rm -f NUL
git status  # Should not show NUL anymore
```

### Error: Branch already exists
```bash
git branch -D overnight-20251031
git push origin --delete overnight-20251031
# Then restart overnight mode
```

### Error: Compilation fails
```bash
# Check if build directory is correct
ls build/

# Test compilation manually
cd build
cmake --build . --target worldserver --config RelWithDebInfo -j 8
```

### Error: Deploy directory not found
```bash
# Create it or update script
mkdir -p M:/Wplayerbot
# Or edit script to use different path
```

---

## 📊 Expected Test Results

### Scenario 1: No Crashes (Most Likely for 60 Min Test)
```
Result:
- overnight-20251031 branch created
- No commits to overnight branch (no crashes to fix)
- Clean shutdown after 60 minutes
- No worldserver deployment (nothing changed)

Conclusion: ✅ System working, just waiting for crashes
```

### Scenario 2: 1-2 Crashes
```
Result:
- overnight-20251031 branch created
- 1-2 commits to overnight branch
- worldserver deployed 1-2 times
- Clean shutdown after 60 minutes

Conclusion: ✅ Full pipeline working!
Next: Review overnight commits and merge
```

### Scenario 3: Crash but Compilation Failed
```
Result:
- overnight-20251031 branch created
- Fixes applied but reverted (compilation failed)
- No commits to overnight branch
- Error logged

Conclusion: ⚠️ Compilation issue
Next: Review logs, fix compilation issue
```

---

## 🎯 Recommendation

Since there are many uncommitted files in your working directory, I recommend:

1. **Review and commit** the autonomous system files first:
   ```bash
   git add .claude/scripts/overnight_autonomous_mode.py
   git add .claude/OVERNIGHT_MODE_GUIDE.md
   git add .claude/AUTONOMOUS_SYSTEM_COMPLETE.md
   git add .claude/THREE_AUTONOMOUS_MODES.md
   # etc...

   git commit -m "feat: Add overnight autonomous mode for crash fixing"
   git push
   ```

2. **Then start overnight mode** with clean working directory

3. **Let it run for 60 minutes**

4. **Review results** using this checklist

---

**Ready to test when you are!** 🚀
