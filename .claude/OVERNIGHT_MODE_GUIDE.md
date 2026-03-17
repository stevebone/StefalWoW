# Overnight Autonomous Mode - Complete Guide 🌙

## 🎯 Purpose

Run fully autonomous crash fixing overnight with ZERO human intervention. In the morning, review all fixes in git and merge the good ones.

---

## 🔒 Safety Mechanisms

### 1. Separate Branch Protection
```
playerbot-dev ← Production branch (PROTECTED)
     ↓ (creates from)
overnight-YYYYMMDD ← All overnight fixes go here
```

**What This Means:**
- playerbot-dev is never touched during overnight mode
- All fixes go to a separate overnight branch
- Review and merge manually in the morning

### 2. Compilation Verification
- Every fix is compiled BEFORE committing
- If compilation fails → Fix is reverted, not committed
- Only working fixes make it to the overnight branch

### 3. Auto-Deployment
- After successful compilation → Copy to M:/Wplayerbot/
- Server automatically uses new worldserver.exe
- Continues processing crashes with fixed code

---

## 🚀 How to Start

### Quick Start
```bash
cd C:\TrinityBots\TrinityCore
python .claude\scripts\overnight_autonomous_mode.py
```

### What Happens at Startup

#### Step 1: Branch Safety Setup
```
1. Checks current branch
2. Switches to playerbot-dev if needed
3. Commits any uncommitted changes to playerbot-dev
4. Pushes playerbot-dev to origin
5. Creates overnight-YYYYMMDD branch from playerbot-dev
6. Pushes overnight branch to origin
```

**You'll see:**
```
═══════════════════════════════════════════════════════
OVERNIGHT BRANCH SETUP
═══════════════════════════════════════════════════════
Current branch: playerbot-dev
✅ playerbot-dev is clean
Creating overnight-20251031 from playerbot-dev...
Pushing overnight-20251031 to origin...

✅ OVERNIGHT BRANCH SETUP COMPLETE
   Branch: overnight-20251031
   All fixes will be committed to this branch
   Review and merge to playerbot-dev in the morning
```

#### Step 2: Autonomous Loop Starts
```
🌙 OVERNIGHT MODE ACTIVE - Processing crashes until Ctrl+C
   All fixes will go to branch: overnight-20251031
   Review in morning and merge good fixes to playerbot-dev
```

---

## 🔄 Overnight Workflow

### Fully Automated Loop

```
┌─────────────────────────────────────────────────────────┐
│ 1. CRASH DETECTION (30 seconds)                        │
└─────────────────────────────────────────────────────────┘
Crash occurs → Python detects → Creates request

┌─────────────────────────────────────────────────────────┐
│ 2. ANALYSIS TRIGGER (30 seconds)                       │
└─────────────────────────────────────────────────────────┘
Python creates marker → Claude detects → Starts analysis

┌─────────────────────────────────────────────────────────┐
│ 3. FIX GENERATION (5-10 minutes)                       │
└─────────────────────────────────────────────────────────┘
Claude analyzes → Generates fix → Writes response JSON

┌─────────────────────────────────────────────────────────┐
│ 4. OVERNIGHT DEPLOYMENT (NO HUMAN APPROVAL)            │
└─────────────────────────────────────────────────────────┘
Python detects response → Applies fix → Compiles

┌─────────────────────────────────────────────────────────┐
│ 5. COMPILATION CHECK (15 minutes)                      │
└─────────────────────────────────────────────────────────┘
If SUCCESS:
  → Create git commit
  → Push to overnight-YYYYMMDD branch
  → Copy worldserver.exe + .pdb to M:/Wplayerbot/
  → Mark as deployed
  → Continue to next crash

If FAILED:
  → Revert changes
  → Log error
  → Skip deployment
  → Continue to next crash

┌─────────────────────────────────────────────────────────┐
│ 6. LOOP CONTINUES UNTIL Ctrl+C                         │
└─────────────────────────────────────────────────────────┘
```

**This runs ALL NIGHT without any human intervention.**

---

## 📊 What You'll See Running

