# Daily Automation Script for PlayerBot Project - Debug Version
# Runs scheduled tasks and code reviews with enhanced debugging

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("morning", "midday", "evening", "full", "critical")]
    [string]$CheckType = "full",
    
    [Parameter(Mandatory=$false)]
    [string]$ProjectRoot = "C:\TrinityBots\TrinityCore",
    
    [Parameter(Mandatory=$false)]
    [switch]$AutoFix = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$EmailReport = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$DebugMode = $false
)

# Configuration
$ClaudeDir = Join-Path $ProjectRoot ".claude"
$ScriptsDir = Join-Path $ClaudeDir "scripts"
$ReportsDir = Join-Path $ClaudeDir "reports"
$LogFile = Join-Path $ClaudeDir "automation.log"

# Ensure directories exist
@($ClaudeDir, $ScriptsDir, $ReportsDir) | ForEach-Object {
    if (-not (Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ -Force | Out-Null
    }
}

# Enhanced logging function
function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "$timestamp [$Level] $Message"
    Add-Content -Path $LogFile -Value $logEntry
    
    switch ($Level) {
        "ERROR" { Write-Host $logEntry -ForegroundColor Red }
        "WARNING" { Write-Host $logEntry -ForegroundColor Yellow }
        "SUCCESS" { Write-Host $logEntry -ForegroundColor Green }
        "DEBUG" { 
            if ($DebugMode) { 
                Write-Host $logEntry -ForegroundColor Cyan 
            }
        }
        default { Write-Host $logEntry }
    }
}

# Debug function to inspect variables
function Write-DebugInfo {
    param([string]$VariableName, [object]$Variable)
    
    if ($DebugMode) {
        $type = if ($Variable) { $Variable.GetType().Name } else { "null" }
        Write-Log "DEBUG: $VariableName is of type [$type]" "DEBUG"
        
        if ($Variable -is [hashtable] -or $Variable -is [System.Collections.IDictionary]) {
            Write-Log "DEBUG: $VariableName has keys: $($Variable.Keys -join ', ')" "DEBUG"
        } elseif ($Variable -is [array]) {
            Write-Log "DEBUG: $VariableName is an array with $($Variable.Count) items" "DEBUG"
        } elseif ($Variable -is [string]) {
            $preview = if ($Variable.Length -gt 50) { $Variable.Substring(0, 50) + "..." } else { $Variable }
            Write-Log "DEBUG: $VariableName = '$preview'" "DEBUG"
        }
    }
}

# Execute Claude Code agent
function Invoke-ClaudeAgent {
    param(
        [string]$AgentName,
        [hashtable]$InputData = @{}
    )
    
    Write-Log "Executing agent: $AgentName"
    
    # Fixed: Changed path separator from backslash to forward slash for cross-platform compatibility
    $agentFile = Join-Path $ClaudeDir "agents/$AgentName.md"
    if (-not (Test-Path $agentFile)) {
        Write-Log "Agent file not found: $agentFile" "ERROR"
        return @{ status = "error"; message = "Agent not found" }
    }
    
    # Prepare command for Claude Code
    $command = @{
        agent = $AgentName
        input = $InputData
        project_root = $ProjectRoot
        timestamp = (Get-Date -Format "o")
    } | ConvertTo-Json -Depth 10
    
    $commandFile = Join-Path $ClaudeDir "command_$AgentName.json"
    $command | Out-File -FilePath $commandFile -Encoding UTF8
    
    try {
        # Execute via Claude Code CLI (adjust path as needed)
        # In production, this would call actual Claude Code
        # $result = & claude-code execute --agent $agentFile --input $commandFile
        
        # Simulated execution for demonstration
        $result = Get-SimulatedAgentResult -AgentName $AgentName
        
        Write-DebugInfo "Agent Result for $AgentName" $result
        
        Write-Log "Agent $AgentName completed successfully" "SUCCESS"
        return $result
    }
    catch {
        Write-Log "Agent $AgentName failed: $_" "ERROR"
        return @{ status = "error"; message = $_.ToString() }
    }
    finally {
        if (Test-Path $commandFile) {
            Remove-Item $commandFile -Force
        }
    }
}

