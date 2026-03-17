# Quick Start - Correct Autonomous Workflow ⚡

## The Right Way: Human Reviews FIX Before Deployment

---

## 🚀 Start in 10 Seconds

```bash
cd C:\TrinityBots\TrinityCore
python .claude\scripts\autonomous_crash_monitor_with_approval.py
```

---

## 🔄 What Happens

### 1. Autonomous Detection & Analysis (No Human)
```
Crash → Detected (30s) → Analyzed (10min) → Fix Generated
```
**You do nothing - system works automatically**

### 2. Human Reviews FIX (Your Action Required)
```
System creates: REVIEW_{id}.txt
You read it and create: approval_{id}.json
```
**This is where YOU review the fix quality**

### 3. Autonomous Deployment (If You Approved)
```
Apply → Compile (15min) → Commit → PR → Done
```
**You do nothing - system deploys automatically**

---

## 📝 When You Need to Act

### You'll See This in Logs:
```
🔔 NEW FIX READY FOR HUMAN REVIEW
═══════════════════════════════════════
Request ID: abc123
Crash ID: 273f92f0f16d

📝 Review file created: REVIEW_abc123.txt
⏳ WAITING FOR YOUR APPROVAL
```

### Your Action:

#### Step 1: Read Review File
```bash
cat .claude/crash_analysis_queue/approvals/REVIEW_abc123.txt
```

#### Step 2: Decide

**If fix is good (APPROVE):**
```bash
cat > .claude/crash_analysis_queue/approvals/approval_abc123.json << 'EOF'
{
  "request_id": "abc123",
  "status": "approved",
  "approved_by": "YourName",
  "approved_at": "2025-10-31T12:00:00",
  "comments": "Fix looks good"
}
EOF
```

**If fix is bad (REJECT):**
```bash
cat > .claude/crash_analysis_queue/approvals/approval_abc123.json << 'EOF'
{
  "request_id": "abc123",
  "status": "rejected",
  "rejected_by": "YourName",
  "rejected_at": "2025-10-31T12:00:00",
  "reason": "Fix needs improvement - missing edge case X"
}
EOF
```

#### Step 3: Wait 30 Seconds
System detects your approval and proceeds automatically.

---

## ⏱️ Time Breakdown

### If You Approve:
- Autonomous work: 27 minutes
- Your review: 3 minutes
- **Total: 30 minutes**

### If You Reject:
- Autonomous work: 11 minutes
- Your review: 3 minutes
- **Total: 14 minutes** ✅ No wasted compilation!

---

## 📊 Why This Is Better

### ❌ Wrong: Review PR After Deployment
```
Generate → Apply → Compile (15min) → PR → Review
```
**Problem:** Wasted 15 minutes of compilation if you reject

### ✅ Right: Review FIX Before Deployment
```
Generate → Review → Apply → Compile (15min) → PR
```
**Benefit:** No wasted time if you reject

---

## 💡 Key Point

**You review the PROPOSED FIX, not the deployed code.**

This means:
- ✅ No compilation if fix is bad
- ✅ No commits if fix is bad
- ✅ No PRs if fix is bad
- ✅ Fast rejection (14 min vs 30 min)
- ✅ Clean git history

---

## 📁 Files You'll See

```
.claude/crash_analysis_queue/approvals/
├── REVIEW_abc123.txt          ← READ THIS (human-readable summary)
└── approval_abc123.json       ← CREATE THIS (your decision)
```

---

That's it! Simple, efficient, correct approval gate placement. 🎉

**Full docs:** See `CORRECT_AUTONOMOUS_WORKFLOW.md`