### Heartbeat (Every 5 Minutes)
```
💓 Heartbeat: Iteration 10
   Running: 2:30:15
   Fixes Applied: 5
   Fixes Failed: 1
```

### When Crash Detected
```
🔔 Found 1 pending request(s)
✅ Created auto-process marker for abc123
```

### When Fix Ready
```
🌙 OVERNIGHT FIX DEPLOYMENT (NO HUMAN APPROVAL)
═══════════════════════════════════════════════════════
Request ID: abc123
Crash ID: 273f92f0f16d

📝 Step 1/5: Applying fix to source files...
  Writing fix to: src/modules/Playerbot/Core/Events/EventDispatcher.cpp
✅ Applied fix to 1 file(s)

🔨 Step 2/5: Compiling worldserver...
  Running: cmake --build build --target worldserver...
✅ Compilation SUCCESSFUL

💾 Step 3/5: Creating git commit...
✅ Git commit created

📤 Step 4/5: Pushing to overnight branch...
✅ Pushed to overnight branch

🚀 Step 5/5: Copying worldserver + pdb to Wplayerbot...
  Copying worldserver.exe -> M:/Wplayerbot/worldserver.exe
  Copying worldserver.pdb -> M:/Wplayerbot/worldserver.pdb
✅ Deployed to M:/Wplayerbot

✅ OVERNIGHT DEPLOYMENT COMPLETE
   Fix applied, compiled, committed to overnight-20251031
   worldserver deployed to M:/Wplayerbot
   Total fixes applied: 5
```

### When Compilation Fails
```
🔨 Step 2/5: Compiling worldserver...
❌ Compilation FAILED
   Fix applied but not compiled - will be in git diff
   Reverting changes...
   Continuing to next crash...
```

---

## 🛑 Stopping Overnight Mode

Press `Ctrl+C` in the terminal:

```
🛑 OVERNIGHT MODE STOPPED BY USER
═══════════════════════════════════════════════════════
Running time: 8:15:32
Fixes Applied: 12
Fixes Failed: 2
Overnight Branch: overnight-20251031

Next steps:
1. Review commits in overnight-20251031
2. Test the fixes
3. Merge good fixes to playerbot-dev:
   git checkout playerbot-dev
   git merge overnight-20251031
   git push
```

---

## 🌅 Morning Review Process

### Step 1: Check the Overnight Branch
```bash
git checkout overnight-20251031
git log --oneline

# You'll see:
# abc123f fix(playerbot): NULL_PTR_DEREFERENCE in EventDispatcher::UnsubscribeAll
# def456g fix(playerbot): RACE_CONDITION in BotManager::Update
# ghi789h fix(playerbot): MEMORY_LEAK in ActionManager::Cleanup
# ...
```

### Step 2: Review Each Commit
```bash
# Review each fix
git show abc123f

# Check the code changes
# Read the commit message
# Assess fix quality
```

### Step 3: Test the Fixes
```bash
# The worldserver in M:/Wplayerbot already has all fixes
# Just check if server has been stable overnight

# Check crash logs
ls M:/Wplayerbot/Crashes/

# If no new crashes → Fixes are working
```

### Step 4: Merge Good Fixes to playerbot-dev

#### Option A: Merge All (If all fixes are good)
```bash
git checkout playerbot-dev
git merge overnight-20251031
git push
```

#### Option B: Cherry-Pick Individual Fixes
```bash
git checkout playerbot-dev

# Pick only the good commits
git cherry-pick abc123f  # Good fix
git cherry-pick def456g  # Good fix
# Skip ghi789h - bad fix

git push
```

#### Option C: Reject All (If fixes caused issues)
```bash
# Just delete the overnight branch
git branch -D overnight-20251031
git push origin --delete overnight-20251031

# playerbot-dev is untouched
```

---

## 📁 Files and Logs

### Overnight Log
```bash
tail -f .claude/logs/overnight_autonomous.log
```

### Deployment Markers
```
.claude/crash_analysis_queue/overnight_deployed/
├── deployed_abc123.txt
├── deployed_def456.txt
└── deployed_ghi789.txt
```

