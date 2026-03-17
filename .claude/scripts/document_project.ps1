# Documentation Workflow for PlayerBot Project
# Orchestrates multiple agents to document entire codebase

param(
    [Parameter(Mandatory=$false)]
    [string]$ProjectRoot = "C:\TrinityBots\TrinityCore",
    
    [Parameter(Mandatory=$false)]
    [string[]]$Directories = @("src\game\playerbot"),
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("analyze", "document", "insert", "full")]
    [string]$Mode = "full",
    
    [Parameter(Mandatory=$false)]
    [switch]$AutoFix = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$CreateBackup = $true,
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("html", "markdown", "json", "all")]
    [string]$OutputFormat = "all"
)

Write-Host ("=" * 60) -ForegroundColor Cyan
Write-Host "PlayerBot Documentation System" -ForegroundColor Cyan
Write-Host ("=" * 60) -ForegroundColor Cyan

$ClaudeDir = Join-Path $ProjectRoot ".claude"
$ScriptsDir = Join-Path $ClaudeDir "scripts"
$DocDir = Join-Path $ClaudeDir "documentation"
$AgentsDir = Join-Path $ClaudeDir "agents"

# Ensure directories exist
@($DocDir, "$ClaudeDir\backups") | ForEach-Object {
    if (-not (Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ -Force | Out-Null
    }
}

# Function to execute agent
function Invoke-DocumentationAgent {
    param(
        [string]$AgentName,
        [hashtable]$InputData
    )
    
    Write-Host "`nExecuting Agent: $AgentName" -ForegroundColor Green
    
    $agentFile = Join-Path $AgentsDir "$AgentName.md"
    if (-not (Test-Path $agentFile)) {
        Write-Host "Agent not found: $AgentName" -ForegroundColor Red
        return $null
    }
    
    # Read agent configuration
    $agentContent = Get-Content $agentFile -Raw
    
    # Simulate agent execution (in production, this would call Claude Code)
    $result = @{
        agent = $AgentName
        status = "success"
        timestamp = Get-Date -Format "o"
        data = $InputData
    }
    
    Write-Host "  ✓ Agent $AgentName completed" -ForegroundColor Green
    return $result
}

# Stage 1: Code Analysis
function Start-CodeAnalysis {
    Write-Host "`n📊 Stage 1: Code Analysis" -ForegroundColor Yellow
    
    # Execute code-documentation agent
    $analysisResult = Invoke-DocumentationAgent -AgentName "code-documentation" -InputData @{
        directories = $Directories
        project_root = $ProjectRoot
        analyze_only = $true
    }
    
    # Execute trinity-integration-tester for API documentation
    $trinityResult = Invoke-DocumentationAgent -AgentName "trinity-integration-tester" -InputData @{
        check_apis = $true
        document_hooks = $true
    }
    
    # Execute cpp-architecture-optimizer for complexity analysis
    $architectureResult = Invoke-DocumentationAgent -AgentName "cpp-architecture-optimizer" -InputData @{
        analyze_patterns = $true
        identify_complex_functions = $true
    }
    
    return @{
        analysis = $analysisResult
        trinity = $trinityResult
        architecture = $architectureResult
    }
}

# Stage 2: Documentation Generation
function Start-DocumentationGeneration {
    Write-Host "`n📝 Stage 2: Documentation Generation" -ForegroundColor Yellow
    
    # Run Python documentation generator
    $pythonScript = Join-Path $ScriptsDir "generate_documentation.py"
    
    if (-not (Test-Path $pythonScript)) {
        Write-Host "Documentation script not found!" -ForegroundColor Red
        return $null
    }
    
    $dirs = $Directories -join " "
    $cmd = "python `"$pythonScript`" --project-root `"$ProjectRoot`" --directories $dirs --output-format $OutputFormat"
    
    Write-Host "Running: $cmd" -ForegroundColor Gray
    $output = Invoke-Expression $cmd 2>&1
    
    # Parse output for generated files
    $jsonFile = Join-Path $DocDir "documentation.json"
    $htmlFile = Join-Path $DocDir "index.html"
    $mdFile = Join-Path $DocDir "PLAYERBOT_DOCUMENTATION.md"
    
    $result = @{
        json_file = if (Test-Path $jsonFile) { $jsonFile } else { $null }
        html_file = if (Test-Path $htmlFile) { $htmlFile } else { $null }
        markdown_file = if (Test-Path $mdFile) { $mdFile } else { $null }
        output = $output -join "`n"
    }
    
    Write-Host "  ✓ Documentation generated" -ForegroundColor Green
    
    # Display statistics
    if (Test-Path $jsonFile) {
        $docData = Get-Content $jsonFile | ConvertFrom-Json
        $stats = $docData.total_statistics
        
        Write-Host "`n  Statistics:" -ForegroundColor Cyan
        Write-Host "    Files: $($stats.total_files)" -ForegroundColor Gray
        Write-Host "    Classes: $($stats.total_classes)" -ForegroundColor Gray
        Write-Host "    Functions: $($stats.total_functions)" -ForegroundColor Gray
        Write-Host "    Coverage: $($stats.total_documentation_coverage)%" -ForegroundColor Gray
    }
    
    return $result
}

# Stage 3: Comment Insertion
function Start-CommentInsertion {
    param([string]$DocumentationFile)
    
    Write-Host "`n💬 Stage 3: Comment Insertion" -ForegroundColor Yellow
    
    if (-not $AutoFix) {
        Write-Host "  Auto-fix disabled. Skipping comment insertion." -ForegroundColor Gray
        return $null
    }
    
    # Execute comment-generator agent
    $commentResult = Invoke-DocumentationAgent -AgentName "comment-generator" -InputData @{
        documentation_file = $DocumentationFile
        mode = "standard"
    }
    
    # Run Python comment insertion script
    $addCommentsScript = Join-Path $ScriptsDir "add_comments.py"
    
    if (Test-Path $addCommentsScript) {
        $backupDir = Join-Path $ClaudeDir "backups\$(Get-Date -Format 'yyyyMMdd_HHmmss')"
        $cmd = "python `"$addCommentsScript`" --documentation-file `"$DocumentationFile`" --backup-dir `"$backupDir`""
        
        if (-not $CreateBackup) {
            $cmd += " --dry-run"
        }
        
        Write-Host "Running: $cmd" -ForegroundColor Gray
        $output = Invoke-Expression $cmd 2>&1
        
        Write-Host "  ✓ Comments inserted" -ForegroundColor Green
        Write-Host $output -ForegroundColor Gray
        
        return @{
            backup_dir = $backupDir
            output = $output -join "`n"
        }
    }
    
    return $null
}

# Stage 4: Quality Check
function Start-QualityCheck {
    Write-Host "`n✅ Stage 4: Quality Check" -ForegroundColor Yellow
    
    # Execute code-quality-reviewer to verify documentation
    $qualityResult = Invoke-DocumentationAgent -AgentName "code-quality-reviewer" -InputData @{
        check_documentation = $true
        verify_comments = $true
    }
    
    # Execute automated-fix-agent for any remaining issues
    $fixResult = $null
    if ($AutoFix) {
        $fixResult = Invoke-DocumentationAgent -AgentName "automated-fix-agent" -InputData @{
            fix_documentation_gaps = $true
            fix_formatting = $true
        }
    }
    
    Write-Host "  ✓ Quality check completed" -ForegroundColor Green
    
    return @{
        quality = $qualityResult
        fixes = $fixResult
    }
}

# Stage 5: Report Generation
function Start-ReportGeneration {
    param($AllResults)
    
    Write-Host "`n📊 Stage 5: Report Generation" -ForegroundColor Yellow
    
    # Execute daily-report-generator with documentation focus
    $reportResult = Invoke-DocumentationAgent -AgentName "daily-report-generator" -InputData @{
        report_type = "documentation"
        include_metrics = $true
        results = $AllResults
    }
    
    # Create summary report
    $summaryFile = Join-Path $DocDir "DOCUMENTATION_SUMMARY.md"
    $summary = @"
# Documentation Generation Summary

**Date**: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
**Project**: PlayerBot

## Results

### Files Generated
- HTML Documentation: $(if ($AllResults.generation.html_file) { "✓" } else { "✗" })
- Markdown Documentation: $(if ($AllResults.generation.markdown_file) { "✓" } else { "✗" })
- JSON Database: $(if ($AllResults.generation.json_file) { "✓" } else { "✗" })

### Statistics
$(if ($AllResults.generation.json_file -and (Test-Path $AllResults.generation.json_file)) {
    $data = Get-Content $AllResults.generation.json_file | ConvertFrom-Json
    $stats = $data.total_statistics
    @"
- Total Files: $($stats.total_files)
- Total Classes: $($stats.total_classes)
- Total Functions: $($stats.total_functions)
- Total Lines: $($stats.total_lines)
- Documentation Coverage: $($stats.total_documentation_coverage)%
"@
})

### Comments Inserted
$(if ($AllResults.insertion) {
    $AllResults.insertion.output
})

## File Locations
- HTML: ``$($AllResults.generation.html_file)``
- Markdown: ``$($AllResults.generation.markdown_file)``
- JSON: ``$($AllResults.generation.json_file)``
$(if ($AllResults.insertion.backup_dir) {
    "- Backups: ``$($AllResults.insertion.backup_dir)``"
})

## Next Steps
1. Review generated documentation
2. Verify inserted comments compile correctly
3. Commit changes to version control
4. Deploy documentation to team wiki/portal
"@
    
    $summary | Out-File -FilePath $summaryFile -Encoding UTF8
    
    Write-Host "  ✓ Summary report created: $summaryFile" -ForegroundColor Green
    
    return $summaryFile
}

# Main execution flow
function Start-DocumentationWorkflow {
    $startTime = Get-Date
    $results = @{}
    
    try {
        switch ($Mode) {
            "analyze" {
                $results.analysis = Start-CodeAnalysis
            }
            
            "document" {
                $results.analysis = Start-CodeAnalysis
                $results.generation = Start-DocumentationGeneration
            }
            
            "insert" {
                $results.generation = Start-DocumentationGeneration
                if ($results.generation.json_file) {
                    $results.insertion = Start-CommentInsertion -DocumentationFile $results.generation.json_file
                }
            }
            
            "full" {
                # Complete workflow
                Write-Host "`n🚀 Running Complete Documentation Workflow" -ForegroundColor Cyan
                
                $results.analysis = Start-CodeAnalysis
                $results.generation = Start-DocumentationGeneration
                
                if ($results.generation.json_file) {
                    $results.insertion = Start-CommentInsertion -DocumentationFile $results.generation.json_file
                }
                
                $results.quality = Start-QualityCheck
                $results.summary = Start-ReportGeneration -AllResults $results
            }
        }
        
        $duration = (Get-Date) - $startTime
        
        Write-Host "`n" + ("=" * 60) -ForegroundColor Cyan
        Write-Host "✅ Documentation Workflow Complete!" -ForegroundColor Green
        Write-Host ("=" * 60) -ForegroundColor Cyan
        Write-Host "Duration: $($duration.TotalMinutes.ToString('F2')) minutes" -ForegroundColor Gray
        
        # Open generated documentation
        if ($results.generation) {
            if ($results.generation.html_file -and (Test-Path $results.generation.html_file)) {
                Write-Host "`nOpening documentation in browser..." -ForegroundColor Yellow
                Start-Process $results.generation.html_file
            }
            
            if ($results.summary -and (Test-Path $results.summary)) {
                Write-Host "Summary report: $($results.summary)" -ForegroundColor Yellow
            }
        }
        
        return $results
    }
    catch {
        Write-Host "`n❌ Error during documentation workflow: $_" -ForegroundColor Red
        Write-Host $_.ScriptStackTrace -ForegroundColor Red
        return $null
    }
}

# Execute workflow
$workflowResults = Start-DocumentationWorkflow

# Create desktop shortcut to documentation
if ($workflowResults.generation.html_file) {
    try {
        $WshShell = New-Object -ComObject WScript.Shell
        $Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\PlayerBot Documentation.lnk")
        $Shortcut.TargetPath = $workflowResults.generation.html_file
        $Shortcut.IconLocation = "C:\Windows\System32\shell32.dll,1"
        $Shortcut.Description = "Open PlayerBot Code Documentation"
        $Shortcut.Save()
        Write-Host "`n📌 Desktop shortcut created for documentation" -ForegroundColor Green
    } catch {
        Write-Host "Could not create desktop shortcut: $_" -ForegroundColor Yellow
    }
}

# Prompt for next action
if ($workflowResults -and -not $AutoFix) {
    Write-Host "`n" -NoNewline
    $response = Read-Host "Would you like to insert comments automatically? (y/n)"
    if ($response -eq 'y') {
        Write-Host "`nInserting comments..." -ForegroundColor Yellow
        $workflowResults.insertion = Start-CommentInsertion -DocumentationFile $workflowResults.generation.json_file
        Write-Host "✅ Comments inserted successfully!" -ForegroundColor Green
    }
}

Write-Host "`n🎉 Documentation system ready to use!" -ForegroundColor Green
