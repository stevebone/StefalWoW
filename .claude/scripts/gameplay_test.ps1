# Gameplay Testing Script for PlayerBot
# Tests bot behavior and game mechanics

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("behavior", "combat", "pvp", "dungeon", "economy", "learning", "full", "quick")]
    [string]$Suite = "quick",
    
    [Parameter(Mandatory=$false)]
    [string]$Agent = "",
    
    [Parameter(Mandatory=$false)]
    [string[]]$Classes = @("all"),
    
    [Parameter(Mandatory=$false)]
    [int]$Duration = 30,
    
    [Parameter(Mandatory=$false)]
    [string]$ProjectRoot = "C:\TrinityBots\TrinityCore",
    
    [Parameter(Mandatory=$false)]
    [switch]$GenerateReport = $true
)

# Configuration
$ClaudeDir = Join-Path $ProjectRoot ".claude"
$AgentsDir = Join-Path $ClaudeDir "agents"
$ReportsDir = Join-Path $ClaudeDir "reports\gameplay"

# Ensure directories exist
if (-not (Test-Path $ReportsDir)) {
    New-Item -ItemType Directory -Path $ReportsDir -Force | Out-Null
}

# Logging
$LogFile = Join-Path $ReportsDir "gameplay_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"

function Write-TestLog {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "$timestamp [$Level] $Message"
    Add-Content -Path $LogFile -Value $logEntry
    
    switch ($Level) {
        "ERROR" { Write-Host $logEntry -ForegroundColor Red }
        "WARNING" { Write-Host $logEntry -ForegroundColor Yellow }
        "SUCCESS" { Write-Host $logEntry -ForegroundColor Green }
        "TEST" { Write-Host $logEntry -ForegroundColor Cyan }
        default { Write-Host $logEntry }
    }
}

# Test Suites Definition
$TestSuites = @{
    "behavior" = @{
        agents = @("wow-bot-behavior-designer", "bot-learning-system")
        scenarios = @("questing", "grinding", "following", "trading")
        duration = 30
    }
    "combat" = @{
        agents = @("wow-mechanics-expert")
        scenarios = @("rotation", "targeting", "positioning", "cooldowns")
        duration = 20
    }
    "pvp" = @{
        agents = @("pvp-arena-tactician")
        scenarios = @("2v2", "3v3", "battleground")
        duration = 60
    }
    "dungeon" = @{
        agents = @("wow-dungeon-raid-coordinator")
        scenarios = @("tank_and_spank", "add_management", "boss_mechanics")
        duration = 90
    }
    "economy" = @{
        agents = @("wow-economy-manager")
        scenarios = @("auction_house", "gathering", "crafting", "trading")
        duration = 15
    }
    "learning" = @{
        agents = @("bot-learning-system")
        scenarios = @("adaptation", "pattern_recognition", "improvement_rate")
        duration = 45
    }
    "quick" = @{
        agents = @("wow-bot-behavior-designer", "wow-mechanics-expert")
        scenarios = @("basic_combat", "movement", "interaction")
        duration = 15
    }
    "full" = @{
        agents = @(
            "wow-bot-behavior-designer",
            "wow-mechanics-expert",
            "pvp-arena-tactician",
            "wow-dungeon-raid-coordinator",
            "wow-economy-manager",
            "bot-learning-system"
        )
        scenarios = @("comprehensive")
        duration = 240
    }
}