Each marker contains:
```
Overnight Deployment
====================
Request ID: abc123
Crash ID: 273f92f0f16d
Deployed: 2025-10-31T23:45:12
Branch: overnight-20251031
Compilation: SUCCESS
Deployment: SUCCESS
```

---

## 🎯 Use Cases

### Use Case 1: Overnight Testing Environment
```
19:00 - Start overnight mode
23:00 - 5 crashes processed, deployed
07:00 - Stop overnight mode, review fixes
08:00 - Merge good fixes, delete bad ones
```

### Use Case 2: Weekend Autonomous Operation
```
Friday 18:00 - Start overnight mode
Monday 08:00 - Stop overnight mode
Monday 09:00 - Review 50+ fixes from weekend
Monday 10:00 - Merge good fixes incrementally
```

### Use Case 3: Stress Test Fixing
```
Start stress test with 1000 bots
Start overnight mode
Let it run for 24 hours
Review all fixes the next day
```

---

## ⚙️ Configuration

### Change Check Interval
Edit the script:
```python
time.sleep(30)  # Check every 30 seconds
# Change to:
time.sleep(60)  # Check every 60 seconds
```

### Change Compilation Timeout
```python
timeout=1800  # 30 minutes
# Change to:
timeout=3600  # 60 minutes for slow machines
```

### Change Deploy Directory
```python
self.deploy_dir = Path("M:/Wplayerbot")
# Change to:
self.deploy_dir = Path("C:/YourPath")
```

---

## 🚨 Troubleshooting

### Problem: Branch Creation Fails
**Error:** `fatal: A branch named 'overnight-20251031' already exists`

**Solution:** Delete the existing branch first:
```bash
git branch -D overnight-20251031
git push origin --delete overnight-20251031
```

### Problem: Compilation Always Fails
**Check:**
1. Is build directory correct?
2. Are CMake/compiler available?
3. Check compilation log in overnight_autonomous.log

**Solution:** Test compilation manually:
```bash
cd build
cmake --build . --target worldserver --config RelWithDebInfo -j 8
```

### Problem: Deploy Directory Not Found
**Error:** `Deploy directory not found: M:/Wplayerbot`

**Solution:** Create the directory or change deploy_dir in script:
```bash
mkdir -p M:/Wplayerbot
```

### Problem: Git Push Fails
**Error:** `failed to push some refs`

**Possible Causes:**
- Network issue
- Authentication issue
- Remote branch deleted

**Solution:** Check git remote:
```bash
git remote -v
git fetch origin
```

---

## 📊 Expected Performance

### Timings Per Fix
- Detection: 30 seconds
- Analysis: 5-10 minutes
- Compilation: 10-20 minutes
- Commit + Push: 1 minute
- Deploy: 10 seconds

**Total: 20-35 minutes per fix**

### Overnight Capacity
- 8 hours = 480 minutes
- ~20 fixes per night (if continuous crashes)
- Realistically: 5-15 fixes per night

---

## ✅ Success Criteria

System working correctly when:
- ✅ Overnight branch created successfully
- ✅ All fixes committed to overnight branch (not playerbot-dev)
- ✅ Every commit has successful compilation
- ✅ Failed compilations are reverted and skipped
- ✅ worldserver deployed to M:/Wplayerbot after each fix
- ✅ Runs continuously until Ctrl+C
- ✅ Clean logs with clear status messages

---

## 🎉 Benefits

### vs. Manual Fixing
- **Manual:** 2-8 hours per crash, during work hours
- **Overnight:** 20-35 minutes per crash, during sleep hours
- **Savings:** 100% of developer daytime hours

### vs. Human Approval Mode
- **Human Approval:** Requires human at 3am to approve fixes
- **Overnight:** Zero human intervention, review in morning
- **Benefit:** Uninterrupted sleep + batch review in morning

### Git Safety
- playerbot-dev never touched during overnight
- Easy rollback (just delete overnight branch)
- Clear commit history showing all overnight fixes
- Cherry-pick individual good fixes

---

**Start overnight mode before bed, review fixes in the morning over coffee. ☕**

🤖 Generated with [Claude Code](https://claude.com/claude-code)
