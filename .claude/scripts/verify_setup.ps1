# =============================================================================
# TrinityCore Playerbot - Setup Verification Script
# =============================================================================
# Verifies that all enhancements are correctly installed
# =============================================================================

$ErrorActionPreference = "Continue"

function Write-Check {
    param($Name, $Status, $Message = "")
    $symbol = if ($Status) { "[OK]" } else { "[ERROR]" }
    $color = if ($Status) { "Green" } else { "Red" }
    Write-Host "  $symbol $Name" -ForegroundColor $color
    if ($Message) {
        Write-Host "     $Message" -ForegroundColor Gray
    }
}

Write-Host @"
================================================================================
  Setup Verification
================================================================================
"@ -ForegroundColor Cyan

# =============================================================================
# Prerequisites
# =============================================================================
Write-Host "`n[Prerequisites]" -ForegroundColor Yellow

$nodeExists = $null -ne (Get-Command node -ErrorAction SilentlyContinue)
Write-Check "Node.js installed" $nodeExists

$npmExists = $null -ne (Get-Command npm -ErrorAction SilentlyContinue)
Write-Check "npm installed" $npmExists

$pythonExists = $null -ne (Get-Command python -ErrorAction SilentlyContinue)
Write-Check "Python installed" $pythonExists

$gitExists = $null -ne (Get-Command git -ErrorAction SilentlyContinue)
Write-Check "Git installed" $gitExists

# =============================================================================
# Configuration Files
# =============================================================================
Write-Host "`n[Configuration Files]" -ForegroundColor Yellow

$envExists = Test-Path ".env"
Write-Check ".env file" $envExists "Required for MCP configuration"

$mcpConfigExists = Test-Path ".claude/mcp-servers-config.json"
Write-Check "MCP servers config" $mcpConfigExists

$gitHooksConfigExists = Test-Path ".claude/git_hooks_config.json"
Write-Check "Git hooks config" $gitHooksConfigExists

$settingsExists = Test-Path ".claude/settings.local.json"
Write-Check "Claude settings" $settingsExists

# =============================================================================
# MCP Servers
# =============================================================================
Write-Host "`n[MCP Servers]" -ForegroundColor Yellow

try {
    $githubMCP = npm list -g @modelcontextprotocol/server-github 2>$null
    $githubInstalled = $githubMCP -match "server-github@"
    Write-Check "GitHub MCP" $githubInstalled
} catch {
    Write-Check "GitHub MCP" $false "Run: npm install -g @modelcontextprotocol/server-github"
}

try {
    $seqMCP = npm list -g @modelcontextprotocol/server-sequential-thinking 2>$null
    $seqInstalled = $seqMCP -match "server-sequential-thinking@"
    Write-Check "Sequential Thinking MCP" $seqInstalled
} catch {
    Write-Check "Sequential Thinking MCP" $false
}

try {
    $fsMCP = npm list -g @modelcontextprotocol/server-filesystem 2>$null
    $fsInstalled = $fsMCP -match "server-filesystem@"
    Write-Check "Filesystem MCP" $fsInstalled
} catch {
    Write-Check "Filesystem MCP" $false
}

try {
    $dbMCP = npm list -g @executeautomation/database-server 2>$null
    $dbInstalled = $dbMCP -match "database-server@"
    Write-Check "Database MCP" $dbInstalled
} catch {
    Write-Check "Database MCP" $false
}

# =============================================================================
# Git Hooks
# =============================================================================
Write-Host "`n[Git Hooks]" -ForegroundColor Yellow

$preCommitExists = Test-Path ".git/hooks/pre-commit"
Write-Check "pre-commit hook" $preCommitExists

$commitMsgExists = Test-Path ".git/hooks/commit-msg"
Write-Check "commit-msg hook" $commitMsgExists

$prePushExists = Test-Path ".git/hooks/pre-push"
Write-Check "pre-push hook" $prePushExists

# =============================================================================
# Python Scripts
# =============================================================================
Write-Host "`n[Python Scripts]" -ForegroundColor Yellow

$cacheManagerExists = Test-Path ".claude/scripts/agent_cache_manager.py"
Write-Check "Agent cache manager" $cacheManagerExists

$smartSelectorExists = Test-Path ".claude/scripts/smart_agent_selector.py"
Write-Check "Smart agent selector" $smartSelectorExists

$gitHooksManagerExists = Test-Path ".claude/scripts/git_hooks_manager.py"
Write-Check "Git hooks manager" $gitHooksManagerExists

# =============================================================================
# Slash Commands
# =============================================================================
Write-Host "`n[Slash Commands]" -ForegroundColor Yellow