# Simulated agent results (replace with actual Claude Code integration)
function Get-SimulatedAgentResult {
    param([string]$AgentName)
    
    $results = @{
        "trinity-integration-tester" = @{
            status = "success"
            compatibility_score = 94
            issues_found = 2
        }
        "code-quality-reviewer" = @{
            status = "success"
            quality_score = 87
            code_smells = 5
        }
        "security-auditor" = @{
            status = "success"
            vulnerabilities = @{
                critical = 0
                high = 1
                medium = 3
            }
        }
        "performance-analyzer" = @{
            status = "success"
            performance_score = 82
            bottlenecks = 2
        }
        "test-automation-engineer" = @{
            status = "success"
            tests_passed = 142
            tests_failed = 3
            coverage = 78.5
        }
    }
    
    if ($results.ContainsKey($AgentName)) {
        return $results[$AgentName]
    } else {
        return @{ status = "not_implemented"; message = "Agent $AgentName not yet implemented" }
    }
}

# Run morning checks
function Start-MorningChecks {
    Write-Log "Starting morning health checks"
    
    $agents = @(
        "test-automation-engineer",
        "trinity-integration-tester",
        "performance-analyzer"
    )
    
    $results = @{}
    foreach ($agent in $agents) {
        $results[$agent] = Invoke-ClaudeAgent -AgentName $agent
    }
    
    Write-DebugInfo "Morning Check Results" $results
    return $results
}

# Run security scan
function Start-SecurityScan {
    Write-Log "Starting security scan"
    
    $result = Invoke-ClaudeAgent -AgentName "security-auditor" -InputData @{
        scope = "full"
        auto_fix = $AutoFix
    }
    
    Write-DebugInfo "Security Scan Result" $result
    
    # Fixed: Added null check for vulnerabilities object
    if ($result -and $result.ContainsKey("vulnerabilities") -and $result.vulnerabilities -and $result.vulnerabilities.ContainsKey("critical") -and $result.vulnerabilities.critical -gt 0) {
        Write-Log "CRITICAL security vulnerabilities found!" "ERROR"
        
        if ($AutoFix) {
            Write-Log "Attempting automatic fix..."
            $fixResult = Invoke-ClaudeAgent -AgentName "automated-fix-agent" -InputData @{
                issues = $result.vulnerabilities
                priority = "critical"
            }
        }
    }
    
    return $result
}

# Run performance analysis
function Start-PerformanceAnalysis {
    Write-Log "Starting performance analysis"
    
    $result = Invoke-ClaudeAgent -AgentName "performance-analyzer" -InputData @{
        profile_duration = 300
        metrics = @("cpu", "memory", "io", "network")
    }
    
    Write-DebugInfo "Performance Analysis Result" $result
    
    # Fixed: Added comprehensive null and type checks
    if ($result -and $result.ContainsKey("performance_score") -and $result.performance_score -is [int] -and $result.performance_score -lt 70) {
        Write-Log "Performance below threshold!" "WARNING"
    }
    
    return $result
}

