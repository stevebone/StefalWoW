# ============================================================================
# MySQL Configuration Deployment Script for Windows
# ============================================================================
#
# This script automates the MySQL configuration changes for deadlock prevention.
# MUST BE RUN AS ADMINISTRATOR!
#
# Usage:
# 1. Right-click PowerShell and "Run as Administrator"
# 2. cd c:\TrinityBots\TrinityCore\sql\custom
# 3. .\apply_mysql_config.ps1
#
# ============================================================================

# Require Administrator
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script must be run as Administrator!"
    Write-Host "Please right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Red
    Pause
    Exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "MySQL Deadlock Prevention Configuration" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Find MySQL service
Write-Host "[1/7] Finding MySQL service..." -ForegroundColor Yellow
$mysqlService = Get-Service | Where-Object {$_.Name -like '*mysql*'} | Select-Object -First 1

if (-not $mysqlService) {
    Write-Error "MySQL service not found!"
    Write-Host "Please install MySQL or verify it's installed correctly." -ForegroundColor Red
    Pause
    Exit 1
}

Write-Host "  Found MySQL service: $($mysqlService.Name)" -ForegroundColor Green
Write-Host "  Status: $($mysqlService.Status)" -ForegroundColor Green
Write-Host ""

# Find my.ini location
Write-Host "[2/7] Locating my.ini configuration file..." -ForegroundColor Yellow
$possiblePaths = @(
    "C:\ProgramData\MySQL\MySQL Server 9.0\my.ini",
    "C:\ProgramData\MySQL\MySQL Server 8.0\my.ini",
    "C:\ProgramData\MySQL\MySQL Server 5.7\my.ini",
    "C:\Program Files\MySQL\MySQL Server 9.0\my.ini",
    "C:\Program Files\MySQL\MySQL Server 8.0\my.ini"
)

$myiniPath = $null
foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $myiniPath = $path
        break
    }
}

if (-not $myiniPath) {
    Write-Error "Could not find my.ini file!"
    Write-Host "Searched in:" -ForegroundColor Yellow
    foreach ($path in $possiblePaths) {
        Write-Host "  - $path" -ForegroundColor Yellow
    }
    Write-Host "`nPlease locate your my.ini file manually and edit it according to:" -ForegroundColor Red
    Write-Host "  sql/custom/mysql_deadlock_prevention.cnf" -ForegroundColor Red
    Pause
    Exit 1
}

Write-Host "  Found my.ini at: $myiniPath" -ForegroundColor Green
Write-Host ""

# Backup my.ini
Write-Host "[3/7] Creating backup of my.ini..." -ForegroundColor Yellow
$backupPath = "$myiniPath.backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
Copy-Item $myiniPath $backupPath
Write-Host "  Backup created: $backupPath" -ForegroundColor Green
Write-Host ""

# Read current my.ini
Write-Host "[4/7] Reading current configuration..." -ForegroundColor Yellow
$currentConfig = Get-Content $myiniPath -Raw
Write-Host "  Configuration file read successfully" -ForegroundColor Green
Write-Host ""

# Prepare configuration additions
Write-Host "[5/7] Preparing configuration changes..." -ForegroundColor Yellow
$configAdditions = @"


# ============================================================================
# TrinityCore Playerbot Deadlock Prevention Configuration
# Added by apply_mysql_config.ps1 on $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
# ============================================================================

# Deadlock Prevention Settings
innodb_lock_wait_timeout = 120
transaction-isolation = READ-COMMITTED
innodb_adaptive_hash_index = ON

# Buffer Pool Optimization (adjust based on available RAM)
innodb_buffer_pool_size = 2G
innodb_buffer_pool_instances = 8

# Log File Optimization
innodb_log_file_size = 512M
innodb_log_buffer_size = 32M
innodb_flush_log_at_trx_commit = 1

# Thread Concurrency
innodb_thread_concurrency = 64
innodb_read_io_threads = 8
innodb_write_io_threads = 8

# Deadlock Detection
innodb_deadlock_detect = ON
innodb_print_all_deadlocks = ON

# ============================================================================
# End of TrinityCore Playerbot Configuration
# ============================================================================
"@

# Check if configuration already exists
if ($currentConfig -match "TrinityCore Playerbot Deadlock Prevention") {
    Write-Host "  Configuration already applied!" -ForegroundColor Yellow
    Write-Host "  Skipping configuration modification." -ForegroundColor Yellow
    Write-Host ""
} else {
    # Add configuration to [mysqld] section
    Write-Host "  Adding configuration to [mysqld] section..." -ForegroundColor Yellow

    if ($currentConfig -match "\[mysqld\]") {
        # Insert after [mysqld]
        $currentConfig = $currentConfig -replace "(\[mysqld\])", "`$1$configAdditions"
        Set-Content -Path $myiniPath -Value $currentConfig -NoNewline
        Write-Host "  Configuration added successfully" -ForegroundColor Green
    } else {
        Write-Error "Could not find [mysqld] section in my.ini!"
        Write-Host "Please manually add the configuration from:" -ForegroundColor Red
        Write-Host "  sql/custom/mysql_deadlock_prevention.cnf" -ForegroundColor Red
        Pause
        Exit 1
    }
    Write-Host ""
}