$reviewCmdExists = Test-Path ".claude/commands/review.md"
Write-Check "/review command" $reviewCmdExists

$buildCmdExists = Test-Path ".claude/commands/build.md"
Write-Check "/build command" $buildCmdExists

$dbCmdExists = Test-Path ".claude/commands/db.md"
Write-Check "/db command" $dbCmdExists

$deployCmdExists = Test-Path ".claude/commands/deploy.md"
Write-Check "/deploy command" $deployCmdExists

# =============================================================================
# GitHub Workflows
# =============================================================================
Write-Host "`n[GitHub Workflows]" -ForegroundColor Yellow

$pbCIExists = Test-Path ".github/workflows/playerbot-ci.yml"
Write-Check "Playerbot CI workflow" $pbCIExists

$nightlyExists = Test-Path ".github/workflows/playerbot-nightly-review.yml"
Write-Check "Nightly review workflow" $nightlyExists

$depUpdatesExists = Test-Path ".github/workflows/playerbot-dependency-updates.yml"
Write-Check "Dependency updates workflow" $depUpdatesExists

# =============================================================================
# Agents
# =============================================================================
Write-Host "`n[New Agents]" -ForegroundColor Yellow

$buildPerfExists = Test-Path ".claude/agents/build-performance-analyzer.md"
Write-Check "Build performance analyzer" $buildPerfExists

$apiMigrationExists = Test-Path ".claude/agents/trinity-api-migration.md"
Write-Check "Trinity API migration" $apiMigrationExists

# =============================================================================
# Directories
# =============================================================================
Write-Host "`n[Directories]" -ForegroundColor Yellow

$cacheDir = Test-Path ".claude/tmp/agent_results"
Write-Check "Cache directory" $cacheDir

$logsDir = Test-Path ".claude/logs"
Write-Check "Logs directory" $logsDir

$reportsDir = Test-Path ".claude/reports"
Write-Check "Reports directory" $reportsDir

# =============================================================================
# TrinityCore MCP Server (Optional)
# =============================================================================
Write-Host "`n[TrinityCore MCP Server (Optional)]" -ForegroundColor Yellow

$trinityMCPExists = Test-Path "trinity-mcp-server/package.json"
Write-Check "Trinity MCP source" $trinityMCPExists

if ($trinityMCPExists) {
    $trinityMCPBuilt = Test-Path "trinity-mcp-server/dist/index.js"
    Write-Check "Trinity MCP built" $trinityMCPBuilt "Run: npm run build in trinity-mcp-server/"
}

# =============================================================================
# Functional Tests
# =============================================================================
Write-Host "`n[Functional Tests]" -ForegroundColor Yellow

# Test Python imports
try {
    python -c "import json, hashlib, pathlib" 2>$null
    Write-Check "Python dependencies" $true
} catch {
    Write-Check "Python dependencies" $false
}

# Test agent cache
if ($cacheManagerExists) {
    try {
        $cacheTest = python .claude/scripts/agent_cache_manager.py stats 2>&1
        Write-Check "Agent cache functional" ($LASTEXITCODE -eq 0)
    } catch {
        Write-Check "Agent cache functional" $false
    }
}

# Test smart selector
if ($smartSelectorExists) {
    try {
        python .claude/scripts/smart_agent_selector.py --mode quick 2>&1 | Out-Null
        Write-Check "Smart selector functional" ($LASTEXITCODE -eq 0)
    } catch {
        Write-Check "Smart selector functional" $false
    }
}

# =============================================================================
# Summary
# =============================================================================
Write-Host @"

================================================================================
  Verification Summary
================================================================================
"@ -ForegroundColor Cyan

$allPassed = $nodeExists -and $npmExists -and $pythonExists -and $gitExists -and
             $envExists -and $mcpConfigExists -and $githubInstalled -and
             $preCommitExists -and $cacheManagerExists

if ($allPassed) {
    Write-Host @"
[OK] All critical components verified!

Your setup is complete and ready to use.

Try these commands:
  /review             # Run code review
  /db schema %        # Query database (requires DB MCP configured)

Documentation:
  .claude/CLAUDE_CODE_ENHANCEMENT_EVALUATION_2025.md
"@ -ForegroundColor Green
} else {
    Write-Host @"
[WARNING]  Some components are missing or not configured.

Please review the failures above and run:
  powershell .claude/scripts/setup_enhancements.ps1

For help, see:
  .claude/CLAUDE_CODE_ENHANCEMENT_EVALUATION_2025.md
"@ -ForegroundColor Yellow
}

Write-Host "`n================================================================================" -ForegroundColor Cyan