# Run complete review with enhanced error handling
function Start-CompleteReview {
    Write-Log "Starting complete code review"
    
    $pythonScript = Join-Path $ScriptsDir "master_review.py"
    
    if (Test-Path $pythonScript) {
        $mode = if ($AutoFix) { "deep" } else { "standard" }
        $fixMode = if ($AutoFix) { "all" } else { "critical" }
        
        Write-Log "Found Python script, attempting execution..." "DEBUG"
        
        try {
            if (Get-Command python -ErrorAction SilentlyContinue) {
                $pythonResult = & python $pythonScript --mode $mode --project-root $ProjectRoot --fix $fixMode 2>&1
                if ($LASTEXITCODE -ne 0) {
                    Write-Log "Python script execution failed with exit code $LASTEXITCODE" "ERROR"
                    Write-Log "Error output: $pythonResult" "ERROR"
                    return @{ 
                        status = "error"; 
                        message = "Python script failed"; 
                        output = $pythonResult;
                        exit_code = $LASTEXITCODE
                    }
                }
                $finalResult = @{ 
                    status = "success"; 
                    message = "Python script completed successfully";
                    output = $pythonResult 
                }
                Write-DebugInfo "Python Execution Result" $finalResult
                return $finalResult
            } else {
                Write-Log "Python not found in PATH" "ERROR"
            }
        }
        catch {
            Write-Log "Error executing Python script: $_" "ERROR"
            return @{ 
                status = "error"; 
                message = "Exception during Python execution: $_"
            }
        }
    }
    else {
        Write-Log "Python review script not found at: $pythonScript" "WARNING"
    }
    
    # Fallback to PowerShell implementation
    Write-Log "Using PowerShell fallback implementation"
    $agents = @(
        "playerbot-project-coordinator",
        "trinity-integration-tester", 
        "code-quality-reviewer",
        "security-auditor",
        "performance-analyzer",
        "cpp-architecture-optimizer",
        "enterprise-compliance-checker",
        "test-automation-engineer",
        "daily-report-generator"
    )
    
    $results = @{}
    $previousOutput = @{}
    
    foreach ($agent in $agents) {
        Write-Log "Processing agent: $agent" "DEBUG"
        $inputData = @{ previous_results = $previousOutput }
        $result = Invoke-ClaudeAgent -AgentName $agent -InputData $inputData
        $results[$agent] = $result
        $previousOutput[$agent] = $result
        
        if ($result -and $result.ContainsKey("status") -and $result.status -eq "critical_failure") {
            Write-Log "Critical failure in $agent, stopping workflow" "ERROR"
            break
        }
    }
    
    Write-DebugInfo "Complete Review Results" $results
    return $results
}

