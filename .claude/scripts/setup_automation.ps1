# Setup Script for PlayerBot Automation System
# Run this script to configure the complete automation system

param(
    [Parameter(Mandatory=$false)]
    [string]$ProjectRoot = "C:\TrinityBots\TrinityCore",
    
    [Parameter(Mandatory=$false)]
    [switch]$InstallDependencies = $true,
    
    [Parameter(Mandatory=$false)]
    [switch]$ConfigureScheduler = $true,
    
    [Parameter(Mandatory=$false)]
    [switch]$TestRun = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=" * 60 -ForegroundColor Cyan
Write-Host "PlayerBot Automation System Setup" -ForegroundColor Cyan
Write-Host "=" * 60 -ForegroundColor Cyan

# Check if running as administrator
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "This script requires Administrator privileges!" -ForegroundColor Red
    Write-Host "Please run PowerShell as Administrator and try again." -ForegroundColor Yellow
    exit 1
}

# Configuration
$ClaudeDir = Join-Path $ProjectRoot ".claude"
$AgentsDir = Join-Path $ClaudeDir "agents"
$ScriptsDir = Join-Path $ClaudeDir "scripts"
$WorkflowsDir = Join-Path $ClaudeDir "workflows"
$ReportsDir = Join-Path $ClaudeDir "reports"
$LogsDir = Join-Path $ClaudeDir "logs"