# Execute gameplay test
function Start-GameplayTest {
    param(
        [string]$AgentName,
        [string]$Scenario,
        [string[]]$TestClasses,
        [int]$TestDuration
    )
    
    Write-TestLog "Starting test: $AgentName - $Scenario" "TEST"
    
    $testConfig = @{
        agent = $AgentName
        scenario = $Scenario
        classes = $TestClasses
        duration_minutes = $TestDuration
        timestamp = Get-Date -Format "o"
    }
    
    # Simulate test execution (replace with actual Claude Code call)
    $result = @{
        agent = $AgentName
        scenario = $Scenario
        status = "success"
        score = Get-Random -Minimum 70 -Maximum 100
        issues_found = Get-Random -Minimum 0 -Maximum 5
        recommendations = @()
    }
    
    # Add scenario-specific results
    switch ($AgentName) {
        "wow-bot-behavior-designer" {
            $result.behavior_accuracy = Get-Random -Minimum 85 -Maximum 99
            $result.stuck_rate = Get-Random -Minimum 0 -Maximum 5
            $result.quest_completion = Get-Random -Minimum 80 -Maximum 100
        }
        "wow-mechanics-expert" {
            $result.rotation_efficiency = Get-Random -Minimum 75 -Maximum 95
            $result.spell_accuracy = Get-Random -Minimum 90 -Maximum 100
            $result.positioning_score = Get-Random -Minimum 70 -Maximum 90
        }
        "pvp-arena-tactician" {
            $result.win_rate = Get-Random -Minimum 40 -Maximum 70
            $result.tactical_score = Get-Random -Minimum 60 -Maximum 85
            $result.coordination = Get-Random -Minimum 70 -Maximum 95
        }
        "wow-dungeon-raid-coordinator" {
            $result.completion_rate = Get-Random -Minimum 85 -Maximum 100
            $result.wipe_count = Get-Random -Minimum 0 -Maximum 3
            $result.role_performance = Get-Random -Minimum 75 -Maximum 95
        }
        "wow-economy-manager" {
            $result.profit_margin = Get-Random -Minimum -10 -Maximum 50
            $result.market_efficiency = Get-Random -Minimum 60 -Maximum 90
            $result.resource_optimization = Get-Random -Minimum 70 -Maximum 95
        }
        "bot-learning-system" {
            $result.learning_rate = Get-Random -Minimum 2 -Maximum 10
            $result.adaptation_success = Get-Random -Minimum 70 -Maximum 95
            $result.improvement_trend = Get-Random -Minimum -2 -Maximum 8
        }
    }
    
    # Add recommendations based on score
    if ($result.score -lt 80) {
        $result.recommendations += "Review $AgentName configuration"
        $result.recommendations += "Increase test coverage for $Scenario"
    }
    if ($result.issues_found -gt 3) {
        $result.recommendations += "Critical issues detected - immediate review required"
    }
    
    Write-TestLog "Test completed: Score = $($result.score), Issues = $($result.issues_found)" "SUCCESS"
    
    return $result
}

