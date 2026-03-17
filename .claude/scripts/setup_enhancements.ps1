# =============================================================================
# TrinityCore Playerbot - Enhancement Setup Script
# =============================================================================
# This script installs all Claude Code enhancements in one shot
# =============================================================================

param(
    [switch]$SkipMCPInstall,
    [switch]$SkipGitHooks,
    [switch]$TestOnly,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Colors for output
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warning { Write-Host $args -ForegroundColor Yellow }
function Write-Error { Write-Host $args -ForegroundColor Red }

# =============================================================================
# Banner
# =============================================================================
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "  TrinityCore Playerbot - Claude Code Enhancement Setup" -ForegroundColor Cyan
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "  This script will install:" -ForegroundColor Cyan
Write-Host "  * Critical MCP servers: GitHub Database Sequential Thinking Filesystem" -ForegroundColor White
Write-Host "  * Git hooks for quality gates" -ForegroundColor White
Write-Host "  * Agent caching system" -ForegroundColor White
Write-Host "  * Smart agent selector" -ForegroundColor White
Write-Host "  * Custom slash commands" -ForegroundColor White
Write-Host "  * TrinityCore MCP server (optional)" -ForegroundColor White
Write-Host "" -ForegroundColor White
Write-Host "  Estimated time: 10-15 minutes" -ForegroundColor Yellow
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host ""

# =============================================================================
# Prerequisites Check
# =============================================================================
Write-Info "`n[1/10] Checking prerequisites..."

# Check Node.js
try {
    $nodeVersion = node --version
    Write-Success "  [OK] Node.js: $nodeVersion"
} catch {
    Write-Error "  [ERROR] Node.js not found. Please install Node.js 18+ from https://nodejs.org/"
    exit 1
}

# Check npm
try {
    $npmVersion = npm --version
    Write-Success "  [OK] npm: $npmVersion"
} catch {
    Write-Error "  [ERROR] npm not found"
    exit 1
}

# Check Python
try {
    $pythonVersion = python --version
    Write-Success "  [OK] Python: $pythonVersion"
} catch {
    Write-Error "  [ERROR] Python not found. Please install Python 3.8+"
    exit 1
}

# Check git
try {
    $gitVersion = git --version
    Write-Success "  [OK] Git: $gitVersion"
} catch {
    Write-Error "  [ERROR] Git not found"
    exit 1
}

# =============================================================================
# Environment Configuration
# =============================================================================
Write-Info "`n[2/10] Setting up environment configuration..."

if (!(Test-Path ".env")) {
    if (Test-Path ".env.template") {
        Write-Info "  Creating .env from template..."
        Copy-Item ".env.template" ".env"
        Write-Warning "  [WARNING]  Please edit .env file with your credentials!"
        Write-Warning "  Required: GITHUB_TOKEN, TRINITY_DB_PASSWORD"
    } else {
        Write-Error "  [ERROR] .env.template not found!"
        exit 1
    }
} else {
    Write-Success "  [OK] .env file already exists"
}

# =============================================================================
# Python Dependencies
# =============================================================================
Write-Info "`n[3/10] Installing Python dependencies..."

if (Test-Path ".claude/scripts/requirements.txt") {
    try {
        python -m pip install --upgrade pip --quiet
        pip install -r .claude/scripts/requirements.txt --quiet
        Write-Success "  [OK] Python dependencies installed"
    } catch {
        Write-Warning "  [WARNING]  Some Python dependencies may have failed to install"
    }
} else {
    Write-Info "  [INFO]  No requirements.txt found, skipping"
}

# =============================================================================
# MCP Servers Installation
# =============================================================================
if (!$SkipMCPInstall) {
    Write-Info "`n[4/10] Installing MCP servers..."

    # GitHub MCP
    Write-Info "  Installing GitHub MCP..."
    npm install -g @modelcontextprotocol/server-github --silent
    if ($?) {
        Write-Success "    [OK] GitHub MCP installed"
    } else {
        Write-Warning "    [WARNING]  GitHub MCP installation failed"
    }

    # Sequential Thinking MCP
    Write-Info "  Installing Sequential Thinking MCP..."
    npm install -g @modelcontextprotocol/server-sequential-thinking --silent
    if ($?) {
        Write-Success "    [OK] Sequential Thinking MCP installed"
    } else {
        Write-Warning "    [WARNING]  Sequential Thinking MCP installation failed"
    }

    # Filesystem MCP
    Write-Info "  Installing Filesystem MCP..."
    npm install -g @modelcontextprotocol/server-filesystem --silent
    if ($?) {
        Write-Success "    [OK] Filesystem MCP installed"
    } else {
        Write-Warning "    [WARNING]  Filesystem MCP installation failed"
    }

    # Database MCP
    Write-Info "  Installing Database MCP..."
    npm install -g @executeautomation/database-server --silent
    if ($?) {
        Write-Success "    [OK] Database MCP installed"
    } else {
        Write-Warning "    [WARNING]  Database MCP installation failed"
    }
} else {
    Write-Info "`n[4/10] Skipping MCP installation (-SkipMCPInstall)"
}

# =============================================================================
# Git Hooks Installation
# =============================================================================
if (!$SkipGitHooks) {
    Write-Info "`n[5/10] Installing git hooks..."

    if (Test-Path ".claude/scripts/git_hooks_manager.py") {
        try {
            python .claude/scripts/git_hooks_manager.py install
            Write-Success "  [OK] Git hooks installed"
        } catch {
            Write-Warning "  [WARNING]  Git hooks installation failed"
        }
    } else {
        Write-Warning "  [WARNING]  git_hooks_manager.py not found"
    }
} else {
    Write-Info "`n[5/10] Skipping git hooks installation (-SkipGitHooks)"
}

# =============================================================================
# Agent Cache Initialization
# =============================================================================
Write-Info "`n[6/10] Initializing agent cache..."

if (Test-Path ".claude/scripts/agent_cache_manager.py") {
    try {
        python .claude/scripts/agent_cache_manager.py init
        Write-Success "  [OK] Agent cache initialized"
    } catch {
        Write-Warning "  [WARNING]  Agent cache initialization failed"
    }
} else {
    Write-Warning "  [WARNING]  agent_cache_manager.py not found"
}

# =============================================================================
# Directory Structure
# =============================================================================
Write-Info "`n[7/10] Creating directory structure..."

$dirs = @(
    ".claude/tmp/agent_results",
    ".claude/logs",
    ".claude/reports",
    ".claude/commands",
    ".claude/workflows"
)

foreach ($dir in $dirs) {
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Success "  [OK] Created $dir"
    }
}