# Stop MySQL service
Write-Host "[6/7] Restarting MySQL service..." -ForegroundColor Yellow
Write-Host "  Stopping MySQL..." -ForegroundColor Yellow
Stop-Service -Name $mysqlService.Name -Force
Start-Sleep -Seconds 3

# Start MySQL service
Write-Host "  Starting MySQL..." -ForegroundColor Yellow
Start-Service -Name $mysqlService.Name
Start-Sleep -Seconds 5

# Verify service is running
$mysqlService = Get-Service -Name $mysqlService.Name
if ($mysqlService.Status -eq 'Running') {
    Write-Host "  MySQL service restarted successfully" -ForegroundColor Green
} else {
    Write-Error "Failed to restart MySQL service!"
    Write-Host "  Status: $($mysqlService.Status)" -ForegroundColor Red
    Write-Host "`nPlease check MySQL error log for details:" -ForegroundColor Red
    Write-Host "  C:\ProgramData\MySQL\MySQL Server 9.0\Data\*.err" -ForegroundColor Red
    Write-Host "`nYou can restore the backup if needed:" -ForegroundColor Yellow
    Write-Host "  Copy-Item '$backupPath' '$myiniPath' -Force" -ForegroundColor Yellow
    Pause
    Exit 1
}
Write-Host ""

# Verify configuration
Write-Host "[7/7] Verifying configuration..." -ForegroundColor Yellow
Write-Host "  Connecting to MySQL to verify settings..." -ForegroundColor Yellow

# Prompt for MySQL credentials
$mysqlUser = Read-Host "Enter MySQL username (default: root)"
if ([string]::IsNullOrWhiteSpace($mysqlUser)) {
    $mysqlUser = "root"
}

$mysqlPassword = Read-Host "Enter MySQL password" -AsSecureString
$mysqlPasswordPlain = [Runtime.InteropServices.Marshal]::PtrToStringAuto(
    [Runtime.InteropServices.Marshal]::SecureStringToBSTR($mysqlPassword)
)

# Test MySQL connection and verify settings
try {
    $verification = @"
mysql -u $mysqlUser -p$mysqlPasswordPlain -e "SHOW VARIABLES LIKE 'innodb_lock_wait_timeout';" 2>&1
"@

    $result = Invoke-Expression "mysql -u $mysqlUser -p$mysqlPasswordPlain -e `"SHOW VARIABLES LIKE 'innodb_lock_wait_timeout';`" 2>&1"

    if ($result -match "120") {
        Write-Host "  ✓ innodb_lock_wait_timeout = 120" -ForegroundColor Green
    } else {
        Write-Host "  ✗ innodb_lock_wait_timeout not set correctly" -ForegroundColor Red
    }

    $result = Invoke-Expression "mysql -u $mysqlUser -p$mysqlPasswordPlain -e `"SHOW VARIABLES LIKE 'transaction_isolation';`" 2>&1"

    if ($result -match "READ-COMMITTED") {
        Write-Host "  ✓ transaction_isolation = READ-COMMITTED" -ForegroundColor Green
    } else {
        Write-Host "  ✗ transaction_isolation not set correctly" -ForegroundColor Red
    }

    $result = Invoke-Expression "mysql -u $mysqlUser -p$mysqlPasswordPlain -e `"SHOW VARIABLES LIKE 'innodb_buffer_pool_size';`" 2>&1"

    if ($result -match "2147483648") {
        Write-Host "  ✓ innodb_buffer_pool_size = 2G" -ForegroundColor Green
    } else {
        Write-Host "  ✗ innodb_buffer_pool_size not set correctly" -ForegroundColor Red
    }

} catch {
    Write-Warning "Could not verify configuration via MySQL client."
    Write-Host "  Please verify manually using:" -ForegroundColor Yellow
    Write-Host "  mysql -u root -p" -ForegroundColor Yellow
    Write-Host "  SHOW VARIABLES LIKE 'innodb_lock_wait_timeout';" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Configuration Applied Successfully!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host "1. Apply table optimization:" -ForegroundColor White
Write-Host "   mysql -u root -p characters < sql/custom/corpse_table_deadlock_optimization.sql" -ForegroundColor Gray
Write-Host ""
Write-Host "2. Test with 100+ bot deaths and monitor deadlocks:" -ForegroundColor White
Write-Host "   mysql -u root -p -e 'SHOW ENGINE INNODB STATUS\G'" -ForegroundColor Gray
Write-Host ""
Write-Host "Backup location: $backupPath" -ForegroundColor Green
Write-Host ""

Pause