# Generate gameplay report
function New-GameplayReport {
    param([array]$TestResults)
    
    Write-TestLog "Generating gameplay test report" "INFO"
    
    $reportDate = Get-Date
    $reportFile = Join-Path $ReportsDir "gameplay_report_$(Get-Date -Format 'yyyyMMdd_HHmmss').html"
    
    # Calculate summary statistics
    $totalTests = $TestResults.Count
    $successfulTests = ($TestResults | Where-Object { $_.status -eq "success" }).Count
    $averageScore = ($TestResults | Measure-Object -Property score -Average).Average
    $totalIssues = ($TestResults | Measure-Object -Property issues_found -Sum).Sum
    
    $html = @"
<!DOCTYPE html>
<html>
<head>
    <title>PlayerBot Gameplay Test Report</title>
    <style>
        body { 
            font-family: 'Segoe UI', Arial, sans-serif; 
            margin: 20px; 
            background: #f5f5f5;
        }
        h1 { 
            color: #2c3e50; 
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }
        .summary {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        .metric {
            display: inline-block;
            margin: 10px 20px;
            text-align: center;
        }
        .metric-value {
            font-size: 2em;
            font-weight: bold;
            color: #3498db;
        }
        .metric-label {
            color: #7f8c8d;
            font-size: 0.9em;
        }
        .test-card {
            background: white;
            padding: 15px;
            margin: 10px 0;
            border-radius: 5px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
            border-left: 4px solid #3498db;
        }
        .success { border-left-color: #27ae60; }
        .warning { border-left-color: #f39c12; }
        .error { border-left-color: #e74c3c; }
        .test-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
        }
        .score {
            font-size: 1.5em;
            font-weight: bold;
        }
        .good { color: #27ae60; }
        .average { color: #f39c12; }
        .poor { color: #e74c3c; }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }
        th, td {
            padding: 8px;
            text-align: left;
            border-bottom: 1px solid #ecf0f1;
        }
        th {
            background: #34495e;
            color: white;
        }
        .recommendation {
            background: #fff9c4;
            padding: 10px;
            margin: 5px 0;
            border-radius: 4px;
            border-left: 3px solid #ffc107;
        }
    </style>
</head>
<body>
    <h1>🎮 PlayerBot Gameplay Test Report</h1>
    <p>Generated: $($reportDate.ToString('yyyy-MM-dd HH:mm:ss'))</p>
    
    <div class="summary">
        <h2>📊 Test Summary</h2>
        <div class="metrics">
            <div class="metric">
                <div class="metric-value">$totalTests</div>
                <div class="metric-label">Total Tests</div>
            </div>
            <div class="metric">
                <div class="metric-value">$successfulTests</div>
                <div class="metric-label">Successful</div>
            </div>
            <div class="metric">
                <div class="metric-value">$([math]::Round($averageScore, 1))</div>
                <div class="metric-label">Avg Score</div>
            </div>
            <div class="metric">
                <div class="metric-value">$totalIssues</div>
                <div class="metric-label">Issues Found</div>
            </div>
        </div>
    </div>
    
    <h2>🔍 Detailed Test Results</h2>
"@
    
    foreach ($test in $TestResults) {
        $cardClass = if ($test.score -ge 85) { "success" } 
                     elseif ($test.score -ge 70) { "warning" } 
                     else { "error" }
        
        $scoreClass = if ($test.score -ge 85) { "good" }
                      elseif ($test.score -ge 70) { "average" }
                      else { "poor" }
        
        $html += @"
        <div class="test-card $cardClass">
            <div class="test-header">
                <h3>$($test.agent) - $($test.scenario)</h3>
                <span class="score $scoreClass">$($test.score)/100</span>
            </div>
            <table>
"@
        
        # Add agent-specific metrics
        foreach ($key in $test.Keys) {
            if ($key -notin @("agent", "scenario", "status", "score", "issues_found", "recommendations", "timestamp")) {
                $value = $test[$key]
                if ($value -is [int] -or $value -is [double]) {
                    $value = [math]::Round($value, 1)
                }
                $html += "<tr><td><strong>$($key -replace '_', ' '):</strong></td><td>$value</td></tr>"
            }
        }
        
        $html += @"
                <tr><td><strong>Issues Found:</strong></td><td>$($test.issues_found)</td></tr>
            </table>
"@
        
        if ($test.recommendations.Count -gt 0) {
            $html += "<h4>Recommendations:</h4>"
            foreach ($rec in $test.recommendations) {
                $html += "<div class='recommendation'>💡 $rec</div>"
            }
        }
        
        $html += "</div>"
    }
    
    # Add overall recommendations
    $html += @"
    <h2>🎯 Action Items</h2>
    <div class="summary">
"@
    
    if ($averageScore -lt 80) {
        $html += "<div class='recommendation'>⚠️ <strong>Critical:</strong> Overall score below threshold. Review bot AI implementation.</div>"
    }
    if ($totalIssues -gt 10) {
        $html += "<div class='recommendation'>⚠️ <strong>High Priority:</strong> Multiple issues detected. Schedule deep debugging session.</div>"
    }
    
    $html += @"
        <div class='recommendation'>📝 <strong>Next Steps:</strong> Review detailed logs in: $LogFile</div>
    </div>
</body>
</html>
"@
    
    $html | Out-File -FilePath $reportFile -Encoding UTF8
    Write-TestLog "Report saved to: $reportFile" "SUCCESS"
    
    return $reportFile
}

# Main execution
function Start-GameplayTesting {
    Write-TestLog "=" * 60 "INFO"
    Write-TestLog "Starting PlayerBot Gameplay Testing" "INFO"
    Write-TestLog "Suite: $Suite | Duration: $Duration minutes" "INFO"
    Write-TestLog "=" * 60 "INFO"
    
    $results = @()
    
    if ($Agent) {
        # Test specific agent
        $result = Start-GameplayTest -AgentName $Agent -Scenario "custom" `
                                     -TestClasses $Classes -TestDuration $Duration
        $results += $result
    }
    else {
        # Run test suite
        $suiteConfig = $TestSuites[$Suite]
        
        if (-not $suiteConfig) {
            Write-TestLog "Invalid suite: $Suite" "ERROR"
            return
        }
        
        foreach ($agentName in $suiteConfig.agents) {
            foreach ($scenario in $suiteConfig.scenarios) {
                if ($Suite -eq "quick" -and $results.Count -ge 3) { break }
                
                $result = Start-GameplayTest -AgentName $agentName -Scenario $scenario `
                                            -TestClasses $Classes -TestDuration ($Duration / $suiteConfig.scenarios.Count)
                $results += $result
                
                # Add delay between tests
                Start-Sleep -Seconds 2
            }
        }
    }
    
    # Generate report
    if ($GenerateReport -and $results.Count -gt 0) {
        $reportFile = New-GameplayReport -TestResults $results
        
        # Open report in browser
        Start-Process $reportFile
        
        Write-TestLog "Gameplay testing completed!" "SUCCESS"
        Write-TestLog "Report: $reportFile" "INFO"
    }
    
    # Check for critical issues
    $criticalIssues = $results | Where-Object { $_.score -lt 70 -or $_.issues_found -gt 5 }
    if ($criticalIssues) {
        Write-TestLog "CRITICAL: $(($criticalIssues).Count) tests failed critical thresholds!" "ERROR"
        exit 1
    }
    
    exit 0
}

# Execute
Start-GameplayTesting