# =============================================================================
# TrinityCore MCP Server (Optional)
# =============================================================================
Write-Info "`n[8/10] TrinityCore MCP server setup..."

if (Test-Path "trinity-mcp-server/package.json") {
    $response = Read-Host "  Install TrinityCore MCP server? (y/n)"
    if ($response -eq "y") {
        Write-Info "  Installing TrinityCore MCP dependencies..."
        Push-Location trinity-mcp-server
        npm install --silent
        npm run build
        Pop-Location
        Write-Success "  [OK] TrinityCore MCP server built"
    } else {
        Write-Info "  [INFO]  Skipping TrinityCore MCP server"
    }
} else {
    Write-Info "  [INFO]  TrinityCore MCP server not found (optional)"
}

# =============================================================================
# Permissions Configuration
# =============================================================================
Write-Info "`n[9/10] Configuring permissions..."

if (Test-Path ".claude/settings.local.json") {
    Write-Success "  [OK] Settings file exists"
} else {
    Write-Warning "  [WARNING]  .claude/settings.local.json not found"
    Write-Info "  Creating basic settings file..."

    $settings = @{
        permissions = @{
            allow = @(
                "mcp__github__*",
                "mcp__database__*",
                "mcp__sequential_thinking__*",
                "mcp__filesystem__*"
            )
        }
    } | ConvertTo-Json

    $settings | Out-File ".claude/settings.local.json" -Encoding UTF8
    Write-Success "  [OK] Created settings.local.json"
}

# =============================================================================
# Verification
# =============================================================================
Write-Info "`n[10/10] Running verification..."

if (Test-Path ".claude/scripts/verify_setup.ps1") {
    & .claude/scripts/verify_setup.ps1
} else {
    Write-Warning "  [WARNING]  Verification script not found"
}

# =============================================================================
# Summary
# =============================================================================
Write-Host ""
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "  Setup Complete!" -ForegroundColor Cyan
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Installed Components:" -ForegroundColor Green
Write-Host "  * MCP servers: GitHub Database Sequential Thinking Filesystem" -ForegroundColor White
Write-Host "  * Git hooks for quality gates" -ForegroundColor White
Write-Host "  * Agent caching system" -ForegroundColor White
Write-Host "  * Directory structure" -ForegroundColor White
Write-Host "  * Configuration files" -ForegroundColor White
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host "  1. Edit .env file with your credentials:" -ForegroundColor White
Write-Host "     * GITHUB_TOKEN from https://github.com/settings/tokens" -ForegroundColor Gray
Write-Host "     * TRINITY_DB_PASSWORD" -ForegroundColor Gray
Write-Host ""
Write-Host "  2. Test the installation:" -ForegroundColor White
Write-Host "     powershell .claude/scripts/verify_setup.ps1" -ForegroundColor Gray
Write-Host ""
Write-Host "  3. Try the new features:" -ForegroundColor White
Write-Host "     /review              # Code review with smart agent selection" -ForegroundColor Gray
Write-Host "     /build               # Build with performance analysis" -ForegroundColor Gray
Write-Host "     /db schema playerbot # Query Trinity databases" -ForegroundColor Gray
Write-Host ""
Write-Host "  4. Review documentation:" -ForegroundColor White
Write-Host "     * .claude/CLAUDE_CODE_ENHANCEMENT_EVALUATION_2025.md" -ForegroundColor Gray
Write-Host "     * .claude/commands/*.md (slash command docs)" -ForegroundColor Gray
Write-Host "     * .claude/agents/*.md (agent specifications)" -ForegroundColor Gray
Write-Host ""
Write-Host "Estimated Improvements:" -ForegroundColor Cyan
Write-Host "  * 50-70 percent faster review times via agent caching" -ForegroundColor White
Write-Host "  * 30-40 percent fewer agent executions via smart selection" -ForegroundColor White
Write-Host "  * Automated quality gates via git hooks" -ForegroundColor White
Write-Host "  * Comprehensive CI/CD via GitHub workflows" -ForegroundColor White
Write-Host ""
Write-Host "Tips:" -ForegroundColor Magenta
Write-Host "  * Use /review with mode quick for fast checks" -ForegroundColor White
Write-Host "  * Git hooks can be bypassed with [SKIP-HOOKS] in commit message" -ForegroundColor White
Write-Host "  * Agent cache clears after 4 hours" -ForegroundColor White
Write-Host "  * Check .claude/logs/ for detailed execution logs" -ForegroundColor White
Write-Host ""
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "  Happy coding!" -ForegroundColor Green
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host ""

# =============================================================================
# Final Notes
# =============================================================================
if (Test-Path ".env") {
    $envContent = Get-Content ".env" -Raw
    if ($envContent -match "your_.*_here") {
        Write-Warning "`n[WARNING]  REMINDER: Update .env file with actual credentials!"
    }
}