# Create directory structure
Write-Host "`nCreating directory structure..." -ForegroundColor Green
@($ClaudeDir, $AgentsDir, $ScriptsDir, $WorkflowsDir, $ReportsDir, $LogsDir) | ForEach-Object {
    if (-not (Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ -Force | Out-Null
        Write-Host "  Created: $_" -ForegroundColor Gray
    } else {
        Write-Host "  Exists: $_" -ForegroundColor DarkGray
    }
}

# Install Python dependencies
if ($InstallDependencies) {
    Write-Host "`nInstalling Python dependencies..." -ForegroundColor Green
    
    # Create requirements.txt
    $requirements = @"
# PlayerBot Automation Dependencies
pathlib
logging
argparse
json
subprocess
typing
datetime

# Optional for advanced features
pandas>=1.3.0
matplotlib>=3.4.0
psutil>=5.8.0
pyyaml>=5.4.0
requests>=2.26.0
jinja2>=3.0.0

# For code analysis
pylint>=2.10.0
autopep8>=1.5.0
black>=21.0

# For reporting
reportlab>=3.6.0
"@
    
    $requirementsFile = Join-Path $ScriptsDir "requirements.txt"
    $requirements | Out-File -FilePath $requirementsFile -Encoding UTF8
    
    try {
        if (Get-Command python -ErrorAction SilentlyContinue) {
            & python -m pip install --upgrade pip
            & python -m pip install -r $requirementsFile
            Write-Host "  Python dependencies installed successfully" -ForegroundColor Green
        } else {
            Write-Host "  Python not found - skipping dependency installation" -ForegroundColor Yellow
        }
    }
    catch {
        Write-Host "  Failed to install Python dependencies: $_" -ForegroundColor Yellow
        Write-Host "  Please install Python and pip manually" -ForegroundColor Yellow
    }
}

# Configure Windows Task Scheduler
if ($ConfigureScheduler) {
    Write-Host "`nConfiguring Windows Task Scheduler..." -ForegroundColor Green
    
    $taskXml = Join-Path $ScriptsDir "task_scheduler_config.xml"
    
    # Update paths in XML
    $xml = [xml](Get-Content $taskXml)
    $xml.Task.Actions.Exec | ForEach-Object {
        if ($_.Command -like "*.bat" -or $_.Command -like "*.ps1") {
            $_.Command = $_.Command.Replace("C:\TrinityBots\TrinityCore", $ProjectRoot)
        }
        if ($_.WorkingDirectory) {
            $_.WorkingDirectory = $ProjectRoot
        }
    }
    $xml.Save($taskXml)
    
    # Register scheduled tasks
    try {
        # Remove existing task if it exists
        $existingTask = Get-ScheduledTask -TaskName "PlayerBot_DailyReview" -ErrorAction SilentlyContinue
        if ($existingTask) {
            Unregister-ScheduledTask -TaskName "PlayerBot_DailyReview" -Confirm:$false
            Write-Host "  Removed existing task" -ForegroundColor Gray
        }
        
        # Register new task
        Register-ScheduledTask -TaskName "PlayerBot_DailyReview" -Xml (Get-Content $taskXml -Raw)
        Write-Host "  Task 'PlayerBot_DailyReview' registered successfully" -ForegroundColor Green
        
        # Create additional tasks for different check types
        $checkTypes = @{
            "PlayerBot_MorningCheck" = @{
                Time = "06:00:00"
                Arguments = "--type morning"
            }
            "PlayerBot_SecurityScan" = @{
                Time = "08:00:00"
                Arguments = "--type critical --autofix"
            }
            "PlayerBot_PerformanceCheck" = @{
                Time = "12:00:00"
                Arguments = "--type midday"
            }
            "PlayerBot_EveningReport" = @{
                Time = "18:00:00"
                Arguments = "--type evening --email"
            }
        }
        
        foreach ($taskName in $checkTypes.Keys) {
            $taskConfig = $checkTypes[$taskName]
            
            $action = New-ScheduledTaskAction `
                -Execute "$ScriptsDir\run_daily_checks.bat" `
                -Argument $taskConfig.Arguments `
                -WorkingDirectory $ProjectRoot
            
            $trigger = New-ScheduledTaskTrigger `
                -Daily `
                -At $taskConfig.Time
            
            $settings = New-ScheduledTaskSettingsSet `
                -AllowStartIfOnBatteries `
                -DontStopIfGoingOnBatteries `
                -StartWhenAvailable `
                -RunOnlyIfNetworkAvailable:$false `
                -ExecutionTimeLimit (New-TimeSpan -Hours 2)
            
            Register-ScheduledTask `
                -TaskName $taskName `
                -Action $action `
                -Trigger $trigger `
                -Settings $settings `
                -Description "PlayerBot automated $($taskName.Replace('PlayerBot_', ''))" `
                -Force | Out-Null
            
            Write-Host "  Task '$taskName' registered (Daily at $($taskConfig.Time))" -ForegroundColor Green
        }
    }
    catch {
        Write-Host "  Failed to configure Task Scheduler: $_" -ForegroundColor Red
        Write-Host "  Please configure scheduled tasks manually" -ForegroundColor Yellow
    }
}

# Create configuration file
Write-Host "`nCreating configuration file..." -ForegroundColor Green
$config = @{
    project_root = $ProjectRoot
    claude_dir = $ClaudeDir
    automation = @{
        enabled = $true
        auto_fix = @{
            critical = $true
            high = $false
            medium = $false
            low = $false
        }
        notifications = @{
            email = @{
                enabled = $false
                recipients = @("team@project.com")
                smtp_server = "smtp.project.com"
                smtp_port = 587
            }
            slack = @{
                enabled = $false
                webhook_url = ""
                channel = "#playerbot-dev"
            }
        }
    }
    thresholds = @{
        quality_score = 80
        security_score = 90
        performance_score = 75
        coverage_minimum = 70
        trinity_compatibility = 95
    }
    reporting = @{
        formats = @("html", "json", "markdown")
        retention_days = 90
        dashboard_enabled = $true
    }
} | ConvertTo-Json -Depth 10

$configFile = Join-Path $ClaudeDir "automation_config.json"
$config | Out-File -FilePath $configFile -Encoding UTF8
Write-Host "  Configuration saved to: $configFile" -ForegroundColor Green

# Test run
if ($TestRun) {
    Write-Host "`nRunning test automation..." -ForegroundColor Green
    
    try {
        & "$ScriptsDir\run_daily_checks.bat" --type morning
        Write-Host "  Test run completed successfully!" -ForegroundColor Green
    }
    catch {
        Write-Host "  Test run failed: $_" -ForegroundColor Red
    }
}

# Create shortcuts
Write-Host "`nCreating desktop shortcuts..." -ForegroundColor Green
$WshShell = New-Object -ComObject WScript.Shell

# Daily Review shortcut
$Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\PlayerBot Daily Review.lnk")
$Shortcut.TargetPath = "powershell.exe"
$Shortcut.Arguments = "-ExecutionPolicy Bypass -File `"$ScriptsDir\daily_automation.ps1`" -CheckType full"
$Shortcut.WorkingDirectory = $ProjectRoot
$Shortcut.IconLocation = "powershell.exe"
$Shortcut.Description = "Run PlayerBot daily code review"
$Shortcut.Save()
Write-Host "  Created: PlayerBot Daily Review.lnk" -ForegroundColor Gray

# Quick Check shortcut
$Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\PlayerBot Quick Check.lnk")
$Shortcut.TargetPath = "$ScriptsDir\run_daily_checks.bat"
$Shortcut.Arguments = "--type morning"
$Shortcut.WorkingDirectory = $ProjectRoot
$Shortcut.IconLocation = "cmd.exe"
$Shortcut.Description = "Run quick morning checks"
$Shortcut.Save()
Write-Host "  Created: PlayerBot Quick Check.lnk" -ForegroundColor Gray

# Reports folder shortcut
$Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\PlayerBot Reports.lnk")
$Shortcut.TargetPath = $ReportsDir
$Shortcut.IconLocation = "explorer.exe"
$Shortcut.Description = "Open PlayerBot reports folder"
$Shortcut.Save()
Write-Host "  Created: PlayerBot Reports.lnk" -ForegroundColor Gray

Write-Host "`n" + ("=" * 60) -ForegroundColor Cyan
Write-Host "Setup completed successfully!" -ForegroundColor Green
Write-Host "=" * 60 -ForegroundColor Cyan

Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "1. Review and customize the configuration file:" -ForegroundColor White
Write-Host "   $configFile" -ForegroundColor Gray
Write-Host "2. Test the automation with:" -ForegroundColor White
Write-Host "   .\run_daily_checks.bat --type morning" -ForegroundColor Gray
Write-Host "3. Check scheduled tasks in Task Scheduler" -ForegroundColor White
Write-Host "4. Configure email/Slack notifications if needed" -ForegroundColor White
Write-Host "5. Use desktop shortcuts for quick access" -ForegroundColor White

Write-Host "`nAutomation is now configured and ready to use!" -ForegroundColor Green

# Display scheduled tasks
Write-Host "`nScheduled Tasks:" -ForegroundColor Cyan
Get-ScheduledTask -TaskName "PlayerBot_*" | Format-Table TaskName, State, @{Label="Next Run";Expression={$_.Triggers[0].StartBoundary}} -AutoSize