# Enhanced report generation with comprehensive type checking
function New-DailyReport {
    param([hashtable]$Results)
    
    Write-Log "Generating daily report"
    Write-DebugInfo "Report Input Data" $Results
    
    $reportDate = Get-Date -Format "yyyy-MM-dd"
    $reportFile = Join-Path $ReportsDir "daily_report_$reportDate.html"
    
    $html = @"
<!DOCTYPE html>
<html>
<head>
    <title>PlayerBot Daily Report - $reportDate</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #333; }
        .metric { 
            display: inline-block; 
            margin: 10px; 
            padding: 15px; 
            border: 1px solid #ddd; 
            border-radius: 5px; 
            min-width: 200px;
        }
        .critical { background-color: #ffcccc; }
        .warning { background-color: #ffffcc; }
        .success { background-color: #ccffcc; }
        .info { background-color: #e6f3ff; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <h1>PlayerBot Daily Report</h1>
    <p>Date: $reportDate</p>
    <p>Time: $(Get-Date -Format "HH:mm:ss")</p>
    
    <h2>Check Results</h2>
    <div class="metrics">
"@
    
    # Enhanced type handling for different result types
    foreach ($resultKey in $Results.Keys) {
        Write-Log "Processing result for: $resultKey" "DEBUG"
        $result = $Results[$resultKey]
        
        Write-DebugInfo "Processing Result $resultKey" $result
        
        # Comprehensive type checking and conversion
        if ($result -eq $null) {
            Write-Log "Result for $resultKey is null, creating default entry" "DEBUG"
            $result = @{ status = "unknown"; message = "No data available" }
        } elseif ($result -is [string]) {
            Write-Log "Result for $resultKey is string, converting to hashtable" "DEBUG"
            $result = @{ status = "info"; message = $result; data_type = "string" }
        } elseif ($result -is [array]) {
            Write-Log "Result for $resultKey is array, converting to hashtable" "DEBUG"
            $result = @{ status = "info"; message = "Array data with $($result.Count) items"; data_type = "array" }
        } elseif ($result -isnot [hashtable] -and $result -isnot [System.Collections.IDictionary]) {
            Write-Log "Result for $resultKey is $($result.GetType().Name), converting to hashtable" "DEBUG"
            $result = @{ status = "info"; message = $result.ToString(); data_type = $result.GetType().Name }
        }
        
        # Now safely get status
        $status = if ($result.ContainsKey("status")) { 
            $result.status 
        } else { 
            Write-Log "No status key found for $resultKey, defaulting to unknown" "DEBUG"
            "unknown" 
        }
        
        $cssClass = switch ($status) {
            "success" { "success" }
            "warning" { "warning" }
            "error" { "critical" }
            "info" { "info" }
            default { "info" }
        }
        
        $html += "<div class='metric $cssClass'>"
        $html += "<h3>$resultKey</h3>"
        $html += "<p><strong>Status:</strong> $status</p>"
        
        # Display message if available
        if ($result.ContainsKey("message")) {
            $message = $result.message
            if ($message.Length -gt 100) {
                $message = $message.Substring(0, 100) + "..."
            }
            $html += "<p><strong>Message:</strong> $message</p>"
        }
        
        # Display other properties that look like metrics
        foreach ($propertyKey in $result.Keys) {
            if ($propertyKey -notmatch "^(status|message|data_type)$" -and $propertyKey -match "(score|rating|percentage|count|passed|failed|coverage)") {
                $html += "<p><strong>$propertyKey</strong>: $($result[$propertyKey])</p>"
            }
        }
        
        $html += "</div>"
    }
    
    $html += @"
    </div>
    
    <h2>Action Items</h2>
    <ul>
        <li>Review and fix any critical issues</li>
        <li>Update documentation for changed code</li>
        <li>Run performance profiling if score below 80</li>
    </ul>
    
    <h2>Debug Information</h2>
    <p>Generated at: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")</p>
    <p>Check Type: $CheckType</p>
    <p>Debug Mode: $DebugMode</p>
    
</body>
</html>
"@
    
    # Enhanced file saving with error handling
    try {
        $html | Out-File -FilePath $reportFile -Encoding UTF8 -Force
        Write-Log "Report saved to $reportFile" "SUCCESS"
    }
    catch {
        Write-Log "Failed to save report: $_" "ERROR"
        return $null
    }
    
    # Send email if requested
    if ($EmailReport) {
        Send-EmailReport -ReportFile $reportFile
    }
    
    return $reportFile
}

# Send email report
function Send-EmailReport {
    param([string]$ReportFile)
    
    Write-Log "Sending email report"
    
    # Enhanced validation for required parameters
    if (-not $ReportFile -or -not (Test-Path $ReportFile)) {
        Write-Log "Report file not found or invalid: $ReportFile" "ERROR"
        return
    }
    
    # Configure email settings
    $emailParams = @{
        From = "playerbot@project.com"
        To = "team@project.com"
        Subject = "PlayerBot Daily Report - $(Get-Date -Format 'yyyy-MM-dd')"
        Body = "Please find the attached daily report"
        Attachments = $ReportFile
        SmtpServer = "smtp.project.com"
        Port = 587
        UseSsl = $true
        # Credential = Get-Credential  # Uncomment for authentication
    }
    
    try {
        # Send-MailMessage @emailParams
        Write-Log "Email report would be sent (email disabled in demo)" "WARNING"
    }
    catch {
        Write-Log "Failed to send email: $_" "ERROR"
    }
}

# Enhanced main execution with detailed debugging
function Start-DailyAutomation {
    # Fixed: String multiplication syntax
    Write-Log ("=" * 60)
    Write-Log "Starting PlayerBot Daily Automation"
    Write-Log "Check Type: $CheckType"
    Write-Log "Project Root: $ProjectRoot"
    Write-Log "Auto-Fix: $AutoFix"
    Write-Log "Debug Mode: $DebugMode"
    Write-Log ("=" * 60)
    
    $allResults = @{}
    
    try {
        switch ($CheckType) {
            "morning" {
                Write-Log "Executing morning checks..." "DEBUG"
                $allResults = Start-MorningChecks
            }
            "midday" {
                Write-Log "Executing midday checks..." "DEBUG"
                $allResults["security"] = Start-SecurityScan
                $allResults["performance"] = Start-PerformanceAnalysis
            }
            "evening" {
                Write-Log "Executing evening summary..." "DEBUG"
                # Generate summary report
                $allResults["summary"] = @{
                    status = "success"
                    message = "Evening summary generated"
                }
            }
            "critical" {
                Write-Log "Executing critical checks..." "DEBUG"
                $allResults["security"] = Start-SecurityScan
                if ($AutoFix) {
                    $allResults["auto_fix"] = Invoke-ClaudeAgent -AgentName "automated-fix-agent"
                }
            }
            "full" {
                Write-Log "Executing full review..." "DEBUG"
                $allResults = Start-CompleteReview
            }
        }
        
        Write-DebugInfo "All Results Before Report Generation" $allResults
        
        # Generate report
        $reportFile = New-DailyReport -Results $allResults
        
        # Enhanced critical issues checking with comprehensive type validation
        $criticalIssues = 0
        foreach ($resultKey in $allResults.Keys) {
            $result = $allResults[$resultKey]
            Write-Log "Checking result for critical issues: $resultKey" "DEBUG"
            Write-DebugInfo "Critical Check for $resultKey" $result
            
            if ($result) {
                # Handle different data types properly
                if ($result -is [string]) {
                    # String results are considered informational, not critical
                    Write-Log "Result $resultKey is string - not critical" "DEBUG"
                    continue
                } elseif ($result -is [hashtable] -or $result -is [System.Collections.IDictionary]) {
                    if ($result.ContainsKey("status")) {
                        $status = $result.status
                        Write-Log "Result $resultKey has status: $status" "DEBUG"
                        if ($status -eq "critical" -or $status -eq "error") {
                            $criticalIssues++
                            Write-Log "Critical issue found in $resultKey" "WARNING"
                        }
                    } else {
                        Write-Log "Result $resultKey has no status key" "DEBUG"
                    }
                } else {
                    Write-Log "Result $resultKey is of unknown type: $($result.GetType().Name)" "DEBUG"
                }
            } else {
                Write-Log "Result $resultKey is null" "DEBUG"
            }
        }
        
        if ($criticalIssues -gt 0) {
            Write-Log "ATTENTION: $criticalIssues critical issues found!" "ERROR"
            
            # Trigger immediate notification
            # This could send Slack message, create Jira ticket, etc.
        }
        
        Write-Log "Daily automation completed" "SUCCESS"
        Write-Log "Report: $reportFile"
        
        return @{
            success = $criticalIssues -eq 0
            report = $reportFile
            critical_issues = $criticalIssues
            results = $allResults
        }
    }
    catch {
        Write-Log "Error in main automation workflow: $_" "ERROR"
        Write-Log "Stack trace: $($_.ScriptStackTrace)" "DEBUG"
        return @{
            success = $false
            report = $null
            critical_issues = 999
            error = $_.ToString()
        }
    }
}

# Execute main function with enhanced error handling
try {
    Write-Log "Script execution started" "DEBUG"
    $result = Start-DailyAutomation
    
    Write-DebugInfo "Final Result" $result
    
    # Exit with appropriate code
    if ($result.success) {
        Write-Log "Script completed successfully" "SUCCESS"
        exit 0
    } else {
        Write-Log "Script completed with issues" "WARNING"
        exit 1
    }
}
catch {
    Write-Log "Fatal error during execution: $_" "ERROR"
    Write-Log "Stack trace: $($_.ScriptStackTrace)" "ERROR"
    exit 2
}
